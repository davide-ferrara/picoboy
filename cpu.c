#include "cpu.h"
#include <stdio.h>

CPU cpu = {0};
uint8_t mmu[0x10000];

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

void flag_assign(uint8_t mask, int condition) {
  if (condition) cpu.f |= mask;
}

int cpu_step(void) {
  uint8_t op = fetch8();
  switch (op) {
    case NOP:
      return 4;
    case LD_A:
      cpu.a = fetch8();
      return 8;
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
    case INC_PHL: {
      uint8_t val = read8(cpu.hl);
      uint8_t old = val;
      val++;
      write8(cpu.hl, val);
      cpu.f = (val == 0) ? 0x80 : 0;
      if ((old & 0x0F) == 0x0F) cpu.f |= 0x20;
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
    case XOR_AA:
      cpu.a ^= cpu.a;
      cpu.f = (cpu.a == 0) ? 0x80 : 0;
      return 4;
    case XOR_AB:
      cpu.a ^= cpu.b;
      cpu.f = (cpu.a == 0) ? 0x80 : 0;
      return 4;
    case XOR_AC:
      cpu.a ^= cpu.c;
      cpu.f = (cpu.a == 0) ? 0x80 : 0;
      return 4;
    case XOR_AD:
      cpu.a ^= cpu.d;
      cpu.f = (cpu.a == 0) ? 0x80 : 0;
      return 4;
    case XOR_AE:
      cpu.a ^= cpu.e;
      cpu.f = (cpu.a == 0) ? 0x80 : 0;
      return 4;
    case XOR_AH:
      cpu.a ^= cpu.h;
      cpu.f = (cpu.a == 0) ? 0x80 : 0;
      return 4;
    case XOR_AL:
      cpu.a ^= cpu.l;
      cpu.f = (cpu.a == 0) ? 0x80 : 0;
      return 4;
    case XOR_AHL:
      cpu.a ^= read8(cpu.hl);
      cpu.f = (cpu.a == 0) ? 0x80 : 0;
      return 8;
    case JP:
      cpu.pc = fetch16();
      return 16;
    default:
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
