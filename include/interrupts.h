#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

/* Debug logging: silent unless -DPICOBOY_DEBUG is passed at compile time. */
#ifdef PICOBOY_DEBUG
#define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DBG(...) ((void)0)
#endif

enum IFLAG {VBLANK = 0, LCD = 2, TIMER = 4, SERIAL = 8, JOYPAD = 16};

void iflag_set(uint8_t iflag);

#endif // !PPU_H
