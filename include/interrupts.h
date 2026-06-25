#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include "debug.h"

enum IFLAG {VBLANK = 0, LCD = 2, TIMER = 4, SERIAL = 8, JOYPAD = 16};

void if_set(uint8_t iflag);
void ie_set(uint8_t iflag);

#endif // !INTERRUPTS_H
