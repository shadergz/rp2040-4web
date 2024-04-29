#include <string.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <pthread.h>

wifi_config_t wlset = {
    .sta = {
        .threshold.rssi = -127,
        .pmf_cfg.capable = true,
    }
};
wifi_ap_record_t router = {};
char ip[16] = {};

extern bool started,
    connected,
    configured;
extern esp_err_t ewifi;
extern pthread_cond_t wifis;
extern pthread_mutex_t wifim;

const char* getevestr(esp_event_base_t b, int32_t e) {
    if (b == IP_EVENT) {
        switch (e) {
        case IP_EVENT_STA_GOT_IP:
            return "station got IP from connected AP";
        case IP_EVENT_STA_LOST_IP:
            return "station lost IP and the IP is reset to 0";
        }
    }
    switch (e) {
    case WIFI_EVENT_SCAN_DONE:
        return "Finished scanning AP";
    case WIFI_EVENT_STA_START:
        return "Station start";
    case WIFI_EVENT_STA_STOP:
        return "Station stop";
    case WIFI_EVENT_STA_CONNECTED:
        return "Station connected to AP";
    case WIFI_EVENT_STA_DISCONNECTED:
        return "Station disconnected from AP";
    case WIFI_EVENT_STA_BEACON_TIMEOUT:
        return "Station beacon timeout";
    default:
        return NULL;
    }
}
static void wifi_eve(void* arg, esp_event_base_t base, int32_t eid, void* data) {
    ip_event_got_ip_t* ipd = data;
    pthread_mutex_lock(&wifim);
    switch (eid) {
        case WIFI_EVENT_STA_START: started = true; break;
        case WIFI_EVENT_STA_STOP: started = false; break;
        case WIFI_EVENT_STA_CONNECTED: connected = true; break;
        case WIFI_EVENT_STA_DISCONNECTED:
            configured = connected = false; break;
        case WIFI_EVENT_STA_BEACON_TIMEOUT:
            break;
    }
    if (base == IP_EVENT && eid == IP_EVENT_STA_GOT_IP && connected)
        sprintf(ip, IPSTR, IP2STR(&ipd->ip_info.ip));
    else if (base == IP_EVENT && eid == IP_EVENT_STA_LOST_IP)
        memset(ip, 0, sizeof(ip));
    ESP_LOGI("host", "Received WiFi event: %s", getevestr(base, eid));
    if (connected)
        ewifi = esp_wifi_sta_get_ap_info(&router);
    else
        memset(&router, 0, sizeof(router));
    pthread_cond_signal(&wifis);
    pthread_mutex_unlock(&wifim);
}

const char wifi_ssid[] = "Gabriel TI";
const char wifi_pass[] = "1realdepao";
void wifi_find_ap(uint16_t* cnt, bool* caught) {
    *cnt = 0;

    bzero(&wlset.sta.ssid, 32);
    bzero(&wlset.sta.password, 64);
    esp_wifi_set_config(WIFI_IF_STA, &wlset);
    do {
        if (*caught)
            return;
        esp_wifi_scan_start(NULL, true);
        if (esp_wifi_scan_get_ap_num(cnt) != ESP_OK)
            *cnt = 0;
    } while (!*cnt);
    ESP_LOGI("host", "The WiFi network scan found %u open access points", *cnt);

    wifi_ap_record_t recp[0xa];
    uint16_t recs = 0xa > *cnt ? *cnt : 0xa;
    esp_wifi_scan_get_ap_records(&recs, recp);
    for (uint16_t id = 0; id < recs && !*caught; id++) {
        const wifi_ap_record_t* ap = &recp[id];
        if (strcmp((char*)&ap->ssid, wifi_ssid))
            continue;
        strcpy((char*)&wlset.sta.ssid, wifi_ssid);
        strcpy((char*)&wlset.sta.password, wifi_pass);
        *caught = true;
    }
    if (!*caught)
        return;

    ESP_LOGI("host", "Name of the chosen AP: %s", wlset.sta.ssid);
    ESP_LOG_BUFFER_HEXDUMP("host", &wlset, sizeof(wlset), ESP_LOG_INFO);

    esp_wifi_clear_ap_list();
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wlset));
}
void wifi_on() {
    ESP_ERROR_CHECK(esp_netif_init());
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t conf = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&conf));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_eve, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_eve, NULL, NULL));

    wlset.sta.scan_method = WIFI_CONNECT_AP_BY_SECURITY;
    wlset.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_wifi_start();
}
void wifi_off() {
    esp_wifi_disconnect();
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
}
