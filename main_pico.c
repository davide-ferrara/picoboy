#include <hardware/timer.h>
#include <pico/stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/rand.h"

/*
 *  RGB555 — 15 bit, 5 bit for color (32768 colors)
 *  bit 15        bit 0
 *  0 B B B B B G G G G G R R R R R
*/

/*
 * stty -F /dev/ttyACM0 raw -echo && cat /dev/ttyACM0 | xxd
*/

#define MAGIC 0xDEADBEEF
#define RGB555_RED   0x001F
#define RGB555_GREEN 0x03E0
#define RGB555_BLUE  0x7C00
#define WIDTH 160
#define HEIGHT 144
#define PIXELS WIDTH * HEIGHT
#define FPS 10

typedef struct __attribute__((packed)) {
  uint32_t magic; // 4 Byte
  uint16_t frame[PIXELS]; // 23040 pixels (46080 Bytes)
} Packet;

static Packet pkt = { .magic = MAGIC };

int main() {
  stdio_init_all();
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);

  absolute_time_t next = make_timeout_time_ms(0);
  uint32_t interval_ms = 1000 / FPS; // 100ms
  while (1) {
    // Build frame
    for (size_t i = 0; i < PIXELS; i++) {
      switch (get_rand_32() % 3) {
        case 0:
          pkt.frame[i] = RGB555_RED;
          break;
        case 1:
          pkt.frame[i] = RGB555_GREEN;
          break;
        case 2:
          pkt.frame[i] = RGB555_BLUE;
          break;
      }
    }

    // Send frame
    fwrite(&pkt, sizeof(Packet), 1, stdout);
    stdio_flush();

    // Wait
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    next = delayed_by_ms(next, interval_ms);
    sleep_until(next);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
  }
}

