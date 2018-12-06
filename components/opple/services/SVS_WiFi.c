#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include "Includes.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "SVS_Coap.h"
#include "SVS_Opple.h"
#include "SVS_MsgMmt.h"
#include "OPP_Debug.h"
#include "SVS_WiFi.h"
#include "SVS_Udp.h"
#include "SVS_Ota.h"
#include "SVS_Para.h"
#include "telnet.h"
#include "cli-interactor.h"
#include <stdio.h>
#include <stdarg.h>



#define EXAMPLE_WIFI_SSID "chanxianchecknb"
#define EXAMPLE_WIFI_PASS "12345678"
#define HOSTNAME          "OPPLENBIOT"	



#define MESSAGE "Hello TCP Client!!"
#define LISTENQ 2
#define EMPH_STR(s) "****** "s" ******"

#define WIFI_ACTION_CHECK(v,e) \
		if((v)!=0){\
		DEBUG_LOG(DEBUG_MODULE_WIFI,DLL_WARN,"%s,err=%d\r\n",e,v);\
		return v;}

#define AP_LIST_MAX 10
static struct
{
	wifi_ap_record_t ap[AP_LIST_MAX];
	unsigned int count;
	unsigned int index;
	unsigned int recon;
}sApList;

extern OS_EVENTGROUP_T g_eventWifi;

T_MUTEX g_stWifiCfgMutex;
T_MUTEX g_stWifiCacheMutex;


ST_WIFI_CONFIG g_stWifiConfigDefault = {
	EXAMPLE_WIFI_SSID,
	EXAMPLE_WIFI_PASS
};
ST_WIFI_CONFIG g_stWifiConfig;
unsigned char g_ucWifiState = SYSTEM_EVENT_MAX;

char* TAG="WiFi";
char* tag = "telnet";

int ApsWifiAutoConnect(void);


int ApsWifiConfigRead(ST_WIFI_CONFIG * pstWifiConfig)
{
	MUTEX_LOCK(g_stWifiCacheMutex, MUTEX_WAIT_ALWAYS);
	memcpy(pstWifiConfig, &g_stWifiConfig,sizeof(ST_WIFI_CONFIG));
	MUTEX_UNLOCK(g_stWifiCacheMutex);
	return OPP_SUCCESS;
}

int ApsWifiConfigWrite(ST_WIFI_CONFIG * pstWifiConfig)
{
	MUTEX_LOCK(g_stWifiCacheMutex, MUTEX_WAIT_ALWAYS);
	memcpy(&g_stWifiConfig, pstWifiConfig,sizeof(ST_WIFI_CONFIG));
	MUTEX_UNLOCK(g_stWifiCacheMutex);
	return OPP_SUCCESS;
}

int ApsWifiConfigReadFromFlash(ST_WIFI_CONFIG * pstWifiConfig)
{
	int ret;
	ST_WIFI_CONFIG stWifiConfig;
	unsigned int len = sizeof(ST_WIFI_CONFIG);
	
	MUTEX_LOCK(g_stWifiCfgMutex, MUTEX_WAIT_ALWAYS);
	ret = AppParaRead(APS_PARA_MODULE_APS_WIFI, WIFICONFIG_ID, (unsigned char *)&stWifiConfig, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "ElecReadFlash fail ret %d\r\n",ret);
		MUTEX_UNLOCK(g_stWifiCfgMutex);
		return OPP_FAILURE;
	}

	memcpy(pstWifiConfig, &stWifiConfig, sizeof(ST_WIFI_CONFIG));
	MUTEX_UNLOCK(g_stWifiCfgMutex);
	return OPP_SUCCESS;
}

int ApsWifiConfigWriteToFlash(ST_WIFI_CONFIG * pstWifiConfig)
{
	int ret;
	
	MUTEX_LOCK(g_stWifiCfgMutex, MUTEX_WAIT_ALWAYS);
	ret = AppParaWrite(APS_PARA_MODULE_APS_WIFI, WIFICONFIG_ID, (unsigned char *)pstWifiConfig, sizeof(ST_WIFI_CONFIG));
	if(OPP_SUCCESS != ret){
		MUTEX_UNLOCK(g_stWifiCfgMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stWifiCfgMutex);
	return OPP_SUCCESS;
}

/*
void telnetPrintf(char* msg)
{
    telnet_esp32_sendData((uint8_t *)msg, strlen(msg));
}
*/

void telnetPrintf(char* msg,...)
{
	va_list vl;

	va_start(vl,msg);
    telnet_esp32_vprintf(msg,vl);
    va_end(vl);
}

static void recvData(uint8_t *buffer, size_t size) {
  int i;
  for(i = 0;i < size;i++)
  {
      CliIttInput(CLI_CHL_TELNET,buffer[i]);
  }
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	g_ucWifiState = event->event_id;
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
		ESP_LOGI(TAG, "event_handler SYSTEM_EVENT_STA_START at %d...",OppTickCountGet());
        esp_wifi_connect();
        sApList.recon = 0;
        break;
    case SYSTEM_EVENT_STA_STOP:
    	ESP_LOGI(TAG, "event_handler SYSTEM_EVENT_STA_STOP at %d...",OppTickCountGet());
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
    	ESP_LOGI(TAG, "event_handler SYSTEM_EVENT_CONNECTED at %d...",OppTickCountGet());
        break;
        
    case SYSTEM_EVENT_STA_GOT_IP:
        //xEventGroupSetBits(g_eventWifi, 1);
		ESP_LOGI(TAG, "event_handler SYSTEM_EVENT_STA_GOT_IP at %d...",OppTickCountGet());
        OS_EVENTGROUP_SETBIT(g_eventWifi,1);
        //xTaskCreatePinnedToCore(&telnetTask, "telnetTask", 8048, NULL, 11, NULL, 0);
        break;
    case SYSTEM_EVENT_STA_LOST_IP:
    	ESP_LOGI(TAG, "event_handler SYSTEM_EVENT_STA_LOST_IP at %d...",OppTickCountGet());
    	break;
    	
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
		ESP_LOGI(TAG, "event_handler SYSTEM_EVENT_STA_DISCONNECTED at %d...",OppTickCountGet());
		//esp_wifi_connect();
		if(sApList.recon == 0)
		{
        	ApsWifiAutoConnect();
        }
        OS_EVENTGROUP_CLEARBIT(g_eventWifi,1);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
	int ret;
	char hostname[128] = {0};
	unsigned char mac[6] = {0};
	wifi_config_t wifi_config;
	ST_WIFI_CONFIG stWifiConfig;

    tcpip_adapter_init();
    //wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    /*wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };*/
    ApsWifiConfigRead(&stWifiConfig);
    ESP_LOGI(TAG, "ApsWifiConfigRead SSID %s pass %s...", stWifiConfig.ssid, stWifiConfig.password);
	memset(&wifi_config, 0, sizeof(wifi_config_t));
	strcpy((char *)wifi_config.sta.ssid, (char *)stWifiConfig.ssid);
	strcpy((char *)wifi_config.sta.password, (char *)stWifiConfig.password);
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s pass %s...", wifi_config.sta.ssid, wifi_config.sta.password);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_LOGI(TAG, "esp_wifi_start at %d...", OppTickCountGet());
    ESP_ERROR_CHECK( esp_wifi_start() );
	
    ESP_LOGI(TAG, EMPH_STR("tcpip_adapter_set_hostname"));
	ret = ApsGetWifiMac(mac);
	if(0 == ret)
		snprintf(hostname,128,"%s-%02x%02x%02x%02x",HOSTNAME,mac[2],mac[3],mac[4],mac[5]);
	else
		snprintf(hostname,128,"%s",HOSTNAME);

	ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname));
	
}

void ApsWifiStopStart(wifi_config_t* wifi_config)
{
	int ret;
	char hostname[128] = {0};
	unsigned char mac[6] = {0};

	sApList.recon = 1;
	sApList.count = 0;
    //wifi stop
    ESP_LOGI(TAG, EMPH_STR("esp_wifi_stop..."));
    ESP_ERROR_CHECK( esp_wifi_stop() );
    //ESP_LOGI(TAG, EMPH_STR("esp_wifi_deinit..."));
    //ESP_ERROR_CHECK(esp_wifi_deinit());

    ESP_LOGI(TAG, EMPH_STR("esp_wifi_init"));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();	
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_LOGI(TAG, EMPH_STR("esp_wifi_set_mode"));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_LOGI(TAG, EMPH_STR("esp_wifi_set_config"));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
    //now start wifi
    ESP_LOGI(TAG, EMPH_STR("esp_wifi_start..."));
    ESP_ERROR_CHECK(esp_wifi_start());
	
    ESP_LOGI(TAG, EMPH_STR("tcpip_adapter_set_hostname"));
	ret = ApsGetWifiMac(mac);
	if(0 == ret)
		snprintf(hostname,128,"%s-%02x%02x%02x%02x",HOSTNAME,mac[2],mac[3],mac[4],mac[5]);
	else
		snprintf(hostname,128,"%s",HOSTNAME);
	
	ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname));
}

int ApsWifiConnectBssid(uint8_t bssid[])
{
	int ret;
	wifi_config_t wifi_config;
	ST_WIFI_CONFIG stWifiConfig;
	uint8_t zero[6] ={0};
	
    ApsWifiConfigRead(&stWifiConfig);

	memset(&wifi_config, 0, sizeof(wifi_config_t));
	strcpy((char *)wifi_config.sta.ssid, (char *)stWifiConfig.ssid);
	strcpy((char *)wifi_config.sta.password, (char *)stWifiConfig.password);
	if(0 != memcmp(bssid,zero,6)){
		wifi_config.sta.bssid_set = true;
		memcpy(wifi_config.sta.bssid,bssid,6);
	}
	
    DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "Setting WiFi configuration SSID %s pass %s...\r\n", wifi_config.sta.ssid, wifi_config.sta.password);
    DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "Setting WiFi configuration bssid_set %d bssid %02x:%02x:%02x:%02x:%02x:%02x...\r\n", wifi_config.sta.bssid_set, 
		wifi_config.sta.bssid[0],wifi_config.sta.bssid[1],wifi_config.sta.bssid[2],wifi_config.sta.bssid[3],wifi_config.sta.bssid[4],wifi_config.sta.bssid[5]);
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ret = esp_wifi_connect();
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "esp_wifi_connect err ret %d\r\n",ret);
		return ret;
	}
	return 0;
}

void printApList(unsigned int apCount,wifi_ap_record_t* list)
{
	DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "===============================================================================\r\n");
	DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "            SSID        |    RSSI    |        AUTH        |        MAC         \r\n");
	DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "===============================================================================\r\n");
	for (int i=0; i<apCount; i++) {
		char *authmode;
		switch(list[i].authmode) {
			case WIFI_AUTH_OPEN:
				authmode = "WIFI_AUTH_OPEN";
				break;
			case WIFI_AUTH_WEP:
				authmode = "WIFI_AUTH_WEP";
				break;			
			case WIFI_AUTH_WPA_PSK:
				authmode = "WIFI_AUTH_WPA_PSK";
				break;
			case WIFI_AUTH_WPA2_PSK:
				authmode = "WIFI_AUTH_WPA2_PSK";
				break;
			case WIFI_AUTH_WPA_WPA2_PSK:
				authmode = "WIFI_AUTH_WPA_WPA2_PSK";
				break;
			default:
				authmode = "Unknown";
				break;
			}
		DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "%-20.20s    |%-4d	|-%-22.22s|%02x:%02x:%02x:%02x:%02x:%02x\r\n",list[i].ssid, list[i].rssi,authmode,
			list[i].bssid[0],list[i].bssid[1],list[i].bssid[2],list[i].bssid[3],list[i].bssid[4],list[i].bssid[5]);
	}
	DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "\r\n\r\n");
}

int ApsWifiAutoConnect(void)
{
	ST_WIFI_CONFIG stWifiConfig;
    unsigned short count=0;
	wifi_scan_config_t scanConf	= {.ssid = (uint8_t *)&stWifiConfig.ssid,.bssid = NULL,.channel = 0,.show_hidden = 1};
	
	ApsWifiConfigRead(&stWifiConfig);
	
	if((sApList.count == 0)||(sApList.count==sApList.index))
	{
		WIFI_ACTION_CHECK(esp_wifi_scan_start(&scanConf, 1),"scan err");//扫描所有可用的AP
		esp_wifi_scan_get_ap_num(&count);//Get number of APs found in last scan
		if(count>AP_LIST_MAX) count = AP_LIST_MAX;
		sApList.count = count;
		sApList.index = 0;
		WIFI_ACTION_CHECK(esp_wifi_scan_get_ap_records(&count, sApList.ap),"get ap recores err");
		printApList(count, sApList.ap);
		if(count == 0) 
		{
		    esp_wifi_connect();
			return 1;
		}
	}

	ApsWifiConnectBssid(sApList.ap[sApList.index++].bssid);
	
	return 0;
}

int ApsWifiConfig(char * ssid, char * pass)
{
	wifi_config_t wifi_config;
	ST_WIFI_CONFIG stWifiConfig;
	int ret;
	
	strcpy((char *)stWifiConfig.ssid, ssid);
	strcpy((char *)stWifiConfig.password, pass);
	ret = ApsWifiConfigWriteToFlash(&stWifiConfig);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "ApsWifiConfigWriteToFlash fail ret %d\r\n",ret);
		return ret;
	}
	ApsWifiConfigWrite(&stWifiConfig);
	
	memset(&wifi_config, 0, sizeof(wifi_config_t));
	strcpy((char *)wifi_config.sta.ssid, ssid);
	strcpy((char *)wifi_config.sta.password, pass);

    ApsWifiStopStart(&wifi_config);

	return 0;
}

int ApsWifiConfigWithoutStart(char * ssid, char * pass)
{
	ST_WIFI_CONFIG stWifiConfig;
	int ret;
	
	strcpy((char *)stWifiConfig.ssid, ssid);
	strcpy((char *)stWifiConfig.password, pass);
	ret = ApsWifiConfigWriteToFlash(&stWifiConfig);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "ApsWifiConfigWriteToFlash fail ret %d\r\n",ret);
		return ret;
	}
	ApsWifiConfigWrite(&stWifiConfig);
	
	return 0;
}

int ApsGetWifiMac(unsigned char *mac)
{
	int ret;
	
	ret = esp_wifi_get_mac(WIFI_IF_STA, mac);

	return ret;
}

int ApsGetWifiCh(unsigned char * ch)
{
	int ret;
	wifi_second_chan_t second;
	
	ret = esp_wifi_get_channel(ch,&second);

	return ret;
}

typedef struct
{
	unsigned int power;
	float dbm;
}ST_WIFI_POWER;

ST_WIFI_POWER wifiTxPower[] = {
	{78,19.5},
	{76,19},
	{74,18.5},
	{68,17},
	{60,15},
	{52,13},
	{44,11},
	{34,8.5},
	{28,7},
	{20,5},
	{8,2},
	{4,-1}
};
int ApsGetWifiPower(float * power)
{
	int ret = 0;
	int i,maxNum = sizeof(wifiTxPower)/sizeof(ST_WIFI_POWER);
	int8_t temp;
	
	ret = esp_wifi_get_max_tx_power(&temp);
	if(0 != ret)
		return ret;
	
	if(temp < wifiTxPower[0].power){
		*power = wifiTxPower[0].dbm;
		return 0;
	}
	if(temp < wifiTxPower[maxNum-1].power){
		*power = wifiTxPower[maxNum-1].dbm;
		return 0;
	}
	for(i=0;i<maxNum;i++){
		if(temp == wifiTxPower[i].power){
			*power = wifiTxPower[i].dbm;
			return 0;
		}
	}
	for(i=0;i<maxNum-1;i++){
		if(temp > wifiTxPower[i].power && temp < wifiTxPower[i+1].power){
			*power = (float)(wifiTxPower[i].dbm + wifiTxPower[i+1].dbm)/(float)2;
			return 0;
		}
	}
	
	return ret;
}

void ApsWifiInit(void)
{
	int ret;
	
	ST_WIFI_CONFIG stWifiConfig;

	sApList.count = 0;
	sApList.index= 0;
	sApList.recon = 0;
	
	MUTEX_CREATE(g_stWifiCfgMutex);
	MUTEX_CREATE(g_stWifiCacheMutex);
	ret = ApsWifiConfigReadFromFlash(&stWifiConfig);
	if(OPP_SUCCESS == ret){
		ApsWifiConfigWrite(&stWifiConfig);
	}else{
		ApsWifiConfigWrite(&g_stWifiConfigDefault);
	}

	initialise_wifi(); /////

	//return OPP_SUCCESS;
}

int ApsWifiRestore(void)
{
	int ret;
	ret = ApsWifiConfigWriteToFlash(&g_stWifiConfigDefault);
	if(OPP_SUCCESS != ret){
		return ret;
	}
	ApsWifiConfigWrite(&g_stWifiConfigDefault);
	return OPP_SUCCESS;
}

void telnetTask(void *data) {
	//ApsWifiInit();
	ESP_LOGD(tag, ">> telnetTask");
	telnet_esp32_listenForClients(recvData);
	ESP_LOGD(tag, "<< telnetTask");
	vTaskDelete(NULL);
}


/*
void WiFiThread(void *pvParameter)
{	  
   ESP_LOGI(TAG, "WiFiThread!");
    
    ESP_LOGI(TAG, "initialise_wifi!");
	ApsWifiInit();
    initialise_wifi();
    
    ESP_LOGI(TAG, "xEventGroupWaitBits!");
    xEventGroupWaitBits(g_eventWifi, 1,false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connect to Wifi ! "); 

    while(1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //telnet_server();
    }
   
}
*/


