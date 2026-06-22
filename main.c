#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "cpu.h"
#include "ppu.h"

int main(void) {
    int fd = open("tetris.gb", O_RDONLY);
    uint8_t buf;
    uint16_t addr = 0;
    while (read(fd, &buf, 1) == 1 && addr < 0x7FFF) {
        mmu[addr++] = buf;
    }

    uint16_t cycles = 0;
    cpu.pc = 0x0100;
    cpu.sp = 0xFFFE;
    cpu.halted = 0;
    int count = 0;
    while (1) {
        cycles = cpu_step();
        ppu_step(cycles);
        print_framebuffer();
        count++;
        if (count % 500000 == 0) {
            fprintf(stderr, "[%dM] PC=%04X LY=%d LCDC=%02X BGP=%02X\n",
                   count/1000000, cpu.pc, ppu.reg[4], ppu.reg[0], ppu.reg[7]);
            fflush(stderr);
        }
    }
}
