#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <esp_log.h>

bool wifi = false,
    started = false,
    connected = false,
    configured = false;
esp_err_t ewifi = ESP_OK;

extern wifi_ap_record_t router;
extern char ip[16];
extern void wifi_reconn(uint16_t* cnt, bool* founded);
void commit_sta_conn() {
    if (configured && wifi)
        return;

    uint16_t apid;
    wifi_reconn(&apid, &configured);
    if (started || !configured)
        return;
    esp_wifi_start();
}
bool chk_wifi() {
    wifi = connected && started && configured && *router.ssid != '\0';
    if (wifi)
        return true;
    if (ewifi == ESP_ERR_WIFI_SSID)
        configured = started = false;
    const char* errs = esp_err_to_name(ewifi);
    if (ewifi == ESP_OK)
        return true;
    ESP_LOGE("host", "Connection issues due to: %s", errs);
    return false;
}

void make_checkpoint() {
    char mac[13] = {};
    commit_sta_conn();
    if (!chk_wifi())
        return;
    const char hex[] = "0123456789abcdef";
    for (uint8_t le = 0; le < 6; le++) {
        mac[le * 2] = hex[(router.bssid[le] >> 4)];
        mac[le * 2 + 1] = hex[(router.bssid[le] & 0xf)];
    }
    if (!started || !wifi)
        return;
    ESP_LOGI("host", "We are connected to (%s), MAC address %s, IP %s", router.ssid, mac, ip);
}

extern void wifi_on();
extern void wifi_off();
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_on();

	ESP_LOGI("host", "ESP32's WIFI module initialized");
    esp_sleep_enable_timer_wakeup(0.8 * 1000000);

    for (;;) {
        make_checkpoint();

        ESP_LOGI("host", "Going to sleep for 0.8 seconds");
        esp_light_sleep_start();
    }
    wifi_off();
}
