.DEFAULT_GOAL := emu
.PHONY: all build flash clean monitor bootsel viewer emu test

CC       := gcc
CFLAGS   := -Wall -Wextra -std=c11 -g -O2 -D_DEFAULT_SOURCE
LDFLAGS  :=

PICO_SDK_PATH ?= $(HOME)/pico-sdk
export PICO_SDK_PATH

BUILD_DIR := build
BIN_DIR   := bin

all: emu

# --- Host emulator ---
emu: $(BIN_DIR)/gbemu

$(BIN_DIR)/gbemu: cpu.c cpu.h ppu.h main.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ cpu.c ppu.c main.c $(LDFLAGS)

# --- Host tests ---
test: $(BIN_DIR)/test
	./$(BIN_DIR)/test

$(BIN_DIR)/test: cpu.c cpu.h test.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ cpu.c test.c $(LDFLAGS)

$(BIN_DIR):
	mkdir -p $@

# --- Pico firmware ---
build:
	@test -d "$(PICO_SDK_PATH)" || { echo "Error: PICO_SDK_PATH=$(PICO_SDK_PATH) not found"; exit 1; }
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake .. -DPICO_PLATFORM=rp2350 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make -j$$(nproc)
	ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json

flash: build
	picotool load -F -x $(BUILD_DIR)/hello.uf2

bootsel:
	picotool reboot -f

monitor:
	screen /dev/ttyACM0 115200

# --- Viewer (raylib) ---
viewer: viewer/viewer

viewer/viewer: viewer/viewer.c
	$(CC) -O2 $< -o $@ -lraylib -lm

# --- Clean ---
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f compile_commands.json
