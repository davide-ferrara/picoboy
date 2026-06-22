#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
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
    while (1) {
        printf("PC=%04X A=%02X F=%02X SP=%04X  ->  ", cpu.pc, cpu.a, cpu.f, cpu.sp);
        cycles += cpu_step();
        ppu_step(cycles);
        printf("+%d ppu clock\n", ppu.clock);
        usleep(1000);
    }
}
