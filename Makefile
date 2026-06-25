.DEFAULT_GOAL := emu
.PHONY: all emu gbtest test-blargg test-roms pico build flash bootsel monitor clean compdb

CC       := gcc
CFLAGS   := -Wall -Wextra -std=c11 -g -O2 -D_DEFAULT_SOURCE
# Silence pedantic/stylistic warnings (mainly from clang -Weverything):
#  - padded: struct alignment padding (CPU unions need specific layout)
#  - declaration-after-statement: mixing decls/code is fine in C11
#  - switch-default: not every switch needs a default case
CFLAGS   += -Wno-padded -Wno-declaration-after-statement -Wno-switch-default
# clang 22+ only: -Wno-unsafe-buffer-usage (very noisy on raw array indexing like mmu[]).
# gcc doesn't recognize this flag, so only add it when CC is clang.
ifneq (,$(findstring clang,$(CC)))
CFLAGS   += -Wno-unsafe-buffer-usage
endif
CPPFLAGS := -Iinclude
LDFLAGS  := -lm -lraylib
LDFLAGS_TEST := -lm

PICO_SDK_PATH ?= $(HOME)/pico-sdk
export PICO_SDK_PATH

BUILD       := build
BUILD_PICO  := build-pico
TEST_ROMS_DIR := test-roms

# --- Sources ---
CORE_SRCS := src/cpu.c src/ppu.c src/timer.c src/debug.c src/interrupts.c
EMU_SRCS  := src/main.c
TEST_SRCS := src/main_test.c

CORE_OBJS := $(patsubst src/%.c,$(BUILD)/%.o,$(CORE_SRCS))
EMU_OBJS  := $(patsubst src/%.c,$(BUILD)/%.o,$(EMU_SRCS))
TEST_OBJS := $(patsubst src/%.c,$(BUILD)/%.o,$(TEST_SRCS))
DEPS      := $(CORE_OBJS:$(BUILD)/%.o=$(BUILD)/%.d) $(EMU_OBJS:$(BUILD)/%.o=$(BUILD)/%.d) $(TEST_OBJS:$(BUILD)/%.o=$(BUILD)/%.d)

all: emu

# --- Host emulator (raylib) ---
emu: $(BUILD)/gbemu

$(BUILD)/gbemu: $(CORE_OBJS) $(EMU_OBJS) | $(BUILD)
	$(CC) $^ -o $@ $(LDFLAGS)

# --- Blargg test harness (headless) ---
gbtest: $(BUILD)/gbtest

$(BUILD)/gbtest: $(CORE_OBJS) $(TEST_OBJS) | $(BUILD)
	$(CC) $^ -o $@ $(LDFLAGS_TEST)

# --- Blargg test suite ---
test-roms:
	@test -d $(TEST_ROMS_DIR)/cpu_instrs || { \
		echo "test-roms/ not initialized. Run:"; \
		echo "  git submodule update --init --recursive"; \
		exit 1; }

test-blargg: gbtest test-roms
	@tools/run_blargg.sh $(TEST_ROMS_DIR)/cpu_instrs/individual

# --- Compilation rule with auto-generated header dependencies ---
$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(BUILD):
	mkdir -p $@

# --- Compile database for clangd/LSP ---
compdb:
	$(MAKE) clean
	bear -- $(MAKE) gbtest

# --- Pico firmware (CMake, separate build dir) ---
pico:
	@test -d "$(PICO_SDK_PATH)" || { echo "Error: PICO_SDK_PATH=$(PICO_SDK_PATH) not found"; exit 1; }
	mkdir -p $(BUILD_PICO) && cd $(BUILD_PICO) && cmake .. -DPICO_PLATFORM=rp2350 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make -j$$(nproc)
	ln -sf $(BUILD_PICO)/compile_commands.json compile_commands.json

flash: pico
	picotool load -F -x $(BUILD_PICO)/hello.uf2

bootsel:
	picotool reboot -f

monitor:
	screen /dev/ttyACM0 115200

# --- Clean ---
clean:
	rm -rf $(BUILD) $(BUILD_PICO) bin
	rm -f compile_commands.json

-include $(DEPS)
