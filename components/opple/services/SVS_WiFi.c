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
#include <DRV_Bc28.h>


#define EXAMPLE_WIFI_SSID "chanxianchecknb"
#define EXAMPLE_WIFI_PASS "12345678"
#define HOSTNAME          "OPPLENBIOT"	
#define EXAMPLE_ESP_WIFI_SSID      "opple"
#define EXAMPLE_ESP_WIFI_PASS      "12YfsA+b?12dfgB"
#define EXAMPLE_MAX_STA_CONN       2

#define MESSAGE "Hello TCP Client!!"
#define LISTENQ 2
#define EMPH_STR(s) "****** "s" ******"
#define SSID_MAX_LEN    32
#define PASS_MAX_LEN    64

#define WIFI_ACTION_CHECK(v,e) \
		if((v)!=0){\
		DEBUG_LOG(DEBUG_MODULE_WIFI,DLL_WARN,"%s,err=%d\r\n",e,v);\
		return v;}

#define AP_LIST_MAX 10
#define EVENT_QUEUE_MAX    10

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
T_MUTEX g_stEventQMutex;

ST_WIFI_CONFIG g_stWifiConfigDefault = {
	EXAMPLE_WIFI_SSID,
	EXAMPLE_WIFI_PASS
};
ST_WIFI_CONFIG g_stWifiConfig;
unsigned char g_ucWifiState = SYSTEM_EVENT_MAX;
static t_queue queue_event;
static unsigned char queue_event_buf[sizeof(system_event_t)*EVENT_QUEUE_MAX];

char* TAG="WiFi";
char* tag = "telnet";

extern unsigned char g_ucOnlyScan;
extern void BC28_STRNCPY(char * dst, char * src, int len);

int ApsWifiAutoConnect(void);
int ApsWifiAutoScanDone(void);
int ApsWifiAutoScan(void);


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

int NbImeiGet(char *imei)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(IMEI_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(IMEI_EVENT,&rArgc,rArgv,SEM_WAIT_ALWAYS);
	if(0 != ret){
		CLI_PRINTF("recvEvent IMEI_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return ret;
	}
	BC28_STRNCPY(imei,rArgv[0],NEUL_IMEI_LEN);
	ARGC_FREE(rArgc,rArgv);
	return OPP_SUCCESS;
}

void wifi_init_softap(char *imei,unsigned char *mac)
{
	char ssid[SSID_MAX_LEN]={0};
    wifi_config_t wifi_config = {
        .ap = {
            /*.ssid = EXAMPLE_ESP_WIFI_SSID,*/
            /*.ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),*/
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
	snprintf(ssid,SSID_MAX_LEN,"%15s%02x%02x%02x",imei,mac[3],mac[4],mac[5]);
	BC28_STRNCPY((char *)wifi_config.ap.ssid,ssid,SSID_MAX_LEN);
	wifi_config.ap.ssid_len = strlen(ssid);
	if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             ssid, EXAMPLE_ESP_WIFI_PASS);
}

void wifi_init_sta()
{
	int ret;
	char hostname[128] = {0};
	unsigned char mac[6] = {0};
	wifi_config_t wifi_config;
	ST_WIFI_CONFIG stWifiConfig;

    ApsWifiConfigRead(&stWifiConfig);
    ESP_LOGI(TAG, "ApsWifiConfigRead SSID %s pass %s...", stWifiConfig.ssid, stWifiConfig.password);
	memset(&wifi_config, 0, sizeof(wifi_config_t));
	strcpy((char *)wifi_config.sta.ssid, (char *)stWifiConfig.ssid);
	strcpy((char *)wifi_config.sta.password, (char *)stWifiConfig.password);
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s pass %s...", wifi_config.sta.ssid, wifi_config.sta.password);
    //ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
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

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	int ret;
	
	MUTEX_LOCK(g_stEventQMutex,MUTEX_WAIT_ALWAYS);
	ret = pushQueue(&queue_event,(unsigned char*)event,0);
	if(ESP_OK != ret){
		ESP_LOGI(TAG, "push event_handler[%d] ret %d~~~~\r\n", event->event_id, ret);
	}
	MUTEX_UNLOCK(g_stEventQMutex);
	return ESP_OK;
}

void apStartLoop()
{
	int ret;
	char imei[NEUL_IMEI_LEN] = {0};
	static unsigned char imeiIsOK = 0;
	unsigned char mac[6] = {0};
	static unsigned char init = 1;
	static unsigned int tick = 0, tick1 = 0;
	static unsigned char times = 0;
	tcpip_adapter_ip_info_t local_ip;
	static unsigned char needSoftap = 1;
	static unsigned char queryStaIp = 1;
	
	if(init){
		tick1 = OppTickCountGet();
		init = 0;
	}
	
	if(0 != tick1){
		if(OppTickCountGet()-tick1 < APSTART_TO)
			return;
		else{
			tick1 = 0;
			tick = 0;
		}
	}

	if(0 == needSoftap)
		return;
	
	if(queryStaIp){
		queryStaIp = 0;
		tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);
		ESP_LOGI(TAG, "sta:(ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR ")\r\n",
				   IP2STR(&local_ip.ip), IP2STR(&local_ip.netmask), IP2STR(&local_ip.gw));
		//ip addr is not zero, connect to wifi router
		if(!ip4_addr_cmp(&local_ip.ip,IP4_ADDR_ANY)&&
			!ip4_addr_cmp(&local_ip.netmask,IP4_ADDR_ANY)&&
			!ip4_addr_cmp(&local_ip.gw,IP4_ADDR_ANY)){
			ESP_LOGI(TAG,"do not start softap!!!\r\n");
			needSoftap = 0;
			return;
		}else{
			ESP_LOGI(TAG,"start softap!!!\r\n");
		}
	}
	if(!imeiIsOK){
		//per 10s get imei one time
		if(OppTickCountGet()-tick > READIMEI_TO){
			ret = NbImeiGet(imei);
			if(OPP_SUCCESS == ret){
				ret = ApsGetWifiMac(mac);
				if(OPP_SUCCESS != ret){
					DEBUG_LOG(DEBUG_MODULE_WIFI,DLL_ERROR,"apStartLoop call ApsGetWifiMac err ret %d",ret);
					memset(mac,0,sizeof(mac));
				}
				wifi_init_softap(imei,mac);
				imeiIsOK = 1;
				return;
			}else{
				tick = OppTickCountGet();
			}
			//one minutes not get imei, use default 0 as imei
			if(++times > 4){
				memset(imei,0,sizeof(imei));
				ret = ApsGetWifiMac(mac);
				if(OPP_SUCCESS != ret){
					DEBUG_LOG(DEBUG_MODULE_WIFI,DLL_ERROR,"apStartLoop call ApsGetWifiMac err ret %d",ret);
					memset(mac,0,sizeof(mac));
				}
				wifi_init_softap(imei,mac);
				imeiIsOK = 1;
			}
		}
	}
}
void eventLoop()
{
	system_event_t event;
	int ret;

	MUTEX_LOCK(g_stEventQMutex,MUTEX_WAIT_ALWAYS);
	ret = pullQueue(&queue_event,(unsigned char*)&event, sizeof(system_event_t));
	MUTEX_UNLOCK(g_stEventQMutex);
	if(ESP_OK == ret) {
		g_ucWifiState = event.event_id;
		switch (event.event_id) {
		case SYSTEM_EVENT_STA_START:{
			ESP_LOGI(TAG, "wifiTask SYSTEM_EVENT_STA_START...");
			esp_wifi_connect();
			sApList.recon = 0;
			break;
		}
		case SYSTEM_EVENT_STA_STOP:{
			ESP_LOGI(TAG, "wifiTask SYSTEM_EVENT_STA_STOP...");
			break;
		}
		case SYSTEM_EVENT_STA_CONNECTED:{
			system_event_sta_connected_t *connected = &event.event_info.connected;
			ESP_LOGI(TAG, "wifiTask SYSTEM_EVENT_STA_CONNECTED, ssid:%s, ssid_len:%d, bssid:" MACSTR ", channel:%d, authmode:%d", \
					   connected->ssid, connected->ssid_len, MAC2STR(connected->bssid), connected->channel, connected->authmode);
			break;
		}	
		case SYSTEM_EVENT_STA_GOT_IP:{
			system_event_sta_got_ip_t *got_ip = &event.event_info.got_ip;
			ESP_LOGI(TAG, "wifiTask SYSTEM_EVENT_STA_GOT_IP, ip:" IPSTR ", mask:" IPSTR ", gw:" IPSTR,
				IP2STR(&got_ip->ip_info.ip),
				IP2STR(&got_ip->ip_info.netmask),
				IP2STR(&got_ip->ip_info.gw));

			OS_EVENTGROUP_SETBIT(g_eventWifi,1);
			break;
		}	
		case SYSTEM_EVENT_STA_LOST_IP:{
			ESP_LOGI(TAG, "wifiTask SYSTEM_EVENT_STA_LOST_IP...");
			break;
		}	
		case SYSTEM_EVENT_STA_DISCONNECTED:{
			/* This is a workaround as ESP32 WiFi libs don't currently
			   auto-reassociate. */
			system_event_sta_disconnected_t *disconnected = &event.event_info.disconnected;
			ESP_LOGI(TAG, "wifiTask SYSTEM_EVENT_STA_DISCONNECTED, ssid:%s, ssid_len:%d, bssid:" MACSTR ", reason:%d", \
					   disconnected->ssid, disconnected->ssid_len, MAC2STR(disconnected->bssid), disconnected->reason);
			if(g_ucOnlyScan){
				OS_EVENTGROUP_CLEARBIT(g_eventWifi,1);
				break;
			}	
			//esp_wifi_connect();
			esp_wifi_disconnect();
			if(sApList.recon == 0)
			{
				ApsWifiAutoScan();
			}
			OS_EVENTGROUP_CLEARBIT(g_eventWifi,1);
			break;
		}	
		case SYSTEM_EVENT_SCAN_DONE:{
			ESP_LOGI(TAG, "wifiTask SYSTEM_EVENT_SCAN_DONE...");
			if(g_ucOnlyScan){
				OS_EVENTGROUP_SETBIT(g_eventWifi,2);
				break;
			}
			ApsWifiAutoScanDone();
			break;
		}	
		case SYSTEM_EVENT_AP_STACONNECTED:{
			ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
					 MAC2STR(event.event_info.sta_connected.mac),
					 event.event_info.sta_connected.aid);
			break;
		}	
		case SYSTEM_EVENT_AP_STADISCONNECTED:{
			ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
					 MAC2STR(event.event_info.sta_disconnected.mac),
					 event.event_info.sta_disconnected.aid);
			break;
		}	
		case SYSTEM_EVENT_AP_START:{
			ESP_LOGI(TAG, "wifiTask SYSTEM_EVENT_AP_START");
			break;
		}
		case SYSTEM_EVENT_AP_STOP:{
			ESP_LOGI(TAG, "wifiTask SYSTEM_EVENT_AP_STOP");
			break;
		}	
		case SYSTEM_EVENT_AP_STAIPASSIGNED:{
			ESP_LOGI(TAG, "wifiTask SYSTEM_EVENT_AP_STAIPASSIGNED");
			break;
		}	
		case SYSTEM_EVENT_AP_PROBEREQRECVED:{
			system_event_ap_probe_req_rx_t *ap_probereqrecved = &event.event_info.ap_probereqrecved;
			ESP_LOGI(TAG, "wifiTask SYSTEM_EVENT_AP_PROBEREQRECVED, rssi:%d, mac:" MACSTR, \
					   ap_probereqrecved->rssi, \
					   MAC2STR(ap_probereqrecved->mac));
			break;
		}	
		default:{
			ESP_LOGI(TAG, "wifiTask defaul event %d\r\n",event.event_id);
			break;
		}
		}
	}
}

void wifiTask(void *pvParameters)
{
    for(;;) {
		apStartLoop();
		eventLoop();
		vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    //wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
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
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
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

void ApsWifiStopStart1(int flag)
{
	if(0 == flag){
		ESP_ERROR_CHECK(esp_wifi_stop());
	}else{
		ESP_ERROR_CHECK(esp_wifi_start());
	}
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
    //ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "esp_wifi_set_config err ret %s\r\n",esp_err_to_name(ret));
	}
	ret = esp_wifi_connect();
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "esp_wifi_connect err ret %s\r\n",esp_err_to_name(ret));
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

int ApsWifiAutoScan(void)
{
	ST_WIFI_CONFIG stWifiConfig;
	int ret;

	if((sApList.count == 0)||(sApList.count==sApList.index)){
		ApsWifiConfigRead(&stWifiConfig);
		wifi_scan_config_t scanConf	= {.ssid = (uint8_t *)&stWifiConfig.ssid,.bssid = NULL,.channel = 0,.show_hidden = 1};

		ret = esp_wifi_scan_start(&scanConf, 1);
		if(ESP_OK != ret){
			ESP_LOGI(TAG,"%s\r\n",esp_err_to_name(ret));
		}else{
			ESP_LOGI(TAG,"esp_wifi_scan_start succ");
		}
	}else{
		ApsWifiConnectBssid(sApList.ap[sApList.index++].bssid);
	}
	return 0;
}
int ApsWifiAutoScanDone(void)
{
    unsigned short count=0;
	int ret;

	ret = esp_wifi_scan_get_ap_num(&count);
	if(OPP_SUCCESS != ret)
		DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_ERROR, "%s\r\n",esp_err_to_name(ret));
	
	if(count>AP_LIST_MAX) count = AP_LIST_MAX;
	sApList.count = count;
	sApList.index = 0;
	WIFI_ACTION_CHECK(esp_wifi_scan_get_ap_records(&count, sApList.ap),"get ap recores err");
	printApList(count, sApList.ap);
	if(count == 0) 
	{
		DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "Nothing AP found\r\n");
		esp_wifi_connect();
		return 1;
	}	
	ApsWifiConnectBssid(sApList.ap[sApList.index++].bssid);
	return 0;
}
int ApsWifiAutoConnect(void)
{
	ST_WIFI_CONFIG stWifiConfig;
    unsigned short count=0;
	int ret;
	
	ApsWifiConfigRead(&stWifiConfig);
	wifi_scan_config_t scanConf	= {.ssid = (uint8_t *)&stWifiConfig.ssid,.bssid = NULL,.channel = 0,.show_hidden = 1};
	
	if((sApList.count == 0)||(sApList.count==sApList.index))
	{
		//WIFI_ACTION_CHECK(esp_wifi_scan_start(&scanConf, 1),"scan err");//扫描所有可用的AP
		ret = esp_wifi_scan_start(&scanConf, 1);
		if(OPP_SUCCESS != ret)
			DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_ERROR, "%s\r\n",esp_err_to_name(ret));
		//Get number of APs found in last scan
		ret = esp_wifi_scan_get_ap_num(&count);
		if(OPP_SUCCESS != ret)
			DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_ERROR, "%s\r\n",esp_err_to_name(ret));
		
		if(count>AP_LIST_MAX) count = AP_LIST_MAX;
		sApList.count = count;
		sApList.index = 0;
		WIFI_ACTION_CHECK(esp_wifi_scan_get_ap_records(&count, sApList.ap),"get ap recores err");
		printApList(count, sApList.ap);
		if(count == 0) 
		{
			DEBUG_LOG(DEBUG_MODULE_WIFI, DLL_INFO, "Nothing AP found\r\n");
		    esp_wifi_connect();
			return 1;
		}
	}

	ApsWifiConnectBssid(sApList.ap[sApList.index++].bssid);
	
	return 0;
}

int ApsWifiStaConfig(char * ssid, char * pass)
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
	/*
	memset(&wifi_config, 0, sizeof(wifi_config_t));
	strcpy((char *)wifi_config.sta.ssid, ssid);
	strcpy((char *)wifi_config.sta.password, pass);

    	ApsWifiStopStart(&wifi_config);
	*/
	
    ApsWifiStopStart1(0);
	wifi_init_sta();
	return 0;
}

int ApsWifiApConfig(char * ssid, char * pass)
{
    wifi_config_t wifi_config = {
        .ap = {
            /*.ssid = EXAMPLE_ESP_WIFI_SSID,*/
            /*.ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),*/
            /*.password = EXAMPLE_ESP_WIFI_PASS,*/
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };	
    ApsWifiStopStart1(0);
	BC28_STRNCPY((char *)wifi_config.ap.ssid,ssid,SSID_MAX_LEN);
	wifi_config.ap.ssid_len = strlen(ssid);
	BC28_STRNCPY((char *)wifi_config.ap.password,pass,PASS_MAX_LEN);
	if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

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
	MUTEX_CREATE(g_stEventQMutex);
	ret = ApsWifiConfigReadFromFlash(&stWifiConfig);
	if(OPP_SUCCESS == ret){
		ApsWifiConfigWrite(&stWifiConfig);
	}else{
		ApsWifiConfigWrite(&g_stWifiConfigDefault);
	}
	initQueue(&queue_event,queue_event_buf,sizeof(system_event_t)*EVENT_QUEUE_MAX,EVENT_QUEUE_MAX,sizeof(system_event_t));
    xTaskCreate(&wifiTask, "wifiTask", 8*1024, NULL, 11, NULL);
	initialise_wifi();
	//wifi_init_softap();
	wifi_init_sta();
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


