#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "WiFi_STA";

#define WIFI_SSID      "ACLAB"
#define WIFI_PASSWORD  "ACLAB2023"

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_SCAN_DONE:
                ESP_LOGI(TAG, "WiFi scan completed");
                {
                    uint16_t apCount = 0;
                    esp_wifi_scan_get_ap_num(&apCount);
                    ESP_LOGI(TAG, "Number of access points found: %d", apCount);

                    if (apCount == 0) {
                        ESP_LOGW(TAG, "No access points found");
                        return;
                    }

                    wifi_ap_record_t *list = malloc(sizeof(wifi_ap_record_t) * apCount);
                    if (list == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate memory for AP list");
                        return;
                    }

                    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, list));

                    ESP_LOGI(TAG, "=== WiFi Scan Results ===");
                    for (int i = 0; i < apCount; i++) {
                        const char *auth_mode_str;
                        switch (list[i].authmode) {
                            case WIFI_AUTH_OPEN:
                                auth_mode_str = "OPEN";
                                break;
                            case WIFI_AUTH_WEP:
                                auth_mode_str = "WEP";
                                break;
                            case WIFI_AUTH_WPA_PSK:
                                auth_mode_str = "WPA_PSK";
                                break;
                            case WIFI_AUTH_WPA2_PSK:
                                auth_mode_str = "WPA2_PSK";
                                break;
                            case WIFI_AUTH_WPA_WPA2_PSK:
                                auth_mode_str = "WPA_WPA2_PSK";
                                break;
                            case WIFI_AUTH_WPA3_PSK:
                                auth_mode_str = "WPA3_PSK";
                                break;
                            default:
                                auth_mode_str = "UNKNOWN";
                                break;
                        }
                        
                        ESP_LOGI(TAG, "[%d] SSID: %-32s | RSSI: %4d dBm | Auth: %s | Channel: %d",
                                i + 1,
                                (char *)list[i].ssid,
                                list[i].rssi,
                                auth_mode_str,
                                list[i].primary);
                    }
                    ESP_LOGI(TAG, "==========================");

                    free(list);
                }
                break;

            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi STA started");
                break;

            case WIFI_EVENT_STA_STOP:
                ESP_LOGI(TAG, "WiFi STA stopped");
                break;

            case WIFI_EVENT_STA_CONNECTED:
                {
                    wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;
                    ESP_LOGI(TAG, "Station connected to AP");
                    ESP_LOGI(TAG, "  SSID: %s", event->ssid);
                    ESP_LOGI(TAG, "  Channel: %d", event->channel);
                    ESP_LOGI(TAG, "  Auth mode: %d", event->authmode);
                }
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                {
                    wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
                    ESP_LOGW(TAG, "Station disconnected from AP");
                    ESP_LOGW(TAG, "  SSID: %s", event->ssid);
                    ESP_LOGW(TAG, "  Reason: %d", event->reason);
                    
                    // Tự động kết nối lại
                    ESP_LOGI(TAG, "Attempting to reconnect...");
                    esp_wifi_connect();
                }
                break;

            default:
                break;
        }
    } 
    else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                {
                    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                    ESP_LOGI(TAG, "Got IP address");
                    ESP_LOGI(TAG, "  IP Address: %d.%d.%d.%d",
                            (event->ip_info.ip.addr >> 0) & 0xFF,
                            (event->ip_info.ip.addr >> 8) & 0xFF,
                            (event->ip_info.ip.addr >> 16) & 0xFF,
                            (event->ip_info.ip.addr >> 24) & 0xFF);
                    ESP_LOGI(TAG, "  Netmask: %d.%d.%d.%d",
                            (event->ip_info.netmask.addr >> 0) & 0xFF,
                            (event->ip_info.netmask.addr >> 8) & 0xFF,
                            (event->ip_info.netmask.addr >> 16) & 0xFF,
                            (event->ip_info.netmask.addr >> 24) & 0xFF);
                    ESP_LOGI(TAG, "  Gateway: %d.%d.%d.%d",
                            (event->ip_info.gw.addr >> 0) & 0xFF,
                            (event->ip_info.gw.addr >> 8) & 0xFF,
                            (event->ip_info.gw.addr >> 16) & 0xFF,
                            (event->ip_info.gw.addr >> 24) & 0xFF);
                }
                break;

            case IP_EVENT_STA_LOST_IP:
                ESP_LOGW(TAG, "Lost IP address");
                break;

            default:
                break;
        }
    }
}

void wifi_scan(void)
{
    ESP_LOGI(TAG, "Starting WiFi scan...");
    
    wifi_scan_config_t scanConf = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, false));
    ESP_LOGI(TAG, "WiFi scan started (non-blocking)");
}

void wifi_connect_sta(const char *ssid, const char *password)
{
    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);
    
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    ESP_LOGI(TAG, "WiFi connection initiated");
}

void app_main_unuse(void)
{
    ESP_LOGI(TAG, "Step 1: Initializing WiFi subsystem...");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_LOGI(TAG, "Step 2: Registering event handlers...");
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL
    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL
    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_LOST_IP,
        &wifi_event_handler,
        NULL,
        NULL
    ));
    
    ESP_LOGI(TAG, "Step 3: Setting WiFi mode to STA...");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "Step 4: Performing WiFi scan...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    wifi_scan();
    
    ESP_LOGI(TAG, "Step 5: Connecting to WiFi...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    wifi_connect_sta(WIFI_SSID, WIFI_PASSWORD);
    
    ESP_LOGI(TAG, "=== Initialization complete ===");
    ESP_LOGI(TAG, "Check serial monitor for WiFi events...");
}

