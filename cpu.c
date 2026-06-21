#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

CPU cpu = {0};
uint8_t mmu[0x10000];
uint8_t *reg[] = {&cpu.b, &cpu.c, &cpu.d, &cpu.e, &cpu.h, &cpu.l, NULL, &cpu.a};

enum { ADD = 0, ADC = 1, SUB = 2, SBC = 3, AND = 4, XOR = 5, OR = 6, CP = 7 };

uint8_t read8(uint16_t addr) {
    return mmu[addr];
}

void write8(uint16_t addr, uint8_t value) {
    if (addr < 0x8000) return;
    mmu[addr] = value;
}

uint8_t fetch8(void) { return read8(cpu.pc++); }

uint16_t fetch16(void) {
    uint8_t lo = fetch8();
    return (fetch8() << 8) | lo;
}

void flag_clear(void) {
    cpu.f = 0x00;
}

void flag_set(uint8_t mask) {
    cpu.f |= mask;
}

// 1001
// 1000 FLAG_Z
// 0111 NOT FLAG_Z
// ...
// 1001 
// 0111 AND
// 0001
void flag_unset(uint8_t flag) {
    cpu.f &= ~flag;
}

uint8_t flag_get(uint8_t flag) {
    return (cpu.f & flag) == flag ? 0x01 : 0x00;
}

void flag_assign(uint8_t mask, int condition) {
    if (condition) cpu.f |= mask;
}

int dec(uint8_t *reg) {
    uint8_t old = *reg;
    (*reg)--;
    flag_assign(FLAG_Z, *reg == 0);
    flag_set(FLAG_N);
    flag_assign(FLAG_H, (old & 0x0F) == 0x00);
    return 4;
}

int add(uint8_t val) {
    cpu.a += val;
    flag_assign(FLAG_Z, cpu.a == 0);
    flag_unset(FLAG_N);
    flag_assign(FLAG_H, (cpu.a & 0x0F) < (val & 0x0F));
    flag_assign(FLAG_C, cpu.a < val);
    return 4;
}

int adc(uint8_t val) {
    int carry = flag_get(FLAG_C);
    uint16_t res = (uint16_t)cpu.a + val + carry;
    flag_assign(FLAG_H, (cpu.a & 0x0F) + (val & 0x0F) + carry > 0x0F);
    flag_assign(FLAG_C, res > 0xFF);
    cpu.a = (uint8_t)res;
    flag_assign(FLAG_Z, cpu.a == 0);
    flag_unset(FLAG_N);
    return 4;
}

int sub(uint8_t val) {
    flag_assign(FLAG_H, (cpu.a & 0x0F) < (val & 0x0F));
    flag_assign(FLAG_C, cpu.a < val);
    cpu.a -= val;
    flag_assign(FLAG_Z, cpu.a == 0);
    flag_set(FLAG_N);
    return 4;
}

int sbc(uint8_t val) {
    int carry = flag_get(FLAG_C);
    flag_assign(FLAG_C, cpu.a < val + carry);
    flag_assign(FLAG_H, (cpu.a & 0x0F) < (val & 0x0F) + carry);
    cpu.a -= val + carry;
    flag_assign(FLAG_Z, cpu.a == 0);
    flag_set(FLAG_N);
    return 4;
}

int and8(uint8_t val) {
    cpu.a &= val;
    flag_assign(FLAG_Z, cpu.a == 0);
    flag_unset(FLAG_N);
    flag_set(FLAG_H);
    flag_unset(FLAG_C);
    return 4;
}

int xor8(uint8_t val) {
    cpu.a ^= val;
    flag_assign(FLAG_Z, cpu.a == 0);
    flag_unset(FLAG_N);
    flag_unset(FLAG_H);
    flag_unset(FLAG_C);
    return 4;
}

int or8(uint8_t val) {
    cpu.a |= val;
    flag_assign(FLAG_Z, cpu.a == 0);
    flag_unset(FLAG_N);
    flag_unset(FLAG_H);
    flag_unset(FLAG_C);
    return 4;
}

int cp(uint8_t val) {
    flag_assign(FLAG_Z, cpu.a == val);
    flag_set(FLAG_N);
    flag_assign(FLAG_H, (cpu.a & 0x0F) < (val & 0x0F));
    flag_assign(FLAG_C, cpu.a < val);
    return 4;
}

int jp_cond(int condition) {
    if (condition) {
        cpu.pc = fetch16();
        return 16;
    }
    fetch16();
    return 12;
}

int jr_cond(int condition) {
    int8_t offset = (int8_t)fetch8();
    if (condition) {
        cpu.pc += offset;
        return 12;
    }
    return 8;
}

int call_cond(int condition) {
    if (condition) {
        uint16_t target = fetch16();
        push(cpu.pc);
        cpu.pc = target;
        return 24;
    }
    fetch16();
    return 12;
}

int ret_cond(int condition) {
    if (condition) {
        pop(&cpu.pc);
        return 20;
    }
    return 8;
}

int push(uint16_t reg) {
    cpu.sp--;
    mmu[cpu.sp] = (uint8_t)(reg >> 8);
    cpu.sp--;
    mmu[cpu.sp] = (uint8_t)reg;
    return 16;
}

int pop(uint16_t *reg) {
    uint8_t low  = mmu[cpu.sp++];
    uint8_t high = mmu[cpu.sp++];
    *reg = ((uint16_t)high << 8) | low;
    return 12;
}

int halt(void) {
    cpu.halted = 1;
    return 4;
};

int ld_r_r(uint8_t op) {
    uint8_t src = op & 0x07; // 111
    uint8_t dst = (op >> 3) & 0x07;
    uint8_t val;

    if (src == 6) val = read8(cpu.hl);
    else val = *reg[src];

    if (dst == 6) { write8(cpu.hl, val); }
    else *reg[dst] = val;

    return (src == 6 || dst == 6) ? 8 : 4;
}

int alu_a(uint8_t op) {
    uint8_t src = op & 0x07;
    uint8_t op_t = (op >> 3) & 0x07;
    uint8_t val;

    if (src == 6) val = read8(cpu.hl);
    else val = *reg[src];

    switch (op_t) {
        case ADD: add(val); break;
        case ADC: adc(val); break;
        case SUB: sub(val); break;
        case SBC: sbc(val); break;
        case AND: and8(val); break;
        case XOR: xor8(val); break;
        case OR:  or8(val);  break;
        case CP:  cp(val);   break;
    }

    return (src == 6) ? 8 : 4;
}

int cpu_step(void) {
    if (cpu.halted) return 4;
    uint8_t op = fetch8();

    switch (op) {
        case NOP:
            return 4;
        case LD_B:
            cpu.b = fetch8();
            return 8;
        case LD_C:
            cpu.c = fetch8();
            return 8;
        case LD_D:
            cpu.d = fetch8();
            return 8;
        case LD_E:
            cpu.e = fetch8();
            return 8;
        case LD_H:
            cpu.h = fetch8();
            return 8;
        case LD_L:
            cpu.l = fetch8();
            return 8;
        case LD_A:
            cpu.a = fetch8();
            return 8;
        case LD_PHL:
            write8(cpu.hl, fetch8());
            return 12;
        case LD_BC:
            cpu.bc = fetch16();
            return 12;
        case LD_DE:
            cpu.de = fetch16();
            return 12;
        case LD_HL:
            cpu.hl = fetch16();
            return 12;
        case LD_SP:
            cpu.sp = fetch16();
            return 12;
        case LD_BC_A:
            write8(cpu.bc, cpu.a);
            return 8;
        case LD_A_BC:
            cpu.a = read8(cpu.bc);
            return 8;
        case LD_DE_A:
            write8(cpu.de, cpu.a);
            return 8;
        case LD_A_DE:
            cpu.a = read8(cpu.de);
            return 8;
        case LD_HLP_A:
            write8(cpu.hl, cpu.a);
            cpu.hl++;
            return 8;
        case LD_A_HLP:
            cpu.a = read8(cpu.hl);
            cpu.hl++;
            return 8;
        case LD_HLM_A:
            write8(cpu.hl, cpu.a);
            cpu.hl--;
            return 8;
        case LD_A_HLM:
            cpu.a = read8(cpu.hl);
            cpu.hl--;
            return 8;
        case INC_A: {
            uint8_t old = cpu.a;
            cpu.a++;
            flag_assign(FLAG_Z, old == 0xFF);
            flag_unset(FLAG_N);
            flag_assign(FLAG_H, (old & 0x0F) == 0x0F);
            return 4;
        }
        case INC_B: {
            uint8_t old = cpu.b;
            cpu.b++;
            flag_assign(FLAG_Z, old == 0xFF);
            flag_assign(FLAG_H, (old & 0x0F) == 0x0F);
            return 4;
        }
        case INC_C: {
            uint8_t old = cpu.c;
            cpu.c++;
            flag_assign(FLAG_Z, old == 0xFF);
            flag_assign(FLAG_H, (old & 0x0F) == 0x0F);
            return 4;
        }
        case INC_D: {
            uint8_t old = cpu.d;
            cpu.d++;
            flag_assign(FLAG_Z, old == 0xFF);
            flag_assign(FLAG_H, (old & 0x0F) == 0x0F);
            return 4;
        }
        case INC_E: {
            uint8_t old = cpu.e;
            cpu.e++;
            flag_assign(FLAG_Z, old == 0xFF);
            flag_assign(FLAG_H, (old & 0x0F) == 0x0F);
            return 4;
        }
        case INC_H: {
            uint8_t old = cpu.h;
            cpu.h++;
            flag_assign(FLAG_Z, old == 0xFF);
            flag_assign(FLAG_H, (old & 0x0F) == 0x0F);
            return 4;
        }
        case INC_L: {
            uint8_t old = cpu.l;
            cpu.l++;
            flag_assign(FLAG_Z, old == 0xFF);
            flag_assign(FLAG_H, (old & 0x0F) == 0x0F);
            return 4;
        }
        case INC_ADDR_HL: { // Incr value at addr hl
            uint8_t val = read8(cpu.hl);
            uint8_t old = val;
            val++;
            write8(cpu.hl, val);
            flag_assign(FLAG_Z, old == 0xFF);
            flag_assign(FLAG_H, (old & 0x0F) == 0x0F);
            flag_unset(FLAG_N);
            return 12;
        }
        case INC_BC:
            cpu.bc++;
            return 8;
        case INC_DE:
            cpu.de++;
            return 8;
        case INC_HL_16:
            cpu.hl++;
            return 8;
        case INC_SP_16:
            cpu.sp++;
            return 8;
        case DEC_A:
            return dec(&cpu.a);
        case DEC_B:
            return dec(&cpu.b);
        case DEC_C:
            return dec(&cpu.c);
        case DEC_D:
            return dec(&cpu.d);
        case DEC_E:
            return dec(&cpu.e);
        case DEC_H:
            return dec(&cpu.h);
        case DEC_L:
            return dec(&cpu.l);
        case DEC_BC:
            cpu.bc--;
            return 8;
        case DEC_DE:
            cpu.de--;
            return 8;
        case DEC_HL_16:
            cpu.hl--;
            return 8;
        case DEC_SP:
            cpu.sp--;
            return 8;
        case DEC_ADDR_HL: {
            uint8_t old = read8(cpu.hl);
            uint8_t val = old - 1;
            write8(cpu.hl, val);
            flag_assign(FLAG_Z, val == 0);
            flag_set(FLAG_N);
            flag_assign(FLAG_H, (old & 0x0F) == 0);
            return 12;
        }
        case CP_U8:
            cp(fetch8());
            return 8;
        case PUSH_BC:
            return push(cpu.bc);
        case PUSH_DE:
            return push(cpu.de);
        case PUSH_HL:
            return push(cpu.hl);
        case PUSH_AF:
            return push(cpu.af);
        case POP_BC:
            return pop(&cpu.bc);
        case POP_DE:
            return pop(&cpu.de);
        case POP_HL:
            return pop(&cpu.hl);
        case POP_AF:
            pop(&cpu.af);
            cpu.f &= 0xF0;
            return 12;
        case JP:
            cpu.pc = fetch16();
            return 16;
        case JP_NZ:
            return jp_cond(flag_get(FLAG_Z) == 0);
        case JP_Z:
            return jp_cond(flag_get(FLAG_Z) == 1);
        case JP_NC:
            return jp_cond(flag_get(FLAG_C) == 0);
        case JP_C:
            return jp_cond(flag_get(FLAG_C) == 1);
        case JP_HL:
            cpu.pc = cpu.hl;
            return 4;
        case JR:
            return jr_cond(1);
        case JR_NZ:
            return jr_cond(flag_get(FLAG_Z) == 0);
        case JR_NC:
            return jr_cond(flag_get(FLAG_C) == 0);
        case JR_Z:
            return jr_cond(flag_get(FLAG_Z) == 1);
        case JR_C:
            return jr_cond(flag_get(FLAG_C) == 1);
        case CALL_NZ:
            return call_cond(flag_get(FLAG_Z) == 0);
        case CALL_Z:
            return call_cond(flag_get(FLAG_Z) == 1);
        case CALL_C:
            return call_cond(flag_get(FLAG_C) == 1);
        case CALL_NC:
            return call_cond(flag_get(FLAG_C) == 0);
        case RET_NZ:
            return ret_cond(flag_get(FLAG_Z) == 0);
        case RET_Z:
            return ret_cond(flag_get(FLAG_Z) == 1);
        case RET_NC:
            return ret_cond(flag_get(FLAG_C) == 0);
        case RET_C:
            return ret_cond(flag_get(FLAG_C) == 1);
        case HALT:
            return halt();
        default:
            if (op >= 0x40 && op <= 0x7F)
                return ld_r_r(op);
            if (op >= 0x80 && op <= 0xBF)
                return alu_a(op);
            printf("Invalid opcode: %04X\n", op);
            return 0;
    }
}

void print_bits8(uint8_t n) {
    for (int i = 7; i >= 0; i--) {
        printf("%X ", (n >> i) & 0x01);
    }
    printf("\n");
}

void print_flag_reg(void) {
    print_bits8(cpu.f);
}
