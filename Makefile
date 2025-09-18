PIO ?= pio
ENV ?= esp32c6
DATA_TEMPLATE := data-template
DATA_DIR := data
PIO_HOME := $(CURDIR)/.pio-home
PIO_CMD := PLATFORMIO_HOME_DIR=$(PIO_HOME) $(PIO)

.PHONY: all build upload uploadfs test monitor clean data-init setup

all: build

build:
	$(PIO_CMD) run -e $(ENV)

upload:
	$(PIO_CMD) run -e $(ENV) -t upload

uploadfs:
	$(PIO_CMD) run -e $(ENV) -t uploadfs

monitor:
	$(PIO_CMD) device monitor -e $(ENV)

clean:
	$(PIO_CMD) run -e $(ENV) -t clean

# Copy versioned template into working LittleFS directory

data-init:
	@if [ ! -d $(DATA_TEMPLATE) ]; then \
		echo "$(DATA_TEMPLATE) not found" >&2; exit 1; \
	fi
	@if [ -d $(DATA_DIR) ]; then \
		echo "$(DATA_DIR) already exists"; \
	else \
		cp -R $(DATA_TEMPLATE) $(DATA_DIR); \
		echo "Created $(DATA_DIR) from template"; \
	fi

# Run Unity tests (host or device depending on env configuration)
test:
	$(PIO_CMD) test -e $(ENV)

setup: scripts/setup.sh
	bash scripts/setup.sh
