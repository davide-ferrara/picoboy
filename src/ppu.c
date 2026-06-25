#include "ppu.h"
#include "cpu.h"
#include "debug.h"
#include "interrupts.h"

#define UPDATE_TICKS 456    /* Needed cylces to draw a line */
#define VBLANK_END   154

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

static const uint8_t shades[] = {255, 192, 96, 0};

static void draw_scanline(void) {
    uint8_t ly = reg_read(LY);

    /* Check if VBLANK */
    if (ly >= HEIGHT) return;

    /* Background */
    if (get_lcdc(BG_EN)) {
        uint8_t scy = reg_read(SCY);
        uint8_t scx = reg_read(SCX);
        uint8_t bgp = reg_read(BGP);

        uint8_t map_y = scy + ly;
        uint16_t map_base = get_lcdc(BG_MAP) ? 0x9C00 : 0x9800;

        for (uint8_t x = 0; x < WIDTH; x++) {
            uint8_t map_x = scx + x;
            uint8_t tile_x = map_x / 8;
            uint8_t tile_y = map_y / 8;
            uint8_t pixel_x = map_x % 8;
            uint8_t pixel_y = map_y % 8;

            uint8_t tile_idx = mmu[map_base + tile_y * 32 + tile_x];

            uint16_t tile_addr;
            if (get_lcdc(BG_SEL))
                tile_addr = 0x8000 + tile_idx * 16;
            else
                tile_addr = 0x9000 + (int8_t)(tile_idx) * 16;

            uint8_t byte0 = mmu[tile_addr + pixel_y * 2];
            uint8_t byte1 = mmu[tile_addr + pixel_y * 2 + 1];

            uint8_t bit = 7 - pixel_x;
            uint8_t color_idx = ((byte1 >> bit) & 1) << 1 | ((byte0 >> bit) & 1);

            uint8_t palette_idx = (bgp >> (color_idx * 2)) & 0x03;
            ppu.framebuffer[ly][x] = shades[palette_idx];
        }
    } else {
        for (uint8_t i = 0; i < WIDTH; i++)
            ppu.framebuffer[ly][i] = 255;
    }

    /* If not sprite return */
    if (!get_lcdc(OBJ_EN)) return;

    /* We can have in OAM 40 sprite per row.
     * Srpite is 4 bytes long. Byte 0 = Y, Byte 1 = X, Byte 2 = Tile, Byte 3 = Flags
     * */
    uint8_t sprite_idx[10];  /* Buffer to store visible sprite indexes */
    uint8_t sprite_count = 0;

    for (int i = 0; i < 40 && sprite_count < 10; i++) {
        uint8_t sy = mmu[0xFE00 + i * 4]; // Y register
        if (sy <= ly + 16 && sy + 8 > ly + 16)
            sprite_idx[sprite_count++] = i;
    }

    /* Sprite size (0 = 8x8, 1 = 8x16) */
    uint8_t obj_size = get_lcdc(OBJ_SIZE) ? 16 : 8;

    /* Draw sprites from back to front (priority: lower X = higher priority) */
    for (uint8_t x = 0; x < WIDTH; x++) {
        for (int s = 0; s < sprite_count; s++) {
            int i = sprite_idx[s];
            uint8_t sy = mmu[0xFE00 + i * 4];
            uint8_t sx = mmu[0xFE00 + i * 4 + 1];
            uint8_t tile = mmu[0xFE00 + i * 4 + 2];
            uint8_t flags = mmu[0xFE00 + i * 4 + 3];

            int sprite_x = sx - 8;
            if (x < sprite_x || x >= sprite_x + 8) continue;

            /* Pixel inside the sprite */
            uint8_t pixel_x = x - sprite_x;
            uint8_t pixel_y = ly - (sy - 16);

            /* Apply X/Y flips */
            if (flags & 0x20) pixel_x = 7 - pixel_x; // X flip
            if (flags & 0x40) pixel_y = (obj_size - 1) - pixel_y; // Y flip

            /* Calculate tile address */
            uint16_t tile_addr;
            if (obj_size == 16) {
                /* bit 0 selects upper/lower tile */
                tile_addr = 0x8000 + ((tile & 0xFE) + (pixel_y >= 8 ? 1 : 0)) * 16;
            } else {
                tile_addr = 0x8000 + tile * 16;
            }

            /* Read the 2 bytes of the tile row */
            uint8_t byte0 = mmu[tile_addr + (pixel_y % 8) * 2];
            uint8_t byte1 = mmu[tile_addr + (pixel_y % 8) * 2 + 1];

            /* Extract color index from the two bit planes */
            uint8_t bit = 7 - pixel_x;
            uint8_t color_idx = ((byte1 >> bit) & 1) << 1 | ((byte0 >> bit) & 1);

            /* color_idx 0 is transparent */
            if (color_idx == 0) continue;

            /* Priority: sprite behind background if bg pixel is not white */
            if ((flags & 0x80) && ppu.framebuffer[ly][x] != 255) continue;

            /* Choose palette (bit 4 of flags) and map through it */
            uint8_t palette = (flags & 0x10) ? reg_read(OBP1) : reg_read(OBP0);
            uint8_t palette_idx = (palette >> (color_idx * 2)) & 0x03;
            ppu.framebuffer[ly][x] = shades[palette_idx];

            break; /* This pixel is taken, check next x */
        }
    }
}

void ppu_step(uint16_t cycles) {
    ppu.clock += cycles;

    /* 456 Update ticks */
    while (ppu.clock >= UPDATE_TICKS) {
        ppu.clock -= UPDATE_TICKS;

        /* If we haven't draw all the lines */
        if (reg_read(LY) < HEIGHT) draw_scanline();

        /* LY++ */
        reg_write(LY, reg_read(LY) + 1);

        if (reg_read(LY) == reg_read(LYC)) {
            set_stat(LYC_STAT);
            if (get_stat(INTR_LYC)) set_if(LCD);
        } else unset_stat(LYC_STAT);

        /* Draw finished, we reached the end on the y axis (143)
         * We send the VBLANK interrupt to the CPU */
        if (reg_read(LY) == HEIGHT) set_if(VBLANK);

        /* Ready to draw anotehr frame. We restart from line y = 0 */
        if (reg_read(LY) == VBLANK_END) reg_write(LY, 0);
    }

     if (reg_read(LY) >= HEIGHT) set_lcd_mode(1); // VBLANK
     else if (ppu.clock < 80) set_lcd_mode(2); // OAM MODE
     else if (ppu.clock < 252) set_lcd_mode(3); // DRAWING
     else set_lcd_mode(0);
}
