#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "epaper.h"
#include "epaper_gfx.h"

static const char *TAG = "main";

// ─── Konfigurasi ──────────────────────────────────────────────────────────────
#define WIFI_SSID           "xxxxxxx"
#define WIFI_PASSWORD       "xxxxxxx"
#define WIFI_MAX_RETRY      5

// Interval refresh layar — ubah sesuai kebutuhan
#define REFRESH_INTERVAL_MS (2 * 60 * 1000)   // 2 menit

// ─── Event group ─────────────────────────────────────────────────────────────
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_count = 0;

// ─── State global ─────────────────────────────────────────────────────────────
static char  s_ssid[33]  = "---";
static char  s_ip[16]    = "---";
static int   s_rssi      = 0;
static bool  s_connected = false;

// Flag untuk trigger render dari timer — hindari render langsung di ISR/timer
static volatile bool s_needs_refresh = false;

// Boot time untuk hitung uptime
static int64_t s_boot_time_us = 0;

// ─────────────────────────────────────────────────────────────────────────────
// WiFi
// ─────────────────────────────────────────────────────────────────────────────

static void _wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        if (s_retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_count++;
            ESP_LOGW(TAG, "WiFi retry %d/%d", s_retry_count, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(s_ip, sizeof(s_ip), IPSTR, IP2STR(&event->ip_info.ip));

        wifi_ap_record_t ap;
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            strncpy(s_ssid, (char *)ap.ssid, sizeof(s_ssid) - 1);
            s_rssi = ap.rssi;
        }

        s_connected   = true;
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Connected: %s | IP: %s | RSSI: %d", s_ssid, s_ip, s_rssi);
    }
}

static void _wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &_wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid               = WIFI_SSID,
            .password           = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    xEventGroupWaitBits(s_wifi_event_group,
                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                        pdFALSE, pdFALSE,
                        pdMS_TO_TICKS(10000));
}

// ─────────────────────────────────────────────────────────────────────────────
// Timer callback — hanya set flag, JANGAN render di sini
// Render di task utama untuk menghindari masalah stack/priority
// ─────────────────────────────────────────────────────────────────────────────

static void _refresh_timer_cb(TimerHandle_t xTimer)
{
    s_needs_refresh = true;
    ESP_LOGD(TAG, "Refresh flag set by timer");
}

// ─────────────────────────────────────────────────────────────────────────────
// Update data sebelum render — ambil RSSI terbaru dari driver WiFi
// ─────────────────────────────────────────────────────────────────────────────

static void _update_data(void)
{
    if (!s_connected) return;

    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        s_rssi = ap.rssi;
        // SSID dan IP tidak berubah selama koneksi aktif, skip update
        ESP_LOGI(TAG, "Data updated: RSSI=%d dBm", s_rssi);
    } else {
        // Bisa jadi koneksi drop tapi event belum diterima
        ESP_LOGW(TAG, "Failed to get AP info — connection may have dropped");
        s_connected = false;
        memcpy(s_ip, "---", 4);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Drawing helpers
// ─────────────────────────────────────────────────────────────────────────────

static int _rssi_to_percent(int rssi)
{
    if (rssi >= -50)  return 100;
    if (rssi <= -100) return 0;
    return 2 * (rssi + 100);
}

static void _draw_signal_bars(int x, int y, int percent, uint8_t color)
{
    int bar_w  = 4;
    int gap    = 2;
    int bars   = 5;
    int filled = (percent * bars + 50) / 100;

    for (int i = 0; i < bars; i++) {
        int bar_h = 3 + i * 2;          // 3,5,7,9,11 px
        int bx    = x + i * (bar_w + gap);
        int by    = y + (11 - bar_h);   // align bottom

        if (i < filled)
            epd_fill_rect(bx, by, bar_w, bar_h, color);
        else
            epd_draw_rect(bx, by, bar_w, bar_h, color);
    }
}

static void _format_uptime(char *buf, size_t len)
{
    // Hitung dari boot time yang disimpan di awal
    int64_t elapsed_us  = esp_timer_get_time() - s_boot_time_us;
    uint32_t sec_total  = (uint32_t)(elapsed_us / 1000000ULL);
    uint32_t hours      = sec_total / 3600;
    uint32_t mins       = (sec_total % 3600) / 60;
    uint32_t secs       = sec_total % 60;
    snprintf(buf, len, "%02lu:%02lu:%02lu",
             (unsigned long)hours,
             (unsigned long)mins,
             (unsigned long)secs);
}

// ─────────────────────────────────────────────────────────────────────────────
// Render layar — dipanggil setiap kali s_needs_refresh = true
// ─────────────────────────────────────────────────────────────────────────────

static void _render_screen(void)
{
    char buf[48];
    int  y = 0;

    ESP_LOGI(TAG, "Rendering screen...");

    epd_wake();           // bangunkan panel (reset + init ulang)
    epd_clear_buffer();

    // ── Header ───────────────────────────────────────────────────────────────
    epd_fill_rect(0, y, EPD_WIDTH, 16, EPD_BLACK);
    epd_draw_string(4, y + 4, "ePaper Messenger", FONT_SMALL, EPD_WHITE);
    y += 17;
    epd_draw_hline(0, y, EPD_WIDTH, EPD_BLACK);
    y += 4;

    // ── WiFi ─────────────────────────────────────────────────────────────────
    if (s_connected) {
        snprintf(buf, sizeof(buf), "WiFi : %.20s", s_ssid);
        epd_draw_string(4, y, buf, FONT_SMALL, EPD_BLACK);
        y += 12;

        snprintf(buf, sizeof(buf), "IP   : %s", s_ip);
        epd_draw_string(4, y, buf, FONT_SMALL, EPD_BLACK);
        y += 12;

        int pct = _rssi_to_percent(s_rssi);
        snprintf(buf, sizeof(buf), "RSSI : %d dBm %d%%", s_rssi, pct);
        epd_draw_string(4, y, buf, FONT_SMALL, EPD_BLACK);

        // Signal bar di kanan, sejajar baris RSSI
        _draw_signal_bars(EPD_WIDTH - 40, y, pct, EPD_BLACK);
        y += 12;

    } else {
        epd_draw_string(4, y, "WiFi : Disconnected", FONT_SMALL, EPD_BLACK);
        y += 12;
        epd_draw_string(4, y, "IP   : ---",          FONT_SMALL, EPD_BLACK);
        y += 12;
        epd_draw_string(4, y, "RSSI : ---",          FONT_SMALL, EPD_BLACK);
        y += 12;
    }

    epd_draw_hline(0, y, EPD_WIDTH, EPD_BLACK);
    y += 4;

    // ── Hello World ───────────────────────────────────────────────────────────
    epd_draw_string(4, y, "Hello, World!", FONT_MEDIUM, EPD_BLACK);
    y += 18;

    epd_draw_string(4, y, "Toaster Co Ltd",    FONT_SMALL, EPD_BLACK); y += 10;
    epd_draw_string(4, y, "ESP32 + SSD1680",   FONT_SMALL, EPD_BLACK); y += 10;
    epd_draw_string(4, y, "MJ Mokhtar", FONT_SMALL, EPD_BLACK); y += 12;

    epd_draw_hline(0, y, EPD_WIDTH, EPD_BLACK);
    y += 4;
    // ── Footer ────────────────────────────────────────────────────────────────
    epd_draw_hline(0, EPD_HEIGHT - 12, EPD_WIDTH, EPD_BLACK);
    epd_draw_string(4, EPD_HEIGHT - 10, "Made with <3 by MJ Mokhtar", FONT_SMALL, EPD_BLACK);

    // Kirim ke panel
    epd_display();      // blocking ~2-8 detik

    // Tidurkan panel setelah selesai untuk hemat daya
    epd_sleep();

    ESP_LOGI(TAG, "Render done, panel sleeping");
}

// ─────────────────────────────────────────────────────────────────────────────
// app_main
// ─────────────────────────────────────────────────────────────────────────────

void app_main(void)
{
    // Simpan boot time
    s_boot_time_us = esp_timer_get_time();

    ESP_LOGI(TAG, "=== ePaper Messenger v0.2.0 ===");

    // 1. NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. ePaper init
    epd_init();

    // 3. Layar "Connecting..." saat boot
    epd_clear_buffer();
    epd_fill_rect(0, 0, EPD_WIDTH, 16, EPD_BLACK);
    epd_draw_string(4, 4,  "ePaper Messenger", FONT_SMALL, EPD_WHITE);
    epd_draw_hline(0, 17,  EPD_WIDTH, EPD_BLACK);
    epd_draw_string(4, 28, "Connecting to WiFi", FONT_SMALL, EPD_BLACK);
    epd_draw_string(4, 40, WIFI_SSID,            FONT_SMALL, EPD_BLACK);
    epd_draw_string(4, 56, "Please wait...",     FONT_SMALL, EPD_BLACK);
    epd_display();
    epd_sleep();    // tidur setelah tampil layar connecting

    // 4. WiFi init (blocking)
    _wifi_init();

    // 5. Render pertama langsung
    _update_data();
    _render_screen();

    // 6. Setup FreeRTOS Software Timer untuk refresh periodik
    TimerHandle_t refresh_timer = xTimerCreate(
        "refresh",                          // nama timer
        pdMS_TO_TICKS(REFRESH_INTERVAL_MS), // periode
        pdTRUE,                             // auto-reload (bukan one-shot)
        NULL,                               // timer ID (tidak dipakai)
        _refresh_timer_cb                   // callback
    );

    if (refresh_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create refresh timer!");
    } else {
        xTimerStart(refresh_timer, 0);
        ESP_LOGI(TAG, "Refresh timer started: every %d ms", REFRESH_INTERVAL_MS);
    }

    // 7. Main loop — tunggu flag dari timer, lalu update & render
    while (1) {
        if (s_needs_refresh) {
            s_needs_refresh = false;    // clear flag duluan sebelum proses

            _update_data();             // ambil RSSI terbaru dari driver WiFi
            _render_screen();           // render + display + sleep panel
        }

        // Delay pendek agar task lain dapat jatah CPU
        // Tidak perlu delay lama karena render sudah blocking ~2-8 detik
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}