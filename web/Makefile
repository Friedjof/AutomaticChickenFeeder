# Simple helper Makefile for the web UI (Vite)

.PHONY: install dev build preview build-esp clean

install:
	npm install

dev: install
	npm run dev -- --host --port 8000

build: install
	npm run build

preview: build
	npm run preview -- --host --port 8000

# Build and generate gzipped headers for the ESP firmware
build-esp: install
	npm run build:esp

clean:
	rm -rf dist node_modules
