/**
******************************************************************************
* @file    SVS_SafeMode.c
* @author  Liqi
* @version V1.0.0
* @date    2018-9-2
* @brief   
******************************************************************************

******************************************************************************
*/ 
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include "Includes.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "OPP_Debug.h"
#include "SVS_WiFi.h"
#include "SVS_Ota.h"
#include "SVS_Coap.h"
#include "SVS_Para.h"
#include "SVS_Aus.h"
#include "SVS_MsgMmt.h"
#include "SVS_Coap.h"
#include "APP_Communication.h"
#include "DEV_Utility.h"
#include "string.h"

#define WIFI_SSID       "chanxianchecknb"
#define WIFI_PASSWD     "12345678"
#define OTA_SERVER_IP   "192.168.10.222"
#define OTA_SERVER_PORT "80"
#define OTA_FILENAME    "/opple.bin"
#define CONNECTED_BIT   1
#define SAFEMODE_WIFI_CONNECT_TIME (6*60*60*1000)
#define SAFEMODE_RUN_TIME          (6*60*60*1000)
#define SAFEMODE_IP   2  // 174/2
#define SAFEMODE_PORT 59877

#define OTA_IP_LEN      18
#define OTA_PORT_LEN    6
#define OTA_FILE_LEN    64

#define DAFEMODE_OTA_STA_IDLE 0
#define DAFEMODE_OTA_STA_START 1
#define DAFEMODE_OTA_STA_ING 2
#define DAFEMODE_OTA_STA_END 3

static char sys_ota_svr_ip[OTA_IP_LEN]={'\0'};
static char sys_ota_svr_port[OTA_PORT_LEN]={'\0'};
static char sys_ota_file[OTA_FILE_LEN]={'\0'};
static unsigned int  sys_ota_start=0;  /* 换成信号量 */
static OS_EVENTGROUP_T wifi_event_group;
static int  wifiConnected=0;
static T_MUTEX mutex_sys;

#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024

static const char *TAG = "ota";
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };
/*an packet receive buffer*/
static char text[BUFFSIZE + 1] = { 0 };
/* an image total length*/
static int binary_file_length = 0;
/*socket id*/
static int socket_id = -1;


/*socket id*/
static int udpSocket = -1;
struct sockaddr_in udpAddr;

static int sysOtaStatus = 0;
static int sysErrorCode = 0;


int sysInfoSet(int otaStatus,int errcode)
{
	if(mutex_sys==NULL) return 1;

	MUTEX_LOCK(mutex_sys, MUTEX_WAIT_ALWAYS);
	if(otaStatus>=0) sysOtaStatus = otaStatus;
	if(errcode>=0) sysErrorCode = errcode;
	MUTEX_UNLOCK(mutex_sys);
	return 0;
}
int sysInfoGet(int *otaStatus,int *errcode)
{
	if(mutex_sys==NULL) return 1;

	MUTEX_LOCK(mutex_sys, MUTEX_WAIT_ALWAYS);
	*otaStatus = sysOtaStatus;
	*errcode = sysErrorCode;
	MUTEX_UNLOCK(mutex_sys);
	return 0;
}


/**/
int setOtaConfig(char* ip,char* port,char* filename)
{
	if(ip==NULL || port==NULL || filename==NULL) return 1;
	if(strlen(ip)>OTA_IP_LEN-1) return 2;
	if(strlen(port)>OTA_PORT_LEN-1) return 3;
	if(strlen(filename)>OTA_FILE_LEN-1) return 4;
	strcpy(sys_ota_svr_ip,ip);
	strcpy(sys_ota_svr_port,port);
	strcpy(sys_ota_file,filename);
	return 0;
}

int getOtaConfig(char* ip,int ipLenIn,char* port,int portLenIn,char* filename,int fileLenIn)
{
	if(ip==NULL || port==NULL || filename==NULL) return 1;
	if(ipLenIn<strlen(sys_ota_svr_ip)+1) return 2;
	if(portLenIn<strlen(sys_ota_svr_port)+1) return 3;
	if(fileLenIn<strlen(sys_ota_file)+1) return 4;
	strcpy(ip,sys_ota_svr_ip);
	strcpy(port,sys_ota_svr_port);
	strcpy(filename,sys_ota_file);
	return 0;
}

void sysOtaStart(void)
{
    sys_ota_start = 0xAA55AA55;
}

/**/


/*read buffer by byte still delim ,return read bytes counts*/
static int read_until(char *buffer, char delim, int len)
{
//  /*TODO: delim check,buffer check,further: do an buffer length limited*/
    int i = 0;
    while (buffer[i] != delim && i < len) {
        ++i;
    }
    return i + 1;
}

/* resolve a packet from http socket
 * return true if packet including \r\n\r\n that means http packet header finished,start to receive packet body
 * otherwise return false
 * */
static bool read_past_http_header(char text[], int total_len, esp_ota_handle_t update_handle)
{
    /* i means current position */
    int i = 0, i_read_len = 0;
    while (text[i] != 0 && i < total_len) {
        i_read_len = read_until(&text[i], '\n', total_len);
        // if we resolve \r\n line,we think packet header is finished
        if (i_read_len == 2) {
            int i_write_len = total_len - (i + 2);
            memset(ota_write_data, 0, BUFFSIZE);
            /*copy first http packet body to write buffer*/
            memcpy(ota_write_data, &(text[i + 2]), i_write_len);

            esp_err_t err = esp_ota_write( update_handle, (const void *)ota_write_data, i_write_len);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error: esp_ota_write failed (%s)!", esp_err_to_name(err));
                return false;
            } else {
                ESP_LOGI(TAG, "esp_ota_write header OK");
                binary_file_length += i_write_len;
            }

            return true;
        }
        i += i_read_len;
    }
    return false;
}

static bool connect_to_http_server()
{
    ESP_LOGI(TAG, "Server IP: %s Server Port:%s", sys_ota_svr_ip, sys_ota_svr_port);

    int  http_connect_flag = -1;
    struct sockaddr_in sock_info;

    socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id == -1) {
        ESP_LOGE(TAG, "Create socket failed!");
        return false;
    }

    // set connect info
    memset(&sock_info, 0, sizeof(struct sockaddr_in));
    sock_info.sin_family = AF_INET;
    sock_info.sin_addr.s_addr = inet_addr(sys_ota_svr_ip);
    sock_info.sin_port = htons(atoi(sys_ota_svr_port));

    // connect to http server
    http_connect_flag = connect(socket_id, (struct sockaddr *)&sock_info, sizeof(sock_info));
    if (http_connect_flag == -1) {
        ESP_LOGE(TAG, "Connect to server failed! errno=%d", errno);
        close(socket_id);
        return false;
    } else {
        ESP_LOGI(TAG, "Connected to server");
        return true;
    }
    return false;
}

//static void __attribute__((noreturn)) task_fatal_error()
//	static void task_fatal_errorx()
//	{
//	    ESP_LOGE(TAG, "Exiting task due to fatal error...");
//	    close(socket_id);
//	    //(void)vTaskDelete(NULL);
//	    
//	    while (1) {
//	        ;
//	    }
//	}
#define task_fatal_error() \
{\
    close(socket_id);\
    sys_ota_start = 0;\
    sysInfoSet(0,-1);\
    goto OTA_START;\
}

void safe_mode_ota(void *pvParameter)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    esp_partition_t *configured;
    esp_partition_t *running;

    strcpy(sys_ota_svr_ip, OTA_SERVER_IP);
    strcpy(sys_ota_svr_port, OTA_SERVER_PORT);
    strcpy(sys_ota_file, OTA_FILENAME);
    
    while(1)
    {
OTA_START:
        //DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Ota wating...\r\n");
		//sysInfoSet(0,0);
        // sempWait(g_aucOtaStart)
        while(0xAA55AA55 != sys_ota_start){
           vTaskDelay(1000 / portTICK_PERIOD_MS); 
        }
        sysInfoSet(1,0);
        //DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Start ota!\r\n");
        if(0 == strlen(sys_ota_svr_ip))
    		strcpy(sys_ota_svr_ip, OTA_SERVER_IP);
      	if(0 == strlen(sys_ota_svr_port))
    		strcpy(sys_ota_svr_port, OTA_SERVER_PORT);
    	  if(0 == strlen(sys_ota_file))
    		strcpy(sys_ota_file, OTA_FILENAME);
        ESP_LOGI(TAG,"ip:%s,port:%s,file:%s\r\n",sys_ota_svr_ip,sys_ota_svr_port,sys_ota_file);
    
        ESP_LOGI(TAG, "Starting OTA example...");
        
        //DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Connect wifi...\r\n");
        //xEventGroupWaitBits(wifi_event_group, 1,false, true, portMAX_DELAY);
        OS_EVENTGROUP_WAITBIT(wifi_event_group,CONNECTED_BIT,OS_EVENTGROUP_WAIT_ALWAYS);
        ESP_LOGI(TAG, "Connect to Wifi ! Start to OTA....");
    
        configured = esp_ota_get_boot_partition();
        running = esp_ota_get_running_partition();
        
    
        if (configured != running) {
            ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                     configured->address, running->address);
            ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
        }
        ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
                 running->type, running->subtype, running->address);
    
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
        
        ESP_LOGI(TAG, "Connect to Wifi ! Start to Connect to OTA Server....");
        
        //DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Connect http server...\r\n");
        /*connect to http server*/
        if (connect_to_http_server()) {
            ESP_LOGI(TAG, "Connected to http server");
        } else {
            ESP_LOGE(TAG, "Connect to http server failed!");
            sysInfoSet(-1,1);
            task_fatal_error();
        }
    
        /*send GET request to http server*/
        const char *GET_FORMAT =
            "GET %s HTTP/1.0\r\n"
            "Host: %s:%s\r\n"
            "User-Agent: esp-idf/1.0 esp32\r\n\r\n";
    
        char *http_request = NULL;
        //int get_len = asprintf(&http_request, GET_FORMAT, OTA_FILENAME, OTA_SERVER_IP, OTA_SERVER_PORT);
        
        int get_len = asprintf(&http_request, GET_FORMAT, sys_ota_file, sys_ota_svr_ip, sys_ota_svr_port);
        if (get_len < 0) {
            ESP_LOGE(TAG, "Failed to allocate memory for GET request buffer");
            sysInfoSet(-1,2);
            task_fatal_error();
        }
        int res = send(socket_id, http_request, get_len, 0);
        free(http_request);
    
        if (res < 0) {
            ESP_LOGE(TAG, "Send GET request to server failed");
            sysInfoSet(-1,3);
            task_fatal_error();
        } else {
            ESP_LOGI(TAG, "Send GET request to server succeeded");
        }
    
        update_partition = esp_ota_get_next_update_partition(NULL);
        ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
                 update_partition->subtype, update_partition->address);
        assert(update_partition != NULL);
    
        err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
            sysInfoSet(-1,4);
            task_fatal_error();
        }
       
        ESP_LOGI(TAG, "esp_ota_begin succeeded");
        //DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Start ota...\r\n");
    
        bool resp_body_start = false, flag = true;
        /*deal with all receive packet*/
        while (flag) {
            sysInfoSet(2,-1);
            memset(text, 0, TEXT_BUFFSIZE);
            memset(ota_write_data, 0, BUFFSIZE);
            //printf("Start recv...\r\n");
            int buff_len = recv(socket_id, text, TEXT_BUFFSIZE, 0);
            //printf("Recved...\r\n");
    		//printf("%s\r\n",text);
            if (buff_len < 0) { /*receive error*/
                ESP_LOGE(TAG, "Error: receive data error! errno=%d", errno);
                task_fatal_error();
            } else if (buff_len > 0 && !resp_body_start) { /*deal with response header*/
                memcpy(ota_write_data, text, buff_len);
                resp_body_start = read_past_http_header(text, buff_len, update_handle);
                //printf("resp_body_start:%s\r\n",resp_body_start==true?"TRUE":"FALSE");
            } else if (buff_len > 0 && resp_body_start) { /*deal with response body*/
            	//printf("deal with response body\r\n");
                memcpy(ota_write_data, text, buff_len);
    
    			err = esp_ota_write( update_handle, (const void *)ota_write_data, buff_len);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Error: esp_ota_write failed (%s)!", esp_err_to_name(err));
                    sysInfoSet(-1,5);
                    task_fatal_error();
                }
                
                binary_file_length += buff_len;
                //ESP_LOGI(TAG, "Have written image length %d", binary_file_length);
                //DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Have written image length %d\r\n", binary_file_length);
                
            } else if (buff_len == 0) {  /*packet over*/
                flag = false;
                ESP_LOGI(TAG, "Connection closed, all packets received");
                close(socket_id);
            } else {
                ESP_LOGE(TAG, "Unexpected recv result");
            }
        }
        
        if (esp_ota_end(update_handle) != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_end failed!");
            //DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"esp_ota_end failed!\r\n");
            sysInfoSet(-1,6);
            task_fatal_error();
        }
        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
            //DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"esp_ota_set_boot_partition failed!\r\n");
            sysInfoSet(-1,7);
            task_fatal_error();
        }
    	sys_ota_start = 0;
        ESP_LOGI(TAG, "Prepare to restart system!");
        sysInfoSet(3,-1);
		
        //vTaskDelay(3000 / portTICK_PERIOD_MS);
        //esp_restart();
    }
    return ;
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        wifiConnected = 1;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        wifiConnected = 0;
        break;
    default:
        break;
    }
    return ESP_OK;
}

void safe_mode_initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    MUTEX_CREATE(mutex_sys);
    sysInfoSet(0,0);
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWD,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}


void safe_mode_main()
{
	static OS_TICK_T tick,sTick=0;
	static int workingMode=0;
	
	while(1)
	{
		if(workingMode == 0)
		{
			if(wifiConnected!=0)
			{
				sTick = OS_GET_TICK();
				workingMode = 1;    // Enter normal mode
			}
			else
			{
				tick = OS_GET_TICK();
				if(OS_TICK_DIFF(sTick, tick) >= SAFEMODE_WIFI_CONNECT_TIME)
				{
					esp_restart();  // Not be connected for a long time
				}
			}
		}
		else
		{
			if(wifiConnected==0)
			{
				workingMode = 0;
				sTick = OS_GET_TICK();  // Disconnected
			}
			else
			{
				tick = OS_GET_TICK();
				if(OS_TICK_DIFF(sTick, tick) >= SAFEMODE_RUN_TIME)
				{
					esp_restart();      // Be in safe mode for a long time
				}
			}
		}
		OS_DELAY_MS(1000);
	}
}

//void safe_mode_ota(void *pvParameter)
//{
//    
//}

// return:0:fail,else:bytes sent successfully
int safemode_udp_send(char * data,unsigned short len)
{
	tcpip_adapter_ip_info_t local_ip;
	ip4_addr_t ip;
	int res;

	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);
	memcpy(&ip,&local_ip.ip,sizeof(ip4_addr_t));
	ip.addr	&= 0x00FFFFFF;
	ip.addr += SAFEMODE_IP<<24;

	printf("%s\r\n",data);

	udpAddr.sin_family = AF_INET;
	udpAddr.sin_addr.s_addr = ip.addr;	
	//inet_aton("192.168.3.174", &udpAddr.sin_addr.s_addr);
	udpAddr.sin_port = htons(SAFEMODE_PORT);	
	res = sendto(udpSocket, data, len, 0, (struct sockaddr*)&udpAddr,sizeof(struct sockaddr_in));
	printf("UDP SEND RES:%d\r\n",res);
	return res;
}

void safe_mode_udp()
{
	int buff_len;
	int ret;
	OS_TICK_T tick;
	unsigned int sec;
	char buf[1024];
	unsigned char mac[10];
	char s[100];
	ip4_addr_t ip;
	int status,err;
	static OS_TICK_T otaTick=0,reportTick=0;
	int lastOtastatus=0;
		
	udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
	
	if(udpSocket == -1)
	{
		printf("Create udp socket fail!\r\n");
	}
	
	while(1)
	{
		OS_EVENTGROUP_WAITBIT(wifi_event_group,CONNECTED_BIT,OS_EVENTGROUP_WAIT_ALWAYS);

		if(sysInfoGet(&status,&err)!=0)
		{
			printf("sysInfoGet fail!\r\n");
			status = 0;
			err = 0;
		}

		if(status == DAFEMODE_OTA_STA_IDLE) // idle
		{
			tick = OS_GET_TICK();
			if(OS_TICK_DIFF(otaTick, tick) >= 20*1000){
				otaTick = tick;
				ip.addr = udpAddr.sin_addr.s_addr;
				snprintf(s,99,IPSTR,IP2STR(&ip));
				setOtaConfig(s,"80","/opp_safe.bin");
				sysOtaStart();
				printf("[Safe mode] start ota!\r\n");
			}
		}
		
		if(status == DAFEMODE_OTA_STA_END) // ota completely
		{
			for(int i =0;i<6 ;i++)
			{
				tick = OS_GET_TICK();	
				OS_DELAY_MS(1000);
				ApsGetWifiMac(mac);
				ip.addr = udpAddr.sin_addr.s_addr;
				snprintf(s,100,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				snprintf(buf,1023,"[Report sysStatus]tick:%u,mac:%s,version:%04X-%04X-%d,otaStatus:%d,ip:"IPSTR",port:%d,bin:%s,err:%d\r\n", \
				tick,s,PRODUCTSKU,PRODUCTCLASS,OPP_LAMP_CTRL_CFG_DATA_VER,status,IP2STR(&ip),80,"/opp_safe.bin",err);

				//snprintf(buf,1023,"[Report sysStatus]tick:%u,mac:%s,version:%04X-%04X-%d\r\n", 
				//tick,s,PRODUCTSKU,PRODUCTCLASS,OPP_LAMP_CTRL_CFG_DATA_VER);

				printf("Report sysStaus\r\n");
				safemode_udp_send(buf,strlen(buf)+1);
			}
			esp_restart();
		}
		else // ota ing
		{
			tick = OS_GET_TICK();	
			if((OS_TICK_DIFF(reportTick, tick) >= 5000) || (lastOtastatus!=status))
			{
				reportTick =tick;
				lastOtastatus = status;
				
				ApsGetWifiMac(mac);
				ip.addr = udpAddr.sin_addr.s_addr;
				snprintf(s,100,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				snprintf(buf,1023,"[Report sysStatus]tick:%u,mac:%s,version:%04X-%04X-%d,otaStatus:%d,ip:"IPSTR",port:%d,bin:%s,err:%d\r\n", \
				tick,s,PRODUCTSKU,PRODUCTCLASS,OPP_LAMP_CTRL_CFG_DATA_VER,status,IP2STR(&ip),80,"/opp_safe.bin",err);

				//snprintf(buf,1023,"[Report sysStatus]tick:%u,mac:%s,version:%04X-%04X-%d\r\n", 
				//tick,s,PRODUCTSKU,PRODUCTCLASS,OPP_LAMP_CTRL_CFG_DATA_VER);

				printf("Report sysStaus\r\n");
				safemode_udp_send(buf,strlen(buf)+1);
			}
			OS_DELAY_MS(200);
		}
	}
}









