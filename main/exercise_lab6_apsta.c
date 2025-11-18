#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/ip4_addr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "WiFi_APSTA";

#define AP_SSID        "ESP32_Config"
#define AP_PASSWORD    "12345678"
#define AP_CHANNEL     1
#define AP_MAX_CONN    4

static int ap_sta_connected_count = 0;
static httpd_handle_t server = NULL;
static bool wifi_configured = false;

void wifi_connect_sta(const char *ssid, const char *password);
void wifi_start_ap(const char *ssid, const char *password, int channel);

static const char* html_config_page = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>ESP32 WiFi Configuration</title>"
"<style>"
"body { font-family: Arial, sans-serif; max-width: 500px; margin: 50px auto; padding: 20px; background: #f5f5f5; }"
"h1 { color: #333; text-align: center; }"
".form-group { margin-bottom: 15px; }"
"label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }"
"input[type='text'], input[type='password'] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }"
"button { width: 100%; padding: 12px; background: #4CAF50; color: white; border: none; border-radius: 4px; font-size: 16px; cursor: pointer; }"
"button:hover { background: #45a049; }"
".info { background: #e7f3ff; padding: 15px; border-radius: 4px; margin-bottom: 20px; }"
".status { margin-top: 20px; padding: 10px; border-radius: 4px; text-align: center; }"
".success { background: #d4edda; color: #155724; }"
".error { background: #f8d7da; color: #721c24; }"
"</style>"
"</head>"
"<body>"
"<h1>🔧 ESP32 WiFi Configuration</h1>"
"<div class='info'>"
"<strong>Hướng dẫn:</strong><br>"
"1. Nhập SSID (tên mạng WiFi) bạn muốn kết nối<br>"
"2. Nhập Password của mạng WiFi<br>"
"3. Nhấn 'Kết nối' để ESP32 kết nối đến mạng WiFi đó"
"</div>"
"<form method='POST' action='/connect'>"
"<div class='form-group'>"
"<label for='ssid'>WiFi SSID:</label>"
"<input type='text' id='ssid' name='ssid' required placeholder='Nhập tên mạng WiFi'>"
"</div>"
"<div class='form-group'>"
"<label for='password'>Password:</label>"
"<input type='password' id='password' name='password' placeholder='Nhập mật khẩu (nếu có)'>"
"</div>"
"<button type='submit'>🔌 Kết nối</button>"
"</form>"
"</body>"
"</html>";

static const char* html_success_page = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>Kết nối thành công</title>"
"<style>"
"body { font-family: Arial, sans-serif; max-width: 500px; margin: 50px auto; padding: 20px; background: #f5f5f5; text-align: center; }"
"h1 { color: #4CAF50; }"
".success-box { background: #d4edda; color: #155724; padding: 20px; border-radius: 4px; margin: 20px 0; }"
"</style>"
"</head>"
"<body>"
"<h1>✅ Kết nối thành công!</h1>"
"<div class='success-box'>"
"<p><strong>ESP32 đang kết nối đến WiFi...</strong></p>"
"<p>Vui lòng kiểm tra Serial Monitor để xem IP address.</p>"
"</div>"
"</body>"
"</html>";

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_config_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t connect_post_handler(httpd_req_t *req)
{
    char content[512];
    size_t recv_size = sizeof(content);
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    
    content[ret] = '\0';
    ESP_LOGI(TAG, "Received POST data: %s", content);
    
    char ssid[64] = {0};
    char password[64] = {0};
    
    char *ssid_start = strstr(content, "ssid=");
    char *password_start = strstr(content, "password=");
    
    if (ssid_start) {
        ssid_start += 5;
        char *ssid_end = strchr(ssid_start, '&');
        if (ssid_end) {
            int len = ssid_end - ssid_start;
            if (len < sizeof(ssid)) {
                strncpy(ssid, ssid_start, len);
                ssid[len] = '\0';
            }
        } else {
            strncpy(ssid, ssid_start, sizeof(ssid) - 1);
        }
        for (int i = 0; ssid[i]; i++) {
            if (ssid[i] == '+') ssid[i] = ' ';
        }
    }
    
    if (password_start) {
        password_start += 9;
        int len = strlen(password_start);
        if (len < sizeof(password)) {
            strncpy(password, password_start, len);
            password[len] = '\0';
        }
        
        for (int i = 0; password[i]; i++) {
            if (password[i] == '+') password[i] = ' ';
        }
    }
    
    ESP_LOGI(TAG, "Parsed SSID: %s, Password: %s", ssid, password);
    
    if (strlen(ssid) > 0) {
        wifi_configured = true;
        wifi_connect_sta(ssid, password);
        
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, html_success_page, HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID không được để trống");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

static void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    
    ESP_LOGI(TAG, "Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root);
        
        httpd_uri_t connect = {
            .uri       = "/connect",
            .method    = HTTP_POST,
            .handler   = connect_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &connect);
        
        ESP_LOGI(TAG, "Web server started successfully");
    } else {
        ESP_LOGE(TAG, "Error starting web server!");
    }
}

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
                ESP_LOGI(TAG, "  AP IP: 192.168.4.1");
                start_webserver();
                break;

            case WIFI_EVENT_AP_STACONNECTED:
                {
                    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
                    ap_sta_connected_count++;
                    ESP_LOGI(TAG, "Station connected to ESP32 AP");
                    ESP_LOGI(TAG, "  MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                            event->mac[0], event->mac[1], event->mac[2],
                            event->mac[3], event->mac[4], event->mac[5]);
                    ESP_LOGI(TAG, "  Total connected stations: %d", ap_sta_connected_count);
                    ESP_LOGI(TAG, "  Please open browser and go to: http://192.168.4.1");
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
                    ESP_LOGI(TAG, "  Total connected stations: %d", ap_sta_connected_count);
                }
                break;

            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi STA started");
                break;

            case WIFI_EVENT_STA_CONNECTED:
                {
                    wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;
                    ESP_LOGI(TAG, "Station connected to AP");
                    ESP_LOGI(TAG, "  SSID: %s", event->ssid);
                    ESP_LOGI(TAG, "  Channel: %d", event->channel);
                }
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                {
                    wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
                    ESP_LOGW(TAG, "Station disconnected from AP");
                    ESP_LOGW(TAG, "  SSID: %s", event->ssid);
                    if (wifi_configured) {
                        ESP_LOGI(TAG, "Attempting to reconnect...");
                        esp_wifi_connect();
                    }
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
                    ESP_LOGI(TAG, "Got IP address (STA mode)");
                    ESP_LOGI(TAG, "  IP Address: %d.%d.%d.%d",
                            (event->ip_info.ip.addr >> 0) & 0xFF,
                            (event->ip_info.ip.addr >> 8) & 0xFF,
                            (event->ip_info.ip.addr >> 16) & 0xFF,
                            (event->ip_info.ip.addr >> 24) & 0xFF);
                    ESP_LOGI(TAG, "  Gateway: %d.%d.%d.%d",
                            (event->ip_info.gw.addr >> 0) & 0xFF,
                            (event->ip_info.gw.addr >> 8) & 0xFF,
                            (event->ip_info.gw.addr >> 16) & 0xFF,
                            (event->ip_info.gw.addr >> 24) & 0xFF);
                    ESP_LOGI(TAG, "✅ ESP32 đã kết nối thành công đến WiFi!");
                }
                break;

            default:
                break;
        }
    }
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
    if (password && strlen(password) > 0) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    ESP_LOGI(TAG, "WiFi connection initiated");
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

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 WiFi Manager - APSTA MODE ===");
    
    ESP_LOGI(TAG, "Step 1: Initializing WiFi subsystem...");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();
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
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL
    ));
    
    ESP_LOGI(TAG, "Step 3: Setting WiFi mode to APSTA...");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    wifi_start_ap(AP_SSID, AP_PASSWORD, AP_CHANNEL);
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "=== Initialization complete ===");
    ESP_LOGI(TAG, "📡 ESP32 AP is running:");
    ESP_LOGI(TAG, "   SSID: %s", AP_SSID);
    ESP_LOGI(TAG, "   Password: %s", AP_PASSWORD);
    ESP_LOGI(TAG, "   IP: 192.168.4.1");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "📱 Hướng dẫn:");
    ESP_LOGI(TAG, "   1. Kết nối điện thoại/máy tính đến WiFi: %s", AP_SSID);
    ESP_LOGI(TAG, "   2. Mở trình duyệt và truy cập: http://192.168.4.1");
    ESP_LOGI(TAG, "   3. Nhập SSID và Password của WiFi bạn muốn kết nối");
    ESP_LOGI(TAG, "   4. Nhấn 'Kết nối'");
}
