#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct {
    union { uint16_t af; struct { uint8_t f; uint8_t a; }; };
    union { uint16_t bc; struct { uint8_t c; uint8_t b; }; };
    union { uint16_t de; struct { uint8_t e; uint8_t d; }; };
    union { uint16_t hl; struct { uint8_t l; uint8_t h; }; };
    uint16_t sp;
    uint16_t pc;
    uint8_t ime;    // Interrupt master enable
    uint8_t ime_pending; // Delay IME by 1 instruction after EI/RETI
    uint8_t halted;
} CPU;

enum Opcode {
    NOP             = 0x00,
    STOP            = 0x10,
    JP_NZ           = 0xC2,
    JP              = 0xC3,
    JP_Z            = 0xCA,
    JP_NC           = 0xD2,
    JP_C            = 0xDA,
    JP_HL           = 0xE9,
    JR              = 0x18,
    JR_NZ           = 0x20,
    JR_NC           = 0x30,
    JR_Z            = 0x28,
    JR_C            = 0x38,
    CALL            = 0xCD,
    CALL_NZ         = 0xC4,
	CALL_Z          = 0xCC,
	CALL_C          = 0xDC,
	CALL_NC         = 0xD4,
    RET             = 0xC9,
	RET_NZ          = 0xC0,
	RET_Z           = 0xC8,
	RET_NC          = 0xD0,
	RET_C           = 0xD8,
    HALT            = 0x76,
    EI              = 0xFB,
    DI              = 0xF3,
    RETI            = 0xD9,
    LDH_IO_A        = 0xE0,
    LDH_A_IO        = 0xF0,
    CB              = 0xCB,
};

enum { FLAG_Z = 0x80, FLAG_N = 0x40, FLAG_H = 0x20, FLAG_C = 0x10 };

extern CPU      cpu;
extern uint8_t  mmu[0x10000];
extern uint8_t  *reg[];
extern uint16_t *reg16[];
extern uint16_t *reg16_stk[];

uint8_t  read8(uint16_t addr);
void     write8(uint16_t addr, uint8_t value);
uint8_t  fetch8(void);
uint16_t fetch16(void);
int      push(uint16_t reg);
int      pop(uint16_t *reg);

void    flag_clear(void);
void    flag_set(uint8_t mask);
void    flag_unset(uint8_t flag);
void    flag_assign(uint8_t mask, int condition);
uint8_t flag_get(uint8_t flag);

int cpu_step(void);

void print_bits8(uint8_t n);
void print_flag_reg(void);

#endif
