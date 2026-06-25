#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "cpu.h"
#include "ppu.h"
#include "timer.h"
#include <raylib.h>

int main(int argc, char **argv) {
    const char *romfile = argc > 1 ? argv[1] : "tetris.gb";
    int fd = open(romfile, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "Cannot open %s\n", romfile); return 1; }
    uint8_t buf;
    uint16_t addr = 0;
    while (read(fd, &buf, 1) == 1 && addr < 0x7FFF) {
        mmu[addr++] = buf;
    }
    close(fd);

    cpu.pc = 0x0100;
    cpu.sp = 0xFFFE;
    cpu.halted = 0;

    const int scale = 4;
    const int screenWidth = 160 * scale;
    const int screenHeight = 144 * scale;

    Image img = {
        .data = ppu.framebuffer,
        .width = 160,
        .height = 144,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE,
        .mipmaps = 1
    };

    InitWindow(screenWidth, screenHeight, "Gameboy Emulator");

    Texture2D texture = LoadTextureFromImage(img);
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);

    SetTargetFPS(60);

    Rectangle src = { 0, 0, 160, 144 };
    Rectangle dst = { 0, 0, screenWidth, screenHeight };

    uint32_t frame_num = 0;
    while (!WindowShouldClose()) {
        uint32_t cycles_this_frame = 0;
        while (cycles_this_frame < 70224) {
            uint16_t c = cpu_step();
            if (c == 0) break;
            ppu_step(c);
            timer_step(c);
            cycles_this_frame += c;
        }
        frame_num++;
        if (frame_num % 60 == 0)
            fprintf(stderr, "Frame %u: cycles=%u ppu.clk=%u LCDC=%02X LY=%02X IF=%02X IE=%02X FF85=%02X halt=%d ime=%d pend=%d pc=%04X\n",
                    frame_num, cycles_this_frame, ppu.clock, mmu[0xFF40], mmu[0xFF44], mmu[0xFF0F], mmu[0xFFFF], mmu[0xFF85], cpu.halted, cpu.ime, cpu.ime_pending, cpu.pc);

        BeginDrawing();
        ClearBackground(BLACK);
        UpdateTexture(texture, ppu.framebuffer);
        DrawTexturePro(texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
