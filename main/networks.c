/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "main.h"

/* The examples use WiFi configuration that you can set via 'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
/* ============================================================================== */
/* ============================================================================== */
char AP_WIFI_PASS[30];
char AP_WIFI_SSID[30];
char STA_WIFI_PASS[30];
char STA_WIFI_SSID[30];

/* ============================================================================== */
/* ============================================================================== */
/* FreeRTOS event group to signal when we are connected*/
EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event 
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

/* ============================================================================== */
/* ============================================================================== */
static const char *TAG = "NETWORKS";

/* ============================================================================== */
/* ============================================================================== */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        FDSTATUS.SYSTEM_EVENT_STA_GOT_IP = false;
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        // xEventGroupClearBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if(FDSTATUS.DisableReWiFi == false){
			vTaskDelay(5000 / portTICK_PERIOD_MS);
			ESP_LOGI(TAG, "retry to connect to the AP");
			esp_wifi_connect();
		}

		if(FDSTATUS.SYSTEM_EVENT_STA_GOT_IP == true){
			FDSTATUS.NetworkDisconnect = true;
		}

		FDSTATUS.SYSTEM_EVENT_STA_GOT_IP = false;
		xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->ip_info.ip));

        memset(IPAddr, '\0', sizeof(IPAddr));
        sprintf(IPAddr, "%s", ip4addr_ntoa(&event->ip_info.ip));
        strcat(IPAddr, DEOF);

        memset(Subnet, '\0', sizeof(Subnet));
        sprintf(Subnet, "%s", ip4addr_ntoa(&event->ip_info.netmask));
        strcat(Subnet, DEOF);

        memset(Gateway, '\0', sizeof(Gateway));
        sprintf(Gateway, "%s", ip4addr_ntoa(&event->ip_info.gw));
        strcat(Gateway, DEOF);

        FDSTATUS.SYSTEM_EVENT_STA_GOT_IP = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ============================================================================== */
/* ============================================================================== */
void wifi_init_networks(void)
{
    /* ========================================= */
    /* ========================================= */
    esp_read_mac(macaddress, 0);

    memset(MAC_Address, '\0', sizeof(MAC_Address));
    sprintf(MAC_Address, "%02X%02X%02X%02X%02X%02X", macaddress[0], macaddress[1], macaddress[2], macaddress[3], macaddress[4], macaddress[5]);
    printf("MAC_Address=%s\n", MAC_Address);

    memset(AP_WIFI_SSID, '\0', sizeof(AP_WIFI_SSID));
    memset(AP_WIFI_PASS, '\0', sizeof(AP_WIFI_PASS));

    sprintf(AP_WIFI_SSID, "FDTECH_%02X%02X%02X", macaddress[3], macaddress[4], macaddress[5]);
    sprintf(AP_WIFI_PASS, "123456789");


    /* ========================================= */
    /* ========================================= */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // configure the wifi connection and start the interface
    wifi_config_t ap_config = {
        .ap = {
          .ssid = AP_SSID,
          .channel = 11,
          .password = AP_PASSPHARSE,
          .ssid_len = 0,
          .authmode = WIFI_AUTH_WPA_WPA2_PSK,
          .ssid_hidden = AP_SSID_HIDDEN,
          .max_connection = AP_MAX_CONNECTIONS,         
        },
    };

    memset(ap_config.ap.ssid, '\0', sizeof(ap_config.ap.ssid));
    memset(ap_config.ap.password, '\0', sizeof(ap_config.ap.password));

    strcpy((char *)ap_config.ap.ssid, AP_WIFI_SSID);
    ap_config.ap.ssid_len = strlen((char *)ap_config.ap.ssid);
    strcpy((char *)ap_config.ap.password, AP_WIFI_PASS);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s", ap_config.ap.ssid, ap_config.ap.password);

    ESP_ERROR_CHECK(esp_wifi_start());

//    vTaskDelay(1000 / portTICK_RATE_MS);

    if(FDSTATUS.HaveWiFiSetting == true){

        // configure the wifi connection and start the interface
        wifi_config_t sta_config = {
            .sta = {
                .ssid = WIFI_SSID,
                .password = WIFI_PASS,
            },
        };

        memset(sta_config.sta.ssid, '\0', sizeof(sta_config.sta.ssid));
        memset(sta_config.sta.password, '\0', sizeof(sta_config.sta.password));

        strncpy((char *) sta_config.sta.ssid, STA_WIFI_SSID, strlen(STA_WIFI_SSID));
        strncpy((char *) sta_config.sta.password, STA_WIFI_PASS, strlen(STA_WIFI_PASS));

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config) );

        ESP_LOGI(TAG, "wifi_init_sta finished.");
        ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
                 sta_config.sta.ssid, sta_config.sta.password);

        ESP_ERROR_CHECK(esp_wifi_connect());
    }

    vTaskDelay(1000 / portTICK_RATE_MS);
}

