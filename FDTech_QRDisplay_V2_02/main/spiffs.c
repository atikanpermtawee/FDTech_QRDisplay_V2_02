/* SPIFFS filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "main.h"

static const char *TAG = "SPIFFS";

void app_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Use POSIX and C standard library functions to work with files.
    // Open renamed file for reading
    ESP_LOGI("SPIFFS", "Reading file : wificonfig.json");
    FILE* f = fopen("/spiffs/wificonfig.json", "r");

    if (f == NULL) {
        ESP_LOGE("SPIFFS", "Failed to open \"wificonfig.json\" file for reading");
        FDSTATUS.HaveWiFiSetting = false;

    }else{

        char *line = (char*) calloc(100, sizeof(char));
        fgets(line, 100, f);
        fclose(f);

        ESP_LOGI("SPIFFS", "Srting Length: '%d'\n", strlen(line));

        char* pos = strchr(line, '\n');
        if (pos) {
            *pos = '\0';
        }

        memset(STA_WIFI_SSID, 0, sizeof(STA_WIFI_SSID));
        memset(STA_WIFI_PASS, 0, sizeof(STA_WIFI_PASS));

        ESP_LOGI("SPIFFS", "Read from file: '%s'\n", line);

        const cJSON *ssidsta = NULL;
        const cJSON *passsta = NULL;

        cJSON *WifiConfig = cJSON_Parse(line);
        free(line);

        if (WifiConfig == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != NULL)
            {
                fprintf(stderr, "Error before: %s\n", error_ptr);
            }

        }else{
            ssidsta = cJSON_GetObjectItemCaseSensitive(WifiConfig, "SSID");
            if (cJSON_IsString(ssidsta) && (ssidsta->valuestring != NULL))
            {
                passsta = cJSON_GetObjectItemCaseSensitive(WifiConfig, "PASS");
                if (cJSON_IsString(passsta) && (passsta->valuestring != NULL))
                {
                    strncpy((char *) STA_WIFI_SSID, ssidsta->valuestring, strlen(ssidsta->valuestring));
                    strncpy((char *) STA_WIFI_PASS, passsta->valuestring, strlen(passsta->valuestring));
                }
            }
        }
        cJSON_Delete(WifiConfig);

        if((strlen(STA_WIFI_SSID) > 2) && (strlen(STA_WIFI_PASS) > 2)){
            FDSTATUS.HaveWiFiSetting = true;
            printf("HaveWiFiSetting\n");

        }else{
            FDSTATUS.HaveWiFiSetting = false;
            printf("NOTHaveWiFiSetting\n");
        }
    }

    // if(FDSTATUS.HaveWiFiSetting == true){

        // Use POSIX and C standard library functions to work with files.
        // Open renamed file for reading
        ESP_LOGI("SPIFFS", "Reading file : register.json");
        FILE* fd = fopen("/spiffs/register.json", "r");

        if (fd == NULL) {
            ESP_LOGE("SPIFFS", "Failed to open \"register.json\" file for reading");
            FDSTATUS.HaveRegistor = false;

        }else{

            char *line = (char*) calloc(500, sizeof(char));
            fgets(line, 500, fd);
            fclose(fd);

            ESP_LOGI("SPIFFS", "Srting Length: '%d'\n", strlen(line));

            char* pos = strchr(line, '\n');
            if (pos) {
                *pos = '\0';
            }

            // memset(ONLINE_Time, 0, sizeof(ONLINE_Time));
            memset(SERVER_Key1, 0, sizeof(SERVER_Key1));
            memset(SERVER_Key2, 0, sizeof(SERVER_Key2));
            memset(Dev_d_id, 0, sizeof(Dev_d_id));
            memset(Dev_m_id, 0, sizeof(Dev_m_id));
            memset(Dev_k_id, 0, sizeof(Dev_k_id));

            ESP_LOGI("SPIFFS", "Read from file: '%s'\n", line);

//            const cJSON *online_t = NULL;
            const cJSON *key1 = NULL;
            const cJSON *key2 = NULL;
            const cJSON *d_id = NULL;
            const cJSON *m_id = NULL;
            const cJSON *k_id = NULL;

            cJSON *Register = cJSON_Parse(line);
            free(line);

            if (Register == NULL)
            {
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr != NULL)
                {
                    fprintf(stderr, "Error before: %s\n", error_ptr);
                }

            }else{

                // online_t = cJSON_GetObjectItemCaseSensitive(Register, "ONLINE_Time");
                // if (cJSON_IsString(online_t) && (online_t->valuestring != NULL))
                // {
                //     // strcpy(ONLINE_Time, online_t->valuestring);
                //     // strcat(ONLINE_Time, DEOF);
                // }


                key1 = cJSON_GetObjectItemCaseSensitive(Register, "SERVER_Key1");
                if (cJSON_IsString(key1) && (key1->valuestring != NULL))
                {
                    strcpy(SERVER_Key1, key1->valuestring);
                    strcat(SERVER_Key1, DEOF);
                }


                key2 = cJSON_GetObjectItemCaseSensitive(Register, "SERVER_Key2");
                if (cJSON_IsString(key2) && (key2->valuestring != NULL))
                {
                    strcpy(SERVER_Key2, key2->valuestring);
                    strcat(SERVER_Key2, DEOF);
                }


                d_id = cJSON_GetObjectItemCaseSensitive(Register, "Dev_d_id");
                if (cJSON_IsString(d_id) && (d_id->valuestring != NULL))
                {
                    strcpy(Dev_d_id, d_id->valuestring);
                    strcat(Dev_d_id, DEOF);
                }


                m_id = cJSON_GetObjectItemCaseSensitive(Register, "Dev_m_id");
                if (cJSON_IsString(m_id) && (m_id->valuestring != NULL))
                {
                    strcpy(Dev_m_id, m_id->valuestring);
                    strcat(Dev_m_id, DEOF);
                }


                k_id = cJSON_GetObjectItemCaseSensitive(Register, "Dev_k_id");
                if (cJSON_IsString(k_id) && (k_id->valuestring != NULL))
                {
                    strcpy(Dev_k_id, k_id->valuestring);
                    strcat(Dev_k_id, DEOF);
                }
            }
            cJSON_Delete(Register);

            if(/*(strlen(ONLINE_Time) > 0) && */(strlen(SERVER_Key1) > 0) && (strlen(SERVER_Key2) > 0) && (strlen(Dev_d_id) > 0) && (strlen(Dev_m_id) > 0) && (strlen(Dev_k_id) > 0)){
                FDSTATUS.HaveRegistor = true;

            }else{
                FDSTATUS.HaveRegistor = false;
            }
        }
    // }

    // Use POSIX and C standard library functions to work with files.
    // Open renamed file for reading
    // ESP_LOGI("SPIFFS", "Reading file : systemconfig.json");
    // FILE* fxx = fopen("/spiffs/systemconfig.json", "r");

    // if (fxx == NULL) {
    //     ESP_LOGE("SPIFFS", "Failed to open \"systemconfig.json\" file for reading");

    // }else{

    //     char *line = (char*) calloc(100, sizeof(char));
    //     fgets(line, 100, f);

    //     ESP_LOGI("SPIFFS", "Srting Length: '%d'\n", strlen(line));

    //     char* pos = strchr(line, '\n');
    //     if (pos) {
    //         *pos = '\0';
    //     }

    //     ESP_LOGI("SPIFFS", "Read from file: '%s'\n", line);

    //     const cJSON *devprice = NULL;
    //     const cJSON *devpulse = NULL;
    //     const cJSON *devpnum = NULL;
    //     // const cJSON *devlong = NULL;

    //     cJSON *SystemConfig = cJSON_Parse(line);
    //     free(line);

    //     if (SystemConfig == NULL)
    //     {
    //         const char *error_ptr = cJSON_GetErrorPtr();
    //         if (error_ptr != NULL)
    //         {
    //             fprintf(stderr, "Error before: %s\n", error_ptr);
    //         }

    //         DevPrices = DefaultPrices;
    //         PulseCost = DefaultPulseCnt;
    //         PulseNumber = DefaultPulseNum;

    //     }else{
    //         devprice = cJSON_GetObjectItemCaseSensitive(SystemConfig, "DevPrices");
    //         if (cJSON_IsString(devprice) && (devprice->valuestring != NULL))
    //         {
    //             DevPrices = atoi(devprice->valuestring);

    //         }else{
    //             DevPrices = DefaultPrices;
    //         }

    //         devpulse = cJSON_GetObjectItemCaseSensitive(SystemConfig, "PulseCost");
    //         if (cJSON_IsString(devpulse) && (devpulse->valuestring != NULL))
    //         {
    //             PulseCost = atoi(devpulse->valuestring);

    //         }else{
    //             PulseCost = DefaultPulseCnt;
    //         }

    //         devpnum = cJSON_GetObjectItemCaseSensitive(SystemConfig, "PulseNumber");
    //         if (cJSON_IsString(devpnum) && (devpnum->valuestring != NULL))
    //         {
    //             PulseNumber = atoi(devpnum->valuestring);

    //         }else{
    //             PulseNumber = DefaultPulseNum;
    //         }

    //         // devlong = cJSON_GetObjectItemCaseSensitive(SystemConfig, "Longitude");
    //         // if (cJSON_IsString(devlong) && (devlong->valuestring != NULL))
    //         // {
    //         //     strncpy((char *) Longitude, devlong->valuestring, strlen(devlong->valuestring));
    //         // }
    //     }
    //     cJSON_Delete(SystemConfig);

    //     printf("DevPrices=%d\n", DevPrices);
    //     printf("PulseCost=%d\n", PulseCost);
    //     printf("PulseNumber=%d\n", PulseNumber);
    //     // printf("Longitude=%s\n", Longitude);
    // }

    // fclose(fxx);
}
