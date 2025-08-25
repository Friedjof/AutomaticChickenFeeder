# ESP-IDF Makefile for AutomaticChickenFeeder
# Requires ESP-IDF v5.5 environment to be set up

# ESP-IDF environment setup
IDF_PATH := /home/friedjof/.esp/v5.5/esp-idf
SHELL := /bin/bash

# Default target
.DEFAULT_GOAL := build

# Help target
.PHONY: help
help:
	@echo "AutomaticChickenFeeder - ESP-IDF Build System"
	@echo "Available targets:"
	@echo "  build     - Build the project"
	@echo "  flash     - Flash firmware to ESP32-C6"
	@echo "  monitor   - Open serial monitor"
	@echo "  run       - Build + Flash + Monitor (complete workflow)"
	@echo "  clean     - Clean build directory"
	@echo "  fullclean - Full clean (including dependencies)"
	@echo "  menuconfig - Open ESP-IDF configuration menu"
	@echo "  size      - Show binary size information"
	@echo "  erase     - Erase entire flash"
	@echo "  bootloader - Build bootloader only"
	@echo ""
	@echo "Hardware Setup:"
	@echo "  Connect DS3231 RTC: VCC→3V3, GND→GND, SDA→D4(GPIO5), SCL→D5(GPIO6)"
	@echo "  Insert CR2032 battery in DS3231 for time backup"
	@echo ""
	@echo "Usage Examples:"
	@echo "  make build      # Build project"
	@echo "  make run        # Complete development cycle"
	@echo "  make flash PORT=/dev/ttyUSB0  # Flash to specific port"

# Build project
.PHONY: build
build:
	@echo "Building AutomaticChickenFeeder..."
	@source $(IDF_PATH)/export.sh && idf.py build
	@echo "✓ Build completed successfully"

# Flash firmware
.PHONY: flash
flash:
	@echo "Flashing firmware to ESP32-C6..."
ifdef PORT
	@source $(IDF_PATH)/export.sh && idf.py -p $(PORT) flash
else
	@source $(IDF_PATH)/export.sh && idf.py flash
endif
	@echo "✓ Flash completed successfully"

# Open serial monitor
.PHONY: monitor
monitor:
	@echo "Opening serial monitor... (Ctrl+] to exit)"
ifdef PORT
	@source $(IDF_PATH)/export.sh && idf.py -p $(PORT) monitor
else
	@source $(IDF_PATH)/export.sh && idf.py monitor
endif

# Complete workflow: Build + Flash + Monitor
.PHONY: run
run: build flash monitor

# Clean build directory
.PHONY: clean
clean:
	@echo "Cleaning build directory..."
	@source $(IDF_PATH)/export.sh && idf.py clean
	@echo "✓ Clean completed"

# Full clean including dependencies
.PHONY: fullclean
fullclean:
	@echo "Performing full clean..."
	@source $(IDF_PATH)/export.sh && idf.py fullclean
	@echo "✓ Full clean completed"

# Open menuconfig
.PHONY: menuconfig
menuconfig:
	@echo "Opening ESP-IDF configuration menu..."
	@source $(IDF_PATH)/export.sh && idf.py menuconfig

# Show binary size information
.PHONY: size
size:
	@echo "Binary size information:"
	@source $(IDF_PATH)/export.sh && idf.py size
	@source $(IDF_PATH)/export.sh && idf.py size-components

# Erase flash
.PHONY: erase
erase:
	@echo "Erasing entire flash... (This will remove all data!)"
	@read -p "Are you sure? (y/N): " confirm && [ "$$confirm" = "y" ] || exit 1
ifdef PORT
	@source $(IDF_PATH)/export.sh && idf.py -p $(PORT) erase-flash
else
	@source $(IDF_PATH)/export.sh && idf.py erase-flash
endif
	@echo "✓ Flash erased"

# Build bootloader only
.PHONY: bootloader
bootloader:
	@echo "Building bootloader..."
	@source $(IDF_PATH)/export.sh && idf.py bootloader
	@echo "✓ Bootloader build completed"

# Development shortcuts
.PHONY: quick-test
quick-test:
	@echo "Quick test: Build and flash without monitor"
	@source $(IDF_PATH)/export.sh && idf.py build flash

# Show project info
.PHONY: info
info:
	@echo "Project Information:"
	@echo "  Project: AutomaticChickenFeeder"
	@echo "  Target: ESP32-C6 (Seeed XIAO)"
	@echo "  IDF Path: $(IDF_PATH)"
	@echo "  Components: feeding, clock (DS3231 RTC)"
	@echo ""
	@echo "Hardware Configuration:"
	@echo "  Servos: GPIO16 (D6), GPIO17 (D7)"
	@echo "  Button: GPIO1 (D1)"
	@echo "  Servo Power: GPIO20 (D9) via S8550 transistor"
	@echo "  RTC SDA: GPIO5 (D4)"
	@echo "  RTC SCL: GPIO6 (D5)"

# Validate environment
.PHONY: check-env
check-env:
	@echo "Checking ESP-IDF environment..."
	@if [ ! -d "$(IDF_PATH)" ]; then \
		echo "✗ ESP-IDF not found at $(IDF_PATH)"; \
		exit 1; \
	fi
	@source $(IDF_PATH)/export.sh && python --version
	@source $(IDF_PATH)/export.sh && idf.py --version
	@echo "✓ Environment check passed"

# Create backup of current firmware
.PHONY: backup
backup:
	@echo "Creating firmware backup..."
	@mkdir -p backups
	@cp -r build backups/build_$(shell date +%Y%m%d_%H%M%S) 2>/dev/null || true
	@echo "✓ Backup created in backups/ directory"

# Show serial devices
.PHONY: ports
ports:
	@echo "Available serial ports:"
	@ls /dev/tty* | grep -E "(USB|ACM)" || echo "No USB serial devices found"
	@echo ""
	@echo "Usage: make flash PORT=/dev/ttyUSB0"