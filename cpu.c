#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

CPU cpu = {0};
uint8_t mmu[0x10000];
uint8_t *reg[] = {&cpu.b, &cpu.c, &cpu.d, &cpu.e, &cpu.h, &cpu.l, NULL, &cpu.a};
uint16_t *reg16[] = {&cpu.bc, &cpu.de, &cpu.hl, &cpu.sp};
uint16_t *reg16_stk[] = {&cpu.bc, &cpu.de, &cpu.hl, &cpu.af};

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

void flag_unset(uint8_t flag) {
    cpu.f &= ~flag;
}

uint8_t flag_get(uint8_t flag) {
    return (cpu.f & flag) == flag ? 0x01 : 0x00;
}

void flag_assign(uint8_t mask, int condition) {
    if (condition) cpu.f |= mask;
}

int inc8(uint8_t src) {
    if (src == 6) {
        uint8_t old = read8(cpu.hl);
        write8(cpu.hl, old + 1);
        flag_assign(FLAG_Z, old == 0xFF);
        flag_assign(FLAG_H, (old & 0x0F) == 0x0F);
        flag_unset(FLAG_N);
        return 12;
    }
    uint8_t old = *reg[src];
    (*reg[src])++;
    flag_assign(FLAG_Z, old == 0xFF);
    flag_assign(FLAG_H, (old & 0x0F) == 0x0F);
    flag_unset(FLAG_N);
    return 4;
}

int dec8(uint8_t src) {
    if (src == 6) {
        uint8_t old = read8(cpu.hl);
        write8(cpu.hl, old - 1);
        flag_assign(FLAG_Z, old == 1);
        flag_set(FLAG_N);
        flag_assign(FLAG_H, (old & 0x0F) == 0);
        return 12;
    }
    uint8_t old = *reg[src];
    (*reg[src])--;
    flag_assign(FLAG_Z, *reg[src] == 0);
    flag_set(FLAG_N);
    flag_assign(FLAG_H, (old & 0x0F) == 0);
    return 4;
}

int inc16(uint8_t pair) {
    (*reg16[pair])++;
    return 8;
}

int dec16(uint8_t pair) {
    (*reg16[pair])--;
    return 8;
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

int push_r16(uint8_t pair) {
    return push(*reg16_stk[pair]);
}

int pop_r16(uint8_t pair) {
    pop(reg16_stk[pair]);
    if (pair == 3) cpu.f &= 0xF0;
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

int ld_r_u8(uint8_t src) {
    if (src == 6) {
        write8(cpu.hl, fetch8());
        return 12;
    }
    *reg[src] = fetch8();
    return 8;
}

int ld_r16_u16(uint8_t pair) {
    *reg16[pair] = fetch16();
    return 12;
}

int ld_r16_a(uint8_t pair) {
    switch (pair) {
        case 0: write8(cpu.bc, cpu.a); break;
        case 1: write8(cpu.de, cpu.a); break;
        case 2: write8(cpu.hl, cpu.a); cpu.hl++; break;
        case 3: write8(cpu.hl, cpu.a); cpu.hl--; break;
    }
    return 8;
}

int ld_a_r16(uint8_t pair) {
    switch (pair) {
        case 0: cpu.a = read8(cpu.bc); break;
        case 1: cpu.a = read8(cpu.de); break;
        case 2: cpu.a = read8(cpu.hl); cpu.hl++; break;
        case 3: cpu.a = read8(cpu.hl); cpu.hl--; break;
    }
    return 8;
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

int alu_imm(uint8_t op) {
    uint8_t op_t = (op >> 3) & 0x07;
    uint8_t val = fetch8();
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
    return 8;
}

int rst(uint8_t op) {
    push(cpu.pc);
    cpu.pc = op & 0x38;
    return 16;
}

enum { RLCA = 0x00, RRCA = 0x01, RLA = 0x10, RRA = 0x11 };

// RLCA 00 000 111
// RRCA 00 001 111
// RLA  00 010 111
// RRA  00 011 111
// Rotate Left Circular Accumulator
int rlca(uint8_t op) {
    uint8_t op_t = (op >> 3) & 0x07;

    switch (op_t) {
        uint8_t carry = (cpu.a >> 7) & 1;
        case RLCA:
            cpu.a = (cpu.a << 1) | carry;
            if (carry) flag_set(FLAG_C);
            else       flag_unset(FLAG_C);
            break;
        case RRCA:
            cpu.a = (cpu.a >> 1) | carry;
            if (carry) flag_set(FLAG_C);
            else       flag_unset(FLAG_C);
            break;
    }

    flag_unset(FLAG_Z);
    flag_unset(FLAG_N);
    flag_unset(FLAG_H);
    return 4;
}

int cpu_step(void) {
    if (cpu.halted) return 4;
    uint8_t op = fetch8();

    switch (op) {
        case NOP:
            return 4;
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
        case CALL:
            return call_cond(1);
        case RET_NZ:
            return ret_cond(flag_get(FLAG_Z) == 0);
        case RET_Z:
            return ret_cond(flag_get(FLAG_Z) == 1);
        case RET_NC:
            return ret_cond(flag_get(FLAG_C) == 0);
        case RET_C:
            return ret_cond(flag_get(FLAG_C) == 1);
        case RET:
            pop(&cpu.pc);
            return 16;
        case HALT:
            return halt();
        default:
            if (op >= 0x40 && op <= 0x7F)
                return ld_r_r(op);
            if (op >= 0x80 && op <= 0xBF)
                return alu_a(op);
            if ((op & 0xC7) == 0xC6)
                return alu_imm(op);
            if ((op & 0xCF) == 0xC5)
                return push_r16((op >> 4) & 0x03);
            if ((op & 0xCF) == 0xC1)
                return pop_r16((op >> 4) & 0x03);
            if ((op & 0x07) == 4)
                return inc8((op >> 3) & 0x07);
            if ((op & 0x07) == 5)
                return dec8((op >> 3) & 0x07);
            if ((op & 0x0F) == 3)
                return inc16((op >> 4) & 0x03);
            if ((op & 0x0F) == 0xB)
                return dec16((op >> 4) & 0x03);
            if ((op & 0xC7) == 0x06)
                return ld_r_u8((op >> 3) & 0x07);
            if ((op & 0xCF) == 0x01)
                return ld_r16_u16((op >> 4) & 0x03);
            if ((op & 0xCF) == 0x02)
                return ld_r16_a((op >> 4) & 0x03);
            if ((op & 0xCF) == 0x0A)
                return ld_a_r16((op >> 4) & 0x03);
            if ((op & 0xC7) == 0xC7)
                return rst(op);
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
