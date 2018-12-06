#ifndef __SVS_WIFI_H__
#define __SVS_WIFI_H__

#include <esp_wifi.h>

#pragma pack(1)
typedef struct
{
	unsigned char ssid[32];      /**< SSID of target AP*/
    unsigned char password[64]; 	
}ST_WIFI_CONFIG;
#pragma pack()

#define WIFICONFIG_ID    0

extern void ApsWifiInit(void);
extern void telnetTask(void *data);
//extern void WiFiThread(void *pvParameter);
int ApsWifiConfig(char * ssid, char * pass);
int ApsWifiConfigWithoutStart(char * ssid, char * pass);
void ApsWifiStopStart(wifi_config_t* wifi_config);
int ApsWifiConfigRead(ST_WIFI_CONFIG * pstWifiConfig);
int ApsWifiConfigWrite(ST_WIFI_CONFIG * pstWifiConfig);
int ApsWifiConfigReadFromFlash(ST_WIFI_CONFIG * pstWifiConfig);
int ApsWifiConfigWriteToFlash(ST_WIFI_CONFIG * pstWifiConfig);
int ApsGetWifiPower(float * power);
int ApsGetWifiMac(unsigned char *mac);
int ApsWifiRestore(void);
int ApsWifiConnect(uint8_t bssid[]);



#endif
