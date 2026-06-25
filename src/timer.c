#include "timer.h"
#include "cpu.h"
#include "interrupts.h"
#include <stdint.h>

static uint16_t div_counter = 0;
static uint16_t tima_acc = 0;

uint16_t timer_div_read(void) {
    return mmu[0xFF04] = (div_counter >> 8) & 0xFF;
}

void timer_div_reset(void) {
    div_counter = 0;
    mmu[0xFF04] = 0x0;
}

static uint8_t read_tac_clk(void) {
    return mmu[0xFF07] & 0x3;
}

static uint8_t read_tac_en(void) {
    return mmu[0xFF07] & 0x4;
}

static uint8_t read_tma(void) {
    return mmu[0xFF06];
}

static uint8_t read_tima(void) {
    return mmu[0xFF05];
}

static void tima_incr(void) {
    mmu[0xFF05]++;
}

static void tima_reset(void) {
    mmu[0xFF05] = read_tma();
    if_set(TIMER);
}

void timer_step(uint16_t cycles) {
    div_counter += cycles;
    timer_div_read();

    uint16_t N = 0;
    if (read_tac_en()) {
        switch (read_tac_clk()) {
            case 0x00: N = 1024; break;
            case 0x01: N = 16;   break;
            case 0x02: N = 64;   break;
            case 0x03: N = 256;  break;
        }
        tima_acc += cycles;
        while (tima_acc >= N) {
            tima_acc -= N;
            tima_incr();
            if (read_tima() == 0x00) tima_reset();
        }
    }
}
