CMAKE     := /opt/homebrew/bin/cmake
BUILD_DIR := build

.PHONY: all build run run-server run-client open-browser clean

all: build

build:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug --log-level=ERROR
	$(CMAKE) --build $(BUILD_DIR)

# Phase 1/2 — console menu (CSV load + repository CRUD)
run: build
	./$(BUILD_DIR)/phase1_loader students.csv

# Phase 3+ — WebSocket server (loads students.csv, runs on :8080)
run-server: build
	./$(BUILD_DIR)/server

# Phase 5+ — C++ CLI client
run-client: build
	./$(BUILD_DIR)/client

# Phase 10 — open browser UI (requires server already running)
open-browser:
	open view/index.html

clean:
	rm -rf $(BUILD_DIR)
