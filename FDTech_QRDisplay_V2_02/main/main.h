#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED


// #include "freertos/task.h"
#include <esp_http_server.h>

#include "cJSON.h"

#include "networks.h"
#include "spiffs.h"
#include "checksum.h"

#include "components/tft/tftspi.h"
#include "components/tft/tft.h"
#include "qrcode.h"
#include "ota.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#define PULSE_COIN_OUT      13
#define PULSE_COIN_IN       14
#define COIN_POWER_IN       26

#define SWSELECT_PIN        34
#define SWPAIDS_PIN         35

#define a_idRegister		"1"
#define a_idOnline			"2"
#define a_idProblem			"3"
#define a_idAlert			"4"
#define a_idCheckPayment	"5"
#define a_idPayment			"6"
#define a_idUpdateSim		"7"
#define a_idGenQRCode		"8"

#define DefaultPrices       30
#define DefaultPulseCnt     5
#define DefaultPulseNum     1

#define LEDC_HS_CH0_GPIO       (15)
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_TEST_CH_NUM       (1)
#define LEDC_TEST_DUTY         (4000)

#define WIFI_SSID "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
#define WIFI_PASS "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

#define	SetupPIN	"9999988888"

#define DevVersion  "FD-Tech QRPayment V1.08"

#define DEOF "\0\0"

#define WEB_SERVER "f-dtech.com"
#define WEB_PORT 80


#define REGISTER_URL "http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/register?data={\"a_id\":\"1\",\"d_id\":\"1\",\"m_id\":\"15\",\"k_id\":\"56\",\"imei\":\"cc50e3af71d0\",\"sum\":\"8D3D\"}"
#define ONLINE_URL	"http://backend.f-dtech.com/mod_apictl/ctl_dev_v1/api"


#define REQ	"GET REGISTER_URL HTTP/1.0\r\nHost: WEB_SERVER\r\nUser-Agent: f-dtech/1.0 esp32-QR-Payment\r\n\r\n"

typedef struct
{
    unsigned HaveWiFiSetting:1;
    unsigned HaveRegistor:1;
    unsigned RegisterHandle:1;
    unsigned HaveDataLog:1;
    unsigned MQTT_EVENT_CONNECTED:1;
    unsigned MQTT_HAS_START:1;
    unsigned WASHER_HAS_RUNNING:1;
    unsigned DateTime_HasUpdate:1;
    unsigned SYSTEM_EVENT_STA_GOT_IP:1;
    unsigned FirstStart:1;
    unsigned OTA_HAS_UPDATE:1;
    unsigned HasCoinsIN:1;
    unsigned Dot1:1;
    unsigned SetReboot:1;
    unsigned SevenSEGON:1;
    unsigned DeviceACK:1;
    unsigned HaveResponse:1;
    unsigned process_stat:1;
    unsigned MoneyComplete:1;
    unsigned QRPayReceive:1;
    unsigned RestartModule:1;
    unsigned FactoryReset:1;
    unsigned PrintTransError:1;
    unsigned StartCheckPayment:1;
    unsigned PaymentSuccess:1;
    unsigned Start_Generate:1;
    unsigned Coin_Input:1;
    unsigned Pulse_1Baht:1;
    unsigned HaveInsertMoney:1;
    unsigned Start_OTA:1;
    unsigned FailtoReset:1;
    unsigned ReturnHomeScreen:1;
    unsigned Insert1Coil:1;
    unsigned QR_StartWait:1;
    unsigned MachineisRun:1;
    unsigned printLCD:1;
    unsigned APIOnlineFail:1;
    unsigned FirstOnlineCmp:1;
    unsigned DisableReWiFi:1;
    unsigned NetworkDisconnect:1;
}s_stat;

enum dp{
    promppay_disp = 0,
    truemoney_disp,
    rabbitline_disp,
    bluepay_disp,
};

enum pmode{
    payment = 0,
    homepage,
    qrpage,
    running,
};

enum ccmode{
    waitInsert = 0,
};

// extern httpd_handle_t server;

extern xTaskHandle Display;
extern xTaskHandle ChPayment;

extern xTaskHandle OTAx;

extern void API_Problem(char *Massage);
extern void API_Alert(char *Massage);
extern bool API_CheckPayment(void);
extern void CheckPayment(void *pvParameters);
extern bool API_GenQRCode(uint8_t PId, uint16_t Price);
extern void pulse_output(void *pvParameter);
extern httpd_handle_t start_webserver(void);
extern void stop_webserver(httpd_handle_t server);
extern void First_Online(void);

/* ============================================================================== */
/* ============================================================================== */
extern uint16_t QR_WaitTime;
extern uint16_t RunningCnt;
extern uint16_t StopCnt;

extern s_stat FDSTATUS;

extern uint16_t AllMoney;

extern unsigned char DisplayMode;

extern unsigned char PageDisplay;

extern uint16_t PulseNumber;
extern uint16_t PulseCost;

extern uint16_t DevPrices;
// extern uint16_t PusletoBaht;

extern char Latitude[20];
extern char Longitude[20];

// extern uint16_t PulseperBath;

extern char MAC_Address[20];
extern uint8_t macaddress[6];

extern char IPAddr[20];
extern char Subnet[20];
extern char Gateway[20];

// extern char ONLINE_Time[5];
extern char SERVER_Key1[30];
extern char SERVER_Key2[30];

extern char Dev_d_id[10];
extern char Dev_m_id[10];
extern char Dev_k_id[10];

extern char DataResponse[500];
extern char CheckingResponse[500];

extern char OldTransectionID[100];
extern char TransectionID[100];

extern char OldPriceQR[10];
extern char PriceQR[10];

extern uint64_t MoneyMeter;

extern void Display_Initial(void);

#endif /* MAIN_H_INCLUDED */
