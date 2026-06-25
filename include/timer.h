#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "debug.h"

void timer_step(uint16_t cycles);
void timer_div_reset(void);

uint8_t timer_div_read(void);

#endif // !TIMER_H
