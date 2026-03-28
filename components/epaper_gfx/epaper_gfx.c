#include "epaper_gfx.h"
#include "epaper.h"
#include <stdlib.h>

// ─── Font 5×7 (ASCII 32–126) ──────────────────────────────────────────────────
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x00,0x5F,0x00,0x00}, // '!'
    {0x00,0x07,0x00,0x07,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
    {0x23,0x13,0x08,0x64,0x62}, // '%'
    {0x36,0x49,0x55,0x22,0x50}, // '&'
    {0x00,0x05,0x03,0x00,0x00}, // '\''
    {0x00,0x1C,0x22,0x41,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00}, // ')'
    {0x08,0x2A,0x1C,0x2A,0x08}, // '*'
    {0x08,0x08,0x3E,0x08,0x08}, // '+'
    {0x00,0x50,0x30,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08}, // '-'
    {0x00,0x60,0x60,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // '0'
    {0x00,0x42,0x7F,0x40,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46}, // '2'
    {0x21,0x41,0x45,0x4B,0x31}, // '3'
    {0x18,0x14,0x12,0x7F,0x10}, // '4'
    {0x27,0x45,0x45,0x45,0x39}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // '6'
    {0x01,0x71,0x09,0x05,0x03}, // '7'
    {0x36,0x49,0x49,0x49,0x36}, // '8'
    {0x06,0x49,0x49,0x29,0x1E}, // '9'
    {0x00,0x36,0x36,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00}, // ';'
    {0x00,0x08,0x14,0x22,0x41}, // '<'
    {0x14,0x14,0x14,0x14,0x14}, // '='
    {0x41,0x22,0x14,0x08,0x00}, // '>'
    {0x02,0x01,0x51,0x09,0x06}, // '?'
    {0x32,0x49,0x79,0x41,0x3E}, // '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x0C,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 'Z'
    {0x00,0x7F,0x41,0x41,0x00}, // '['
    {0x02,0x04,0x08,0x10,0x20}, // '\'
    {0x00,0x41,0x41,0x7F,0x00}, // ']'
    {0x04,0x02,0x01,0x02,0x04}, // '^'
    {0x40,0x40,0x40,0x40,0x40}, // '_'
    {0x00,0x01,0x02,0x04,0x00}, // '`'
    {0x20,0x54,0x54,0x54,0x78}, // 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 'f'
    {0x0C,0x52,0x52,0x52,0x3E}, // 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 'j'
    {0x7F,0x10,0x28,0x44,0x00}, // 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 'z'
    {0x00,0x08,0x36,0x41,0x00}, // '{'
    {0x00,0x00,0x7F,0x00,0x00}, // '|'
    {0x00,0x41,0x36,0x08,0x00}, // '}'
    {0x02,0x01,0x02,0x04,0x02}, // '~'
};

#define FONT_CHAR_W  5
#define FONT_CHAR_H  7
#define FONT_SPACING 1

// ─── Internal: render glyph dengan scale ─────────────────────────────────────
static void _draw_glyph(int x, int y, const uint8_t *glyph, int scale, uint8_t color)
{
    for (int col = 0; col < FONT_CHAR_W; col++) {
        uint8_t col_data = glyph[col];
        for (int row = 0; row < FONT_CHAR_H; row++) {
            if (col_data & (1 << row)) {
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        epd_draw_pixel(x + col*scale + sx,
                                       y + row*scale + sy, color);
            }
        }
    }
}

// ─── Internal: circle helper — port langsung dari Adafruit drawCircleHelper()
// cornername = bitmask kuadran mana yang digambar:
//   bit 0 (0x1) = kiri-atas
//   bit 1 (0x2) = kanan-atas
//   bit 2 (0x4) = kanan-bawah
//   bit 3 (0x8) = kiri-bawah
static void _draw_circle_helper(int cx, int cy, int r, uint8_t cornername, uint8_t color)
{
    int f     = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x     = 0;
    int y     = r;

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;

        if (cornername & 0x4) {   // kanan-bawah
            epd_draw_pixel(cx + x, cy + y, color);
            epd_draw_pixel(cx + y, cy + x, color);
        }
        if (cornername & 0x2) {   // kanan-atas
            epd_draw_pixel(cx + x, cy - y, color);
            epd_draw_pixel(cx + y, cy - x, color);
        }
        if (cornername & 0x8) {   // kiri-bawah
            epd_draw_pixel(cx - y, cy + x, color);
            epd_draw_pixel(cx - x, cy + y, color);
        }
        if (cornername & 0x1) {   // kiri-atas
            epd_draw_pixel(cx - y, cy - x, color);
            epd_draw_pixel(cx - x, cy - y, color);
        }
    }
}

// ─── Internal: fill circle helper — port dari Adafruit fillCircleHelper()
// corners = bitmask sisi mana (1=kanan, 2=kiri)
// delta   = extra height untuk stretch ke fillRoundRect (0 untuk circle biasa)
static void _fill_circle_helper(int cx, int cy, int r, uint8_t corners, int delta, uint8_t color)
{
    int f     = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x     = 0;
    int y     = r;
    int px    = x;
    int py    = y;

    delta++;  // Adafruit: "Avoid some +1's in the loop"

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;

        // Cek x < y+1 untuk hindari double-draw di diagonal
        if (x < (y + 1)) {
            if (corners & 1) epd_draw_vline(cx + x, cy - y, 2*y + delta, color);
            if (corners & 2) epd_draw_vline(cx - x, cy - y, 2*y + delta, color);
        }
        if (y != py) {
            if (corners & 1) epd_draw_vline(cx + py, cy - px, 2*px + delta, color);
            if (corners & 2) epd_draw_vline(cx - py, cy - px, 2*px + delta, color);
            py = y;
        }
        px = x;
    }
}

// ─── Text ─────────────────────────────────────────────────────────────────────

int epd_draw_char(int x, int y, char c, epd_font_t font, uint8_t color)
{
    int scale = (int)font + 1;
    if (c < 32 || c > 126) c = '?';
    _draw_glyph(x, y, font5x7[c - 32], scale, color);
    return (FONT_CHAR_W + FONT_SPACING) * scale;
}

int epd_draw_string(int x, int y, const char *str, epd_font_t font, uint8_t color)
{
    int scale  = (int)font + 1;
    int char_h = FONT_CHAR_H * scale;
    int char_w = (FONT_CHAR_W + FONT_SPACING) * scale;
    int cur_x  = x, cur_y = y;

    for (const char *p = str; *p; p++) {
        if (*p == '\n' || cur_x + char_w > EPD_WIDTH) {
            cur_x  = x;
            cur_y += char_h + scale;
        }
        if (*p == '\n') continue;
        if (cur_y + char_h > EPD_HEIGHT) break;
        cur_x += epd_draw_char(cur_x, cur_y, *p, font, color);
    }
    return cur_y;
}

// ─── Lines ────────────────────────────────────────────────────────────────────

void epd_draw_hline(int x, int y, int w, uint8_t color)
{
    for (int i = 0; i < w; i++) epd_draw_pixel(x + i, y, color);
}

void epd_draw_vline(int x, int y, int h, uint8_t color)
{
    for (int i = 0; i < h; i++) epd_draw_pixel(x, y + i, color);
}

void epd_draw_line(int x0, int y0, int x1, int y1, uint8_t color)
{
    // Optimasi H/V langsung
    if (x0 == x1) {
        if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
        epd_draw_vline(x0, y0, y1 - y0 + 1, color);
        return;
    }
    if (y0 == y1) {
        if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
        epd_draw_hline(x0, y0, x1 - x0 + 1, color);
        return;
    }

    // Bresenham — port dari Adafruit writeLine() dengan steep swap trick
    int steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep)   { int t; t=x0; x0=y0; y0=t; t=x1; x1=y1; y1=t; }
    if (x0 > x1) { int t; t=x0; x0=x1; x1=t; t=y0; y0=y1; y1=t; }

    int dx     = x1 - x0;
    int dy     = abs(y1 - y0);
    int err    = dx / 2;
    int ystep  = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++) {
        if (steep) epd_draw_pixel(y0, x0, color);
        else       epd_draw_pixel(x0, y0, color);
        err -= dy;
        if (err < 0) { y0 += ystep; err += dx; }
    }
}

// ─── Rectangles ───────────────────────────────────────────────────────────────

void epd_draw_rect(int x, int y, int w, int h, uint8_t color)
{
    epd_draw_hline(x,         y,         w, color);
    epd_draw_hline(x,         y + h - 1, w, color);
    epd_draw_vline(x,         y,         h, color);
    epd_draw_vline(x + w - 1, y,         h, color);
}

void epd_fill_rect(int x, int y, int w, int h, uint8_t color)
{
    for (int i = 0; i < h; i++) epd_draw_hline(x, y + i, w, color);
}

// Port dari Adafruit drawRoundRect() — pakai _draw_circle_helper per sudut
// dengan corner mask yang sama persis
void epd_draw_round_rect(int x, int y, int w, int h, int r, uint8_t color)
{
    int max_r = ((w < h) ? w : h) / 2;
    if (r > max_r) r = max_r;

    epd_draw_hline(x + r,     y,         w - 2*r, color);  // atas
    epd_draw_hline(x + r,     y + h - 1, w - 2*r, color);  // bawah
    epd_draw_vline(x,         y + r,     h - 2*r, color);  // kiri
    epd_draw_vline(x + w - 1, y + r,     h - 2*r, color);  // kanan

    // Corner mask: 0x1=kiri-atas, 0x2=kanan-atas, 0x4=kanan-bawah, 0x8=kiri-bawah
    _draw_circle_helper(x + r,         y + r,         r, 0x1, color);
    _draw_circle_helper(x + w - r - 1, y + r,         r, 0x2, color);
    _draw_circle_helper(x + w - r - 1, y + h - r - 1, r, 0x4, color);
    _draw_circle_helper(x + r,         y + h - r - 1, r, 0x8, color);
}

// Port dari Adafruit fillRoundRect() — pakai _fill_circle_helper dengan delta
void epd_fill_round_rect(int x, int y, int w, int h, int r, uint8_t color)
{
    int max_r = ((w < h) ? w : h) / 2;
    if (r > max_r) r = max_r;

    // Fill bagian tengah (persegi panjang tanpa sudut)
    epd_fill_rect(x + r, y, w - 2*r, h, color);

    // Fill sudut kanan (corners=1) dan kiri (corners=2)
    // delta = h - 2*r - 1 untuk stretch vline sepanjang sisi rounded rect
    _fill_circle_helper(x + w - r - 1, y + r, r, 1, h - 2*r - 1, color);
    _fill_circle_helper(x + r,         y + r, r, 2, h - 2*r - 1, color);
}

// ─── Circles ──────────────────────────────────────────────────────────────────

// Port dari Adafruit drawCircle() — pakai f/ddF_x/ddF_y, bukan d = 3-2r
void epd_draw_circle(int cx, int cy, int r, uint8_t color)
{
    int f     = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x     = 0;
    int y     = r;

    // 4 titik cardinal dulu
    epd_draw_pixel(cx,     cy + r, color);
    epd_draw_pixel(cx,     cy - r, color);
    epd_draw_pixel(cx + r, cy,     color);
    epd_draw_pixel(cx - r, cy,     color);

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;

        epd_draw_pixel(cx + x, cy + y, color);
        epd_draw_pixel(cx - x, cy + y, color);
        epd_draw_pixel(cx + x, cy - y, color);
        epd_draw_pixel(cx - x, cy - y, color);
        epd_draw_pixel(cx + y, cy + x, color);
        epd_draw_pixel(cx - y, cy + x, color);
        epd_draw_pixel(cx + y, cy - x, color);
        epd_draw_pixel(cx - y, cy - x, color);
    }
}

// Port dari Adafruit fillCircle()
void epd_fill_circle(int cx, int cy, int r, uint8_t color)
{
    epd_draw_vline(cx, cy - r, 2*r + 1, color);         // garis tengah vertikal
    _fill_circle_helper(cx, cy, r, 3, 0, color);        // corners=3 = kanan+kiri
}

// ─── Triangles ────────────────────────────────────────────────────────────────

void epd_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
{
    epd_draw_line(x0, y0, x1, y1, color);
    epd_draw_line(x1, y1, x2, y2, color);
    epd_draw_line(x2, y2, x0, y0, color);
}

// Port dari Adafruit fillTriangle() — pakai int32_t untuk sa/sb agar tidak overflow
void epd_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
{
    int a, b, y, last;
    int t;

    // Sort: y2 >= y1 >= y0
    if (y0 > y1) { t=y0; y0=y1; y1=t; t=x0; x0=x1; x1=t; }
    if (y1 > y2) { t=y1; y1=y2; y2=t; t=x1; x1=x2; x2=t; }
    if (y0 > y1) { t=y0; y0=y1; y1=t; t=x0; x0=x1; x1=t; }

    // Edge case: semua titik satu baris horizontal
    if (y0 == y2) {
        a = b = x0;
        if (x1 < a) a = x1; else if (x1 > b) b = x1;
        if (x2 < a) a = x2; else if (x2 > b) b = x2;
        epd_draw_hline(a, y0, b - a + 1, color);
        return;
    }

    int dx01 = x1 - x0, dy01 = y1 - y0;
    int dx02 = x2 - x0, dy02 = y2 - y0;
    int dx12 = x2 - x1, dy12 = y2 - y1;

    // PENTING: pakai int32_t sama seperti Adafruit untuk hindari overflow
    int32_t sa = 0, sb = 0;

    last = (y1 == y2) ? y1 : y1 - 1;

    // Bagian atas segitiga (dari y0 ke y1)
    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        if (a > b) { t=a; a=b; b=t; }
        epd_draw_hline(a, y, b - a + 1, color);
    }

    // Bagian bawah segitiga (dari y1 ke y2)
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if (a > b) { t=a; a=b; b=t; }
        epd_draw_hline(a, y, b - a + 1, color);
    }
}

// ─── Bitmap ───────────────────────────────────────────────────────────────────

// Port dari Adafruit drawBitmap() RAM-resident, transparent background
// Format: 1bpp MSB-first, sama dengan Adafruit
void epd_draw_bitmap(int x, int y, const uint8_t *bitmap, int w, int h, uint8_t color)
{
    int byte_width = (w + 7) / 8;
    uint8_t b = 0;

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            // Adafruit: load byte baru tiap 8 pixel, lalu shift kiri
            if (i & 7) b <<= 1;
            else       b = bitmap[j * byte_width + i / 8];
            if (b & 0x80) epd_draw_pixel(x + i, y + j, color);
        }
    }
}