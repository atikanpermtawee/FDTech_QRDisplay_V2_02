/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>

#include "esp_spiffs.h"

#include "driver/gpio.h"

#include "driver/ledc.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "tcpip_adapter.h"
#include "esp_eth.h"

#include <esp_http_server.h>
#include "main.h"


/* ============================================================================== */
/* ============================================================================== */
xTaskHandle Online;
xTaskHandle LCD_Control;
xTaskHandle mem_checking;
xTaskHandle Display;
xTaskHandle ChPayment;
xTaskHandle Pulse_In;
xTaskHandle Pulse_Out;
xTaskHandle SendMoney;
xTaskHandle PowerIN;

xTaskHandle OTAx;


/* ============================================================================== */
/* ============================================================================== */
char MAC_Address[20];
uint8_t macaddress[6];

char IPAddr[20];
char Subnet[20];
char Gateway[20];

// char ONLINE_Time[5];
char SERVER_Key1[30];
char SERVER_Key2[30];

char DataResponse[500];
char CheckingResponse[500];

char OldTransectionID[100];
char TransectionID[100];

char OldPriceQR[10];
char PriceQR[10];

uint16_t PulseNumber = 0;
uint16_t PulseCost = 0;

uint16_t DevPrices;
// uint16_t PusletoBaht;

char Latitude[20];
char Longitude[20];

char Dev_d_id[10];
char Dev_m_id[10];
char Dev_k_id[10];

uint32_t OnlineTimeSec = 0;
uint16_t NumPulse_Gen = 0;

s_stat FDSTATUS;

uint16_t counter_wait_count;
uint16_t counter_pulse_alive;
uint16_t counter_money;

// uint16_t PulseperBath;

uint16_t InsertMoney;
uint64_t MoneyMeter;

uint16_t QR_WaitTime = 0;

uint16_t RunningCnt = 0;
uint16_t StopCnt = 0;

uint8_t count_LCD;

// httpd_handle_t server = NULL;

/* ============================================================================== */
/* ============================================================================== */
/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */
static const char *TAG = "HTTP_SERVER";

/* ============================================================================== */
/* ============================================================================== */
const static char HeaderCss[] = "<!DOCTYPE html>" \
									"<html>" \
									"<head>" \
                                    "<meta charset=\"utf-8\">" \
									"<style>" \

									"body {" \
									  "width: 50%;" \
									  "margin: auto;" \
									"}" \

									"* {" \
									  "box-sizing: border-box;" \
									"}" \

									"input[type=text], select, textarea {" \
									  "width: 100%;" \
									  "padding: 12px;" \
									  "border: 1px solid #ccc;" \
									  "border-radius: 4px;" \
									  "resize: vertical;" \
									"}" \

									"label {" \
									  "padding: 12px 12px 12px 0;" \
									  "display: inline-block;" \
									"}" \

									"input[type=submit] {" \
									  "background-color: #4CAF50;" \
									  "color: white;" \
									  "padding: 12px 20px;" \
									  "border: none;" \
									  "border-radius: 4px;" \
									  "cursor: pointer;" \
									  "float: right;" \
									  "margin-top: 15px;" \
									"}" \

									"input[type=submit]:hover {" \
									  "background-color: #45a049;" \
									"}" \

									".container {" \
									  "border-radius: 5px;" \
									  "background-color: #f2f2f2;" \
									  "padding: 30px;" \
									"}" \

									".col-25 {" \
									  "float: left;" \
									  "width: 25%;" \
									  "margin-top: 6px;" \
									"}" \

									".col-75 {" \
									  "float: left;" \
									  "width: 75%;" \
									  "margin-top: 6px;" \
									"}" \

									".row:after {" \
									  "content: \"\";" \
									  "display: table;" \
									  "clear: both;" \
									"}" \

									"@media screen and (max-width: 600px) {" \
									  ".col-25, .col-75, input[type=submit] {" \
									    "width: 100%;" \
									    "margin-top: 0;" \
									  "}" \
									"}" \
									"</style>" \
									"</head>";


/* ============================================================================== */
/* ============================================================================== */
/* An HTTP GET handler */
static esp_err_t ota_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    char supin[40];

    memset(supin, '\0', sizeof(supin));


    char *http_hml = (char*) calloc((strlen(HeaderCss) + 5000), sizeof(char));
    char *wifiset = (char*) calloc(200, sizeof(char));
    char *wifistat = (char*) calloc(200, sizeof(char));
    char *network = (char*) calloc(1000, sizeof(char));
    char *updateFeild = (char*) calloc(300, sizeof(char));


    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);

            char param[50];

            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "site", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => site=%s", param);
                // strcpy (OTA_SITEURL, param);

                sprintf(OTA_SITEURL, "https://www.f-dtech.com/ota/%s", param);
            }

            if (httpd_query_key_value(buf, "setpin", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => setpin=%s", param);
                strcpy (supin, param);
            }
        }
        free(buf);
    }

    if(strcmp(supin, SetupPIN) == 0){

        printf("password is correct");
        printf("OTA_SITEURL = %s\n", OTA_SITEURL);

        sprintf(updateFeild, "<form>" \
                              "<h2 style=\"margin-top: 5px; text-align: center;color: green;\">Connected ...</h2>" \
                              "</form>");

        FDSTATUS.Start_OTA = true;

    }else{

        if(strlen(supin) > 2){
            sprintf(updateFeild, "<form>" \
                              "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">คุณใส่รหัส OTA ผิด! ...</h2>" \
                              "</form>");
        }
    }



    /* ============================================================================== */
    /* ============================================================================== */
    sprintf(http_hml, "%s\n" \
                            "<body>" \
                            "<h2 style=\"margin-top: 50px; text-align: center;\">WiFi Networks Setting</h2>" \

                            "<div class=\"container\">" \

                              "%s\n" \

                              "<form action=\"/ota\">" \
                              "<div class=\"row\">" \
                                "<div class=\"col-25\">" \
                                  "<label for=\"site\">ชื่อไฟล์ OTA</label>" \
                                "</div>" \
                                "<div class=\"col-75\">" \
                                  "<input type=\"text\" id=\"site\" name=\"site\" placeholder=\"ใส่เฉพาะชื่อไฟล์ ...\">" \
                                "</div>" \
                              "</div>" \

                              "<div class=\"row\">" \
                                "<div class=\"col-25\">" \
                                  "<label for=\"setpin\">รหัส OTA</label>" \
                                "</div>" \
                                "<div class=\"col-75\">" \
                                  "<input type=\"text\" id=\"setpin\" name=\"setpin\" placeholder=\"ใส่รหัส OTA ...\">" \
                                "</div>" \
                              "</div>" \

                              "<div class=\"row\">" \
                                "<input type=\"submit\" value=\"Submit\">" \
                              "</div>" \
                              "</form>" \
                            "</div>" \

                            "</body>" \
                            "</html>", HeaderCss, updateFeild);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, http_hml, strlen(http_hml));

    free(network);
    free(wifiset);
    free(wifistat);
    free(updateFeild);
    free(http_hml);

    if(FDSTATUS.Start_OTA == true){
        ota_init();
    }

    return ESP_OK;
}

static const httpd_uri_t ota = {
    .uri       = "/ota",
    .method    = HTTP_GET,
    .handler   = ota_get_handler,
};

/* ============================================================================== */
/* ============================================================================== */
/* An HTTP GET handler */
static esp_err_t networks_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    char ssid[40];
    char password[40];
    char supin[40];

    memset(supin, '\0', sizeof(supin));


	char *http_hml = (char*) calloc((strlen(HeaderCss) + 5000), sizeof(char));
    char *wifiset = (char*) calloc(200, sizeof(char));
	char *wifistat = (char*) calloc(200, sizeof(char));
	char *network = (char*) calloc(1000, sizeof(char));
    char *updateFeild = (char*) calloc(300, sizeof(char));


    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);

            char param[32];

            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "ssid", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => ssid=%s", param);
                strcpy (ssid, param);

                char *CheckSSID = strstr(ssid,"%26");
	            if(CheckSSID != NULL){
	            	printf("CheckSSID:%s\n", CheckSSID);
	            	*CheckSSID = '\0';

	            	const char AxD[2] = {'&', 0};

	            	strcat(ssid, AxD);
	            	strcat(ssid, CheckSSID+3);

	            	printf("ssid:%s\n", ssid);
	            }

	            char *CheckSSIDPlus = strchr(ssid,'+');
	            if(CheckSSIDPlus != NULL){
	            	printf("CheckSSIDPlus:%s\n", CheckSSIDPlus);
	            	*CheckSSIDPlus = ' ';

	            	printf("ssid:%s\n", ssid);
	            }
            }

            if (httpd_query_key_value(buf, "password", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => password=%s", param);
                strcpy (password, param);
            }

            if (httpd_query_key_value(buf, "setpin", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => setpin=%s", param);
                strcpy (supin, param);
            }
        }
        free(buf);
    }

    if(strcmp(supin, SetupPIN) == 0){

    	printf("password is correct");


    	if(FDSTATUS.SYSTEM_EVENT_STA_GOT_IP == true){
            ESP_ERROR_CHECK(esp_wifi_disconnect());
        }

        tcpip_adapter_init();

        // configure the wifi connection and start the interface
        wifi_config_t sta_config = {
            .sta = {
                .ssid = WIFI_SSID,
                .password = WIFI_PASS,
            },
        };

        memset(sta_config.sta.ssid, 0, sizeof(sta_config.sta.ssid));
        memset(sta_config.sta.password, 0, sizeof(sta_config.sta.password));

        strncpy((char *) sta_config.sta.ssid, ssid, strlen(ssid));
        strncpy((char *) sta_config.sta.password, password, strlen(password));

		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));

        // esp_wifi_connect();
        ESP_ERROR_CHECK(esp_wifi_connect());

        vTaskDelay(2000 / portTICK_RATE_MS);

        EventBits_t StaConBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdTRUE, pdFALSE, (15000 / portTICK_PERIOD_MS));
        FDSTATUS.HaveWiFiSetting = false;

        if((StaConBits & WIFI_CONNECTED_BIT) != 0){

            FILE* f = fopen("/spiffs/wificonfig.json", "w");
            if (f == NULL) {
                ESP_LOGE("SPIFFS", "Failed to open file for writing");

                printf("WiFi STA Connected But not save\n");

                sprintf(updateFeild, "<form>" \
							  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Conecting Failed! ...</h2>" \
							  "</form>");

            }else{

                fprintf(f, "{ \"SSID\":\"%s\", \"PASS\":\"%s\" }\n", ssid, password);
                fclose(f);

                printf("WiFi STA Connected\n");
                FDSTATUS.HaveWiFiSetting = true;

                sprintf(updateFeild, "<form>" \
							  "<h2 style=\"margin-top: 5px; text-align: center;color: green;\">Connected ...</h2>" \
							  "</form>");
            }

            memset(ssid, '\0', strlen(ssid));
            memset(password, '\0', strlen(password));

        }else{

            printf("WiFi STA Not Connected\n");

            sprintf(updateFeild, "<form>" \
							  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Conecting Failed! ...</h2>" \
							  "</form>");
        }
    }else{

    	if(strlen(supin) > 2){
    		sprintf(updateFeild, "<form>" \
							  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Invalid Setup PIN! ...</h2>" \
							  "</form>");
	    }
    }



    /* ============================================================================== */
	/* ============================================================================== */
    sprintf(http_hml, "%s\n" \
    						"<body>" \
							"<h2 style=\"margin-top: 50px; text-align: center;\">WiFi Networks Setting</h2>" \

							"<div class=\"container\">" \

							  "%s\n" \

							  "<form action=\"/networks\">" \
							  "<div class=\"row\">" \
							    "<div class=\"col-25\">" \
							      "<label for=\"ssid\">AP SSID</label>" \
							    "</div>" \
							    "<div class=\"col-75\">" \
							      "<input type=\"text\" id=\"ssid\" name=\"ssid\" placeholder=\"AP SSID...\">" \
							    "</div>" \
							  "</div>" \

							  "<div class=\"row\">" \
							    "<div class=\"col-25\">" \
							      "<label for=\"pass\">AP Password</label>" \
							    "</div>" \
							    "<div class=\"col-75\">" \
							      "<input type=\"text\" id=\"pass\" name=\"password\" placeholder=\"AP Password...\">" \
							    "</div>" \
							  "</div>" \

							  "<div class=\"row\">" \
							    "<div class=\"col-25\">" \
							      "<label for=\"setpin\">Setup Password</label>" \
							    "</div>" \
							    "<div class=\"col-75\">" \
							      "<input type=\"text\" id=\"setpin\" name=\"setpin\" placeholder=\"Setup Password...\">" \
							    "</div>" \
							  "</div>" \

							  "<div class=\"row\">" \
							    "<input type=\"submit\" value=\"Submit\">" \
							  "</div>" \
							  "</form>" \
							"</div>" \

							"</body>" \
							"</html>", HeaderCss, updateFeild);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, http_hml, strlen(http_hml));

    free(network);
    free(wifiset);
    free(wifistat);
    free(updateFeild);
    free(http_hml);

    return ESP_OK;
}

static const httpd_uri_t networks = {
    .uri       = "/networks",
    .method    = HTTP_GET,
    .handler   = networks_get_handler,
};

/* ============================================================================== */
/* ============================================================================== */
/* An HTTP GET handler */
static esp_err_t register_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;


    char supin[40];
    char a_id[5];
    char d_id[5];
    char m_id[5];
    char k_id[5];


    memset(supin, '\0', sizeof(supin));
    memset(a_id, '\0', sizeof(a_id));
    memset(d_id, '\0', sizeof(d_id));
    memset(m_id, '\0', sizeof(m_id));
    memset(k_id, '\0', sizeof(k_id));


	char *http_hml = (char*) calloc((strlen(HeaderCss) + 5000), sizeof(char));
    char *wifiset = (char*) calloc(200, sizeof(char));
	char *wifistat = (char*) calloc(200, sizeof(char));
	char *network = (char*) calloc(1000, sizeof(char));
    char *updateFeild = (char*) calloc(300, sizeof(char));


    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);

            char param[32];

            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "a_id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => a_id=%s", param);
                strcpy (a_id, param);
            }

            if (httpd_query_key_value(buf, "d_id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => d_id=%s", param);
                strcpy (d_id, param);
            }

            if (httpd_query_key_value(buf, "m_id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => m_id=%s", param);
                strcpy (m_id, param);
            }

            if (httpd_query_key_value(buf, "k_id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => k_id=%s", param);
                strcpy (k_id, param);
            }

            if (httpd_query_key_value(buf, "setpin", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => setpin=%s", param);
                strcpy (supin, param);
            }
        }
        free(buf);
    }


	if(strcmp(supin, SetupPIN) == 0){

		if((strlen(a_id) > 0) && (strlen(d_id) > 0) && (strlen(m_id) > 0) && (strlen(k_id) > 0)){

			const struct addrinfo hints = {
		        .ai_family = AF_INET,
		        .ai_socktype = SOCK_STREAM,
		    };

		    struct addrinfo *res;
		    struct in_addr *addr;
		    int s, r;
		    char recv_buf[1024];

		    uint16_t checksum = 0x0000;

		    // crc_ccitt_ffff( const unsigned char *input_str, size_t num_bytes )
		    // a_id + d_id + m_id + k_id + imei
		    char *SumCalc = (char*) calloc((strlen(a_id) + strlen(d_id) + strlen(m_id) + strlen(k_id) + strlen(MAC_Address) + 10), sizeof(char));
		    sprintf(SumCalc, "%s%s%s%s00%s", a_id, d_id, m_id, k_id, MAC_Address);

		    checksum = crc_ccitt_ffff((unsigned char *)SumCalc, strlen(SumCalc));

		    free(SumCalc);

		    char *REG_URL = (char*) calloc((strlen(REGISTER_URL) + 20), sizeof(char));
		    sprintf(REG_URL, "http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/register?data={\"a_id\":\"%s\",\"d_id\":\"%s\",\"m_id\":\"%s\",\"k_id\":\"%s\",\"imei\":\"00%s\",\"sum\":\"%04X\"}", a_id, d_id, m_id, k_id, MAC_Address, checksum);


		    char *REQUEST = (char*) calloc((strlen(WEB_SERVER) + strlen(REG_URL) + strlen(REQ)), sizeof(char));
		    sprintf(REQUEST, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: f-dtech/1.0 esp32-QR-Payment\r\n\r\n", REG_URL, WEB_SERVER);

		    printf("REQUEST = %s\n", REQUEST);

		    free(REG_URL);

	        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

	        if(err != 0 || res == NULL) {
	            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);

	            sprintf(updateFeild, "<form>" \
						  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">... DNS lookup failed</h2>" \
						  "</form>");

	            free(REQUEST);
	            goto exit;
	        }

	        /* Code to print the resolved IP.
	           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
	        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
	        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

	        s = socket(res->ai_family, res->ai_socktype, 0);
	        if(s < 0) {
	            ESP_LOGE(TAG, "... Failed to allocate socket.");
	            freeaddrinfo(res);

	            sprintf(updateFeild, "<form>" \
						  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">... Failed to allocate socket.</h2>" \
						  "</form>");

	            close(s);
	            free(REQUEST);
	            goto exit;
	        }
	        ESP_LOGI(TAG, "... allocated socket");

	        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
	            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
	            freeaddrinfo(res);

	            sprintf(updateFeild, "<form>" \
						  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">... socket connect failed</h2>" \
						  "</form>");

	            close(s);
	            free(REQUEST);
	            goto exit;
	        }

	        ESP_LOGI(TAG, "... connected");
	        freeaddrinfo(res);

	        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
	            ESP_LOGE(TAG, "... socket send failed");

	            sprintf(updateFeild, "<form>" \
						  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">... socket send failed</h2>" \
						  "</form>");

	            close(s);
	            free(REQUEST);
	            goto exit;
	        }

	        free(REQUEST);
	        ESP_LOGI(TAG, "... socket send success");

	        struct timeval receiving_timeout;
	        receiving_timeout.tv_sec = 5;
	        receiving_timeout.tv_usec = 0;

	        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0) {
	            ESP_LOGE(TAG, "... failed to set socket receiving timeout");

	            sprintf(updateFeild, "<form>" \
						  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">... failed to set socket receiving timeout</h2>" \
						  "</form>");

	            close(s);
	            goto exit;
	        }

	        ESP_LOGI(TAG, "... set socket receiving timeout success");

	        char Data[300];

	        /* Read HTTP response */
	        do {
	            bzero(recv_buf, sizeof(recv_buf));
	            r = read(s, recv_buf, sizeof(recv_buf)-1);


	            if(r > 0){
	            	char *p = NULL;

		            p = strstr(recv_buf, "\r\n\r\n");

		            if(p != NULL){
			            p += 4;

			            strcpy(Data, p);
			        }
		    	}

	        } while(r > 0);

	        putchar('\n');
	        printf("body is %s\n", Data);

	        cJSON *json = cJSON_Parse(Data);

            if (json == NULL)
            {
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr != NULL)
                {
                    fprintf(stderr, "Parse Error : %s\n", error_ptr);

                    sprintf(updateFeild, "<form>" \
						  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Register Failed! ...</h2>" \
						  "</form>");

                    close(s);
	            	goto exit;
                }

            }

            const cJSON *responseCode = NULL;
            responseCode = cJSON_GetObjectItemCaseSensitive(json, "code");

            if (cJSON_IsString(responseCode) && (responseCode->valuestring != NULL))
            {
                if(strcmp(responseCode->valuestring, "E00") == 0){

                    const cJSON *online_t = NULL;
	                const cJSON *key1 = NULL;
	                const cJSON *key2 = NULL;

	                online_t = cJSON_GetObjectItemCaseSensitive(json, "online_t");
	                if (cJSON_IsString(online_t) && (online_t->valuestring != NULL)){

	                    OnlineTimeSec = (atoi(online_t->valuestring));
                        printf("OnlineTimeSec = %d\n", OnlineTimeSec);

	                }else{
	                    printf("Json Data Error\n");

	                    sprintf(updateFeild, "<form>" \
							  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Register Failed! ...</h2>" \
							  "</form>");

	                    close(s);
	            		goto exit;
	                }

	                key1 = cJSON_GetObjectItemCaseSensitive(json, "key1");
	                if (cJSON_IsString(key1) && (key1->valuestring != NULL)){
	                    memset(SERVER_Key1, 0, sizeof(SERVER_Key1));
	                    strcpy(SERVER_Key1, key1->valuestring);
	                    strcat(SERVER_Key1, DEOF);

	                }else{
	                    printf("Json Data Error\n");

	                    sprintf(updateFeild, "<form>" \
							  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Register Failed! ...</h2>" \
							  "</form>");

	                    close(s);
	            		goto exit;
	                }

	                key2 = cJSON_GetObjectItemCaseSensitive(json, "key2");
	                if (cJSON_IsString(key2) && (key2->valuestring != NULL)){
	                    memset(SERVER_Key2, 0, sizeof(SERVER_Key2));
	                    strcpy(SERVER_Key2, key2->valuestring);
	                    strcat(SERVER_Key2, DEOF);

	                }else{
	                    printf("Json Data Error\n");

	                    sprintf(updateFeild, "<form>" \
							  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Register Failed! ...</h2>" \
							  "</form>");

	                    close(s);
	            		goto exit;
	                }

					FILE* fx = fopen("/spiffs/register.json", "w");
                    if (fx == NULL) {

                        ESP_LOGE("SPIFFS", "Failed to open file \"register.json\" for writing");

                        sprintf(updateFeild, "<form>" \
						  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Failed to open file \"register.json\" for writing</h2>" \
						  "</form>");

                    }else{


                        fprintf(fx, "{\"SERVER_Key1\":\"%s\", \"SERVER_Key2\":\"%s\", \"Dev_d_id\":\"%s\", \"Dev_m_id\":\"%s\", \"Dev_k_id\":\"%s\"}\n", SERVER_Key1, SERVER_Key2, d_id, m_id, k_id);
                        fclose(fx);

                        FDSTATUS.RestartModule = true;

                        sprintf(updateFeild, "<form>" \
						  "<h2 style=\"margin-top: 5px; text-align: center;color: green;\">Register Successful</h2>" \
						  "</form>");

                    }

                }else{

                	printf("response = %s\n", responseCode->valuestring);

                	if(strcmp(responseCode->valuestring, "E18") == 0){

                		sprintf(updateFeild, "<form>" \
							  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Your device is already registered! ...</h2>" \
							  "</form>");

                	}else{

                    sprintf(updateFeild, "<form>" \
							  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Register Failed! ...</h2>" \
							  "</form>");
                	}
                }

            }else{
                printf("Json Data Error\n");
                // MCSTATUS.RegisterHandle = false;

                sprintf(updateFeild, "<form>" \
						  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Register Failed! ...</h2>" \
						  "</form>");

            }

	        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
	        close(s);
		}


    }else{

    	if(strlen(supin) > 2){
    		sprintf(updateFeild, "<form>" \
							  "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Invalid Setup PIN! ...</h2>" \
							  "</form>");
	    }
    }


    exit:

	    /* ============================================================================== */
		/* ============================================================================== */
	    sprintf(http_hml, "%s\n<body>" \
							"<h2 style=\"margin-top: 50px; text-align: center;\">Register to Networks Setting</h2>" \

							"<div class=\"container\">" \

							  "%s\n" \

							  "<form action=\"/register\">" \

							  "<div class=\"row\">" \
							    "<div class=\"col-25\">" \
							      "<label for=\"a_id\">API ID</label>" \
							    "</div>" \
							    "<div class=\"col-75\">" \
							      "<input type=\"text\" id=\"a_id\" name=\"a_id\" value=\"1\" readonly>" \
							    "</div>" \
							  "</div>" \

							  "<div class=\"row\">" \
							    "<div class=\"col-25\">" \
							      "<label for=\"d_id\">Dealer ID</label>" \
							    "</div>" \
							    "<div class=\"col-75\">" \
							      "<input type=\"text\" id=\"d_id\" name=\"d_id\" placeholder=\"Dealer ID...\">" \
							    "</div>" \
							  "</div>" \

							  "<div class=\"row\">" \
							    "<div class=\"col-25\">" \
							      "<label for=\"m_id\">Member ID</label>" \
							    "</div>" \
							    "<div class=\"col-75\">" \
							      "<input type=\"text\" id=\"m_id\" name=\"m_id\" placeholder=\"Member ID...\">" \
							    "</div>" \
							  "</div>" \

							  "<div class=\"row\">" \
							    "<div class=\"col-25\">" \
							      "<label for=\"k_id\">Kiosk ID</label>" \
							    "</div>" \
							    "<div class=\"col-75\">" \
							      "<input type=\"text\" id=\"k_id\" name=\"k_id\" placeholder=\"Kiosk ID...\">" \
							    "</div>" \
							  "</div>" \

							  "<div class=\"row\">" \
							    "<div class=\"col-25\">" \
							      "<label for=\"imei\">MAC Address</label>" \
							    "</div>" \
							    "<div class=\"col-75\">" \
							      "<input type=\"text\" id=\"imei\" name=\"imei\" value=\"%s\" readonly>" \
							    "</div>" \
							  "</div>" \

							  "<div class=\"row\">" \
							    "<div class=\"col-25\">" \
							      "<label for=\"setpin\">Setup Password</label>" \
							    "</div>" \
							    "<div class=\"col-75\">" \
							      "<input type=\"text\" id=\"setpin\" name=\"setpin\" placeholder=\"Setup Password...\">" \
							    "</div>" \
							  "</div>" \

							  "<div class=\"row\">" \
							    "<input type=\"submit\" value=\"Submit\">" \
							  "</div>" \
							  "</form>" \
							"</div>" \

							"</body>" \
							"</html>", HeaderCss, updateFeild, MAC_Address);

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    // const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, http_hml, strlen(http_hml));

    free(network);
    free(wifiset);
    free(wifistat);
    free(updateFeild);
    free(http_hml);

    if(FDSTATUS.RestartModule == true){
	    vTaskDelay(5000 / portTICK_RATE_MS);
	    esp_restart();
	}

    return ESP_OK;
}


static const httpd_uri_t registers = {
    .uri       = "/register",
    .method    = HTTP_GET,
    .handler   = register_get_handler,
};


/* ============================================================================== */
/* ============================================================================== */
/* An HTTP GET handler */
static esp_err_t systems_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    char prices[10];
    char pulse[10];
    char gpsLatitude[30];
    char gpsLongitude[30];
    char supin[20];

    memset(supin, '\0', sizeof(supin));

    char *http_hml = (char*) calloc((strlen(HeaderCss) + 5000), sizeof(char));
    char *updateFeild = (char*) calloc(300, sizeof(char));


    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);

        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);

            char param[32];

            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "prices", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => prices=%s", param);
                strcpy (prices, param);
            }

            if (httpd_query_key_value(buf, "pulse", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => pulse=%s", param);
                strcpy (pulse, param);
            }

            if (httpd_query_key_value(buf, "latitude", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => latitude=%s", param);
                strcpy (gpsLatitude, param);
            }

            if (httpd_query_key_value(buf, "longitude", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => longitude=%s", param);
                strcpy (gpsLongitude, param);
            }

            if (httpd_query_key_value(buf, "setpin", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => setpin=%s", param);
                strcpy (supin, param);
            }
        }
        free(buf);
    }

    if(strcmp(supin, SetupPIN) == 0){

        if((strlen(prices) > 0) && (strlen(pulse) > 0) && (strlen(gpsLatitude) > 5) && (strlen(gpsLongitude) > 5)){

            FILE* f = fopen("/spiffs/systemconfig.json", "w");
            if (f == NULL) {
                ESP_LOGE("SPIFFS", "Failed to open file \"systemconfig.json\" for writing");

                sprintf(updateFeild, "<form>" \
                              "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Setting Failed! ...</h2>" \
                              "</form>");

            }else{

                FDSTATUS.RestartModule = true;

                fprintf(f, "{ \"Prices\":\"%s\", \"PulseBaht\":\"%s\", \"Latitude\":\"%s\", \"Longitude\":\"%s\" }\n", prices, pulse, gpsLatitude, gpsLongitude);

                sprintf(updateFeild, "<form>" \
                              "<h2 style=\"margin-top: 5px; text-align: center;color: green;\">Setting Successful ...</h2>" \
                              "</form>");
            }

            fclose(f);

        }else{
            sprintf(updateFeild, "<form>" \
                              "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">กรุณากรอกข้อมูลให้ครบถ้วน! ...</h2>" \
                              "</form>");
        }

    }else{

        if(strlen(supin) > 2){
            sprintf(updateFeild, "<form>" \
                              "<h2 style=\"margin-top: 5px; text-align: center;color: red;\">Invalid Setup PIN! ...</h2>" \
                              "</form>");
        }
    }



    /* ============================================================================== */
    /* ============================================================================== */
    sprintf(http_hml, "%s\n" \
                            "<body>" \
                            "<h2 style=\"margin-top: 50px; text-align: center;\">Systems Setting</h2>" \

                            "<div class=\"container\">" \

                              "%s\n" \

                              "<form action=\"/systems\">" \
                              "<div class=\"row\">" \
                                "<div class=\"col-25\">" \
                                  "<label for=\"prices\">ราคา:</label>" \
                                "</div>" \
                                "<div class=\"col-75\">" \
                                  "<input type=\"text\" id=\"prices\" name=\"prices\" placeholder=\"ราคาที่แสดงหน้าจอ...\">" \
                                "</div>" \
                              "</div>" \

                              "<div class=\"row\">" \
                                "<div class=\"col-25\">" \
                                  "<label for=\"pulse\">เครื่องหยอดเหรียญ:</label>" \
                                "</div>" \
                                "<div class=\"col-75\">" \
                                  "<input type=\"text\" id=\"pulse\" name=\"pulse\" placeholder=\"1 พัลซ์ เท่ากับกี่บาท...\">" \
                                "</div>" \
                              "</div>" \

                              "<div class=\"row\">" \
                                "<div class=\"col-25\">" \
                                  "<label for=\"latitude\">พิกัด ละติจูด:</label>" \
                                "</div>" \
                                "<div class=\"col-75\">" \
                                  "<input type=\"text\" id=\"latitude\" name=\"latitude\" placeholder=\"ละติจูด (13.774225)...\">" \
                                "</div>" \
                              "</div>" \

                              "<div class=\"row\">" \
                                "<div class=\"col-25\">" \
                                  "<label for=\"longitude\">พิกัด ลองจิจูด:</label>" \
                                "</div>" \
                                "<div class=\"col-75\">" \
                                  "<input type=\"text\" id=\"longitude\" name=\"longitude\" placeholder=\"ลองจิจูด (100.5409206)...\">" \
                                "</div>" \
                              "</div>" \

                              "<div class=\"row\">" \
                                "<div class=\"col-25\">" \
                                  "<label for=\"setpin\">ยืนยันพาสเวิร์ด:</label>" \
                                "</div>" \
                                "<div class=\"col-75\">" \
                                  "<input type=\"text\" id=\"setpin\" name=\"setpin\" placeholder=\"พาสเวิร์ด ยืนยันการทำงานรายการ...\">" \
                                "</div>" \
                              "</div>" \

                              "<div class=\"row\">" \
                                "<input type=\"submit\" value=\"Submit\">" \
                              "</div>" \
                              "</form>" \
                            "</div>" \

                            "</body>" \
                            "</html>", HeaderCss, updateFeild);

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    // const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, http_hml, strlen(http_hml));

    free(updateFeild);
    free(http_hml);

    if(FDSTATUS.RestartModule == true){
        vTaskDelay(5000 / portTICK_RATE_MS);
        esp_restart();
    }

    return ESP_OK;
}

static const httpd_uri_t systems = {
    .uri       = "/systems",
    .method    = HTTP_GET,
    .handler   = systems_get_handler,
};

bool WriteRegister(char *SERVER_Key1, char *SERVER_Key2, char *D_Id, char *M_Id, char *K_Id)
{
	// Check if destination file exists before renaming
	struct stat st;
	if (stat("/spiffs/register.json", &st) == 0) {
		// Delete it if it exists
		unlink("/spiffs/register.json");
		printf("Delete register\n");
	}

	FILE* fx = fopen("/spiffs/register.json", "w+");

	if (fx == NULL) {

		ESP_LOGE("SPIFFS", "Failed to open file \"register.json\" for writing");

		return false;

	}else{

		char *Data = (char*) calloc(500, sizeof(char));

		sprintf(Data, "{\"SERVER_Key1\":\"%s\", \"SERVER_Key2\":\"%s\", \"Dev_d_id\":\"%s\", \"Dev_m_id\":\"%s\", \"Dev_k_id\":\"%s\"}\n", SERVER_Key1, SERVER_Key2, D_Id, M_Id, K_Id);
		printf("Data:%s\n", Data);

		fprintf(fx, Data);
		printf("fprint\n");
		fclose(fx);
		printf("fclose\n");
		free(Data);
		printf("free\n");
		return true;
	}
}

bool Register(char *A_Id, char *D_Id, char *M_Id, char *K_Id, char *Response)
{
	if(FDSTATUS.SYSTEM_EVENT_STA_GOT_IP == true){
		if((strlen(A_Id) > 0) && (strlen(D_Id) > 0) && (strlen(M_Id) > 0) && (strlen(K_Id) > 0)){

			const struct addrinfo hints = {
				.ai_family = AF_INET,
				.ai_socktype = SOCK_STREAM,
			};

			struct addrinfo *res;
			struct in_addr *addr;
			int s, r;
			char recv_buf[1024];

			uint16_t checksum = 0x0000;

			// crc_ccitt_ffff( const unsigned char *input_str, size_t num_bytes )
			// a_id + d_id + m_id + k_id + imei
			char *SumCalc = (char*) calloc((strlen(A_Id) + strlen(D_Id) + strlen(M_Id) + strlen(K_Id) + strlen(MAC_Address) + 50), sizeof(char));
			sprintf(SumCalc, "%s%s%s%s00%s", A_Id, D_Id, M_Id, K_Id, MAC_Address);

			checksum = crc_ccitt_ffff((unsigned char *)SumCalc, strlen(SumCalc));

			free(SumCalc);

			char *REG_URL = (char*) calloc((strlen(REGISTER_URL) + 20), sizeof(char));
			sprintf(REG_URL, "http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/register?data={\"a_id\":\"%s\",\"d_id\":\"%s\",\"m_id\":\"%s\",\"k_id\":\"%s\",\"imei\":\"00%s\",\"sum\":\"%04X\"}", A_Id, D_Id, M_Id, K_Id, MAC_Address, checksum);


			char *REQUEST = (char*) calloc((strlen(WEB_SERVER) + strlen(REG_URL) + strlen(REQ)), sizeof(char));
			sprintf(REQUEST, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: f-dtech/1.0 esp32-QR-Payment\r\n\r\n", REG_URL, WEB_SERVER);

			printf("REQUEST = %s\n", REQUEST);

			free(REG_URL);

			int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

			if(err != 0 || res == NULL) {
				ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);

				strcpy(Response, "{\n"
						"\"resp_code\":\"07\",\n"
						"\"desp\":\"dns lookup failed\"\n"
						"}\n");

				free(REQUEST);
				return false;
			}

			/* Code to print the resolved IP.
			   Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
			addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
			ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

			s = socket(res->ai_family, res->ai_socktype, 0);
			if(s < 0) {
				ESP_LOGE(TAG, "... Failed to allocate socket.");
				freeaddrinfo(res);

				strcpy(Response, "{\n"
						"\"resp_code\":\"07\",\n"
						"\"desp\":\"failed to allocate socket\"\n"
						"}\n");

				close(s);
				free(REQUEST);
				return false;
			}
			ESP_LOGI(TAG, "... allocated socket");

			if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
				ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
				freeaddrinfo(res);

				strcpy(Response, "{\n"
						"\"resp_code\":\"07\",\n"
						"\"desp\":\"socket connect failed\"\n"
						"}\n");

				close(s);
				free(REQUEST);
				return false;
			}

			ESP_LOGI(TAG, "... connected");
			freeaddrinfo(res);

			if (write(s, REQUEST, strlen(REQUEST)) < 0) {
				ESP_LOGE(TAG, "... socket send failed");

				strcpy(Response, "{\n"
						"\"resp_code\":\"07\",\n"
						"\"desp\":\"socket send failed\"\n"
						"}\n");

				close(s);
				free(REQUEST);
				return false;
			}

			free(REQUEST);
			ESP_LOGI(TAG, "... socket send success");

			struct timeval receiving_timeout;
			receiving_timeout.tv_sec = 5;
			receiving_timeout.tv_usec = 0;

			if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0) {
				ESP_LOGE(TAG, "... failed to set socket receiving timeout");

				strcpy(Response, "{\n"
						"\"resp_code\":\"07\",\n"
						"\"desp\":\"socket receiving timeout\"\n"
						"}\n");

				close(s);
				return false;
			}

			ESP_LOGI(TAG, "... set socket receiving timeout success");

			char Data[500];

			/* Read HTTP response */
			do {
				bzero(recv_buf, sizeof(recv_buf));
				r = read(s, recv_buf, sizeof(recv_buf)-1);


				if(r > 0){
					char *p = NULL;

					p = strstr(recv_buf, "\r\n\r\n");

					if(p != NULL){
						p += 4;

						strcpy(Data, p);
					}
				}

			} while(r > 0);

			printf("\nbody is %s\n", Data);

			ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
			close(s);

			cJSON *DTjson = cJSON_Parse(Data);

			if (DTjson == NULL)
			{
				const char *error_ptr = cJSON_GetErrorPtr();
				if (error_ptr != NULL)
				{
					fprintf(stderr, "Parse Error : %s\n", error_ptr);

					strcpy(Response, "{\n"
							"\"resp_code\":\"07\",\n"
							"\"desp\":\"json data error\"\n"
							"}\n");

					return false;
				}

			}

			const cJSON *responseCode = NULL;
			const cJSON *online_t = NULL;
			const cJSON *key1 = NULL;
			const cJSON *key2 = NULL;

			responseCode = cJSON_GetObjectItemCaseSensitive(DTjson, "code");

			if (cJSON_IsString(responseCode) && (responseCode->valuestring != NULL))
			{
				if(strcmp(responseCode->valuestring, "E00") == 0){

					online_t = cJSON_GetObjectItemCaseSensitive(DTjson, "online_t");
					if (cJSON_IsString(online_t) && (online_t->valuestring != NULL)){

						OnlineTimeSec = (atoi(online_t->valuestring));
//						printf("OnlineTimeSec = %d\n", OnlineTimeSec);

					}else{
						printf("Json Data Error\n");

						strcpy(Response, "{\n"
								"\"resp_code\":\"07\",\n"
								"\"desp\":\"json data error\"\n"
								"}\n");

						return false;
					}

					key1 = cJSON_GetObjectItemCaseSensitive(DTjson, "key1");
					if (cJSON_IsString(key1) && (key1->valuestring != NULL)){
						memset(SERVER_Key1, 0, sizeof(SERVER_Key1));
						strcpy(SERVER_Key1, key1->valuestring);
						strcat(SERVER_Key1, DEOF);
//						printf("SERVER_Key1 = %s\n", SERVER_Key1);

					}else{
						printf("Json Data Error\n");

						strcpy(Response, "{\n"
								"\"resp_code\":\"07\",\n"
								"\"desp\":\"json data error\"\n"
								"}\n");

						return false;
					}

					key2 = cJSON_GetObjectItemCaseSensitive(DTjson, "key2");
					if (cJSON_IsString(key2) && (key2->valuestring != NULL)){
						memset(SERVER_Key2, 0, sizeof(SERVER_Key2));
						strcpy(SERVER_Key2, key2->valuestring);
						strcat(SERVER_Key2, DEOF);
//						printf("SERVER_Key2 = %s\n", SERVER_Key2);

					}else{
						printf("Json Data Error\n");

						strcpy(Response, "{\n"
								"\"resp_code\":\"07\",\n"
								"\"desp\":\"json data error\"\n"
								"}\n");

						return false;
					}

					if(WriteRegister(SERVER_Key1, SERVER_Key2, D_Id, M_Id, K_Id) == true){
						strcpy(Response, "{\n"
									"\"resp_code\":\"00\",\n"
									"\"desp\":\"register success\"\n"
									"}\n");

						return true;

					}else{
						strcpy(Response, "{\n"
									"\"resp_code\":\"06\",\n"
									"\"desp\":\"write register failed\"\n"
									"}\n");

						return false;
					}

				}else{

					printf("response = %s\n", responseCode->valuestring);

					if(strcmp(responseCode->valuestring, "E18") == 0){

						strcpy(Response, "{\n"
								"\"resp_code\":\"02\",\n"
								"\"desp\":\"id is already register\"\n"
								"}\n");

						return false;

					}else{

						strcpy(Response, "{\n"
								"\"resp_code\":\"01\",\n"
								"\"desp\":\"register failed\"\n"
								"}\n");

						return false;
					}
				}

			}else{
				printf("Json Data Error\n");

				strcpy(Response, "{\n"
						"\"resp_code\":\"07\",\n"
						"\"desp\":\"json data error\"\n"
						"}\n");

				return false;
			}

		}else{
			strcpy(Response, "{\n"
					"\"resp_code\":\"01\",\n"
					"\"desp\":\"register failed\"\n"
					"}\n");
			return false;
		}

	}else{
		strcpy(Response, "{\n"
					"\"resp_code\":\"01\",\n"
					"\"desp\":\"register failed\"\n"
					"}\n");
		return false;
	}
}

/* ============================================================================== */
/* ============================================================================== */
/* An HTTP GET handler */
static esp_err_t online_get_handler(httpd_req_t *req)
{
//	http://192.168.4.1/online?a_id=1&d_id=1&m_id=15&k_id=56&imei=C44F330B62A1&ssid=XXXXXX&password=XXXXX&setpin=9999988888

    char*  buf;
    size_t buf_len;

    char A_Id[5];
    char D_Id[5];
    char M_Id[5];
    char K_Id[5];
    char SSId[30];
    char Pass[30];
    char supin[20];

    memset(supin, '\0', sizeof(supin));
//    memset(Macc, '\0', sizeof(Macc));
	memset(A_Id, '\0', sizeof(A_Id));
	memset(D_Id, '\0', sizeof(D_Id));
	memset(M_Id, '\0', sizeof(M_Id));
	memset(K_Id, '\0', sizeof(K_Id));
	memset(SSId, '\0', sizeof(SSId));
	memset(Pass, '\0', sizeof(Pass));

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);

        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);

            char param[32];

            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "a_id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => prices=%s", param);
                strcpy (A_Id, param);
            }

            if (httpd_query_key_value(buf, "d_id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => pulse=%s", param);
                strcpy (D_Id, param);
            }

            if (httpd_query_key_value(buf, "m_id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => latitude=%s", param);
                strcpy (M_Id, param);
            }

            if (httpd_query_key_value(buf, "k_id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => longitude=%s", param);
                strcpy (K_Id, param);
            }

            if (httpd_query_key_value(buf, "ssid", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(TAG, "Found URL query parameter => longitude=%s", param);
				strcpy (SSId, param);
				char *CheckSSID = strstr(SSId,"%26");
				if(CheckSSID != NULL){
					printf("CheckSSID:%s\n", CheckSSID);
					*CheckSSID = '\0';

					const char AxD[2] = {'&', 0};

					strcat(SSId, AxD);
					strcat(SSId, CheckSSID+3);

					printf("ssid:%s\n", SSId);
				}

				char *CheckSSIDPlus = strchr(SSId,'+');
				if(CheckSSIDPlus != NULL){
					printf("CheckSSIDPlus:%s\n", CheckSSIDPlus);
					*CheckSSIDPlus = ' ';

					printf("ssid:%s\n", SSId);
				}
			}

            if (httpd_query_key_value(buf, "password", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(TAG, "Found URL query parameter => longitude=%s", param);
				strcpy (Pass, param);
			}

            if (httpd_query_key_value(buf, "setpin", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => setpin=%s", param);
                strcpy (supin, param);
            }
        }
        free(buf);
    }

	if((strlen(A_Id) > 0) && (strlen(D_Id) > 0) && (strlen(M_Id) > 0) && (strlen(K_Id) > 0) /*&& (strlen(Macc) > 5)*/ && (strlen(SSId) > 3) && (strlen(Pass) > 3) && (strlen(supin) > 3)){
		if(strcmp(supin, SetupPIN) == 0){
			if(FDSTATUS.SYSTEM_EVENT_STA_GOT_IP == true){
				FDSTATUS.HaveWiFiSetting = true;
				ESP_ERROR_CHECK(esp_wifi_disconnect());
				FDSTATUS.DisableReWiFi = true;
				xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
				FDSTATUS.SYSTEM_EVENT_STA_GOT_IP = false;
				vTaskDelay(500 / portTICK_RATE_MS);
			}

			FDSTATUS.SYSTEM_EVENT_STA_GOT_IP = false;
			tcpip_adapter_init();

			// configure the wifi connection and start the interface
			wifi_config_t sta_config = {
				.sta = {
					.ssid = WIFI_SSID,
					.password = WIFI_PASS,
				},
			};

			memset(sta_config.sta.ssid, 0, sizeof(sta_config.sta.ssid));
			memset(sta_config.sta.password, 0, sizeof(sta_config.sta.password));

			strncpy((char *) sta_config.sta.ssid, SSId, strlen(SSId));
			strncpy((char *) sta_config.sta.password, Pass, strlen(Pass));

			ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
			ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));

			printf("Checking ssidsta \"%s\"\n", sta_config.sta.ssid);
			printf("Checking passsta \"%s\"\n", sta_config.sta.password);

			esp_wifi_connect();

			vTaskDelay(1000 / portTICK_RATE_MS);

			EventBits_t VStaConBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdTRUE, pdFALSE, (10000 / portTICK_PERIOD_MS));
			FDSTATUS.HaveWiFiSetting = false;
			FDSTATUS.DisableReWiFi = false;

			if((VStaConBits & WIFI_CONNECTED_BIT) != 0){

				struct stat st;
					if (stat("/spiffs/wificonfig.json", &st) == 0) {
						// Delete it if it exists
						unlink("/spiffs/wificonfig.json");
						printf("Delete wificonfig\n");
					}
				FILE* ssf = fopen("/spiffs/wificonfig.json", "w+");
				if (ssf == NULL) {
					ESP_LOGE("SPIFFS", "Failed to open file for writing");

					printf("WiFi STA Connected But not save\n");

					httpd_resp_send(req, "{\n"
							"\"resp_code\":\"06\",\n"
							"\"desp\":\"write config failed\"\n"
							"}\n", -1);

					ESP_ERROR_CHECK(esp_wifi_disconnect());
					xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
					FDSTATUS.SYSTEM_EVENT_STA_GOT_IP = false;
					FDSTATUS.RestartModule = true;
					goto exit;

				}else{

					fprintf(ssf, "{ \"SSID\":\"%s\", \"PASS\":\"%s\" }\n", SSId, Pass);
					fclose(ssf);

					printf("WiFi STA Connected\n");

				}

			}else{

				httpd_resp_send(req, "{\n"
						"\"resp_code\":\"01\",\n"
						"\"desp\":\"wifi not connected\"\n"
						"}\n", -1);

				FDSTATUS.RestartModule = true;
				goto exit;
			}

		}else{

			httpd_resp_send(req, "{\n"
					"\"resp_code\":\"06\",\n"
					"\"desp\":\"invalid password\"\n"
					"}\n", -1);
		}

	}else{

		httpd_resp_send(req, "{\n"
							"\"resp_code\":\"04\",\n"
							"\"desp\":\"argument issue\"\n"
							"}\n", -1);
	}

	//(strlen(A_Id) > 0) && (strlen(D_Id) > 0) && (strlen(M_Id) > 0) && (strlen(K_Id) > 0)
	char *resq = (char*) calloc(1000, sizeof(char));
	if(Register(A_Id, D_Id, M_Id, K_Id, resq) == true){
		httpd_resp_send(req, "{\n"
				"\"resp_code\":\"00\",\n"
				"\"desp\":\"success\"\n"
				"}\n", -1);

		FDSTATUS.RestartModule = true;

		printf("Register Success\n");

	}else{
		httpd_resp_send(req, resq, -1);

		printf("Register Not Success\n");
	}
	free(resq);


	exit:

		if(FDSTATUS.RestartModule == true){
			vTaskDelay(5000 / portTICK_RATE_MS);
			esp_restart();
		}

		return ESP_OK;
}

static const httpd_uri_t online = {
    .uri       = "/online",
    .method    = HTTP_GET,
    .handler   = online_get_handler,
};


/* ============================================================================== */
/* ============================================================================== */
/* This handler allows the custom error handling functionality to be
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/networks", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/networks URI is not available");

        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;

    }
    else if (strcmp("/register", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/register URI is not available");

        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    else if (strcmp("/systems", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/systems URI is not available");

        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    else if (strcmp("/ota", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/systems URI is not available");

        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    else if (strcmp("/online", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/systems URI is not available");

        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }


    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

/* ============================================================================== */
/* ============================================================================== */
httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &networks);
        httpd_register_uri_handler(server, &registers);
        httpd_register_uri_handler(server, &systems);
        httpd_register_uri_handler(server, &ota);
        httpd_register_uri_handler(server, &online);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

/* ============================================================================== */
/* ============================================================================== */
void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}

/* ============================================================================== */
/* ============================================================================== */
static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server != NULL) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

/* ============================================================================== */
/* ============================================================================== */
static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}


/* ############################################################################ */
void memory_checking(void *pvParameter)
{
	while(true){
		vTaskDelay(10000 / portTICK_PERIOD_MS);

        uint64_t FreeMem = esp_get_free_heap_size();

		printf("[APP] Free memory: %lld bytes\n", FreeMem);

        if(FreeMem < 10000){
            char *Massage = (char*) calloc((1000), sizeof(char));

            sprintf(Massage, "Memmory_less_than_10KB");
            API_Problem(Massage);
            free(Massage);
        }
	}
}

/* ############################################################################ */
bool API_GenQRCode(uint8_t PId, uint16_t Price)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[1024];

    if(FDSTATUS.SYSTEM_EVENT_STA_GOT_IP == false){
        return false;
    }

    if(FDSTATUS.HaveRegistor == false){
        return false;
    }

    if(PId < 1){
        return false;
    }

    ESP_LOGI("API_GenQRCode", "Starting Send Online API!");

    uint16_t checksum = 0x0000;

    char *keym = (char*) calloc((strlen(MAC_Address) + strlen(SERVER_Key2) + strlen(Dev_d_id) + strlen(Dev_m_id) + strlen(Dev_k_id) + 50), sizeof(char));
    sprintf(keym, "%s%s%s%s%s00%s", a_idGenQRCode, Dev_d_id, Dev_m_id, Dev_k_id, SERVER_Key2, MAC_Address);

    checksum = crc_ccitt_ffff((unsigned char *)keym, strlen(keym));

    char *OLN_URL = (char*) calloc((strlen(ONLINE_URL) + strlen(keym) + strlen(SERVER_Key1) + 100), sizeof(char));
    sprintf(OLN_URL, "http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/api?data={\"a_id\":\"%s\",\"keym\":\"%s\",\"key1\":\"%s\",\"p_id\":\"%d\",\"price\":\"%d\",\"sum\":\"%04X\"}", a_idGenQRCode, keym, SERVER_Key1, PId, Price, checksum);

    char *REQUEST = (char*) calloc((strlen(WEB_SERVER) + strlen(OLN_URL) + strlen(REQ) + 50), sizeof(char));
    sprintf(REQUEST, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: f-dtech/1.0 esp32-QR-Payment\r\n\r\n", OLN_URL, WEB_SERVER);

    printf("OLN_URL = %s\n", OLN_URL);
    printf("REQUEST = %s\n", REQUEST);

    free(keym);
    free(OLN_URL);

    int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

    if(err != 0 || res == NULL) {
        ESP_LOGE("API_GenQRCode", "DNS lookup failed err=%d res=%p", err, res);

        free(REQUEST);
        return false;
    }

    /* Code to print the resolved IP.
    Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI("API_GenQRCode", "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGE("API_GenQRCode", "... Failed to allocate socket.");
        freeaddrinfo(res);

        free(REQUEST);
        FDSTATUS.FailtoReset = true;
        return false;
    }
    ESP_LOGI("API_GenQRCode", "... allocated socket");

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE("API_GenQRCode", "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);

        free(REQUEST);
        return false;
    }

    ESP_LOGI("API_GenQRCode", "... connected");
    freeaddrinfo(res);

    if (write(s, REQUEST, strlen(REQUEST)) < 0) {
        ESP_LOGE("API_GenQRCode", "... socket send failed");
        close(s);

        free(REQUEST);
        return false;
    }
    ESP_LOGI("API_GenQRCode", "... socket send success");

    free(REQUEST);

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout)) < 0) {
        ESP_LOGE("API_GenQRCode", "... failed to set socket receiving timeout");
        close(s);

        return false;
    }
    ESP_LOGI("API_GenQRCode", "... set socket receiving timeout success");

//    char Data[500];
    char *Data = (char*) calloc(500, sizeof(char));

//    memset(Data, 0, sizeof(Data));

    /* Read HTTP response */
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);

        if(r > 0){
            char *p = NULL;

            p = strstr(recv_buf, "\r\n\r\n");

            if(p != NULL){
                p += 4;

                strcpy(Data, p);
            }
        }

    } while(r > 0);

    printf("Data : %s\n", Data);
    memset(DataResponse, 0, sizeof(DataResponse));

    strcpy(DataResponse, Data);

    ESP_LOGI("API_GenQRCode", "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);
    free(Data);

    if(strlen(DataResponse) > 10){
    	printf("strlen=%d",strlen(DataResponse));
        return true;

    }else{
        return false;
    }
}


/* ############################################################################ */
bool API_CheckPayment(void)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[500];

    if(FDSTATUS.SYSTEM_EVENT_STA_GOT_IP == false){
        return false;
    }

    if(FDSTATUS.HaveRegistor == false){
        return false;
    }

    ESP_LOGI("API_CheckPayment", "Starting Send Online API!");

    uint16_t checksum = 0x0000;

    char *keym = (char*) calloc((strlen(MAC_Address) + strlen(SERVER_Key2) + strlen(Dev_d_id) + strlen(Dev_m_id) + strlen(Dev_k_id) + 50), sizeof(char));
    sprintf(keym, "%s%s%s%s%s00%s", a_idCheckPayment, Dev_d_id, Dev_m_id, Dev_k_id, SERVER_Key2, MAC_Address);

    // printf("keym = %s\n", keym);

    checksum = crc_ccitt_ffff((unsigned char *)keym, strlen(keym));

    // printf("checksum = %04X\n", checksum);

    char *OLN_URL = (char*) calloc((strlen(ONLINE_URL) + strlen(keym) + strlen(SERVER_Key1) + 100), sizeof(char));
    sprintf(OLN_URL, "http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/api?data={\"a_id\":\"%s\",\"keym\":\"%s\",\"key1\":\"%s\",\"sum\":\"%04X\"}", a_idCheckPayment, keym, SERVER_Key1, checksum);

    char *REQUEST = (char*) calloc((strlen(WEB_SERVER) + strlen(OLN_URL) + strlen(REQ) + 50), sizeof(char));
    sprintf(REQUEST, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: f-dtech/1.0 esp32-QR-Payment\r\n\r\n", OLN_URL, WEB_SERVER);


    free(keym);
    free(OLN_URL);

    int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

    if(err != 0 || res == NULL) {
        ESP_LOGE("API_CheckPayment", "DNS lookup failed err=%d res=%p", err, res);

        free(REQUEST);
        return false;
    }

    /* Code to print the resolved IP.
    Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI("API_CheckPayment", "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGE("API_CheckPayment", "... Failed to allocate socket.");
        freeaddrinfo(res);

        free(REQUEST);
        return false;
    }
    ESP_LOGI("API_CheckPayment", "... allocated socket");

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE("API_CheckPayment", "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);

        free(REQUEST);
        return false;
    }

    ESP_LOGI("API_CheckPayment", "... connected");
    freeaddrinfo(res);

    if (write(s, REQUEST, strlen(REQUEST)) < 0) {
        ESP_LOGE("API_CheckPayment", "... socket send failed");
        close(s);

        free(REQUEST);
        return false;
    }
    ESP_LOGI("API_CheckPayment", "... socket send success");

    free(REQUEST);

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout)) < 0) {
        ESP_LOGE("API_CheckPayment", "... failed to set socket receiving timeout");
        close(s);

        return false;
    }
    ESP_LOGI("API_CheckPayment", "... set socket receiving timeout success");

//    char Data[500];
    char *Data = (char*) calloc(500, sizeof(char));

//    memset(Data, 0, sizeof(Data));

    /* Read HTTP response */
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);
        printf("recv_buf=%s",recv_buf);
        if(r > 0){
            char *p = NULL;

            p = strstr(recv_buf, "{");

            if(p != NULL){
               // p += 4;

                strcpy(Data, p);
            }
        }

    } while(r > 0);

    printf("Data : %s\n", Data);
   // printf("HTTP_END");
    memset(CheckingResponse, 0, sizeof(CheckingResponse));

    strcpy(CheckingResponse, Data);

    ESP_LOGI("API_CheckPayment", "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);
    free(Data);
    printf("CheckingResponse: %s\n", CheckingResponse);
	if (strlen(CheckingResponse) > 10){
        return true;

    }else{
        return false;
    }
}


/* ============================================================================== */
/* ============================================================================== */
void CheckPayment(void *pvParameters)
{
    while(1){
        if(FDSTATUS.StartCheckPayment == true){

            if(API_CheckPayment() == true){

                // printf("CheckingResponse : %s\n", CheckingResponse);

                cJSON *CheckResponse = cJSON_Parse(CheckingResponse);

                const cJSON *chcode = NULL;
                const cJSON *transid = NULL;
                const cJSON *price = NULL;

                if (CheckResponse == NULL)
                {
                    const char *error_ptr = cJSON_GetErrorPtr();
                    if (error_ptr != NULL)
                    {
                        fprintf(stderr, "Error before: %s\n", error_ptr);
                    }

                }else{

                    chcode = cJSON_GetObjectItemCaseSensitive(CheckResponse, "code");
                    if (cJSON_IsString(chcode) && (chcode->valuestring != NULL))
                    {
                        if(strcmp(chcode->valuestring, "E00") == 0){

                            transid = cJSON_GetObjectItemCaseSensitive(CheckResponse, "transid");
                            if (cJSON_IsString(transid) && (transid->valuestring != NULL))
                            {
                                if(strcmp(transid->valuestring, OldTransectionID) != 0)
                                {
                                    memset(OldTransectionID, 0, sizeof(OldTransectionID));
                                    strcpy(OldTransectionID, transid->valuestring);
                                    strcat(OldTransectionID, DEOF);

                                    price = cJSON_GetObjectItemCaseSensitive(CheckResponse, "price");
                                    if (cJSON_IsString(price) && (transid->valuestring != NULL))
                                    {
                                        memset(PriceQR, 0, sizeof(PriceQR));
                                        strcpy(PriceQR, price->valuestring);
                                        strcat(PriceQR, DEOF);

                                        TFT_fillRect(0,135,300,245,TFT_WHITE);

                                        _fg = TFT_RED;
                                        _bg = TFT_WHITE;
                                        TFT_setFont(TOONEY32_FONT, NULL);

                                        char Str[20];

                                        memset(Str, 0, sizeof(Str));

                                        sprintf(Str, "%s Bath",PriceQR);
                                        TFT_print(Str, CENTER, 210);
                                        printf("PriceQR:%s\n", PriceQR);

                                        FDSTATUS.Start_Generate = true;

                                        // uint16_t NumBaht = 0;
                                        // NumBaht = atoi(PriceQR);

                                        // Demo Test Only
                                        // if(DisplayMode != rabbitline_disp){
                                        //     NumPulse_Gen = (atof(PriceQR) * 1000);

                                        // }else{

                                        //     NumPulse_Gen = (atof(PriceQR) * 30);
                                        // }


                                        NumPulse_Gen = PulseNumber;     //(NumBaht/PusletoBaht);
                                        xTaskCreate(&pulse_output, "pulse_output", 4096, NULL, 7, &Pulse_Out);

                                        printf("NumPulse_Gen=%d\n", NumPulse_Gen);

                                        vTaskDelay((5000) / portTICK_PERIOD_MS);

                                        FDSTATUS.PaymentSuccess = true;
                                        FDSTATUS.StartCheckPayment = false;
                                    }
                                }
                            }
                        }
                    }
                }

                cJSON_Delete(CheckResponse);

            }
        }

        vTaskDelay((1500) / portTICK_PERIOD_MS);
    }
}


/* ############################################################################ */
static void API_Online(void *pvParameters)
{
    while(true) {

    	if(FDSTATUS.SYSTEM_EVENT_STA_GOT_IP == false){
    		goto exit;
    	}

    	if(FDSTATUS.HaveRegistor == false){
    		goto exit;
    	}


        if(FDSTATUS.APIOnlineFail == true){
            vTaskDelay((10000) / portTICK_PERIOD_MS);

        }else{
            if(OnlineTimeSec > 0){
                vTaskDelay((OnlineTimeSec*60000) / portTICK_PERIOD_MS);

            }else{
                vTaskDelay((10000) / portTICK_PERIOD_MS);
            }
        }

        const struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
        };

        struct addrinfo *res;
        struct in_addr *addr;
        int s, r;


        char recv_buf[500];

    	ESP_LOGI("API_Online", "Starting Send Online API!");

		uint16_t checksum = 0x0000;

	    char *keym = (char*) calloc((strlen(MAC_Address) + strlen(SERVER_Key2) + strlen(Dev_d_id) + strlen(Dev_m_id) + strlen(Dev_k_id) + 50), sizeof(char));
	    sprintf(keym, "%s%s%s%s%s00%s", a_idOnline, Dev_d_id, Dev_m_id, Dev_k_id, SERVER_Key2, MAC_Address);

	    checksum = crc_ccitt_ffff((unsigned char *)keym, strlen(keym));

	    char *OLN_URL = (char*) calloc((strlen(ONLINE_URL) + strlen(keym) + strlen(SERVER_Key1) + 100), sizeof(char));
	    sprintf(OLN_URL, "http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/api?data={\"a_id\":\"%s\",\"keym\":\"%s\",\"key1\":\"%s\",\"sum\":\"%04X\"}", a_idOnline, keym, SERVER_Key1, checksum);

	    char *REQUEST = (char*) calloc((strlen(WEB_SERVER) + strlen(OLN_URL) + strlen(REQ) + 50), sizeof(char));
	    sprintf(REQUEST, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: f-dtech/1.0 esp32-QR-Payment\r\n\r\n", OLN_URL, WEB_SERVER);

	    free(keym);
	    free(OLN_URL);

        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE("API_Online", "DNS lookup failed err=%d res=%p", err, res);

            free(REQUEST);
            goto exit;
        }

        /* Code to print the resolved IP.
        Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI("API_Online", "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE("API_Online", "... Failed to allocate socket.");
            freeaddrinfo(res);

            free(REQUEST);
            goto exit;
        }
        ESP_LOGI("API_Online", "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE("API_Online", "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);

            free(REQUEST);
            goto exit;
        }

        ESP_LOGI("API_Online", "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE("API_Online", "... socket send failed");
            close(s);

            free(REQUEST);
            goto exit;
        }
        ESP_LOGI("API_Online", "... socket send success");

        free(REQUEST);

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;

        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE("API_Online", "... failed to set socket receiving timeout");
            close(s);

            goto exit;
        }
        ESP_LOGI("API_Online", "... set socket receiving timeout success");

        char *DTResponse = (char*) calloc(500, sizeof(char));

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);

            if(r > 0){
            	char *p = NULL;

	            p = strstr(recv_buf, "\r\n\r\n");

	            if(p != NULL){
		            p += 4;

		            strcpy(DTResponse, p);
		        }
	    	}

        } while(r > 0);

        printf("Data : %s\n", DTResponse);

        ESP_LOGI("API_Online", "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);

        if(strlen(DTResponse) < 10){
            FDSTATUS.APIOnlineFail = true;
            printf("First Online has Fail\n");
            goto exit;
        }

        cJSON *Response = cJSON_Parse(DTResponse);

        const cJSON *online_t = NULL;
        const cJSON *chcode = NULL;
        const cJSON *p_num = NULL;
        const cJSON *p_val = NULL;
        const cJSON *p_price = NULL;

        if (Response == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != NULL)
            {
                fprintf(stderr, "Error before: %s\n", error_ptr);
            }

        }else{

            chcode = cJSON_GetObjectItemCaseSensitive(Response, "code");
            if (cJSON_IsString(chcode) && (chcode->valuestring != NULL))
            {
                if(strcmp(chcode->valuestring, "E00") == 0){

                    online_t = cJSON_GetObjectItemCaseSensitive(Response, "online_t");
                    if (cJSON_IsString(online_t) && (online_t->valuestring != NULL))
                    {
                        OnlineTimeSec = (atoi(online_t->valuestring));
                    }

                    p_num = cJSON_GetObjectItemCaseSensitive(Response, "p_num");
                    if (cJSON_IsString(p_num) && (p_num->valuestring != NULL))
                    {
                        uint16_t Pulse_Num = atoi(p_num->valuestring);

                        if(Pulse_Num != PulseNumber){
                            PulseNumber = Pulse_Num;
                        }
                    }

                    p_val = cJSON_GetObjectItemCaseSensitive(Response, "p_val");
                    if (cJSON_IsString(p_val) && (p_val->valuestring != NULL))
                    {
                        uint16_t P_Cost = atoi(p_val->valuestring);

                        if(P_Cost != PulseCost){
                            PulseCost = P_Cost;
                        }
                    }

                    p_price = cJSON_GetObjectItemCaseSensitive(Response, "p_price");
                    if (cJSON_IsString(p_price) && (p_price->valuestring != NULL))
                    {
                        uint16_t D_Prices = atoi(p_price->valuestring);

                        if(D_Prices != DevPrices){
                            DevPrices = D_Prices;
                        }
                    }
                }
            }
        }

        printf("OnlineTimeSec = %d\n", OnlineTimeSec);

        cJSON_Delete(Response);
        free(DTResponse);

        exit:
			printf("API_ONLine Terminate\n");
    }
}


/* ############################################################################ */
void API_Problem(char *Massage)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[500];


	if(FDSTATUS.SYSTEM_EVENT_STA_GOT_IP == false){
		return;
	}

	if(FDSTATUS.HaveRegistor == false){
		return;
	}

	ESP_LOGI("API_Problem", "Starting Send Problem API!");

	uint16_t checksum = 0x0000;

    char *keym = (char*) calloc((strlen(MAC_Address) + strlen(SERVER_Key2) + strlen(Dev_d_id) + strlen(Dev_m_id) + strlen(Dev_k_id) + 50), sizeof(char));
    sprintf(keym, "%s%s%s%s%s00%s", a_idProblem, Dev_d_id, Dev_m_id, Dev_k_id, SERVER_Key2, MAC_Address);

    char *SumData = (char*) calloc((strlen(keym) + strlen(Massage)), sizeof(char));

    sprintf(SumData, "%s%s", keym, Massage);

    checksum = crc_ccitt_ffff((unsigned char *)SumData, strlen(SumData));

    free(SumData);

    char *OLN_URL = (char*) calloc((strlen(ONLINE_URL) + strlen(keym) + strlen(SERVER_Key1) + 100), sizeof(char));
    sprintf(OLN_URL, "http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/api?data={\"a_id\":\"%s\",\"keym\":\"%s\",\"key1\":\"%s\",\"message\":\"%s\",\"sum\":\"%04X\"}", a_idProblem, keym, SERVER_Key1, Massage, checksum);

    char *REQUEST = (char*) calloc((strlen(WEB_SERVER) + strlen(OLN_URL) + strlen(REQ) + 50), sizeof(char));
    sprintf(REQUEST, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: f-dtech/1.0 esp32-QR-Payment\r\n\r\n", OLN_URL, WEB_SERVER);

    free(keym);
    free(OLN_URL);

    int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

    if(err != 0 || res == NULL) {
        ESP_LOGE("API_Problem", "DNS lookup failed err=%d res=%p", err, res);

        free(REQUEST);
        return;
    }

    /* Code to print the resolved IP.
    Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI("API_Problem", "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGE("API_Problem", "... Failed to allocate socket.");
        freeaddrinfo(res);

        free(REQUEST);
        return;
    }
    ESP_LOGI("API_Problem", "... allocated socket");

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE("API_Problem", "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);

        free(REQUEST);
        return;
    }

    ESP_LOGI("API_Problem", "... connected");
    freeaddrinfo(res);

    if (write(s, REQUEST, strlen(REQUEST)) < 0) {
        ESP_LOGE("API_Problem", "... socket send failed");
        close(s);

        free(REQUEST);
        return;
    }
    ESP_LOGI("API_Problem", "... socket send success");

    free(REQUEST);

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout)) < 0) {
        ESP_LOGE("API_Problem", "... failed to set socket receiving timeout");
        close(s);

        return;
    }
    ESP_LOGI("API_Problem", "... set socket receiving timeout success");

//    char Data[100];
    char *Data = (char*) calloc(500, sizeof(char));

//    memset(Data, 0, sizeof(Data));

    /* Read HTTP response */
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);

        if(r > 0){
        	char *p = NULL;

            p = strstr(recv_buf, "\r\n\r\n");

            if(p != NULL){
	            p += 4;

	            strcpy(Data, p);
	        }
    	}

    } while(r > 0);

    ESP_LOGI("API_Problem", "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);

    printf("Data : %s\n", Data);
    free(Data);
}


/* ############################################################################ */
void API_Credit(char *Massage)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[500];


    if(FDSTATUS.SYSTEM_EVENT_STA_GOT_IP == false){
        return;
    }

    if(FDSTATUS.HaveRegistor == false){
        return;
    }

    ESP_LOGI("API_Credit", "Starting Send Credit API!");

    uint16_t checksum = 0x0000;

    char *keym = (char*) calloc((strlen(MAC_Address) + strlen(SERVER_Key2) + strlen(Dev_d_id) + strlen(Dev_m_id) + strlen(Dev_k_id) + 50), sizeof(char));
    sprintf(keym, "%s%s%s%s%s00%s", a_idAlert, Dev_d_id, Dev_m_id, Dev_k_id, SERVER_Key2, MAC_Address);

    char *SumData = (char*) calloc((strlen(keym) + strlen(Massage)), sizeof(char));

    sprintf(SumData, "%s%s", keym, Massage);
    // printf("SumData = %s\n", SumData);

    checksum = crc_ccitt_ffff((unsigned char *)SumData, strlen(SumData));

    free(SumData);

    char *OLN_URL = (char*) calloc((strlen(ONLINE_URL) + strlen(keym) + strlen(SERVER_Key1) + 100), sizeof(char));
    sprintf(OLN_URL, "http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/api?data={\"a_id\":\"%s\",\"keym\":\"%s\",\"key1\":\"%s\",\"message\":\"%s\",\"sum\":\"%04X\"}", a_idAlert, keym, SERVER_Key1, Massage, checksum);

    char *REQUEST = (char*) calloc((strlen(WEB_SERVER) + strlen(OLN_URL) + strlen(REQ) + 50), sizeof(char));
    sprintf(REQUEST, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: f-dtech/1.0 esp32-QR-Payment\r\n\r\n", OLN_URL, WEB_SERVER);

    free(keym);
    free(OLN_URL);

    int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

    if(err != 0 || res == NULL) {
        ESP_LOGE("API_Alert", "DNS lookup failed err=%d res=%p", err, res);

        free(REQUEST);
        return;
    }

    /* Code to print the resolved IP.
    Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI("API_Alert", "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGE("API_Alert", "... Failed to allocate socket.");
        freeaddrinfo(res);

        free(REQUEST);
        return;
    }
    ESP_LOGI("API_Alert", "... allocated socket");

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE("API_Alert", "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);

        free(REQUEST);
        return;
    }

    ESP_LOGI("API_Alert", "... connected");
    freeaddrinfo(res);

    if (write(s, REQUEST, strlen(REQUEST)) < 0) {
        ESP_LOGE("API_Alert", "... socket send failed");
        close(s);

        free(REQUEST);
        return;
    }
    ESP_LOGI("API_Alert", "... socket send success");

    free(REQUEST);

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout)) < 0) {
        ESP_LOGE("API_Alert", "... failed to set socket receiving timeout");
        close(s);

        return;
    }
    ESP_LOGI("API_Alert", "... set socket receiving timeout success");

//    char Data[100];
    char *Data = (char*) calloc(500, sizeof(char));

//    memset(Data, 0, sizeof(Data));

    /* Read HTTP response */
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);

        if(r > 0){
            char *p = NULL;

            p = strstr(recv_buf, "\r\n\r\n");

            if(p != NULL){
                p += 4;

                strcpy(Data, p);
            }
        }

    } while(r > 0);

    ESP_LOGI("API_Alert", "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);

    printf("Data : %s\n", Data);
    free(Data);
}


/* ############################################################################ */
void API_Alert(char *Massage)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[500];


    if(FDSTATUS.SYSTEM_EVENT_STA_GOT_IP == false){
        return;
    }

    if(FDSTATUS.HaveRegistor == false){
        return;
    }

    ESP_LOGI("API_Alert", "Starting Send Alert API!");

    uint16_t checksum = 0x0000;

    char *keym = (char*) calloc((strlen(MAC_Address) + strlen(SERVER_Key2) + strlen(Dev_d_id) + strlen(Dev_m_id) + strlen(Dev_k_id) + 50), sizeof(char));
    sprintf(keym, "%s%s%s%s%s00%s", a_idAlert, Dev_d_id, Dev_m_id, Dev_k_id, SERVER_Key2, MAC_Address);

    char *SumData = (char*) calloc((strlen(keym) + strlen(Massage)), sizeof(char));

    sprintf(SumData, "%s%s", keym, Massage);
    // printf("SumData = %s\n", SumData);

    checksum = crc_ccitt_ffff((unsigned char *)SumData, strlen(SumData));

    free(SumData);

    char *OLN_URL = (char*) calloc((strlen(ONLINE_URL) + strlen(keym) + strlen(SERVER_Key1) + 100), sizeof(char));
    sprintf(OLN_URL, "http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/api?data={\"a_id\":\"%s\",\"keym\":\"%s\",\"key1\":\"%s\",\"message\":\"%s\",\"sum\":\"%04X\"}", a_idAlert, keym, SERVER_Key1, Massage, checksum);

    char *REQUEST = (char*) calloc((strlen(WEB_SERVER) + strlen(OLN_URL) + strlen(REQ) + 50), sizeof(char));
    sprintf(REQUEST, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: f-dtech/1.0 esp32-QR-Payment\r\n\r\n", OLN_URL, WEB_SERVER);

    free(keym);
    free(OLN_URL);

    int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

    if(err != 0 || res == NULL) {
        ESP_LOGE("API_Alert", "DNS lookup failed err=%d res=%p", err, res);

        free(REQUEST);
        return;
    }

    /* Code to print the resolved IP.
    Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI("API_Alert", "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGE("API_Alert", "... Failed to allocate socket.");
        freeaddrinfo(res);

        free(REQUEST);
        return;
    }
    ESP_LOGI("API_Alert", "... allocated socket");

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE("API_Alert", "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);

        free(REQUEST);
        return;
    }

    ESP_LOGI("API_Alert", "... connected");
    freeaddrinfo(res);

    if (write(s, REQUEST, strlen(REQUEST)) < 0) {
        ESP_LOGE("API_Alert", "... socket send failed");
        close(s);

        free(REQUEST);
        return;
    }
    ESP_LOGI("API_Alert", "... socket send success");

    free(REQUEST);

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout)) < 0) {
        ESP_LOGE("API_Alert", "... failed to set socket receiving timeout");
        close(s);

        return;
    }
    ESP_LOGI("API_Alert", "... set socket receiving timeout success");

//    char Data[100];
    char *Data = (char*) calloc(500, sizeof(char));

//    memset(Data, 0, sizeof(Data));

    /* Read HTTP response */
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);

        if(r > 0){
            char *p = NULL;

            p = strstr(recv_buf, "\r\n\r\n");

            if(p != NULL){
                p += 4;

                strcpy(Data, p);
            }
        }

    } while(r > 0);

    ESP_LOGI("API_Alert", "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);

    printf("Data : %s\n", Data);
    free(Data);
}


/* ############################################################################ */
bool API_Payment(uint16_t bank, uint16_t coin, uint64_t meter)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;

    if(FDSTATUS.SYSTEM_EVENT_STA_GOT_IP == false){
        return false;
    }

    if(FDSTATUS.HaveRegistor == false){
        return false;
    }


    ESP_LOGI("API_Payment", "Starting Send Payment API!");

    uint16_t checksum = 0x0000;

    char *keym = (char*) calloc((strlen(MAC_Address) + strlen(SERVER_Key2) + strlen(Dev_d_id) + strlen(Dev_m_id) + strlen(Dev_k_id) + 50), sizeof(char));
    sprintf(keym, "%s%s%s%s%s00%s", a_idPayment, Dev_d_id, Dev_m_id, Dev_k_id, SERVER_Key2, MAC_Address);

    checksum = crc_ccitt_ffff((unsigned char *)keym, strlen(keym));

    char *OLN_URL = (char*) calloc((strlen(ONLINE_URL) + strlen(keym) + strlen(SERVER_Key1) + 100), sizeof(char));
    sprintf(OLN_URL, "http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/api?data={\"a_id\":\"%s\",\"keym\":\"%s\",\"key1\":\"%s\",\"bank\":\"%d\",\"coin\":\"%d\",\"meter\":\"%lld\",\"sum\":\"%04X\"}", a_idPayment, keym, SERVER_Key1, bank, coin, meter, checksum);

    char *REQUEST = (char*) calloc((strlen(WEB_SERVER) + strlen(OLN_URL) + strlen(REQ) + 250), sizeof(char));
    sprintf(REQUEST, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: f-dtech/1.0 esp32-QR-Payment\r\n\r\n", OLN_URL, WEB_SERVER);

    printf("OLN_URL = %s\n", OLN_URL);
    printf("REQUEST = %s\n", REQUEST);

    free(keym);
    free(OLN_URL);

    int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

    if(err != 0 || res == NULL) {
        ESP_LOGE("API_Payment", "DNS lookup failed err=%d res=%p", err, res);

        free(REQUEST);
        return false;
    }

    /* Code to print the resolved IP.
    Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI("API_Payment", "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGE("API_Payment", "... Failed to allocate socket.");
        freeaddrinfo(res);

        free(REQUEST);
        return false;
    }
    ESP_LOGI("API_Payment", "... allocated socket");

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE("API_Payment", "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);

        free(REQUEST);
        return false;
    }

    ESP_LOGI("API_Payment", "... connected");
    freeaddrinfo(res);

    if (write(s, REQUEST, strlen(REQUEST)) < 0) {
        ESP_LOGE("API_Payment", "... socket send failed");
        close(s);

        free(REQUEST);
        return false;
    }
    ESP_LOGI("API_Payment", "... socket send success");

    free(REQUEST);

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout)) < 0) {
        ESP_LOGE("API_Payment", "... failed to set socket receiving timeout");
        close(s);

        return false;
    }
    ESP_LOGI("API_Payment", "... set socket receiving timeout success");

    char recv_buf[500];

//    char Data[500];
    char *Data = (char*) calloc(500, sizeof(char));

//    memset(Data, 0, sizeof(Data));

    /* Read HTTP response */
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);

        if(r > 0){
            char *p = NULL;

            p = strstr(recv_buf, "\r\n\r\n");

            if(p != NULL){
                p += 4;

                strcpy(Data, p);
            }
        }

    } while(r > 0);

    printf("Data : %s\n", Data);
    memset(DataResponse, 0, sizeof(DataResponse));

    strcpy(DataResponse, Data);

    ESP_LOGI("API_Payment", "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);

    free(Data);

    if(strlen(DataResponse) > 10){
        return true;

    }else{
        return false;
    }
}


uint16_t AllMoney;

/* ############################################################################ */
void First_Online(void)
{
    if(FDSTATUS.HaveRegistor == false){
        return;
    }

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;

    FDSTATUS.FirstOnlineCmp = false;

    ESP_LOGI("First_Online", "Starting Send First Online Status!");

    uint16_t checksum = 0x0000;

    char *keym = (char*) calloc((strlen(MAC_Address) + strlen(SERVER_Key2) + strlen(Dev_d_id) + strlen(Dev_m_id) + strlen(Dev_k_id) + 100), sizeof(char));
    sprintf(keym, "%s%s%s%s%s00%s", a_idOnline, Dev_d_id, Dev_m_id, Dev_k_id, SERVER_Key2, MAC_Address);

    checksum = crc_ccitt_ffff((unsigned char *)keym, strlen(keym));

    char *OLN_URL = (char*) calloc((strlen(ONLINE_URL) + strlen(keym) + strlen(SERVER_Key1) + 100), sizeof(char));
    sprintf(OLN_URL, "http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/api?data={\"a_id\":\"%s\",\"keym\":\"%s\",\"key1\":\"%s\",\"sum\":\"%04X\"}", a_idOnline, keym, SERVER_Key1, checksum);

    char *REQUEST = (char*) calloc((strlen(WEB_SERVER) + strlen(OLN_URL) + strlen(REQ) + 50), sizeof(char));
    sprintf(REQUEST, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: f-dtech/1.0 esp32-QR-Payment\r\n\r\n", OLN_URL, WEB_SERVER);

    free(keym);
    free(OLN_URL);

    int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

    if(err != 0 || res == NULL) {
        ESP_LOGE("First_Online", "DNS lookup failed err=%d res=%p", err, res);

        free(REQUEST);
        return;
    }

    /* Code to print the resolved IP.
    Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI("First_Online", "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGE("First_Online", "... Failed to allocate socket.");
        freeaddrinfo(res);

        free(REQUEST);
        return;
    }
    ESP_LOGI("First_Online", "... allocated socket");

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE("First_Online", "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);

        free(REQUEST);
        return;
    }

    ESP_LOGI("First_Online", "... connected");
    freeaddrinfo(res);

    if (write(s, REQUEST, strlen(REQUEST)) < 0) {
        ESP_LOGE("First_Online", "... socket send failed");
        close(s);

        free(REQUEST);
        return;
    }
    ESP_LOGI("First_Online", "... socket send success");

    free(REQUEST);

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0) {
        ESP_LOGE("First_Online", "... failed to set socket receiving timeout");
        close(s);
        return;
    }
    ESP_LOGI("First_Online", "... set socket receiving timeout success");

    char recv_buf[500];
    char *DTResponse = (char*) calloc(500, sizeof(char));

    /* Read HTTP response */
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);

        if(r > 0){
            char *p = NULL;

            p = strstr(recv_buf, "\r\n\r\n");

            if(p != NULL){
                p += 4;

                strcpy(DTResponse, p);
            }
        }

    } while(r > 0);

    printf("Data : %s\n", DTResponse);

    ESP_LOGI("First_Online", "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);

    if(strlen(DTResponse) < 10){
        FDSTATUS.APIOnlineFail = true;
        printf("First Online has Fail\n");
        return;
    }

    cJSON *Response = cJSON_Parse(DTResponse);

    const cJSON *online_t = NULL;
    const cJSON *chcode = NULL;
    const cJSON *p_num = NULL;
    const cJSON *p_val = NULL;
    const cJSON *p_price = NULL;

    if (Response == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }

    }else{

        chcode = cJSON_GetObjectItemCaseSensitive(Response, "code");
        if (cJSON_IsString(chcode) && (chcode->valuestring != NULL))
        {
            if(strcmp(chcode->valuestring, "E00") == 0){
                FDSTATUS.FirstOnlineCmp = true;

                online_t = cJSON_GetObjectItemCaseSensitive(Response, "online_t");
                if (cJSON_IsString(online_t) && (online_t->valuestring != NULL))
                {
                    OnlineTimeSec = (atoi(online_t->valuestring));
                    printf("OnlineTimeSec = %d\n", OnlineTimeSec);
                }

                p_num = cJSON_GetObjectItemCaseSensitive(Response, "p_num");
                if (cJSON_IsString(p_num) && (p_num->valuestring != NULL))
                {
                    uint16_t Pulse_Num = atoi(p_num->valuestring);

                    if(Pulse_Num != PulseNumber){
                        PulseNumber = Pulse_Num;
                    }
                }

                p_val = cJSON_GetObjectItemCaseSensitive(Response, "p_val");
                if (cJSON_IsString(p_val) && (p_val->valuestring != NULL))
                {
                    uint16_t P_Cost = atoi(p_val->valuestring);

                    if(P_Cost != PulseCost){
                        PulseCost = P_Cost;
                    }
                }

                p_price = cJSON_GetObjectItemCaseSensitive(Response, "p_price");
                if (cJSON_IsString(p_price) && (p_price->valuestring != NULL))
                {
                    uint16_t D_Prices = atoi(p_price->valuestring);

                    if(D_Prices != DevPrices){
                        DevPrices = D_Prices;
                    }
                }
            }
        }
    }

    cJSON_Delete(Response);
    free(DTResponse);

    if(FDSTATUS.FirstOnlineCmp == true){
        xTaskCreate(&API_Online, "API_Online", 4096, NULL, 4, &Online);
    }
}

/* ============================================================================== */
/* ============================================================================== */
void SendMoney_Server(void *pvParameter)
{
    while(true)
    {
        vTaskDelay(60000UL);

        MoneyMeter = MoneyMeter + AllMoney;
        printf("Send Money to Server\n");

        if(API_Payment(0, AllMoney, MoneyMeter) == true){
            FDSTATUS.HaveInsertMoney = false;
            AllMoney = 0;
            vTaskDelete(SendMoney);
        }
    }
}

/* ============================================================================== */
/* ============================================================================== */
void coin_in_process()
{
    if(FDSTATUS.Start_Generate == false)
    {
        if(FDSTATUS.Coin_Input == 0){
            if(gpio_get_level(PULSE_COIN_IN) == 0){
                FDSTATUS.Coin_Input = 1;
                FDSTATUS.Pulse_1Baht = false;
                counter_wait_count = 0;
                counter_money = 0;
            }

        }else{
            if(gpio_get_level(PULSE_COIN_IN) == 0){

                if(FDSTATUS.Pulse_1Baht == false){
                    if(++counter_pulse_alive >= 20){
                        counter_money += 1;
                        counter_wait_count = 0;
                        counter_pulse_alive = 0;
                        FDSTATUS.Pulse_1Baht = true;
                    }
                }

            }else{
                counter_wait_count += 1;

                if(counter_wait_count > 20){
                    FDSTATUS.Pulse_1Baht = false;

                    if(counter_wait_count > 110){
                        counter_wait_count = 0;
                        FDSTATUS.Coin_Input = 0;

                        InsertMoney = counter_money*PulseCost;
                        printf("Insert Coin = %d Baht\n", (InsertMoney));

                        counter_money = 0;

                        AllMoney = AllMoney + InsertMoney;
                        InsertMoney = 0;
                        FDSTATUS.Insert1Coil = true;

                        if(AllMoney >= DevPrices){

                            printf("AllMoney=%d\n", AllMoney);

                            MoneyMeter = MoneyMeter + AllMoney;

                            if(FDSTATUS.HaveInsertMoney == true){
                                FDSTATUS.HaveInsertMoney = false;
                                vTaskDelete(SendMoney);
                            }

                            if(API_Payment(0, AllMoney, MoneyMeter) == true){
                                AllMoney = 0;
                                NumPulse_Gen = PulseNumber;
                                FDSTATUS.Start_Generate = true;
                                xTaskCreate(&pulse_output, "pulse_output", 4096, NULL, 7, &Pulse_Out);
                                printf("NumPulse_Gen=%d\n", NumPulse_Gen);

                            }
                        }else{
                            if(FDSTATUS.HaveInsertMoney == false){
                                FDSTATUS.HaveInsertMoney = true;
                                xTaskCreate(&SendMoney_Server, "SendMoney_Server", 4096, NULL, 6, &SendMoney);
                            }
                        }
                    }
                }
            }
        }
    }
}

void pulse_coin_input(void *pvParameter)
{
    while(true)
    {
        coin_in_process();
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}


/* ============================================================================== */
/* ============================================================================== */
void pulse_output(void *pvParameter)
{
    while(true)
    {
        if(FDSTATUS.Start_Generate == true)
        {
            if(NumPulse_Gen > 0){
                vTaskDelay(30 / portTICK_PERIOD_MS);
                gpio_set_level(PULSE_COIN_OUT, 1);
                vTaskDelay(30 / portTICK_PERIOD_MS);
                gpio_set_level(PULSE_COIN_OUT, 0);
                NumPulse_Gen--;

            }else{
                PageDisplay = running;
                FDSTATUS.MachineisRun = false;
                FDSTATUS.Start_Generate = false;
                gpio_set_level(PULSE_COIN_OUT, 0);
                vTaskDelete(Pulse_Out);
            }
        }
    }
}

void LED_PWM_Init()
{
	 ledc_timer_config_t ledc_timer = {
	        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
	        .freq_hz = 5000,                      // frequency of PWM signal
	        .speed_mode = LEDC_HIGH_SPEED_MODE,           // timer mode
	        .timer_num = LEDC_TIMER_0,            // timer index
	        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
	    };
	    // Set configuration of timer0 for high speed channels
	    ledc_timer_config(&ledc_timer);


}

void LED_CONTROL()
{

	while(true)
	{
		ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {
						            {
						                .channel    = LEDC_HS_CH0_CHANNEL,
						                .duty       = 0,
						                .gpio_num   = LEDC_HS_CH0_GPIO,
						                .speed_mode = LEDC_HIGH_SPEED_MODE,
						                .hpoint     = 0,
						                .timer_sel  = LEDC_TIMER_0
						            },

						        };
						    ledc_channel_config(&ledc_channel[0]);

			ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, LEDC_TEST_DUTY);
			ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
			vTaskDelay(1 / portTICK_PERIOD_MS);
	}

}

/* ============================================================================== */
/* ============================================================================== */
void app_main()
{
    static httpd_handle_t server = NULL;

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // configure button and led pins as GPIO pins
    gpio_pad_select_gpio(PULSE_COIN_IN);
    gpio_pad_select_gpio(PULSE_COIN_OUT);
   // gpio_pad_select_gpio(PWM_DISPLAY);
    gpio_pad_select_gpio(COIN_POWER_IN);

    // set the correct direction
  //  gpio_set_direction(PWM_DISPLAY, GPIO_MODE_OUTPUT);
    gpio_set_direction(PULSE_COIN_OUT, GPIO_MODE_OUTPUT);
    gpio_set_direction(PULSE_COIN_IN, GPIO_MODE_INPUT);
    gpio_set_direction(COIN_POWER_IN, GPIO_MODE_INPUT);

    // set the Pull-Up
    gpio_set_pull_mode(PULSE_COIN_IN, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(COIN_POWER_IN, GPIO_PULLUP_ONLY);

    gpio_pad_select_gpio(SWSELECT_PIN);
    gpio_pad_select_gpio(SWPAIDS_PIN);

    gpio_set_direction(SWSELECT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(SWPAIDS_PIN, GPIO_MODE_INPUT);



    LED_PWM_Init();


    /* ========================================= */
	/* ========================================= */
   	app_spiffs();

//    if(gpio_get_level(SWPAIDS_PIN) == 0){
//        vTaskDelay(5000 / portTICK_RATE_MS);
//
//        if(gpio_get_level(SWPAIDS_PIN) == 0){
//
//            printf("WiFi Setting Reset\n");
//
//            struct stat st;
//            if (stat("/spiffs/ext1.txt", &st) == 0) {
//                // Delete it if it exists
//                unlink("/spiffs/ext1.txt");
//            }
//
//            // Rename original file
//            // ESP_LOGI(TAG, "Renaming file");
//            if (rename("/spiffs/wificonfig.json", "/spiffs/ext1.txt") != 0) {
//                ESP_LOGE("SPIFFS", "Rename failed");
//            }
//
//            vTaskDelay(1000 / portTICK_PERIOD_MS);
//            esp_restart();
//        }
//    }

//    vTaskDelay(2000 / portTICK_PERIOD_MS);
    wifi_init_networks();

    // printf("ONLINE_Time = %s\n", ONLINE_Time);
	printf("SERVER_Key1 = %s\n", SERVER_Key1);
	printf("SERVER_Key2 = %s\n", SERVER_Key2);

    // FDSTATUS.Start_Generate = true;
    gpio_set_level(PULSE_COIN_OUT, 0);

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, &server));

    // start the task that will handle the button
    xTaskCreate(&pulse_coin_input, "pulse_input_task", 4096, NULL, 2, &Pulse_In);

    xTaskCreate(&memory_checking, "memory_checking", 4096, NULL, 9, &mem_checking);

    xTaskCreate(&LED_CONTROL, "LED", 1024, NULL, 9, &mem_checking);
  //  xTaskCreate(PWM_LCD, "PWM_LCD", 1024, NULL, 8, &LCD_Control);

    /* Start the server for the first time */
    Display_Initial();
}
