.PHONY: build flash clean monitor bootsel viewer

export PICO_SDK_PATH := $(HOME)/pico-sdk
BUILD_DIR := build

build:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake .. -DPICO_PLATFORM=rp2350 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make -j$$(nproc)
	ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json

bootsel:
	picotool reboot -f

flash: build
	picotool load -F -x $(BUILD_DIR)/hello.uf2

viewer:
	gcc -O2 viewer/viewer.c -o viewer/viewer -lraylib -lm

monitor:
	screen /dev/ttyACM0 115200

emu: bin/gbemu

bin/gbemu: cpu.c cpu.h main.c
	mkdir -p bin && gcc -o $@ cpu.c main.c

test: bin/test
	./bin/test

bin/test: cpu.c cpu.h test.c
	mkdir -p bin && gcc -o $@ cpu.c test.c

clean:
	rm -rf $(BUILD_DIR) viewer bin
