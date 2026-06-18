CMAKE     := /opt/homebrew/bin/cmake
BUILD_DIR := build

.PHONY: all build run run-server run-client clean

all: build

build:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug --log-level=ERROR
	$(CMAKE) --build $(BUILD_DIR)

# Phase 1/2 — console menu (CSV load + repository CRUD)
run: build
	./$(BUILD_DIR)/phase1_loader students.csv

# Phase 3 — bare WebSocket echo server
run-server: build
	./$(BUILD_DIR)/server

# Phase 5 — C++ CLI client (sends one hardcoded message, prints echo)
run-client: build
	./$(BUILD_DIR)/client

clean:
	rm -rf $(BUILD_DIR)
