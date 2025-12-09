# Makefile für PlatformIO (ESP32-C3, Ubuntu /dev/ttyACM<N>)

PLATFORMIO ?= pio
BOARD ?= esp32c3

# Optionales "Argument" nach flash/monitor/run (z. B. "make flash 1")
ACTION_TARGETS := flash monitor run
ifneq ($(filter $(ACTION_TARGETS),$(firstword $(MAKECMDGOALS))),)
  ARG := $(word 2,$(MAKECMDGOALS))
  ifneq ($(ARG),)
    NR := $(ARG)
    # Dummy-Ziel erzeugen, damit die Zahl (z. B. "1") kein echtes Target ist
    $(eval $(ARG):;@:)
  endif
endif

# Optionale Flags je nach NR
ifdef NR
  UPLOAD_FLAG := --upload-port /dev/ttyACM$(NR)
  MONITOR_FLAG := --port /dev/ttyACM$(NR)
else
  UPLOAD_FLAG :=
  MONITOR_FLAG :=
endif

.PHONY: all build flash monitor run clean list deploy-web deploy-fs deploy-flash web-headers

all: build

build: web-headers
	$(PLATFORMIO) run --environment $(BOARD)

# make flash        -> ohne --upload-port (auto-detect)
# make flash 1      -> Upload auf /dev/ttyACM1
flash:
	$(PLATFORMIO) run --target upload --environment $(BOARD) $(UPLOAD_FLAG)

# make monitor      -> ohne --port (auto-detect)
# make monitor 2    -> Monitor auf /dev/ttyACM2
monitor:
	$(PLATFORMIO) device monitor --environment $(BOARD) $(MONITOR_FLAG)

# make run          -> flash danach monitor (ohne Port)
# make run 1        -> flash/monitor auf /dev/ttyACM1
run: build flash monitor

clean:
	$(PLATFORMIO) run --target clean --environment $(BOARD)

# Web Interface Deployment Targets
# =================================

# Convert web files to gzipped C headers (embedded in firmware)
web-headers:
	@echo "🐔 Building web UI (Vite) ..."
	@cd web && npm install
	@cd web && npm run build
	@echo "🐔 Converting web files to gzipped C headers..."
	@python3 scripts/web-to-header.py web/dist -o lib/WebService/generated
	@echo "✅ Headers generated in lib/WebService/generated/"
	@echo "   Include with: #include \"web_files.h\""

# Build Docker image for web optimization (only when needed)
build-web-image:
	@echo "🐔 Building ESP32 web optimization container..."
	docker build -t esp32-web-builder ./build/

# Deploy web interface from /web/ to /data-template/ (professional optimization)
deploy-web: build-web-image
	@echo "🚀 Starting professional web optimization pipeline..."
	docker run --rm -v $(PWD):/workspace esp32-web-builder

# Legacy deployment (simple bash minification)
deploy-web-simple:
	@echo "🐔 Deploying web interface for ESP32 (simple)..."
	./scripts/deploy-web.sh

# Upload filesystem (data-template/) to ESP32
deploy-fs:
	@echo "📁 Uploading filesystem to ESP32..."
	$(PLATFORMIO) run --target uploadfs --environment $(BOARD) $(UPLOAD_FLAG)

# Deploy web interface and flash ESP32 with firmware + filesystem
deploy-flash: deploy-web build flash deploy-fs
	@echo "🚀 Complete deployment finished!"
	@echo "✅ Firmware flashed"
	@echo "✅ Web interface deployed" 
	@echo "✅ Filesystem uploaded"

# make list         -> nur ESP-Geräte auf /dev/ttyACM<N> mit Nummern (ohne Duplikate)
list:
	@echo "NR  PORT          DESCRIPTION"
	@echo "--- ------------- --------------------------------------------------"
	@$(PLATFORMIO) device list --json-output | jq -r 'map(select(((.hwid // "") | test("VID:PID=303A:", "i")) or ((.description // "") | test("Espressif|USB JTAG/serial", "i")))) | map(select(.port | test("^/dev/ttyACM[0-9]+"))) | unique_by(.port) | .[] | (.port | capture("ACM(?<n>[0-9]+)").n) + "   " + .port + "  " + (.description // "")'

.PHONY: release
release:
	@if [ -z "$(VERSION)" ]; then echo "❌ VERSION env var required (e.g. make release VERSION=v2.0.0)"; exit 1; fi
	@echo "🐔 Starting automated release $(VERSION)..."
	@echo ""
	@echo "📝 Step 1/5: Updating version files..."
	@echo "// $(VERSION)" > VERSION
	@cd web && npm version --no-git-tag-version --allow-same-version $${VERSION#v}
	@echo "✅ Version files updated"
	@echo ""
	@echo "🌐 Step 2/5: Building web interface with new version..."
	@$(MAKE) web-headers
	@echo "✅ Web interface built and embedded"
	@echo ""
	@echo "📦 Step 3/5: Committing release..."
	@git add VERSION web/package.json web/package-lock.json
	@git add -f lib/WebService/generated
	@git commit -m "Release $(VERSION)" || (echo "⚠️  No changes to commit"; true)
	@echo "✅ Release committed"
	@echo ""
	@echo "🏷️  Step 4/5: Creating and pushing tag..."
	@if git rev-parse $(VERSION) >/dev/null 2>&1; then \
		echo "⚠️  Tag $(VERSION) already exists, deleting old tag..."; \
		git tag -d $(VERSION); \
		git push origin :refs/tags/$(VERSION) 2>/dev/null || true; \
	fi
	@git tag -a $(VERSION) -m "Release $(VERSION)"
	@echo "✅ Tag $(VERSION) created"
	@echo ""
	@echo "🚀 Step 5/5: Pushing to GitHub..."
	@BRANCH=$$(git rev-parse --abbrev-ref HEAD); \
	echo "📤 Pushing branch: $$BRANCH"; \
	git push origin $$BRANCH; \
	echo "📤 Pushing tag: $(VERSION)"; \
	git push origin $(VERSION); \
	REPO=$$(git config --get remote.origin.url | sed 's/.*github.com[:\/]\(.*\)\.git/\1/'); \
	echo ""; \
	echo "✅ ✅ ✅ Release $(VERSION) completed! ✅ ✅ ✅"; \
	echo ""; \
	echo "🔗 GitHub Actions: https://github.com/$$REPO/actions"; \
	echo "🔗 Releases: https://github.com/$$REPO/releases"; \
	echo ""; \
	echo "⏳ The release build will take ~5-10 minutes"; \
	echo "📦 Artifacts: firmware-$(VERSION).bin, firmware-$(VERSION).elf"
