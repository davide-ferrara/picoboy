#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include "cpu.h"

#define WIDTH  160
#define HEIGHT 144

typedef struct {
    uint32_t clock;
    uint8_t  framebuffer[HEIGHT][WIDTH]; /* Raylib wants this order */
} PPU;

enum LCDC {BG_EN, OBJ_EN, OBJ_SIZE, BG_MAP, BG_SEL, WIN_EN, WIN_MAP, LCD_END};
enum STAT {LCD_MODE = 1, LYC_STAT, INTR_M0, INTR_M1, INTR_M2, INTR_LYC};
/* FF40-FF4B */
enum REGS  {LCDC, STAT, SCY, SCX, LY, LYC, DMA, BGP, OBP0, OBP1, WX, WY};

extern PPU ppu;

static inline uint8_t reg_read(uint8_t reg)  { return mmu[0xFF40 + reg]; }
static inline void    reg_write(uint8_t reg, uint8_t val) { mmu[0xFF40 + reg] = val; }

void ppu_step(uint16_t cycles);

#endif /* PPU_H */
