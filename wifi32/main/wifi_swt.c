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

static void wifi_eve(void* arg, esp_event_base_t base, int32_t eid, void* data) {
    ip_event_got_ip_t* ipd = data;
    pthread_mutex_lock(&wifim);
    const char* event = NULL;

    switch (eid) {
        case WIFI_EVENT_STA_START:
            started = true; event = "WIFI_EVENT_STA_START"; break;
        case WIFI_EVENT_STA_STOP:
            started = false; event = "WIFI_EVENT_STA_STOP"; break;
        case WIFI_EVENT_STA_CONNECTED:
            connected = true; event = "WIFI_EVENT_STA_CONNECTED"; break;
        case WIFI_EVENT_STA_DISCONNECTED:
            connected = false; event = "WIFI_EVENT_STA_DISCONNECTED"; break;
        case IP_EVENT_STA_GOT_IP:
            sprintf(ip, IPSTR, IP2STR(&ipd->ip_info.ip)); event = "IP_EVENT_STA_GOT_IP"; break;
    }
    char eve[0x40] = {};
    sprintf(eve, "%s:?", base == IP_EVENT ? "IP" : base == WIFI_EVENT ? "WIFI" : "");
    if (event != NULL)
        sprintf(strchr(eve, '?'), "%s", event);
    else
        sprintf(strchr(eve, '?'), "%d", (int)eid);

    ESP_LOGI("host", "Received WiFi event: %s, value (PTR): %p", eve, data);
    if (eid == WIFI_EVENT_STA_CONNECTED || eid == WIFI_EVENT_STA_DISCONNECTED) {
        if (connected)
            ewifi = esp_wifi_sta_get_ap_info(&router);
        else
            memset(&router, 0, sizeof(router));
        pthread_cond_signal(&wifis);
    }
    pthread_mutex_unlock(&wifim);
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
