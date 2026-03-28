#include "epaper.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

static const char *TAG = "epaper";

// ─── SSD1680 Command Bytes ────────────────────────────────────────────────────
#define CMD_DRIVER_OUTPUT_CTRL      0x01
#define CMD_DEEP_SLEEP              0x10
#define CMD_DATA_ENTRY_MODE         0x11
#define CMD_SW_RESET                0x12
#define CMD_DISPLAY_UPDATE_CTRL1    0x21
#define CMD_DISPLAY_UPDATE_CTRL2    0x22
#define CMD_WRITE_RAM_BW            0x24
#define CMD_WRITE_RAM_RED           0x26
#define CMD_BORDER_WAVEFORM_CTRL    0x3C
#define CMD_SET_RAM_X_ADDR          0x44
#define CMD_SET_RAM_Y_ADDR          0x45
#define CMD_SET_RAM_X_COUNTER       0x4E
#define CMD_SET_RAM_Y_COUNTER       0x4F
#define CMD_MASTER_ACTIVATION       0x20

// ─── State ───────────────────────────────────────────────────────────────────
static spi_device_handle_t s_spi = NULL;
static uint8_t s_buf[EPD_BUF_SIZE];

// ─── Low-Level SPI ───────────────────────────────────────────────────────────

static void _dc_cmd(void)  { gpio_set_level(EPD_PIN_DC, 0); }
static void _dc_data(void) { gpio_set_level(EPD_PIN_DC, 1); }

static void _spi_write(const uint8_t *data, size_t len)
{
    spi_transaction_t t = {
        .length    = len * 8,
        .tx_buffer = data,
    };
    ESP_ERROR_CHECK(spi_device_transmit(s_spi, &t));
}

static void _send_cmd(uint8_t cmd)
{
    _dc_cmd();
    _spi_write(&cmd, 1);
}

static void _send_data(const uint8_t *data, size_t len)
{
    _dc_data();
    _spi_write(data, len);
}

static void _send_byte(uint8_t byte)
{
    _dc_data();
    _spi_write(&byte, 1);
}

// ─── BUSY Wait ───────────────────────────────────────────────────────────────

static void _wait_busy(void)
{
    // SSD1680: BUSY HIGH = panel sedang proses, tunggu sampai LOW
    while (gpio_get_level(EPD_PIN_BUSY) == 1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ─── Hardware Reset ───────────────────────────────────────────────────────────

static void _hw_reset(void)
{
    gpio_set_level(EPD_PIN_RST, 1); vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(EPD_PIN_RST, 0); vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(EPD_PIN_RST, 1); vTaskDelay(pdMS_TO_TICKS(10));
}

// ─── Panel Init — disesuaikan dengan WeAct reference ─────────────────────────
//
// Perubahan vs versi lama:
//   - X_Start = 0x01, X_End = 0x10  (WeAct, bukan 0x00/0x0F)
//   - Y_Start = 0xF9,0x00  Y_End = 0x00,0x00  (WeAct style, Y decrement)
//   - Data entry mode = 0x01  (Y decrement, X increment — sesuai WeAct)
//   - Border waveform = 0x05  (WeAct, bukan 0x01)
//   - CMD 0x21 dikirim dengan 0x00,0x00  (WeAct kirim ini)
//   - RAM cursor X=0x01, Y=0xF9,0x00  (sesuai WeAct)
//   - HAPUS LUT manual — WeAct pakai OTP internal panel, tidak upload LUT
//   - HAPUS Booster, VCOM, DummyLine, GateWidth, TempSensor
//     karena WeAct tidak kirim ini dan panel sudah punya default OTP

static void _panel_init(void)
{
    _hw_reset();

    // Software reset
    _send_cmd(CMD_SW_RESET);    // 0x12
    _wait_busy();

    // Driver output control
    // Y_Addr_Start_L = 0xF9 (249), Y_Addr_Start_H = 0x00
    _send_cmd(CMD_DRIVER_OUTPUT_CTRL);  // 0x01
    _send_byte(0xF9);   // MUX = 249, Low byte
    _send_byte(0x00);   // High byte
    _send_byte(0x00);   // GD=0, SM=0, TB=0

    // Data entry mode: Y decrement, X increment
    // WeAct pakai 0x01, bukan 0x03
    _send_cmd(CMD_DATA_ENTRY_MODE);     // 0x11
    _send_byte(0x01);

    // Set RAM X address range: 0x01 s/d 0x10
    _send_cmd(CMD_SET_RAM_X_ADDR);      // 0x44
    _send_byte(0x01);   // X start
    _send_byte(0x10);   // X end

    // Set RAM Y address range: Start=0xF9,0x00  End=0x00,0x00
    _send_cmd(CMD_SET_RAM_Y_ADDR);      // 0x45
    _send_byte(0xF9);   // Y start Low
    _send_byte(0x00);   // Y start High
    _send_byte(0x00);   // Y end Low
    _send_byte(0x00);   // Y end High

    // Border waveform: WeAct pakai 0x05
    _send_cmd(CMD_BORDER_WAVEFORM_CTRL); // 0x3C
    _send_byte(0x05);

    // Display update control 1 — WeAct kirim ini
    _send_cmd(CMD_DISPLAY_UPDATE_CTRL1); // 0x21
    _send_byte(0x00);
    _send_byte(0x00);

    // Set RAM X address counter ke start position
    _send_cmd(CMD_SET_RAM_X_COUNTER);   // 0x4E
    _send_byte(0x01);

    // Set RAM Y address counter ke start position
    _send_cmd(CMD_SET_RAM_Y_COUNTER);   // 0x4F
    _send_byte(0xF9);
    _send_byte(0x00);

    // Tidak upload LUT manual — pakai OTP internal panel (seperti WeAct)

    ESP_LOGI(TAG, "SSD1680 initialized (WeAct reference)");
}

// ─── RAM Cursor Reset — dipanggil sebelum tulis framebuffer ──────────────────

static void _set_ram_cursor(void)
{
    // Reset ke posisi start, konsisten dengan WeAct
    _send_cmd(CMD_SET_RAM_X_COUNTER);
    _send_byte(0x01);   // X start

    _send_cmd(CMD_SET_RAM_Y_COUNTER);
    _send_byte(0xF9);   // Y start Low
    _send_byte(0x00);   // Y start High
}

// ─── Public API ──────────────────────────────────────────────────────────────

void epd_init(void)
{
    // Output pins: DC, RST, CS
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << EPD_PIN_DC)  |
                        (1ULL << EPD_PIN_RST) |
                        (1ULL << EPD_PIN_CS),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Input pin: BUSY
    gpio_config_t busy_conf = {
        .pin_bit_mask = (1ULL << EPD_PIN_BUSY),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&busy_conf);

    // SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = EPD_PIN_MOSI,
        .miso_io_num     = -1,
        .sclk_io_num     = EPD_PIN_CLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = EPD_BUF_SIZE + 16,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(EPD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    // SPI device
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = EPD_SPI_FREQ_HZ,
        .mode           = 0,        // CPOL=0, CPHA=0
        .spics_io_num   = EPD_PIN_CS,
        .queue_size     = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(EPD_SPI_HOST, &dev_cfg, &s_spi));

    _panel_init();
    memset(s_buf, EPD_WHITE, EPD_BUF_SIZE);
}

void epd_clear_buffer(void)
{
    memset(s_buf, EPD_WHITE, EPD_BUF_SIZE);
}

void epd_display(void)
{
    // Reset cursor ke posisi awal
    _set_ram_cursor();

    // Tulis BW RAM (0x24) — kirim seluruh framebuffer sekaligus via DMA
    _send_cmd(CMD_WRITE_RAM_BW);
    _send_data(s_buf, EPD_BUF_SIZE);

    // Trigger full refresh — sama persis dengan WeAct
    // 0x22 → sequence: clock on, analog on, display mode 1, analog off, clock off
    _send_cmd(CMD_DISPLAY_UPDATE_CTRL2);
    _send_byte(0xF7);
    _send_cmd(CMD_MASTER_ACTIVATION);   // 0x20
    _wait_busy();

    ESP_LOGI(TAG, "Display refreshed");
}

void epd_clear_screen(void)
{
    epd_clear_buffer();
    epd_display();
}

void epd_sleep(void)
{
    _send_cmd(CMD_DEEP_SLEEP);  // 0x10
    _send_byte(0x01);
    ESP_LOGI(TAG, "Panel in deep sleep");
}

void epd_wake(void)
{
    // SSD1680 deep sleep tidak bisa di-wake dengan command biasa
    // Satu-satunya cara: hardware reset + full init ulang
    _panel_init();
    memset(s_buf, EPD_WHITE, EPD_BUF_SIZE);
    ESP_LOGI(TAG, "Panel woke from deep sleep");
}

void epd_draw_pixel(int x, int y, uint8_t color)
{
    if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) return;

    int px = y;
    int py = x;

    int byte_idx = py * EPD_BUF_WIDTH + (px / 8);
    int bit_pos  = 7 - (px % 8);

    if (color == EPD_BLACK)
        s_buf[byte_idx] &= ~(1 << bit_pos);
    else
        s_buf[byte_idx] |=  (1 << bit_pos);
}

uint8_t *epd_get_buffer(void)
{
    return s_buf;
}