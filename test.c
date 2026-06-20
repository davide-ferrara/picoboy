#include <assert.h>
#include <stdio.h>
#include "cpu.h"

void test_set_flags(void) {
  flag_set(FLAG_Z);
  print_flag_reg();
  assert(cpu.f == FLAG_Z);
  flag_clear();

  flag_set(FLAG_N);
  print_flag_reg();
  assert(cpu.f == FLAG_N);
  flag_clear();

  flag_set(FLAG_H);
  print_flag_reg();
  assert(cpu.f == FLAG_H);
  flag_clear();

  flag_set(FLAG_C);
  print_flag_reg();
  assert(cpu.f == FLAG_C);
  flag_clear();

  flag_set(FLAG_Z);
  flag_set(FLAG_N);
  flag_set(FLAG_H);
  flag_set(FLAG_C);
  print_flag_reg();
  assert(cpu.f == 0xF0);
  flag_clear();

  assert(cpu.f == 0x00);
  print_flag_reg();
}

void test_incr(void) {
    printf("         A  Z N H C 0 0 0 0\n");

    mmu[0] = LD_A;   mmu[1] = 0x00;
    mmu[2] = INC_A;
    cpu.pc = 0; cpu.f = 0x00;

    printf("\n-- INC A: 0x00 -> 0x01 --\n");
    printf("  LD:  %02X  ", cpu.a); print_flag_reg();
    cpu_step(); cpu_step();
    printf("  INC: %02X  ", cpu.a); print_flag_reg();

    mmu[0] = LD_A;   mmu[1] = 0x0F;
    mmu[2] = INC_A;
    cpu.pc = 0; cpu.f = 0x00;

    printf("\n-- INC A: 0x0F -> 0x10 (expect H=1) --\n");
    cpu_step(); cpu_step();
    printf("  INC: %02X  ", cpu.a); print_flag_reg();

    mmu[0] = LD_A;   mmu[1] = 0xFF;
    mmu[2] = INC_A;
    cpu.pc = 0; cpu.f = 0x00;

    printf("\n-- INC A: 0xFF -> 0x00 (expect Z=1 H=1) --\n");
    cpu_step(); cpu_step();
    printf("  INC: %02X  ", cpu.a); print_flag_reg();
    printf("\n");
}

int main(void) {
    test_set_flags();
    test_incr();
    return 0;
}
