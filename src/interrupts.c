#include "interrupts.h"
#include "cpu.h"
#include <stdint.h>

/* Set the specified interrupt flag at IF 0xFF0F*/
void set_if(uint8_t iflag) {
    mmu[0xFF0F] |= iflag;
}

/* Enable the specified interrupt at IE 0xFFFF */
void set_ie(uint8_t iflag) {
    mmu[0xFFFF] |= iflag;
}
