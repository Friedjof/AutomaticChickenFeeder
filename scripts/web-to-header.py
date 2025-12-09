#!/usr/bin/env python3
"""
Web to C Header Converter
Converts HTML/CSS/JS files to gzipped C header files for ESP32 embedding
"""

import os
import gzip
import sys
import argparse
from pathlib import Path
from datetime import datetime

def minify_css(content):
    """Simple CSS minification"""
    import re
    # Remove comments
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    # Remove extra whitespace
    content = re.sub(r'\s+', ' ', content)
    content = re.sub(r'\s*{\s*', '{', content)
    content = re.sub(r'\s*}\s*', '}', content)
    content = re.sub(r'\s*:\s*', ':', content)
    content = re.sub(r'\s*;\s*', ';', content)
    content = re.sub(r'\s*,\s*', ',', content)
    return content.strip()

def minify_js(content):
    """Simple JS minification (preserves functionality)"""
    import re
    # Remove single-line comments (but preserve URLs)
    content = re.sub(r'(?<!:)//.*?$', '', content, flags=re.MULTILINE)
    # Remove multi-line comments
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    # Remove extra whitespace (but preserve strings)
    lines = content.split('\n')
    lines = [line.strip() for line in lines if line.strip()]
    content = '\n'.join(lines)
    return content

def minify_html(content):
    """Simple HTML minification"""
    import re
    # Remove HTML comments
    content = re.sub(r'<!--.*?-->', '', content, flags=re.DOTALL)
    # Remove extra whitespace between tags
    content = re.sub(r'>\s+<', '><', content)
    # Remove extra whitespace
    content = re.sub(r'\s+', ' ', content)
    return content.strip()

def file_to_gzip_bytes(file_path, minify=True):
    """Read file, optionally minify, and return gzipped bytes"""
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Minify based on file extension
    ext = file_path.suffix.lower()
    if minify:
        if ext == '.css':
            content = minify_css(content)
        elif ext == '.js':
            content = minify_js(content)
        elif ext == '.html' or ext == '.htm':
            content = minify_html(content)

    # Gzip compress
    content_bytes = content.encode('utf-8')
    compressed = gzip.compress(content_bytes, compresslevel=9)

    return compressed, len(content_bytes), len(compressed)

def bytes_to_c_array(data, var_name):
    """Convert bytes to C array format"""
    hex_bytes = ', '.join(f'0x{b:02x}' for b in data)

    # Format with 16 bytes per line
    formatted_lines = []
    bytes_list = [f'0x{b:02x}' for b in data]
    for i in range(0, len(bytes_list), 16):
        line = ', '.join(bytes_list[i:i+16])
        formatted_lines.append(f'    {line}')

    formatted_array = ',\n'.join(formatted_lines)

    return f"""const uint8_t {var_name}[] PROGMEM = {{
{formatted_array}
}};
const size_t {var_name}_len = {len(data)};
"""

def get_mime_type(file_path):
    """Get MIME type based on file extension"""
    ext = file_path.suffix.lower()
    mime_types = {
        '.html': 'text/html',
        '.htm': 'text/html',
        '.css': 'text/css',
        '.js': 'application/javascript',
        '.json': 'application/json',
        '.png': 'image/png',
        '.jpg': 'image/jpeg',
        '.jpeg': 'image/jpeg',
        '.gif': 'image/gif',
        '.svg': 'image/svg+xml',
        '.ico': 'image/x-icon',
    }
    return mime_types.get(ext, 'application/octet-stream')

def process_file(file_path, input_dir, minify=True):
    """Process a single file and return info"""
    print(f"Processing: {file_path.name}")

    # Generate variable name from filename
    var_name = file_path.stem.replace('-', '_').replace('.', '_')
    var_name = f"web_{var_name}_gz"

    # Read and compress file
    compressed_data, original_size, compressed_size = file_to_gzip_bytes(file_path, minify)

    compression_ratio = (1 - compressed_size / original_size) * 100 if original_size > 0 else 0

    print(f"  Original: {original_size} bytes")
    print(f"  Compressed: {compressed_size} bytes ({compression_ratio:.1f}% reduction)")

    # Get MIME type
    mime_type = get_mime_type(file_path)

    # Get relative path from input_dir for web path
    relative_path = file_path.relative_to(input_dir)
    web_path = '/' + str(relative_path).replace('\\', '/')

    return {
        'filename': file_path.name,
        'web_path': web_path,
        'var_name': var_name,
        'mime_type': mime_type,
        'original_size': original_size,
        'compressed_size': compressed_size,
        'compressed_data': compressed_data
    }

def generate_single_header(files_info, output_dir):
    """Generate a single header file containing all web files"""

    total_original = sum(info['original_size'] for info in files_info)
    total_compressed = sum(info['compressed_size'] for info in files_info)

    # Generate file comment section
    file_comments = []
    for info in files_info:
        file_comments.append(f"//   {info['filename']}: {info['original_size']} -> {info['compressed_size']} bytes")

    file_comments_str = '\n'.join(file_comments)

    # Generate all byte arrays
    byte_arrays = []
    for info in files_info:
        array_code = bytes_to_c_array(info['compressed_data'], info['var_name'])
        mime_code = f"const char* {info['var_name']}_mime = \"{info['mime_type']}\";"
        byte_arrays.append(f"// {info['filename']}\n{array_code}\n{mime_code}")

    byte_arrays_str = '\n\n'.join(byte_arrays)

    # Generate struct array
    struct_entries = []
    for info in files_info:
        struct_entries.append(f"""    {{
        .path = "{info['web_path']}",
        .data = {info['var_name']},
        .size = {info['var_name']}_len,
        .mime_type = {info['var_name']}_mime
    }}""")

    struct_array = ',\n'.join(struct_entries)

    # Generate complete header
    header_content = f"""// Auto-generated web files header
// Generated: {datetime.now().isoformat()}
// Total files: {len(files_info)}
// Total original size: {total_original} bytes
// Total compressed size: {total_compressed} bytes
// Overall compression: {(1 - total_compressed / total_original) * 100:.1f}%
//
// Files included:
{file_comments_str}

#ifndef WEB_FILES_H
#define WEB_FILES_H

#include <Arduino.h>

// ============================================================================
// Gzipped web file data
// ============================================================================

{byte_arrays_str}

// ============================================================================
// Web file index
// ============================================================================

// Structure to hold web file information
struct WebFile {{
    const char* path;
    const uint8_t* data;
    size_t size;
    const char* mime_type;
}};

// Array of all web files
const WebFile webFiles[] = {{
{struct_array}
}};

const size_t webFilesCount = {len(files_info)};

#endif // WEB_FILES_H
"""

    output_file = output_dir / "web_files.h"
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(header_content)

    # Calculate space saved
    space_saved = total_original - total_compressed
    compression_percent = (1 - total_compressed / total_original) * 100

    print(f"\n✅ Generated: {output_file.name}")
    print(f"📊 {len(files_info)} files: {total_original} bytes → {total_compressed} bytes")
    print(f"💾 Space saved: {space_saved} bytes ({compression_percent:.1f}% reduction)")

def main():
    parser = argparse.ArgumentParser(
        description='Convert web files to gzipped C headers for ESP32'
    )
    parser.add_argument(
        'input_dir',
        type=str,
        help='Input directory containing web files'
    )
    parser.add_argument(
        '-o', '--output',
        type=str,
        default='lib/WebService',
        help='Output directory for generated headers (default: lib/WebService)'
    )
    parser.add_argument(
        '--no-minify',
        action='store_true',
        help='Disable minification'
    )

    args = parser.parse_args()

    input_dir = Path(args.input_dir)
    output_dir = Path(args.output)

    if not input_dir.exists():
        print(f"Error: Input directory '{input_dir}' does not exist")
        sys.exit(1)

    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"Converting web files from '{input_dir}' to C headers in '{output_dir}'")
    print(f"Minification: {'disabled' if args.no_minify else 'enabled'}\n")

    # Process all web files
    files_info = []
    for ext in ['*.html', '*.css', '*.js', '*.json']:
        for file_path in input_dir.rglob(ext):
            # Skip development files
            if 'mock' in str(file_path) or 'node_modules' in str(file_path):
                continue

            info = process_file(file_path, input_dir, minify=not args.no_minify)
            files_info.append(info)

    if not files_info:
        print("Warning: No files found to process")
        sys.exit(1)

    # Generate single header with all files
    print("\n" + "="*50)
    generate_single_header(files_info, output_dir)

    print("\n✅ Conversion complete!")
    print(f"\nInclude in your ESP32 code:")
    print(f'  #include "web_files.h"')

if __name__ == '__main__':
    main()
