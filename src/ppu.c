#include "ppu.h"
#include "cpu.h"
#include "interrupts.h"

#define UPDATE_TICKS 456    /* Needed cylces to draw a line */
#define VBLANK_END   154
#define WIDTH  160
#define HEIGHT 144

/* Why UPDATE_TICKS = 456? 456 × 154 rows = 70.224 cycles.
 * At 4.194.304 Hz (clock of GB): 4.194.304 ÷ 70.224 ≈ 59.7 fps
 * They added more rows to fit this calculation, crazy.
 * */

PPU ppu = {0};

/* TODO: A future refactor could include a new register.c file that handles
 * set, unset, reset/clear functions. Basically we could have generalized funcitons
 * that modify the mmu registers allowing to remove code duplication. */

/* Set the specified LCDC flag. */
static void set_lcdc(uint8_t flag) {
    reg_write(LCDC, reg_read(LCDC) | (0x1 << flag));
}

/* Unset the specified LCDC flag. */
static void unset_lcdc(uint8_t flag) {
    reg_write(LCDC, reg_read(LCDC) & ~(0x1 << flag));
}

/* Get the specified LCDC flag. */
static uint8_t get_lcdc(uint8_t flag) {
    return (reg_read(LCDC) >> flag) & 0x1;
}

/* Set the specified STAT flag.
 * NOTE: This doesnt not allow you to set LCD_MODE, use set_lcd_mode()
 * */
static void set_stat(uint8_t flag) {
    if (flag == LCD_MODE) return;
    reg_write(STAT, reg_read(STAT) | (0x1 << flag));
}

/* Unset the specified STAT flag. */
static void unset_stat(uint8_t flag) {
    if (flag == LCD_MODE) return;
    reg_write(STAT, reg_read(STAT) & ~(0x1 << flag));
}

/* Get the specified STAT flag. */
static uint8_t get_stat(uint8_t flag) {
    return (reg_read(STAT) >> flag) & 0x1;
}

static void set_lcd_mode(uint8_t val) {
    if (val > 3) return;
    reg_write(STAT, (reg_read(STAT) & ~0x03) | (val & 0x03));
}

static uint8_t get_lcd_mode(void) {
    return reg_read(STAT) & 0x03;
}

static void draw_scanline(void) {
    ;
}

void ppu_step(uint16_t cycles) {
    ppu.clock += cycles;

    while (ppu.clock >= UPDATE_TICKS) {
        ppu.clock -= UPDATE_TICKS;

        /* If we haven't draw all the lines */
        if (reg_read(LY) < HEIGHT) draw_scanline();

        reg_write(LY, reg_read(LY) + 1);

        /* Draw finished, we reached the end on the y axis (143)
         * We send the VBLANK interrupt to the CPU */
        if (reg_read(LY) == HEIGHT) set_if(VBLANK);

        /* Ready to draw anotehr frame. We restart from line y = 0 */
        if (reg_read(LY) == VBLANK_END) reg_write(LY, 0);
    }
}
