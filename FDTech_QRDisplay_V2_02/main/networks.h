#ifndef NETWORKS_H_INCLUDED
#define NETWORKS_H_INCLUDED


#include "esp_err.h"
#include "tcpip_adapter.h"
#include "freertos/event_groups.h"


#define AP_SSID "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
#define AP_PASSPHARSE "000000000000000000000000000"
#define AP_SSID_HIDDEN 0
#define AP_MAX_CONNECTIONS 5
#define AP_AUTHMODE WIFI_AUTH_WPA2_PSK
#define AP_BEACON_INTERVAL 120

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================== */
/* ============================================================================== */

extern EventGroupHandle_t s_wifi_event_group;

extern const int WIFI_CONNECTED_BIT;

extern char AP_WIFI_PASS[30];
extern char AP_WIFI_SSID[30];
extern char STA_WIFI_PASS[30];
extern char STA_WIFI_SSID[30];

void wifi_init_networks(void);

#ifdef __cplusplus
}
#endif

#endif /* NETWORKS_H_INCLUDED */
