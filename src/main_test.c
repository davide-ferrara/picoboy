/* Headless test harness for Blargg CPU test ROMs.
 *
 * Usage: ./gbtest <rom.gb> [max_cycles]
 *
 * Captures the serial output written to SB (FF01) / SC (FF02) by the test
 * ROM, then inspects the captured text for "Passed" / "Failed".
 *
 * Exit codes:
 *   0  -> test passed
 *   1  -> test failed (rom reported failure)
 *   2  -> timeout / no result
 *   3  -> usage / IO error
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "cpu.h"
#include "ppu.h"
#include "timer.h"

#define SERIAL_BUF_LEN 8192
#define DEFAULT_MAX_CYCLES 120000000ULL  /* ~2s of GB time */

static char  serial_buf[SERIAL_BUF_LEN];
static size_t serial_len = 0;

/* Override the serial hook: instead of putchar() in write8() we accumulate
 * into a buffer so we can search it at the end. We do this by hooking the
 * same addresses write8() already checks. */

static void serial_emit(char c) {
    if (serial_len < SERIAL_BUF_LEN - 1)
        serial_buf[serial_len++] = c;
}

/* Default DMG post-bootstrap register values. Blargg ROMs are tolerant
 * (they only need PC=0x0100 and SP=0xFFFE really) but providing the
 * canonical values avoids spurious failures when a test reads a register
 * before setting it. */
static void init_dmg_post_boot(void) {
    cpu.af  = 0x01B0;
    cpu.bc  = 0x0013;
    cpu.de  = 0x00D1;
    cpu.hl  = 0x014D;
    cpu.sp  = 0xFFFE;
    cpu.pc  = 0x0100;
    cpu.ime = 0;
    cpu.ime_pending = 0;
    cpu.halted = 0;

    /* IO registers a real DMG has set after the boot ROM. */
    mmu[0xFF04] = 0x00;                    /* DIV   */
    mmu[0xFF05] = 0x00;                    /* TIMA  */
    mmu[0xFF06] = 0x00;                    /* TMA   */
    mmu[0xFF07] = 0x00;                    /* TAC   */
    mmu[0xFF10] = 0x80;                    /* NR10  */
    mmu[0xFF11] = 0xBF;                    /* NR11  */
    mmu[0xFF12] = 0xF3;                    /* NR12  */
    mmu[0xFF14] = 0xBF;                    /* NR14  */
    mmu[0xFF16] = 0x3F;                    /* NR21  */
    mmu[0xFF17] = 0x00;                    /* NR22  */
    mmu[0xFF19] = 0xBF;                    /* NR24  */
    mmu[0xFF1A] = 0x7F;                    /* NR30  */
    mmu[0xFF1B] = 0xFF;                    /* NR31  */
    mmu[0xFF1C] = 0x9F;                    /* NR32  */
    mmu[0xFF1E] = 0xBF;                    /* NR33  */
    mmu[0xFF20] = 0xFF;                    /* NR41  */
    mmu[0xFF21] = 0x00;                    /* NR42  */
    mmu[0xFF22] = 0x00;                    /* NR43  */
    mmu[0xFF23] = 0xBF;                    /* NR30  */
    mmu[0xFF24] = 0x77;                    /* NR50  */
    mmu[0xFF25] = 0xF3;                    /* NR51  */
    mmu[0xFF26] = 0xF1;                    /* NR52  */
    mmu[0xFF40] = 0x91;                    /* LCDC  (LCD on) */
    mmu[0xFF47] = 0xFC;                    /* BGP   */
    mmu[0xFF0F] = 0xE1;                    /* IF    */
    mmu[0xFFFF] = 0x00;                    /* IE    */
}

static int load_rom(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s\n", path);
        return -1;
    }
    uint8_t buf;
    uint16_t addr = 0;
    while (read(fd, &buf, 1) == 1 && addr < 0x7FFF) {
        mmu[addr++] = buf;
    }
    close(fd);
    return 0;
}

/* Serial hook: accumulate SB bytes into a buffer for end-of-run inspection. */
static void test_serial_hook(uint8_t sb_val) {
    serial_emit((char)sb_val);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom.gb> [max_cycles]\n", argv[0]);
        return 3;
    }
    unsigned long long max_cycles = DEFAULT_MAX_CYCLES;
    if (argc >= 3) {
        max_cycles = strtoull(argv[2], NULL, 0);
    }

    if (load_rom(argv[1]) < 0) return 3;
    init_dmg_post_boot();

    /* Route serial output (SB/SC) into our buffer instead of stdout. */
    serial_out = test_serial_hook;

    unsigned long long total = 0;
    int result = 2;  /* default: no result / timeout */

    while (total < max_cycles) {
        uint16_t c = cpu_step();
        if (c == 0) {
            /* invalid opcode -> bail */
            result = 2;
            break;
        }
        ppu_step(c);
        timer_step(c);
        total += c;

        /* Check stop conditions after each step. */
        if (serial_len >= 6) {
            /* "Passed" */
            if (strstr(serial_buf, "Passed")) { result = 0; break; }
            /* "Failed" -- Blargg prints "Failed\n" then the failure code */
            if (strstr(serial_buf, "Failed")) { result = 1; break; }
        }
    }

    /* Always print captured serial output to stdout for inspection. */
    if (serial_len > 0) {
        fwrite(serial_buf, 1, serial_len, stdout);
        if (serial_buf[serial_len - 1] != '\n') fputc('\n', stdout);
    }

    const char *verdict =
        result == 0 ? "PASS" :
        result == 1 ? "FAIL" :
        result == 2 ? "TIMEOUT" : "UNKNOWN";
    fprintf(stderr, "[%s] cycles=%llu serial_len=%zu\n", verdict, total, serial_len);

    return result;
}
