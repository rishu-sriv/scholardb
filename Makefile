CMAKE     := /opt/homebrew/bin/cmake
BUILD_DIR := build

.PHONY: all build run run-server run-client open-browser test clean

all: build

build:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug --log-level=ERROR
	$(CMAKE) --build $(BUILD_DIR)

# Phase 1/2 — console menu
run: build
	./$(BUILD_DIR)/phase1_loader students.csv

# WebSocket server (loads students.csv, runs on :8080, Ctrl-C prints stats)
run-server: build
	./$(BUILD_DIR)/server

# C++ CLI client
run-client: build
	./$(BUILD_DIR)/client

# Browser UI (requires server already running)
open-browser:
	open view/index.html

# Catch2 test suite
test: build
	./$(BUILD_DIR)/run_tests

clean:
	rm -rf $(BUILD_DIR)
