idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        epaper
        epaper_gfx
        driver
        esp_wifi
        esp_event
        esp_netif
        esp_timer
        nvs_flash
)