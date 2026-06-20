#include <assert.h>
#include <stdio.h>
#include "cpu.h"

void test_flag_unset(void) {
  printf("-- FLAG UNSET --\n");
  flag_set(FLAG_Z);
  print_flag_reg();
  flag_unset(FLAG_Z);
  print_flag_reg();
}

void test_set_flags(void) {
  printf("-- FLAGS REGISTER --\n");
  printf("-- Z|N|H|C|0|0|0|0 --\n\n");
  printf("-- FLAG_Z set to 1 --\n");
  flag_set(FLAG_Z);
  print_flag_reg();
  assert(cpu.f == FLAG_Z);
  flag_clear();
  printf("\n");

  printf("-- FLAG_N set to 1 --\n");
  flag_set(FLAG_N);
  print_flag_reg();
  assert(cpu.f == FLAG_N);
  flag_clear();
  printf("\n");

  printf("-- FLAG_H set to 1 --\n");
  flag_set(FLAG_H);
  print_flag_reg();
  assert(cpu.f == FLAG_H);
  flag_clear();
  printf("\n");

  printf("-- FLAG_C set to 1 --\n");
  flag_set(FLAG_C);
  print_flag_reg();
  assert(cpu.f == FLAG_C);
  flag_clear();
  printf("\n");

  printf("-- FLAGS: Z N H C set to 1 --\n");
  flag_set(FLAG_Z);
  flag_set(FLAG_N);
  flag_set(FLAG_H);
  flag_set(FLAG_C);
  print_flag_reg();
  assert(cpu.f == 0xF0);
  flag_clear();
  printf("\n");

  printf("-- FLAGS CLEARED --\n");
  assert(cpu.f == 0x00);
  print_flag_reg();
}

/* --- INC test helper --- */
static void inc_test(const char *reg,
                     uint8_t ld_op, uint8_t inc_op,
                     uint8_t *dst_reg) {
    printf("\n-- INC %s --\n", reg);

    /* 0x00 -> 0x01 */
    mmu[0] = ld_op;  mmu[1] = 0x00;
    mmu[2] = inc_op;
    cpu.pc = 0; cpu.f = 0x00;
    cpu_step(); cpu_step();
    printf("0x00+1: %s=%02X  ", reg, *dst_reg); print_flag_reg();

    /* 0x0F -> 0x10, H=1 */
    mmu[0] = ld_op;  mmu[1] = 0x0F;
    mmu[2] = inc_op;
    cpu.pc = 0; cpu.f = 0x00;
    cpu_step(); cpu_step();
    printf("0x0F+1: %s=%02X  ", reg, *dst_reg); print_flag_reg();

    /* 0xFF -> 0x00, Z=1 H=1 */
    mmu[0] = ld_op;  mmu[1] = 0xFF;
    mmu[2] = inc_op;
    cpu.pc = 0; cpu.f = 0x00;
    cpu_step(); cpu_step();
    printf("0xFF+1: %s=%02X  ", reg, *dst_reg); print_flag_reg();

    /* C must be preserved */
    mmu[0] = ld_op;  mmu[1] = 0x00;
    mmu[2] = inc_op;
    cpu.pc = 0; cpu.f = FLAG_C;
    cpu_step(); cpu_step();
    printf("C kept: %s=%02X  ", reg, *dst_reg); print_flag_reg();
    printf("\n");
}

void test_incr_a(void) { inc_test("A", LD_A, INC_A, &cpu.a); }
void test_incr_b(void) { inc_test("B", LD_B, INC_B, &cpu.b); }
void test_incr_c(void) { inc_test("C", LD_C, INC_C, &cpu.c); }
void test_incr_d(void) { inc_test("D", LD_D, INC_D, &cpu.d); }
void test_incr_e(void) { inc_test("E", LD_E, INC_E, &cpu.e); }
void test_incr_h(void) { inc_test("H", LD_H, INC_H, &cpu.h); }
void test_incr_l(void) { inc_test("L", LD_L, INC_L, &cpu.l); }

int main(void) {
    test_set_flags();
    test_incr_a();
    test_incr_b();
    test_incr_c();
    test_incr_d();
    test_incr_e();
    test_incr_h();
    test_incr_l();
    test_flag_unset();
    return 0;
}
