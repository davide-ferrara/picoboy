#include "ppu.h"
#include "cpu.h"
#include <stdint.h>

PPU ppu = {0};
enum { LCDC = 0, STAT, SCY, SCX, LY, LYC, DMA, BGP, OBP0, OBP1, WY, WX};
enum lcdc { LCD_EN = 7, WIN_MAP, WIN_EN, TILE_SEL, BG_MAP, OBJ_SIZE, OBJ_EN, BG_EN };
enum stat { NTR_LYC = 6, INTR_M2, INTR_M1, INTR_M0, LYC_STAT, LCD_MODE };

uint8_t ppu_read8(uint16_t addr) {
    // VRAM
    if (addr >= 0x8000 && addr <= 0x9FFF)
        return ppu.vram[addr - 0x8000];
    // OAM
    if (addr >= 0xFE00 && addr <= 0xFE9F)
        return ppu.oam[addr - 0xFE00];
    // Registri
    if (addr >= 0xFF40 && addr <= 0xFF4B)
        return ppu.reg[addr - 0xFF40];
    return 0xFF;
}

void ppu_write8(uint16_t addr, uint8_t val) {
    // VRAM
    if (addr >= 0x8000 && addr <= 0x9FFF)
        ppu.vram[addr - 0x8000] = val;
    // OAM
    else if (addr >= 0xFE00 && addr <= 0xFE9F)
        ppu.oam[addr - 0xFE00] = val;
    // Registri
    else if (addr >= 0xFF40 && addr <= 0xFF4B)
        ppu.reg[addr - 0xFF40] = val;
}

void ppu_lcdc_set(uint8_t flag) {
    ppu.reg[LCDC] |= (1 << flag);
}

void ppu_stat_set(uint8_t flag) {
    if (flag == LCD_MODE) return;
    ppu.reg[STAT] |= (1 << flag);
}

void ppu_lcdc_unset(uint8_t flag) {
    ppu.reg[LCDC] &= ~(1 << flag);
}

void ppu_stat_unset(uint8_t flag) {
    if (flag == LCD_MODE) return;
    ppu.reg[STAT] &= ~(1 << flag);
}

void ppu_lcd_mode_set(uint8_t val) {
    if (val > 3) return;
    ppu.reg[LCD_MODE] = val;
}

void ppu_step(uint16_t cycles) {
    ppu.clock += cycles;
    uint8_t *ly = &(ppu.reg[LY]);

    // Scanline finished (456 Cycles)
    while (ppu.clock >= 456) {
        ppu.clock -= 456;
        (*ly)++;

        // VBLANK STARTED
        if ((*ly) == 144) {
            mmu[0xFF0F] |= 0x1; // IF_VBLANK
        }
        // RESET
        if ((*ly) == 154) (*ly) = 0;
    }
    if ((*ly) >= 144) ppu_lcd_mode_set(1); // VBlank
    else if (ppu.clock < 80) ppu_lcd_mode_set(2); // OAM Scan
    else if (ppu.clock < 252) ppu_lcd_mode_set(3); // Draw
    else ppu_lcd_mode_set(0); // HBlank
    return;
}
