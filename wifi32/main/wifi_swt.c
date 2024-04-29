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

extern bool started;
extern bool connected;
extern esp_err_t ewifi;
extern pthread_cond_t wifis;
extern pthread_mutex_t wifim;

const char* getevestr(int32_t e) {
    switch (e) {
    case WIFI_EVENT_STA_START:
        return "WIFI_EVENT_STA_START";
    case WIFI_EVENT_STA_STOP:
        return "WIFI_EVENT_STA_STOP";
    case WIFI_EVENT_STA_CONNECTED:
        return "WIFI_EVENT_STA_CONNECTED";
    case WIFI_EVENT_STA_DISCONNECTED:
        return "WIFI_EVENT_STA_DISCONNECTED";
    case WIFI_EVENT_STA_BEACON_TIMEOUT:
        return "WIFI_EVENT_STA_BEACON_TIMEOUT";
    default:
        return NULL;
    }
}

static void wifi_eve(void* arg, esp_event_base_t base, int32_t eid, void* data) {
    ip_event_got_ip_t* ipd = data;
    pthread_mutex_lock(&wifim);
    switch (eid) {
        case WIFI_EVENT_STA_START:
        case WIFI_EVENT_STA_STOP:
            started = !started;
            break;
        case WIFI_EVENT_STA_CONNECTED:
        case WIFI_EVENT_STA_DISCONNECTED:
        case WIFI_EVENT_STA_BEACON_TIMEOUT:
            connected = !connected;
    }
    if (eid == WIFI_EVENT_STA_DISCONNECTED || eid == WIFI_EVENT_STA_BEACON_TIMEOUT)
    if (eid == IP_EVENT_STA_GOT_IP && connected)
        sprintf(ip, IPSTR, IP2STR(&ipd->ip_info.ip));

    char eves[64] = {};
    sprintf(eves, "%s:?", base == IP_EVENT ? "IP" : base == WIFI_EVENT ? "WIFI" : "");
    if (getevestr(eid) != NULL)
        sprintf(strchr(eves, '?'), "%s", getevestr(eid));
    else
        sprintf(strchr(eves, '?'), "%d", (int)eid);

    ESP_LOGI("host", "Received WiFi event: %s, value (PTR): %p", eves, data);
    if (eid == WIFI_EVENT_STA_CONNECTED || eid == WIFI_EVENT_STA_DISCONNECTED) {
        if (connected)
            ewifi = esp_wifi_sta_get_ap_info(&router);
        else
            memset(&router, 0, sizeof(router));
        pthread_cond_signal(&wifis);
    }
    pthread_mutex_unlock(&wifim);
}

const char wifi_ssid[] = "Gabriel TI";
const char wifi_pass[] = "1realdepao";
void wifi_find_ap(uint16_t* cnt, bool* found) {
    *cnt = 0;
    do {
        if (*found)
            return;
        esp_wifi_scan_start(NULL, true);
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
