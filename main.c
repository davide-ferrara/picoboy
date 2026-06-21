#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "cpu.h"

int main(void) {
    int fd = open("tetris.gb", O_RDONLY);
    uint8_t buf;
    uint16_t addr = 0;
    while (read(fd, &buf, 1) == 1 && addr < 0x7FFF) {
        mmu[addr++] = buf;
    }

    cpu.pc = 0x0100;
    cpu.sp = 0xFFFE;
    cpu.halted = 0;
    while (1) {
        printf("PC=%04X A=%02X F=%02X SP=%04X  ->  ", cpu.pc, cpu.a, cpu.f, cpu.sp);
        int cycles = cpu_step();
        printf("+%d cycles\n", cycles);
        usleep(100 * 1000);
    }
}
