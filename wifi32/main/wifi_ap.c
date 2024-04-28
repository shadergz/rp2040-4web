#include <string.h>
#include <stdbool.h>

#include <esp_wifi.h>
#include <esp_log.h>
const char wifi_ssid[] = "Gabriel TI";
const char wifi_pass[] = "1realdepao";
extern wifi_config_t wlset;
extern bool wifi;

void wifi_reconn(uint16_t* cnt, bool* found) {
    *cnt = 0;
    do {
        if (*found)
            return;
        esp_wifi_scan_start(NULL, true);
        if (!wifi)
            if (esp_wifi_scan_get_ap_num(cnt) != ESP_OK)
                *cnt = 0;
    } while (!*cnt);
    ESP_LOGI("host", "The WiFi network scan found %u open access points", *cnt);

    wifi_ap_record_t recp[0xa];
    uint16_t recs = 0xa;
    esp_wifi_scan_get_ap_records(&recs, recp);

    for (uint16_t id = 0; id < recs && !*found; id++) {
        const wifi_ap_record_t* ap = &recp[id];
        if (strcmp((char*)&ap->ssid, wifi_ssid))
            continue;
        strcpy((char*)&wlset.sta.ssid, wifi_ssid);
        strcpy((char*)&wlset.sta.password, wifi_pass);
        *found = true;
    }
    if (!*found)
        return;
    ESP_LOGI("host", "Name of the chosen AP: %s", wlset.sta.ssid);
    ESP_LOG_BUFFER_HEXDUMP("host", &wlset, sizeof(wlset), ESP_LOG_INFO);

    esp_wifi_clear_ap_list();
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wlset));
}

