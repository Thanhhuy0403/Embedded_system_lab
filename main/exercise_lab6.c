#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <stdio.h>
#include <stdlib.h>

// static const char *TAG = "WiFi_Scan";

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        uint16_t apCount = 0;
        esp_wifi_scan_get_ap_num(&apCount);
        printf("Number of access points found: %d\n", apCount);

        if (apCount == 0) return;

        wifi_ap_record_t *list = malloc(sizeof(wifi_ap_record_t) * apCount);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, list));

        for (int i = 0; i < apCount; i++) {
            printf("SSID: %s, RSSI: %d, AUTH: %d\n",
                   (char *)list[i].ssid,
                   list[i].rssi,
                   list[i].authmode);
        }

        free(list);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Register WiFi scan done event
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        WIFI_EVENT_SCAN_DONE,
        &wifi_event_handler,
        NULL,
        NULL
    ));

    ESP_ERROR_CHECK(esp_wifi_start());

    uint8_t ssid[] = "ACLAB";   // SSID kiểu uint8_t

    wifi_scan_config_t scanConf = {
        .ssid = ssid,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };

    printf("Starting WiFi scan...\n");
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, false));
}
