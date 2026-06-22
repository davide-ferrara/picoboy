#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "cpu.h"
#include "ppu.h"

int main(int argc, char **argv) {
    const char *romfile = argc > 1 ? argv[1] : "tetris.gb";
    int fd = open(romfile, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "Cannot open %s\n", romfile); return 1; }
    uint8_t buf;
    uint16_t addr = 0;
    while (read(fd, &buf, 1) == 1 && addr < 0x7FFF) {
        mmu[addr++] = buf;
    }
    close(fd);

    cpu.pc = 0x0100;
    cpu.sp = 0xFFFE;
    cpu.halted = 0;

    while (1) {
        uint16_t cycles = cpu_step();
        if (cycles == 0) break;
        ppu_step(cycles);
        print_framebuffer();
    }
    return 0;
}
