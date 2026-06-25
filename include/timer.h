#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* Debug logging: silent unless -DPICOBOY_DEBUG is passed at compile time. */
#ifdef PICOBOY_DEBUG
#define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DBG(...) ((void)0)
#endif

void timer_step(uint16_t cycles);
void timer_div_reset(void);

uint16_t timer_div_read(void);

#endif // !PPU_H
