#ifndef _SVS_CONFIG_H
#define _SVS_CONFIG_H

#include <SVS_Time.h>

#define COM_PLT "117.60.157.137"
#define TEST_PLT "180.101.147.115"

#define IOTCONFIG_ID    0
#define DEVCONFIG_ID    1
#define IOTSTATE_DEFAULT_TO    180000

#pragma pack(1)
typedef enum{
	NB_FEATURE = 0,
	WIFI_FEATURE,
	GPS_FEATURE,
	METER_FEATURE,
	SENSOR_FEATURE,
	RS485_FEATURE,
	LIGHT_FEATURE,
	UNKNOW_FEATURE
}EN_FEATURE;

typedef struct
{
	char ocIp[16]; /*IPµÿ÷∑xxx.xxx.xxx.xxx*/
	U16 ocPort;
}ST_IOT_CONFIG;

//nb 2,wifi 3,gps 4,meter 5,sensor 6,rs485 7,light
typedef struct
{
	unsigned char nb;
	unsigned char wifi;
	unsigned char gps;
	unsigned char meter;
	unsigned char sensor;
	unsigned char rs485;
	unsigned char light;
}ST_DEV_CONFIG;
#pragma pack()

extern ST_IOT_CONFIG g_stIotConfigDefault;

void OppCoapConfig(void);
void OppCoapLoop(void);
void OppCoapConfigInit(void);
int OppCoapIOTStateChg(void);
int OppCoapIOTSetConfig(ST_IOT_CONFIG *pstConfigPara);
int OppCoapIOTGetConfig(ST_IOT_CONFIG *pstConfigPara);
int OppCoapIOTGetConfigFromFlash(ST_IOT_CONFIG *pstConfigPara);
int OppCoapIOTSetConfigToFlash(ST_IOT_CONFIG *pstConfigPara);
int OppCoapIOTRestoreFactory(void);
int ApsDevConfigInit();
int ApsDevConfigRead(ST_DEV_CONFIG * pstDevConfig);
int ApsDevConfigWrite(ST_DEV_CONFIG * pstDevConfig);
int ApsDevConfigReadFromFlash(ST_DEV_CONFIG * pstDevConfig);
int ApsDevConfigWriteToFlash(ST_DEV_CONFIG * pstDevConfig);
int ApsDevFeaturesSet(EN_FEATURE feature, int value);
int ApsDevFeaturesGet(EN_FEATURE feature, int * value);

#endif
