#include "cpu.h"
#include <stdint.h>
#include <stdio.h>
#include "ppu.h"
#include "timer.h"

/* Debug logging: silent unless -DPICOBOY_DEBUG is passed at compile time. */
#ifdef PICOBOY_DEBUG
#define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DBG(...) ((void)0)
#endif

CPU cpu = {0};

/* Serial output hook. When non-NULL, SB bytes triggered by SC=0x81 are
 * routed here instead of being written to stdout. Used by the headless
 * test harness (main_test.c) to capture Blargg test output. */
void (*serial_out)(uint8_t) = NULL;
uint8_t mmu[0x10000];
uint8_t *reg[] = {&cpu.b, &cpu.c, &cpu.d, &cpu.e, &cpu.h, &cpu.l, NULL, &cpu.a};
uint16_t *reg16[] = {&cpu.bc, &cpu.de, &cpu.hl, &cpu.sp};
uint16_t *reg16_stk[] = {&cpu.bc, &cpu.de, &cpu.hl, &cpu.af};

enum { ADD = 0, ADC = 1, SUB = 2, SBC = 3, AND = 4, XOR = 5, OR = 6, CP = 7, CPL = 5, SCF = 6, CCF = 7 };
enum { RLCA = 0, RRCA = 1, RLA = 2, RRA = 3 };

uint8_t read8(uint16_t addr) {
    if      (addr >= 0x8000 && addr <= 0x9FFF) return ppu_read8(addr);
    else if (addr >= 0xFE00 && addr <= 0xFE9F) return ppu_read8(addr);
    else if (addr >= 0xFF40 && addr <= 0xFF4B) return ppu_read8(addr);
    else if (addr == 0xFF00) {
        uint8_t p1 = mmu[0xFF00] & 0x30;
        if (!(p1 & 0x10)) p1 |= 0x0F; // buttons not pressed
        if (!(p1 & 0x20)) p1 |= 0x0F; // d-pad not pressed
        return p1;
    }
    else if (addr == 0xFF04) return timer_div_read();
    else return mmu[addr];
}

void write8(uint16_t addr, uint8_t val) {
    if      (addr >= 0x8000 && addr <= 0x9FFF) ppu_write8(addr, val);
    else if (addr >= 0xFE00 && addr <= 0xFE9F) ppu_write8(addr, val);
    else if (addr >= 0xFF40 && addr <= 0xFF4B) ppu_write8(addr, val);
    else if (addr == 0xFF04) timer_div_reset();
    else {
        mmu[addr] = val;
        if (addr == 0xFF02 && val == 0x81) {
            uint8_t c = mmu[0xFF01];
            if (serial_out) serial_out(c);
            else            putchar(c);
        }
        if (addr == 0xFF85) {
            static uint32_t ff85_w = 0;
            ff85_w++;
            if (ff85_w <= 100 || (ff85_w % 100) == 0)
                DBG("  FF85[%u] write %02X (PC=%04X)\n", ff85_w, val, cpu.pc);
        }
    }
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
    else           cpu.f &= ~mask;
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

// RLCA 00 000 111
// RRCA 00 001 111
// RLA  00 010 111
// RRA  00 011 111
// Rotate Left Circular Accumulator
int rlca(uint8_t op) {
    uint8_t op_t = (op >> 3) & 0x07;

    switch (op_t) {
        case RLCA: {
            uint8_t carry = (cpu.a >> 7) & 1;
            cpu.a = (cpu.a << 1) | carry;
            flag_assign(FLAG_C, carry);
            break;
        }
        case RRCA: {
            uint8_t carry = cpu.a & 1;
            cpu.a = (cpu.a >> 1) | (carry << 7);
            flag_assign(FLAG_C, carry);
            break;
        }
        case RLA: {
            uint8_t carry = (cpu.a >> 7) & 1;
            cpu.a = (cpu.a << 1) | flag_get(FLAG_C);
            flag_assign(FLAG_C, carry);
            break;
        }
        case RRA: {
            uint8_t carry = cpu.a & 1;
            cpu.a = (cpu.a >> 1) | (flag_get(FLAG_C) << 7);
            flag_assign(FLAG_C, carry);
            break;
        }
    }
    flag_unset(FLAG_Z);
    flag_unset(FLAG_N);
    flag_unset(FLAG_H);
    return 4;
}

int add_hl_r16(uint8_t op) {
    uint8_t pair = (op >> 4) & 0x03;
    uint16_t val = *reg16[pair];
    uint32_t res = (uint32_t)cpu.hl + val;
    flag_assign(FLAG_H, ((cpu.hl & 0x0FFF) + (val & 0x0FFF)) > 0x0FFF);
    flag_assign(FLAG_C, res > 0xFFFF);
    flag_unset(FLAG_N);
    cpu.hl = (uint16_t)res;
    return 8;
}

int cb(uint8_t op) {
    uint8_t src = op & 0x07;
    uint8_t bit = (op >> 3) & 0x07;
    uint8_t val;

    if (src == 6) val = read8(cpu.hl);
    else          val = *reg[src];

    switch (op >> 6) {
        case 0: // Shift/rotate
            switch (bit) {
                case 0: { // RLC
                    uint8_t c = val >> 7;
                    val = (val << 1) | c;
                    flag_assign(FLAG_C, c);
                    break;
                }
                case 1: { // RRC
                    uint8_t c = val & 1;
                    val = (val >> 1) | (c << 7);
                    flag_assign(FLAG_C, c);
                    break;
                }
                case 2: { // RL
                    uint8_t c = val >> 7;
                    val = (val << 1) | flag_get(FLAG_C);
                    flag_assign(FLAG_C, c);
                    break;
                }
                case 3: { // RR
                    uint8_t c = val & 1;
                    val = (val >> 1) | (flag_get(FLAG_C) << 7);
                    flag_assign(FLAG_C, c);
                    break;
                }
                case 4: { // SLA
                    flag_assign(FLAG_C, val >> 7);
                    val <<= 1;
                    break;
                }
                case 5: { // SRA
                    flag_assign(FLAG_C, val & 1);
                    val = (val >> 1) | (val & 0x80);
                    break;
                }
                case 6: { // SWAP
                    val = (val << 4) | (val >> 4);
                    flag_unset(FLAG_C);
                    break;
                }
                case 7: { // SRL
                    flag_assign(FLAG_C, val & 1);
                    val >>= 1;
                    break;
                }
            }
            flag_assign(FLAG_Z, val == 0);
            flag_unset(FLAG_N);
            flag_unset(FLAG_H);
            break;
        case 1: // BIT
            flag_unset(FLAG_N);
            flag_set(FLAG_H);
            flag_assign(FLAG_Z, (val & (1 << bit)) == 0);
            return src == 6 ? 12 : 8;
        case 2: // RES
            val &= ~(1 << bit);
            break;
        case 3: // SET
            val |= (1 << bit);
            break;
        default:
            fprintf(stderr, "Invalid CB opcode: %04X\n", op);
            return 0;
    }

    if (src == 6) write8(cpu.hl, val);
    else          *reg[src] = val;
    return src == 6 ? 16 : 8;
}

// 0x27 DAA
int daa(void) {
    uint16_t a = cpu.a;
    if (!flag_get(FLAG_N)) {
        if (flag_get(FLAG_H) || (a & 0x0F) > 9) a += 6;
        if (flag_get(FLAG_C) || a > 0x9F) a += 0x60;
    } else {
        if (flag_get(FLAG_H)) a = (a - 6) & 0xFF;
        if (flag_get(FLAG_C)) a -= 0x60;
    }
    flag_assign(FLAG_Z, (a & 0xFF) == 0);
    flag_unset(FLAG_H);
    cpu.a = (uint8_t)a;
    if (a & 0x100) flag_set(FLAG_C);
    return 4;
}

// 0x2F CPL 00 101 111 5
// 0x37 SCF 00 110 111 6
// 0x3F CCF 00 111 111 7
int flag_ops(uint8_t op) {
    uint8_t op_t = (op >> 3) & 0x07;
    switch(op_t) {
        case CPL:
            cpu.a = ~cpu.a;
            flag_set(FLAG_N);
            flag_set(FLAG_H);
            break;
        case SCF:
            flag_set(FLAG_C);
            flag_unset(FLAG_N);
            flag_unset(FLAG_H);
            break;
        case CCF:
            if (flag_get(FLAG_C))
                flag_unset(FLAG_C);
            else
                flag_set(FLAG_C);
            flag_unset(FLAG_N);
            flag_unset(FLAG_H);
            break;
    }
    return 4;
}

int cpu_step(void) {
    static uint32_t int_count = 0;
    if (cpu.halted) {
        uint8_t flagged = (mmu[0xFF0F] & mmu[0xFFFF]) & 0x1F;
        if (!flagged) return 4;
        cpu.halted = 0;
        if (!cpu.ime && !cpu.ime_pending) return 4;
    }

    // Interrupt handling (delayed by 1 instruction after EI/RETI)
    if (cpu.ime_pending) {
        cpu.ime = 1;
        cpu.ime_pending = 0;
    } else if (cpu.ime) {
        uint8_t flagged = (mmu[0xFF0F] & mmu[0xFFFF]) & 0x1F;
        uint8_t addr;
        for (uint8_t i = 0; i < 5; i++) {
            uint8_t curr = (flagged >> i) & 0x1;
            if (curr) {
                push(cpu.pc);
                switch (i) {
                    case 0:
                        addr = 0x0040; // VBlank
                        break;
                    case 1:
                        addr = 0x0048; // Stat
                        break;
                    case 2:
                        addr = 0x0050; // Timer
                        break;
                    case 3:
                        addr = 0x0058; // Serial
                        break;
                    case 4:
                        addr = 0x0060; // Joypad
                        break;
                }
                cpu.ime = 0;
                cpu.pc = addr;
                mmu[0xFF0F] &= ~(1 << i);
                int_count++;
                if (int_count <= 50 || (int_count % 100) == 0) {
                    DBG("  INT#%u i=%u new_pc=%04X FF85=%02X LY=%02X\n",
                        int_count, i, addr, mmu[0xFF85], ppu.reg[4]);
                }
                return 20;
            }
        }
    }

    uint8_t op = fetch8();

    switch (op) {
        case NOP:
            return 4;
        case STOP: {
            fetch8();  // skip speed byte
            return 4;
        }
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
        case EI:
            cpu.ime_pending = 1;
            return 4;
        case DI:
            cpu.ime = 0;
            return 4;
        case RETI:
            pop(&cpu.pc);
            cpu.ime_pending = 1;
            return 16;
        case HALT:
            return halt();
        case LDH_IO_A: {
            uint8_t a8 = fetch8();
            write8(0xFF00 + a8, cpu.a);
            return 12;
        }
        case LDH_A_IO: {
            uint8_t a8 = fetch8();
            cpu.a = read8(0xFF00 + a8);
            return 12;
            }
        case CB: {
            uint8_t op2 = fetch8();
            return cb(op2);
        }
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
            if (op == 0x07 || op == 0x0F || op == 0x17 || op == 0x1F)
                return rlca(op);
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
            if ((op & 0xCF) == 0x09)
                return add_hl_r16(op);
            if ((op & 0xC7) == 0xC7)
                return rst(op);
            if (op == 0x08) {
                uint16_t addr = fetch16();
                write8(addr, cpu.sp & 0xFF);
                write8(addr + 1, cpu.sp >> 8);
                return 20;
            }
            if (op == 0xE0) {
                write8(0xFF00 + fetch8(), cpu.a);
                return 12;
            }
            if (op == 0xE2) {
                write8(0xFF00 + cpu.c, cpu.a);
                return 8;
            }
            if (op == 0xEA) {
                write8(fetch16(), cpu.a);
                return 16;
            }
            if (op == 0xF0) {
                cpu.a = read8(0xFF00 + fetch8());
                return 12;
            }
            if (op == 0xF2) {
                cpu.a = read8(0xFF00 + cpu.c);
                return 8;
            }
            if (op == 0xF9) {
                cpu.sp = cpu.hl;
                return 8;
            }
            if (op == 0xFA) {
                cpu.a = read8(fetch16());
                return 16;
            }
            if (op == 0x27)
                return daa();
            if (op == 0x2F || op == 0x37 || op == 0x3F)
                return flag_ops(op);
            if (op == 0xE8) {
                int8_t off = (int8_t)fetch8();
                uint16_t old = cpu.sp;
                cpu.sp += off;
                flag_unset(FLAG_Z);
                flag_unset(FLAG_N);
                flag_assign(FLAG_H, (old & 0x0F) + (off & 0x0F) > 0x0F);
                flag_assign(FLAG_C, (old & 0xFF) + (off & 0xFF) > 0xFF);
                return 16;
            }
            if (op == 0xF8) {
                int8_t off = (int8_t)fetch8();
                flag_unset(FLAG_Z);
                flag_unset(FLAG_N);
                flag_assign(FLAG_H, (cpu.sp & 0x0F) + (off & 0x0F) > 0x0F);
                flag_assign(FLAG_C, (cpu.sp & 0xFF) + (off & 0xFF) > 0xFF);
                cpu.hl = cpu.sp + off;
                return 12;
            }
            else fprintf(stderr, "Invalid opcode: %04X\n", op);
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
