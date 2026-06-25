#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdint.h>

/* Debug logging: silent unless -DPICOBOY_DEBUG is passed at compile time.
 * Single source of truth for the DBG() macro used across the codebase. */
#ifdef PICOBOY_DEBUG
#define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DBG(...) ((void)0)
#endif

/* Pretty-printing helpers for debugging. */
void print_bits8(uint8_t n);
void print_flag_reg(void);

#endif // !DEBUG_H
