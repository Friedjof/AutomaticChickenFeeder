#!/bin/bash

# ESP32 Web Interface Deployment Script
# Copies and optimizes web files from /web/ to /data-template/ for ESP32 deployment

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SOURCE_DIR="web"
TARGET_DIR="data-template"
MAX_SIZE_KB=150
TEMP_DIR=$(mktemp -d)

echo -e "${BLUE}🐔 ESP32 Chicken Feeder - Web Interface Deployment${NC}"
echo "=================================================="

# Check if required directories exist
if [ ! -d "$SOURCE_DIR" ]; then
    echo -e "${RED}❌ Error: Source directory '$SOURCE_DIR' not found${NC}"
    exit 1
fi

if [ ! -d "$TARGET_DIR" ]; then
    echo -e "${YELLOW}⚠️  Creating target directory '$TARGET_DIR'${NC}"
    mkdir -p "$TARGET_DIR"
fi

# Backup existing config.json if it exists
CONFIG_BACKUP=""
if [ -f "$TARGET_DIR/config.json" ]; then
    CONFIG_BACKUP="$TEMP_DIR/config.json.backup"
    cp "$TARGET_DIR/config.json" "$CONFIG_BACKUP"
    echo -e "${YELLOW}📦 Backed up existing config.json${NC}"
fi

echo -e "${BLUE}🚀 Starting deployment...${NC}"

# Clean target directory (except config.json backup)
echo -e "${YELLOW}🧹 Cleaning target directory...${NC}"
find "$TARGET_DIR" -type f -not -name "config.json" -delete 2>/dev/null || true
find "$TARGET_DIR" -type d -empty -delete 2>/dev/null || true

# Copy web files to temporary location for processing
echo -e "${YELLOW}📂 Copying source files...${NC}"
cp -r "$SOURCE_DIR"/* "$TEMP_DIR/"

# Remove development-specific files
echo -e "${YELLOW}🗑️  Removing development files...${NC}"
rm -rf "$TEMP_DIR/compose.yml" 2>/dev/null || true
rm -rf "$TEMP_DIR/README.md" 2>/dev/null || true
rm -rf "$TEMP_DIR/mock/" 2>/dev/null || true
rm -rf "$TEMP_DIR/.git*" 2>/dev/null || true
rm -rf "$TEMP_DIR/node_modules/" 2>/dev/null || true
rm -rf "$TEMP_DIR/package*.json" 2>/dev/null || true

# Function to minify CSS
minify_css() {
    local input_file="$1"
    local output_file="$2"
    
    echo -e "${YELLOW}  Minifying CSS: $(basename "$input_file")${NC}"
    
    # Simple CSS minification (remove comments, extra whitespace, newlines)
    sed '/^[[:space:]]*\/\*/,/\*\//d' "$input_file" | \
    sed 's/[[:space:]]*{[[:space:]]*/{/g' | \
    sed 's/[[:space:]]*}[[:space:]]*/}/g' | \
    sed 's/[[:space:]]*;[[:space:]]*/;/g' | \
    sed 's/[[:space:]]*:[[:space:]]*/:/g' | \
    sed 's/[[:space:]]*,[[:space:]]*/,/g' | \
    tr -d '\n' | \
    sed 's/[[:space:]]\+/ /g' > "$output_file"
}

# Function to minify JavaScript
minify_js() {
    local input_file="$1"
    local output_file="$2"
    
    echo -e "${YELLOW}  Minifying JS: $(basename "$input_file")${NC}"
    
    # Simple JS minification (remove comments, extra whitespace)
    sed '/^[[:space:]]*\/\//d' "$input_file" | \
    sed '/^[[:space:]]*\/\*/,/\*\//d' | \
    sed 's/[[:space:]]\+/ /g' | \
    sed 's/[[:space:]]*{[[:space:]]*/{ /g' | \
    sed 's/[[:space:]]*}[[:space:]]*/ }/g' | \
    sed 's/[[:space:]]*;[[:space:]]*/; /g' > "$output_file"
}

# Function to optimize HTML
optimize_html() {
    local input_file="$1"
    local output_file="$2"
    
    echo -e "${YELLOW}  Optimizing HTML: $(basename "$input_file")${NC}"
    
    # Remove HTML comments and extra whitespace
    sed '/<!--.*-->/d' "$input_file" | \
    sed 's/[[:space:]]\+/ /g' | \
    sed 's/> </></g' > "$output_file"
}

# Process CSS files
echo -e "${BLUE}🎨 Processing CSS files...${NC}"
find "$TEMP_DIR" -name "*.css" -type f | while read -r css_file; do
    minify_css "$css_file" "$css_file.tmp"
    mv "$css_file.tmp" "$css_file"
done

# Process JavaScript files
echo -e "${BLUE}⚙️  Processing JavaScript files...${NC}"
find "$TEMP_DIR" -name "*.js" -type f | while read -r js_file; do
    minify_js "$js_file" "$js_file.tmp"
    mv "$js_file.tmp" "$js_file"
done

# Process HTML files
echo -e "${BLUE}📄 Processing HTML files...${NC}"
find "$TEMP_DIR" -name "*.html" -type f | while read -r html_file; do
    optimize_html "$html_file" "$html_file.tmp"
    mv "$html_file.tmp" "$html_file"
done

# Update API base URL for ESP32 (remove Docker-specific URLs)
echo -e "${BLUE}🔧 Updating API URLs for ESP32...${NC}"
find "$TEMP_DIR" -name "*.js" -type f -exec sed -i 's|http://localhost:3000/api|/api/v1|g' {} \;
find "$TEMP_DIR" -name "*.js" -type f -exec sed -i 's|/api|/api/v1|g' {} \;

# Copy processed files to target directory
echo -e "${BLUE}📦 Copying optimized files to target...${NC}"
cp -r "$TEMP_DIR"/* "$TARGET_DIR/"

# Restore config.json if it was backed up
if [ -n "$CONFIG_BACKUP" ] && [ -f "$CONFIG_BACKUP" ]; then
    cp "$CONFIG_BACKUP" "$TARGET_DIR/config.json"
    echo -e "${GREEN}✅ Restored config.json backup${NC}"
fi

# Calculate total size
total_size_bytes=$(find "$TARGET_DIR" -type f -exec stat -f%z {} \; 2>/dev/null | awk '{sum+=$1} END {print sum}' || \
                   find "$TARGET_DIR" -type f -exec stat -c%s {} \; 2>/dev/null | awk '{sum+=$1} END {print sum}')
total_size_kb=$((total_size_bytes / 1024))

echo ""
echo -e "${BLUE}📊 Deployment Summary${NC}"
echo "====================="

# List deployed files with sizes
echo -e "${YELLOW}Deployed files:${NC}"
find "$TARGET_DIR" -type f | while read -r file; do
    size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)
    size_kb=$((size / 1024))
    if [ $size_kb -eq 0 ]; then
        size_kb="<1"
    fi
    echo "  $(basename "$file"): ${size_kb}kB"
done

echo ""
echo -e "${YELLOW}Total size: ${total_size_kb}kB${NC}"

# Check size constraint
if [ $total_size_kb -gt $MAX_SIZE_KB ]; then
    echo -e "${RED}❌ Warning: Total size (${total_size_kb}kB) exceeds ESP32 limit (${MAX_SIZE_KB}kB)${NC}"
    echo -e "${YELLOW}Consider removing assets or further optimization${NC}"
    exit 1
else
    echo -e "${GREEN}✅ Size check passed (${total_size_kb}kB ≤ ${MAX_SIZE_KB}kB)${NC}"
fi

# Generate file manifest for ESP32
echo -e "${BLUE}📋 Generating file manifest...${NC}"
manifest_file="$TARGET_DIR/manifest.json"
cat > "$manifest_file" << EOF
{
  "generated": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
  "total_size_kb": $total_size_kb,
  "max_size_kb": $MAX_SIZE_KB,
  "files": [
EOF

first=true
find "$TARGET_DIR" -type f -not -name "manifest.json" | while read -r file; do
    if [ "$first" = true ]; then
        first=false
    else
        echo "," >> "$manifest_file"
    fi
    
    filename=$(basename "$file")
    size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)
    
    echo -n "    {\"name\": \"$filename\", \"size\": $size}" >> "$manifest_file"
done

cat >> "$manifest_file" << EOF

  ]
}
EOF

echo -e "${GREEN}✅ Generated manifest.json${NC}"

# Cleanup
rm -rf "$TEMP_DIR"

echo ""
echo -e "${GREEN}🎉 Deployment completed successfully!${NC}"
echo ""
echo -e "${BLUE}Next steps:${NC}"
echo "1. Flash ESP32 firmware: ${YELLOW}make flash${NC}"
echo "2. Upload filesystem: ${YELLOW}pio run --target uploadfs${NC}"
echo "3. Or do both: ${YELLOW}make deploy-flash${NC}"
echo ""
echo -e "${BLUE}Files deployed to: ${YELLOW}$TARGET_DIR/${NC}"
echo -e "${BLUE}Total size: ${YELLOW}${total_size_kb}kB${NC} (${GREEN}$(($MAX_SIZE_KB - $total_size_kb))kB remaining${NC})"
