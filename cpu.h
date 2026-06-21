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
    uint8_t halted;
} CPU;

enum Opcode {
    NOP = 0x00,
    LD_B = 0x06,
    LD_C = 0x0E,
    LD_D = 0x16,
    LD_E = 0x1E,
    LD_H = 0x26,
    LD_L = 0x2E,
    LD_A = 0x3E,
    LD_PHL = 0x36,
    LD_BC = 0x01,
    LD_DE = 0x11,
    LD_HL = 0x21,
    LD_SP = 0x31,
    LD_BC_A = 0x02,
    LD_A_BC = 0x0A,
    LD_DE_A = 0x12,
    LD_A_DE = 0x1A,
    LD_HLP_A = 0x22,
    LD_A_HLP = 0x2A,
    LD_HLM_A = 0x32,
    LD_A_HLM = 0x3A,
    INC_BC = 0x03,
    INC_DE = 0x13,
    INC_HL_16 = 0x23,
    INC_SP_16 = 0x33,
    INC_A = 0x3C,
    INC_B = 0x04,
    INC_C = 0x0C,
    INC_D = 0x14,
    INC_E = 0x1C,
    INC_H = 0x24,
    INC_L = 0x2C,
    INC_ADDR_HL = 0x34,
    DEC_BC = 0x0B,
    DEC_DE = 0x1B,
    DEC_HL_16 = 0x2B,
    DEC_SP = 0x3B,
    DEC_A = 0x3D,
    DEC_B = 0x05,
    DEC_C = 0x0D,
    DEC_D = 0x15,
    DEC_E = 0x1D,
    DEC_H = 0x25,
    DEC_L = 0x2D,
    DEC_ADDR_HL = 0x35,
    XOR_AA = 0xAF,
    XOR_AB = 0xA8,
    XOR_AC = 0xA9,
    XOR_AD = 0xAA,
    XOR_AE = 0xAB,
    XOR_AH = 0xAC,
    XOR_AL = 0xAD,
    XOR_AHL = 0xAE,
    CP_B = 0xB8,
    CP_C = 0xB9,
    CP_D = 0xBA,
    CP_E = 0xBB,
    CP_H = 0xBC,
    CP_L = 0xBD,
    CP_HL = 0xBE,
    CP_A = 0xBF,
    CP_U8 = 0xFE,
    JP_NZ = 0xC2,
    JP = 0xC3,
    JP_Z = 0xCA,
    JP_NC = 0xD2,
    JP_C = 0xDA,
    JP_HL = 0xE9,
    JR = 0x18,
    JR_NZ = 0x20,
    JR_NC = 0x30,
    JR_Z = 0x28,
    JR_C = 0x38,
    PUSH_BC = 0xC5,
    PUSH_DE = 0xD5,
    PUSH_HL = 0xE5,
    PUSH_AF = 0xF5,
    POP_BC = 0xC1,
    POP_DE = 0xD1,
    POP_HL = 0xE1,
    POP_AF = 0xF1,
    HALT = 0x76,
};

enum { FLAG_Z = 0x80, FLAG_N = 0x40, FLAG_H = 0x20, FLAG_C = 0x10 };

extern CPU cpu;
extern uint8_t mmu[0x10000];

uint8_t read8(uint16_t addr);
void write8(uint16_t addr, uint8_t value);
uint8_t fetch8(void);
uint16_t fetch16(void);

void flag_clear(void);
void flag_set(uint8_t mask);
void flag_unset(uint8_t flag);
void flag_assign(uint8_t mask, int condition);
uint8_t flag_get(uint8_t flag);

int cpu_step(void);

void print_bits8(uint8_t n);
void print_flag_reg(void);

#endif
