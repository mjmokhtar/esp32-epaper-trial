#pragma once

#include <stdint.h>
#include "epaper.h"

// ─── Font Sizes ───────────────────────────────────────────────────────────────
typedef enum {
    FONT_SMALL  = 0,   // 5×7  pixels (1× scale)
    FONT_MEDIUM = 1,   // 10×14 pixels (2× scale)
    FONT_LARGE  = 2,   // 15×21 pixels (3× scale)
} epd_font_t;

// ─── Text ─────────────────────────────────────────────────────────────────────
int  epd_draw_char  (int x, int y, char c,          epd_font_t font, uint8_t color);
int  epd_draw_string(int x, int y, const char *str, epd_font_t font, uint8_t color);

// ─── Lines ────────────────────────────────────────────────────────────────────
void epd_draw_hline(int x, int y, int w,            uint8_t color);
void epd_draw_vline(int x, int y, int h,            uint8_t color);
void epd_draw_line (int x0, int y0, int x1, int y1, uint8_t color);

// ─── Rectangles ───────────────────────────────────────────────────────────────
void epd_draw_rect      (int x, int y, int w, int h,        uint8_t color);
void epd_fill_rect      (int x, int y, int w, int h,        uint8_t color);
void epd_draw_round_rect(int x, int y, int w, int h, int r, uint8_t color);
void epd_fill_round_rect(int x, int y, int w, int h, int r, uint8_t color);

// ─── Circles ──────────────────────────────────────────────────────────────────
void epd_draw_circle(int cx, int cy, int r, uint8_t color);
void epd_fill_circle(int cx, int cy, int r, uint8_t color);

// ─── Triangles ────────────────────────────────────────────────────────────────
void epd_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color);
void epd_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color);

// ─── Bitmap ───────────────────────────────────────────────────────────────────
// Format: 1bpp MSB-first — cocok dengan output GIMP monochrome export
void epd_draw_bitmap(int x, int y, const uint8_t *bitmap, int w, int h, uint8_t color);