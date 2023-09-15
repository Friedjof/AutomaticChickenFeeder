# ESP32 Makefile

# Define your project name here
PROJECT_NAME = Automatic chicken feeder

# Define your build environment (e.g., Arduino or PlatformIO)
BUILD_ENV = platformio

# Serial port for uploading (you may need to adjust this)
UPLOAD_PORT = /dev/ttyUSB0

# Serial baud rate (you may need to adjust this)
UPLOAD_SPEED = 115200

# Define the ESP32 board model (e.g., esp32, esp32dev, etc.)
BOARD = esp32dev

# Location of the SPIFFS filesystem directory
SPIFFS_DIR = data

# Location of PlatformIO build directory
BUILD_DIR = .pio/build/$(BOARD)

# Define the default target (upload)
.DEFAULT_GOAL := upload

# Targets
all: build

build:
ifeq ($(BUILD_ENV), platformio)
	pio run -e $(BOARD)
else
	@echo "Unsupported build environment: $(BUILD_ENV)"
endif

flash: build
ifeq ($(BUILD_ENV), platformio)
	pio run -t upload --upload-port=$(UPLOAD_PORT) -e $(BOARD)
else
	@echo "Unsupported build environment: $(BUILD_ENV)"
endif

clean:
ifeq ($(BUILD_ENV), platformio)
	pio run -t clean
else
	@echo "Unsupported build environment: $(BUILD_ENV)"
endif

fs:
ifeq ($(BUILD_ENV), platformio)
	pio run -t buildfs -e $(BOARD)
else
	@echo "Unsupported build environment: $(BUILD_ENV)"
endif

uploadfs: fs
ifeq ($(BUILD_ENV), platformio)
	pio run -t uploadfs -e $(BOARD)
else
	@echo "Unsupported build environment: $(BUILD_ENV)"
endif

monitor:
ifeq ($(BUILD_ENV), platformio)
	pio device monitor --port $(UPLOAD_PORT) --baud $(UPLOAD_SPEED)
else
	@echo "Unsupported build environment: $(BUILD_ENV)"
endif

reupload: uploadfs monitor

reload: flash monitor

start: uploadfs flash monitor

shell:
	nix-shell

help:
	@echo "Usage:"
	@echo "  make              - Build and upload firmware to ESP32"
	@echo "  make build        - Build the project"
	@echo "  make flash        - Upload firmware to ESP32"
	@echo "  make clean        - Clean build files"
	@echo "  make fs           - Build SPIFFS filesystem image"
	@echo "  make uploadfs     - Upload SPIFFS filesystem image"
	@echo "  make monitor      - Monitor serial output"
	@echo "  make start        - Upload filesystem and firmware, then monitor serial output"
	@echo "  make reupload     - Upload filesystem, then monitor serial output"
	@echo "  make reload       - Upload firmware, then monitor serial output"
	@echo "  make help         - Display this help message"
	@echo "  make shell        - Enter Nix shell"

.PHONY: all build upload clean fs uploadfs monitor help
