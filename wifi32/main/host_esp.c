#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <pthread.h>
bool wifi = false,
    started = false,
    connected = false,
    configured = false;
esp_err_t ewifi = ESP_OK;

pthread_cond_t wifis;
pthread_mutex_t wifim;

extern wifi_ap_record_t router;
extern char ip[16];
extern void wifi_find_ap(uint16_t* cnt, bool* founded);

void check_wifi() {
    static char mac[19] = {};
    pthread_mutex_lock(&wifim);
    if (!configured && !wifi) {
        static uint16_t apid;
        wifi_find_ap(&apid, &configured);

        if (configured && apid)
            esp_wifi_connect();
    }
    uint32_t slen = strlen((char*)router.ssid);
    wifi = started && configured && connected;

    if (!wifi && configured) {
        memset(mac, 0, sizeof(mac));
        ewifi = esp_wifi_connect();
        pthread_cond_wait(&wifis, &wifim);
        pthread_mutex_unlock(&wifim);
    }
    if (!strlen(mac) && slen && connected) {
        const char hex[] = "0123456789abcdef";
        char* maci = (char*)mac;
        for (uint8_t le = 0; le < 6; le++) {
            *maci++ = hex[(router.bssid[le] >> 4)];
            *maci++ = hex[(router.bssid[le] & 0xf)];
            if (le != 5)
                *maci++ = ':';
        }
    }
    switch (ewifi) {
    case ESP_OK:
        if (strlen(mac) && wifi)
            ESP_LOGI("host", "We are connected to (%s), MAC address %s, IP %s", router.ssid, mac, ip);
        break;
    case ESP_ERR_WIFI_SSID:
        configured = connected = false;
        break;
    default:
        ESP_LOGE("host", "Connection issues due to: %s", esp_err_to_name(ewifi));
    }
    pthread_mutex_trylock(&wifim);
    pthread_mutex_unlock(&wifim);
}

extern void wifi_on();
extern void wifi_off();
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi = started = connected = configured = false;

    pthread_cond_init(&wifis, NULL);
    pthread_mutex_init(&wifim, NULL);

    wifi_on();
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM));
    ESP_LOGI("host", "ESP32's WIFI module initialized");
    esp_sleep_enable_timer_wakeup(1000000);

    for (;;) {
        check_wifi();

        ESP_LOGI("host", "Going to sleep for 1 seconds");
        esp_light_sleep_start();
    }
    pthread_cond_destroy(&wifis);
    pthread_mutex_destroy(&wifim);
    wifi_off();
}
