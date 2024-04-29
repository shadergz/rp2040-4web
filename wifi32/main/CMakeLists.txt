idf_component_register(SRCS
        "host_esp.c"
        "wifi_swt.c"
        "." REQUIRES esp_wifi nvs_flash)
