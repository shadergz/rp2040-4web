#include <string.h>
#include <esp_wifi.h>
#include <esp_log.h>

wifi_config_t wlset = {};
wifi_ap_record_t router = {};
char ip[16] = {};

extern bool started;
extern bool connected;
extern esp_err_t ewifi;
static void wifi_eve(void* arg, esp_event_base_t base, int32_t eid, void* data) {
    ip_event_got_ip_t* ipd = data;
    switch (eid) {
        case WIFI_EVENT_STA_START: started = true; break;
        case WIFI_EVENT_STA_STOP: started = false; break;
        case WIFI_EVENT_STA_CONNECTED: connected = true; break;
        case WIFI_EVENT_STA_DISCONNECTED: connected = false; break;
    }
    if (started)
        ewifi = esp_wifi_connect();
    else
        esp_wifi_disconnect();
    if (connected)
        ewifi = esp_wifi_sta_get_ap_info(&router);
    else
        memset(&router, 0, sizeof(router));

    if (eid == IP_EVENT_STA_GOT_IP)
        sprintf(ip, IPSTR, IP2STR(&ipd->ip_info.ip));
}

void wifi_on() {
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();
    esp_event_loop_create_default();

    wifi_init_config_t conf = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&conf));

    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_eve, NULL, NULL);
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_eve, NULL, NULL);

    wlset.sta.scan_method = WIFI_CONNECT_AP_BY_SECURITY;
    wlset.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
	wlset.sta.threshold.rssi = -127;
	wlset.sta.pmf_cfg.capable = true;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}
void wifi_off() {
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
}
