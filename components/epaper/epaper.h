#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

// ─── Panel Specs ──────────────────────────────────────────────────────────────
// WeAct Studio 2.13" SSD1680
// MAX_LINE_BYTES   = 16   (128px / 8bit)
// MAX_COLUMN_BYTES = 250
// ALLSCREEN_BYTES  = 4000
#define EPD_PHYSICAL_WIDTH   122
#define EPD_PHYSICAL_HEIGHT  250
#define EPD_WIDTH            250   // logical width = landscape
#define EPD_HEIGHT           122   // logical height = landscape
#define EPD_BUF_WIDTH        ((EPD_PHYSICAL_WIDTH + 7) / 8)  // tetap 16
#define EPD_BUF_SIZE         (EPD_BUF_WIDTH * EPD_PHYSICAL_HEIGHT)  // tetap 4000

// ─── RAM Address — sesuai WeAct reference ────────────────────────────────────
#define EPD_X_ADDR_START    0x01
#define EPD_X_ADDR_END      0x10
#define EPD_Y_ADDR_START_L  0xF9    // 249 decimal
#define EPD_Y_ADDR_START_H  0x00
#define EPD_Y_ADDR_END_L    0x00
#define EPD_Y_ADDR_END_H    0x00

// ─── GPIO Pin Assignment ──────────────────────────────────────────────────────
// Sesuaikan dengan wiring ESP32 kamu
#define EPD_PIN_MOSI    23
#define EPD_PIN_CLK     18
#define EPD_PIN_CS      5
#define EPD_PIN_DC      17
#define EPD_PIN_RST     16
#define EPD_PIN_BUSY    4

// ─── SPI ─────────────────────────────────────────────────────────────────────
#define EPD_SPI_HOST    SPI2_HOST
#define EPD_SPI_FREQ_HZ (4 * 1000 * 1000)  // 4 MHz

// ─── Color ───────────────────────────────────────────────────────────────────
// SSD1680: bit 1 = putih, bit 0 = hitam
#define EPD_WHITE   0xFF
#define EPD_BLACK   0x00

// ─── Public API ──────────────────────────────────────────────────────────────

/**
 * @brief Inisialisasi SPI bus, GPIO, dan panel SSD1680.
 *        Panggil sekali di app_main sebelum fungsi lain.
 */
void epd_init(void);

/**
 * @brief Isi seluruh framebuffer dengan putih.
 *        Belum tampil ke layar — perlu epd_display().
 */
void epd_clear_buffer(void);

/**
 * @brief Kirim framebuffer ke panel dan trigger full refresh.
 *        Blocking ~2–8 detik. Jangan panggil dari ISR atau timer callback.
 */
void epd_display(void);

/**
 * @brief Full clear + display sekaligus. Layar jadi putih bersih.
 */
void epd_clear_screen(void);

/**
 * @brief Masukkan panel ke deep sleep (<1µA).
 *        Butuh epd_wake() sebelum bisa display lagi.
 */
void epd_sleep(void);

/**
 * @brief Bangunkan panel dari deep sleep.
 *        Melakukan hardware reset + init ulang penuh.
 *        Framebuffer di-reset ke putih setelah wake.
 */
void epd_wake(void);

/**
 * @brief Set satu piksel di framebuffer (belum ke layar).
 * @param x     kolom, 0 = kiri, max EPD_WIDTH-1
 * @param y     baris, 0 = atas, max EPD_HEIGHT-1
 * @param color EPD_BLACK atau EPD_WHITE
 */
void epd_draw_pixel(int x, int y, uint8_t color);

/**
 * @brief Akses langsung ke framebuffer.
 *        Dipakai oleh epaper_gfx layer.
 * @return Pointer ke array uint8_t[EPD_BUF_SIZE]
 */
uint8_t *epd_get_buffer(void);
