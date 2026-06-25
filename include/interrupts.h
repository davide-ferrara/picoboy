#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include "debug.h"

enum IFLAG {VBLANK = 0, LCD = 2, TIMER = 4, SERIAL = 8, JOYPAD = 16};

void set_if(uint8_t iflag);
void set_ie(uint8_t iflag);

#endif // !INTERRUPTS_H
