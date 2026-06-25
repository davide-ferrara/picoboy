#include "interrupts.h"
#include "cpu.h"
#include <stdint.h>

/* Set the specified interrupt flag at IF 0xFF0F*/
void if_set(uint8_t iflag) {
    mmu[0xFF0F] |= iflag;
}

/* Enable the specified interrupt at IE 0xFFFF */
void ie_set(uint8_t iflag) {
    mmu[0xFFFF] |= iflag;
}
