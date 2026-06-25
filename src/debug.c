#include "debug.h"
#include "cpu.h"
#include <stdio.h>

void print_bits8(uint8_t n) {
    for (int i = 7; i >= 0; i--) {
        printf("%X ", (unsigned int)((n >> i) & 0x01));
    }
    printf("\n");
}

void print_flag_reg(void) {
    print_bits8(cpu.f);
}
