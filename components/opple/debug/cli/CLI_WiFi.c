#include "Includes.h"
#include "cli-interpreter.h"
#include "esp_wifi.h"
#include "string.h"
#include "tcpip_adapter.h"
#include "SVS_WiFi.h"
#include "Opp_ErrorCode.h"

//extern LIST_HEAD(listr,_list_ap_entry) list_ap;
extern OS_EVENTGROUP_T g_eventWifi;

void CommandWiFiInfo(void);
void CommandApInfo(void);
void CommandWiFiConfig(void);
void CommandWiFiConfigGet(void);
void CommandWiFiConfigGetFlash(void);
void CommandWiFiOnOff(void);
void CommandWiFiScan(void);
void CommandWiFiApConfig(void);


const char* const nbArguments_wifiConfig[] = {
  "ssid:[32]",
  "pass:[64]"
};
const char* const nbArguments_wifiScan[] = {
  "null str:\"\"",
};
const char* const nbArguments_wifiBssid[] = {
  "xx:xx:xx:xx:xx:xx",
};

extern unsigned char g_ucWifiState;

char *wifiStateDesc[] = {
    "SYSTEM_EVENT_WIFI_READY",
    "SYSTEM_EVENT_SCAN_DONE",
    "SYSTEM_EVENT_STA_START",
    "SYSTEM_EVENT_STA_STOP",
    "SYSTEM_EVENT_STA_CONNECTED",
    "SYSTEM_EVENT_STA_DISCONNECTED",
    "SYSTEM_EVENT_STA_AUTHMODE_CHANGE",
    "SYSTEM_EVENT_STA_GOT_IP",
    "SYSTEM_EVENT_STA_LOST_IP",
    "SYSTEM_EVENT_STA_WPS_ER_SUCCESS",
    "SYSTEM_EVENT_STA_WPS_ER_FAILED",
    "SYSTEM_EVENT_STA_WPS_ER_TIMEOUT",
    "SYSTEM_EVENT_STA_WPS_ER_PIN",
    "SYSTEM_EVENT_AP_START",
    "SYSTEM_EVENT_AP_STOP",
    "SYSTEM_EVENT_AP_STACONNECTED",
    "SYSTEM_EVENT_AP_STADISCONNECTED",
    "SYSTEM_EVENT_AP_PROBEREQRECVED",
    "SYSTEM_EVENT_GOT_IP6",
    "SYSTEM_EVENT_ETH_START",
    "SYSTEM_EVENT_ETH_STOP",
    "SYSTEM_EVENT_ETH_CONNECTED",
    "SYSTEM_EVENT_ETH_DISCONNECTED",
    "SYSTEM_EVENT_ETH_GOT_IP",
    "SYSTEM_EVENT_MAX"
};

CommandEntry CommandTableWiFi[] =
{
	CommandEntryActionWithDetails("info", CommandWiFiInfo, "", "", NULL),
	CommandEntryActionWithDetails("apInfo", CommandApInfo, "", "", NULL),
	CommandEntryActionWithDetails("configSet", CommandWiFiConfig, "ss", "", nbArguments_wifiConfig),
	CommandEntryActionWithDetails("configGet", CommandWiFiConfigGet, "", "", NULL),
	CommandEntryActionWithDetails("configFlashGet", CommandWiFiConfigGetFlash, "", "", NULL),
	CommandEntryActionWithDetails("onOff", CommandWiFiOnOff, "u", "", NULL),
	CommandEntryActionWithDetails("scan", CommandWiFiScan, "s", "wifi scan", nbArguments_wifiScan),
	CommandEntryActionWithDetails("apConfigSet", CommandWiFiApConfig, "ss", "", nbArguments_wifiConfig),
	CommandEntryTerminator()
};

char *desc[] ={
    "WIFI_MODE_NULL",
    "WIFI_MODE_STA",
    "WIFI_MODE_AP",
    "WIFI_MODE_APSTA",
    "WIFI_MODE_MAX"
};
char *htDesc[] = {
	"BW_HT20",
	"BW_HT40"
};

char *policyDesc[] = {
	"WIFI_COUNTRY_POLICY_AUTO",
    "WIFI_COUNTRY_POLICY_MANUAL",
};

char *chDesc[] = {
	"the channel width is HT20",
	"the channel width is HT40 and the second channel is above the primary channel",
    "the channel width is HT40 and the second channel is below the primary channel"
};

char *authDesc[] = {
	"open",
	"WEP",
	"WPA_PSK",
	"WPA2_PSK",
	"WPA_WPA2_PSK",
	"WPA2_ENTERPRISE"
};

char *cipherDesc[] = {
	"none",
	"WEP40",
	"WEP104",
	"TKIP",
	"CCMP",
	"TKIP and CCMP"
};

#define WIFI_PROTOCOL_11B         1
#define WIFI_PROTOCOL_11G         2
#define WIFI_PROTOCOL_11N         4
#define WIFI_PROTOCOL_LR          8
#define WIFI_PROTOCOL_WPS         16

void CommandWiFiInfo(void)
{
    char info[512];
	unsigned char mac[6];
    unsigned char aucon_en;
	wifi_mode_t mode = 0;
	wifi_config_t conf;
    int len = 0;
	float power;
	uint8_t protocol_bitmap;
	wifi_bandwidth_t bw;
	unsigned char primary;
	wifi_second_chan_t second;
	wifi_country_t country;
	tcpip_adapter_ip_info_t local_ip;
	char *hostname[0];
	int ret;

	CLI_PRINTF("**********sta config info*************\r\n");
	tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA,&hostname[0]);
    len += snprintf(info,512,"hostname:%s\r\n",hostname[0]);
    ret = esp_wifi_get_auto_connect(&aucon_en);
	if(OPP_SUCCESS == ret)
		len += snprintf(info+len,512,"auto connect en:%u\r\n",aucon_en);
	ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
	if(OPP_SUCCESS == ret)
    	len += snprintf(info+len,512,"mac:%02X:%02X:%02X:%02X:%02X:%02X\r\n",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);
	len += snprintf(info+len,512,"sta:(ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR ")\r\n",
		   IP2STR(&local_ip.ip), IP2STR(&local_ip.netmask), IP2STR(&local_ip.gw));
	/*ret = esp_wifi_get_channel(&primary,&second);
	if(OPP_SUCCESS == ret)
		len += snprintf(info+len,512,"primary ch:%d, second ch:%d\r\n",primary,chDesc[second]);*/
	ret = esp_wifi_get_mode(&mode);
	if(OPP_SUCCESS == ret)
		len += snprintf(info+len,512,"mode:%s\r\n",desc[mode]);
	ret = esp_wifi_get_config(WIFI_IF_STA, &conf);
	if(OPP_SUCCESS == ret)
		len += snprintf(info+len,512,"ssid:%s, passwd:%s,scan_method:%d,bssid_set:%d,"MACSTR",listen_interval:%d,sort_method:%d,threshold.rssi:%d\r\n",
		conf.sta.ssid,conf.sta.password,conf.sta.scan_method,conf.sta.bssid_set,MAC2STR(conf.sta.bssid),conf.sta.listen_interval,conf.sta.sort_method,conf.sta.threshold.rssi);
	//esp_wifi_get_max_tx_power(&power);
	ret = ApsGetWifiPower(&power);
	if(OPP_SUCCESS == ret)
    	len += snprintf(info+len,512,"power:%f dbm\r\n",power);
	esp_wifi_get_protocol(WIFI_IF_STA,&protocol_bitmap);
	if(protocol_bitmap|WIFI_PROTOCOL_11B)
    	len += snprintf(info+len,512,"protocol:%s\r\n","11b");
	if(protocol_bitmap|WIFI_PROTOCOL_11G)
		len += snprintf(info+len,512,"protocol:%s\r\n","11g");
	if(protocol_bitmap|WIFI_PROTOCOL_11N)
		len += snprintf(info+len,512,"protocol:%s\r\n","11n");
	if(protocol_bitmap|WIFI_PROTOCOL_LR)
		len += snprintf(info+len,512,"protocol:%s\r\n","lr");
	esp_wifi_get_bandwidth(WIFI_IF_STA,&bw);
    len += snprintf(info+len,512,"HT:%s\r\n",htDesc[bw-1]);
	ret = esp_wifi_get_country(&country);
	if(OPP_SUCCESS == ret){
		country.cc[2] = '\0';
    	len += snprintf(info+len,512,"country code:%s start ch:%d total ch num:%d policy:%s\r\n",country.cc, country.schan, country.nchan,policyDesc[country.policy]);
		len += snprintf(info+len,512,"wifi state:%s\r\n", wifiStateDesc[g_ucWifiState]);
	}
	CLI_PRINTF("%s",info);

	/*softap info*/
	if(mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA){
		CLI_PRINTF("**********ap config info*************\r\n");
		len = 0;
		memset(info,0,sizeof(info));
		tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_AP,&hostname[0]);
		len += snprintf(info,512,"hostname:%s\r\n",hostname[0]);
		ret = esp_wifi_get_mac(WIFI_IF_AP, mac);
		if(OPP_SUCCESS == ret)
			len += snprintf(info+len,512,"mac:%02X:%02X:%02X:%02X:%02X:%02X\r\n",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &local_ip);
		len += snprintf(info+len,512,"ap:(ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR ")\r\n",
			   IP2STR(&local_ip.ip), IP2STR(&local_ip.netmask), IP2STR(&local_ip.gw));
		ret = esp_wifi_get_config(WIFI_IF_AP, &conf);
		if(OPP_SUCCESS == ret)
			len += snprintf(info+len,512,"ssid:%s, passwd:%s channel:%d, authmode:%d, hidden:%d, max_con:%d, beacon_interval:%d\r\n",
			conf.ap.ssid,conf.ap.password,conf.ap.channel,conf.ap.authmode,
			conf.ap.ssid_hidden,conf.ap.max_connection,conf.ap.beacon_interval);		
		esp_wifi_get_protocol(WIFI_IF_AP,&protocol_bitmap);
		if(protocol_bitmap|WIFI_PROTOCOL_11B)
			len += snprintf(info+len,512,"protocol:%s\r\n","11b");
		if(protocol_bitmap|WIFI_PROTOCOL_11G)
			len += snprintf(info+len,512,"protocol:%s\r\n","11g");
		if(protocol_bitmap|WIFI_PROTOCOL_11N)
			len += snprintf(info+len,512,"protocol:%s\r\n","11n");
		if(protocol_bitmap|WIFI_PROTOCOL_LR)
			len += snprintf(info+len,512,"protocol:%s\r\n","lr");
		esp_wifi_get_bandwidth(WIFI_IF_AP,&bw);
		len += snprintf(info+len,512,"HT:%s\r\n",htDesc[bw-1]);
		CLI_PRINTF("%s",info);
	}
}

void CommandApInfo(void)
{
    char info[512];
	int ret;
	wifi_ap_record_t ap_info;
	int len = 0;
	
	ret = esp_wifi_sta_get_ap_info(&ap_info);
	if(0 != ret){
		CLI_PRINTF("esp_wifi_sta_get_ap_info ret %d",ret);
		return;
	}
    len += snprintf(info+len,512,"mac:%02X:%02X:%02X:%02X:%02X:%02X\r\n",ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2], ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
    len += snprintf(info+len,512,"ssid:%s\r\n",ap_info.ssid);
    len += snprintf(info+len,512,"primary channel:%d, second channel:%s\r\n",ap_info.primary, chDesc[ap_info.second]);
    len += snprintf(info+len,512,"rssi:%d dbm\r\n",ap_info.rssi);
    len += snprintf(info+len,512,"auth mode:%s\r\n",authDesc[ap_info.authmode]);
    len += snprintf(info+len,512,"pairwise cipher mode:%s\r\n",cipherDesc[ap_info.pairwise_cipher]);
    len += snprintf(info+len,512,"group cipher cipher mode:%s\r\n",cipherDesc[ap_info.group_cipher]);
	if(ap_info.phy_11b|WIFI_PROTOCOL_11B)
    	len += snprintf(info+len,512,"protocol:%s\r\n","11b");
	if(ap_info.phy_11g|WIFI_PROTOCOL_11G)
		len += snprintf(info+len,512,"protocol:%s\r\n","11g");
	if(ap_info.phy_11n|WIFI_PROTOCOL_11N)
		len += snprintf(info+len,512,"protocol:%s\r\n","11n");
	if(ap_info.phy_lr|WIFI_PROTOCOL_LR)
		len += snprintf(info+len,512,"protocol:%s\r\n","lr");
	if(ap_info.wps|WIFI_PROTOCOL_WPS)
		len += snprintf(info+len,512,"support wps\r\n");
	
    CLI_PRINTF("%s",info);
	return;
}
void CommandWiFiConfig(void)
{
	ApsWifiStaConfig((char *)&CliIptArgList[0][0],(char *)&CliIptArgList[1][0]);

}

void CommandWiFiConfigGet(void)
{
	ST_WIFI_CONFIG stWifiConfig;
	
	ApsWifiConfigRead(&stWifiConfig);

	CLI_PRINTF("ssid:%s, passwd:%s\r\n", stWifiConfig.ssid, stWifiConfig.password);
}

void CommandWiFiApConfig(void)
{
	ApsWifiApConfig((char *)&CliIptArgList[0][0],(char *)&CliIptArgList[1][0]);
}

void CommandWiFiConfigGetFlash(void)
{
	int ret;
	ST_WIFI_CONFIG stWifiConfig;
	
	ret = ApsWifiConfigReadFromFlash(&stWifiConfig);
	if(0 != ret){
		CLI_PRINTF("ApsWifiConfigReadFromFlash fail %d\r\n", ret);
		return;
	}
	CLI_PRINTF("ssid:%s, passwd:%s\r\n", stWifiConfig.ssid, stWifiConfig.password);
}

void CommandWiFiOnOff(void)
{
	if(0 == CliIptArgList[0][0])
		esp_wifi_stop();
	else
		esp_wifi_start();
}

unsigned char g_ucOnlyScan = 0;
void CommandWiFiScan(void)
{
	int i = 0;
	uint16_t apCount;
	wifi_ap_record_t *ap_records;
	wifi_scan_config_t scanConf	= {.ssid = NULL,.bssid = NULL,.channel = 0,.show_hidden = 1};
	ST_WIFI_CONFIG stWifiConfig;
	int ret;
	
	if(strlen((char *)&CliIptArgList[0][0]))
		scanConf.ssid = (uint8_t *)&CliIptArgList[0][0];
	if(scanConf.ssid)
		CLI_PRINTF("scanConf.ssid:%s\n\n",(char *)scanConf.ssid);
	//ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, 1));//扫描所有可用的AP。;
	//before scan, must disconnect
	g_ucOnlyScan = 1;
	esp_wifi_disconnect();
	ret = esp_wifi_scan_start(&scanConf, 1);
	if(OPP_SUCCESS != ret)
		CLI_PRINTF("%s\r\n",esp_err_to_name(ret));
	
	OS_EVENTGROUP_WAITBIT(g_eventWifi,2,30000);
	printf("scan done");
	OS_EVENTGROUP_CLEARBIT(g_eventWifi,2);
	g_ucOnlyScan = 0;

	ret = esp_wifi_scan_get_ap_num(&apCount);//Get number of APs found in last scan
	if(OPP_SUCCESS != ret)
		CLI_PRINTF("%s\r\n",esp_err_to_name(ret));
	if (apCount == 0) {
		CLI_PRINTF("Nothing AP found\r\n");
		return;
	}	
	wifi_ap_record_t *list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
	//获取上次扫描中找到的AP列表
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, list));
	CLI_PRINTF("===============================================================================\r\n");
	CLI_PRINTF("            SSID        |    RSSI    |        AUTH        |        MAC         \r\n");
	CLI_PRINTF("===============================================================================\r\n");
	for (i=0; i<apCount; i++) {
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
		CLI_PRINTF("%-20.20s    |%-4d	|-%-22.22s|%02x:%02x:%02x:%02x:%02x:%02x\r\n",list[i].ssid, list[i].rssi,authmode,
			list[i].bssid[0],list[i].bssid[1],list[i].bssid[2],list[i].bssid[3],list[i].bssid[4],list[i].bssid[5]);
	}
	free(list);
	CLI_PRINTF("\r\n\r\n");
	//scan over must reconnect
	esp_wifi_connect();
}



