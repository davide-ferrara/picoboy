#ifndef PPU_H
#define PPU_H

#include <stdint.h>

enum Color {WHITE = 0x0, LIGHT_GREY = 0x1, DARK_GREY = 0x2, BLACK = 0x3};

typedef uint8_t tile[16];

typedef struct {
    uint16_t clock;
    uint8_t reg[0xC];                // 12 registri (0xFF40–0xFF4B)
    uint8_t vram[0x2000];            // 8192 byte VRAM
    uint8_t oam[0xA0];               // 160 byte OAM
    uint8_t framebuffer[144][160];   // framebuffer (contains all the pixels)
} PPU;

extern PPU ppu;

uint8_t ppu_read8(uint16_t addr);
void    ppu_write8(uint16_t addr, uint8_t val);
void    ppu_step(uint16_t cycles);
void    print_framebuffer(void);

#endif // !PPU_H
