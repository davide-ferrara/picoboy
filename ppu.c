#include "ppu.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

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

uint8_t ppu_lcdc_get(uint8_t flag) {
    return ppu.reg[LCDC] >> flag;
}

void ppu_stat_set(uint8_t flag) {
    ppu.reg[STAT] |= (1 << flag);
}

uint8_t ppu_lcd_mode_get(void) {
    return ppu.reg[STAT] & 0x03;
}

void ppu_lcdc_unset(uint8_t flag) {
    ppu.reg[LCDC] &= ~(1 << flag);
}

void ppu_stat_unset(uint8_t flag) {
    ppu.reg[STAT] &= ~(1 << flag);
}

void ppu_lcd_mode_set(uint8_t val) {
    if (val > 3) return;
    ppu.reg[STAT] = (ppu.reg[STAT] & ~0x03) | (val & 0x03);
}

void print_framebuffer(void) {
    static uint8_t last_ly = 0;
    uint8_t ly = ppu.reg[LY];
    if (ly == 144 && last_ly != 144) {
        const char map[] = " .+@";
        for (int i = 0; i < 144; i++) {
            for (int j = 0; j < 160; j++)
                putchar(map[ppu.framebuffer[i][j] & 3]);
            putchar('\n');
        }
    }
    last_ly = ly;
}

void ppu_draw_scanline() {
    if (ppu_lcdc_get(LCD_EN) && ppu.reg[LY] < 144) {
        uint8_t map_y = ppu.reg[SCY] + ppu.reg[LY];
        uint16_t map_base = ppu_lcdc_get(BG_MAP) ? 0x9C00 : 0x9800;

        for (uint8_t i = 0; i < 160; i++) {
            uint8_t map_x = (ppu.reg[SCX] + i) & 0xFF;
            uint8_t cell_x = map_x / 8;
            uint8_t cell_y = map_y / 8;

            uint8_t tile_idx = ppu.vram[(map_base - 0x8000) + cell_y * 32 + cell_x];
            uint16_t tile_addr;
            if (ppu_lcdc_get(TILE_SEL)) tile_addr = 0x8000 + tile_idx * 16;
            else tile_addr = 0x9000 + (int8_t)(tile_idx) * 16;

            uint8_t pixel_x = map_x % 8;
            uint8_t pixel_y = map_y % 8;
            uint8_t byte0 = ppu.vram[(tile_addr - 0x8000) + pixel_y * 2];
            uint8_t byte1 = ppu.vram[(tile_addr - 0x8000) + pixel_y * 2 + 1];

            uint8_t bit = 7 - pixel_x;
            uint8_t color_idx = ((byte1 >> bit) & 1) << 1 | ((byte0 >> bit) & 1);

            ppu.framebuffer[ppu.reg[LY]][i] = (ppu.reg[BGP] >> (color_idx * 2)) & 0x03;
        }
    }
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
    uint8_t old_mode = ppu_lcd_mode_get();
    uint8_t new_mode;

    if ((*ly) >= 144)
        new_mode = 1;                        // VBlank
    else if (ppu.clock < 80)
        new_mode = 2;                        // OAM scan
    else if (ppu.clock < 252)
        new_mode = 3;                        // Draw
    else
        new_mode = 0;                        // HBlank

    if (old_mode != 3 && new_mode == 3)      // Start Draw
        ppu_draw_scanline();

    ppu_lcd_mode_set(new_mode);
}

