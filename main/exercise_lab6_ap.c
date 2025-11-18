#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "lwip/ip4_addr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "WiFi_AP";

// Cấu hình Access Point
#define AP_SSID        "ESP32_AP"
#define AP_PASSWORD    "12345678"
#define AP_CHANNEL     1
#define AP_MAX_CONN    4

static int ap_sta_connected_count = 0;

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "WiFi AP started");
                ESP_LOGI(TAG, "  AP SSID: %s", AP_SSID);
                ESP_LOGI(TAG, "  AP Password: %s", AP_PASSWORD);
                ESP_LOGI(TAG, "  AP Channel: %d", AP_CHANNEL);
                ESP_LOGI(TAG, "  AP IP: 192.168.4.1");
                break;

            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "WiFi AP stopped");
                ap_sta_connected_count = 0;
                break;

            case WIFI_EVENT_AP_STACONNECTED:
                {
                    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
                    ap_sta_connected_count++;
                    ESP_LOGI(TAG, "Station connected to ESP32 AP");
                    ESP_LOGI(TAG, "  MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                            event->mac[0], event->mac[1], event->mac[2],
                            event->mac[3], event->mac[4], event->mac[5]);
                    ESP_LOGI(TAG, "  AID: %d", event->aid);
                    ESP_LOGI(TAG, "  Total connected stations: %d", ap_sta_connected_count);
                }
                break;

            case WIFI_EVENT_AP_STADISCONNECTED:
                {
                    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
                    ap_sta_connected_count--;
                    ESP_LOGI(TAG, "Station disconnected from ESP32 AP");
                    ESP_LOGI(TAG, "  MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                            event->mac[0], event->mac[1], event->mac[2],
                            event->mac[3], event->mac[4], event->mac[5]);
                    ESP_LOGI(TAG, "  AID: %d", event->aid);
                    ESP_LOGI(TAG, "  Total connected stations: %d", ap_sta_connected_count);
                }
                break;

            default:
                break;
        }
    }
}

void wifi_start_ap(const char *ssid, const char *password, int channel)
{
    ESP_LOGI(TAG, "Starting Access Point: %s", ssid);
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(ssid),
            .channel = channel,
            .password = "",
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    
    if (strlen(password) >= 8) {
        strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password) - 1);
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        ESP_LOGW(TAG, "Password too short, using OPEN mode");
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGI(TAG, "Access Point configured");
}

void app_main_unuse(void)
{
    ESP_LOGI(TAG, "Step 1: Initializing WiFi subsystem...");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    
    esp_netif_ip_info_t ip_info;
    ip4_addr_t ip_addr, gw_addr, netmask_addr;
    IP4_ADDR(&ip_addr, 192, 168, 4, 1);
    IP4_ADDR(&gw_addr, 192, 168, 4, 1);
    IP4_ADDR(&netmask_addr, 255, 255, 255, 0);
    ip_info.ip.addr = ip_addr.addr;
    ip_info.gw.addr = gw_addr.addr;
    ip_info.netmask.addr = netmask_addr.addr;
    esp_netif_dhcps_stop(ap_netif);
    esp_netif_set_ip_info(ap_netif, &ip_info);
    esp_netif_dhcps_start(ap_netif);
    
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
    
    ESP_LOGI(TAG, "Step 3: Setting WiFi mode to AP...");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    wifi_start_ap(AP_SSID, AP_PASSWORD, AP_CHANNEL);
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "=== Initialization complete ===");
    ESP_LOGI(TAG, "AP is running. Connect to SSID: %s, Password: %s", AP_SSID, AP_PASSWORD);
    ESP_LOGI(TAG, "AP IP: 192.168.4.1");
    ESP_LOGI(TAG, "Check serial monitor for connection events...");
}

