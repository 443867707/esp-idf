/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
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
#include "opple_main.h"
#include "SVS_Exep.h"
#include "SVS_Para.h"

#define OTA_SERVER_IP   "192.168.10.222"
#define OTA_SERVER_PORT "80"
#define OTA_FILENAME "/opple.bin"

#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024

char g_aucOtaSvrIp[IP_LEN]={'\0'};
char g_aucOtaSvrPort[PORT_LEN]={'\0'};
char g_aucOtaFile[FILE_LEN]={'\0'};
int g_aucOtaStart=0;  /* 鎹㈡垚淇″彿閲? */
int g_iOtaFileLen = 0;
extern OS_EVENTGROUP_T g_eventWifi;


static const char *TAG = "ota";
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };
/*an packet receive buffer*/
static char text[BUFFSIZE + 1] = { 0 };
/* an image total length*/
static int binary_file_length = 0;
/*socket id*/
static int socket_id = -1;

/*****************************************************************************
Version History Module
1. Original Version
2. Last[n] Version
*****************************************************************************/
#define OTA_PARA_INDEX_OV  0  // U32
#define OTA_PARA_INDEX_HV  1  // U32 Array
#define OTA_HV_NUM_MAX     10 // 

/*
* @param version the version to hold the origianl version
* @return 0:success
*/
int OriginalVersinGet(unsigned int* version)
{
	if(version == 0) return 1;
	return AppParaReadU32(APS_PARA_MODULE_APS_OTA,OTA_PARA_INDEX_OV,version);
}

/*
* @param version the version to set the origianl version
* @return 0:success
*/
int OriginalVersionSet(unsigned int version)
{
	return AppParaWriteU32(APS_PARA_MODULE_APS_OTA,OTA_PARA_INDEX_OV,version);
}

/*
* @param version the version to push
* @return 0:success
*/
int HistoryVersionPush(unsigned int version)
{
	unsigned int vh[OTA_HV_NUM_MAX];
	unsigned int len = OTA_HV_NUM_MAX*sizeof(unsigned int);
	int start = 0;
	
	memset(vh,0,sizeof(vh));
	if(AppParaRead(APS_PARA_MODULE_APS_OTA,OTA_PARA_INDEX_HV,(unsigned char*)vh,&len) != 0)
	{
		AppParaRead(APS_PARA_MODULE_APS_OTA,OTA_PARA_INDEX_HV,(unsigned char*)vh,&len);
	}
	if(vh[0] != 0)
	{
		start = 1;
	}
	for(int i=start ;i < OTA_HV_NUM_MAX-1;i++)
	{
		vh[i] = vh[i+1];
	}
	vh[OTA_HV_NUM_MAX-1] = version;
	return(AppParaWrite(APS_PARA_MODULE_APS_OTA,OTA_PARA_INDEX_HV,(unsigned char*)vh,len));
}

/*
* @param vh version history:
* [original version][first rcd] ... [the latest version]
* @param inOutLen the length of vh to hold the vh
* @return 0:success
*/
int HistoryVersionGet(unsigned int *vh,unsigned int* inOutLen)
{
	unsigned int len = OTA_HV_NUM_MAX*sizeof(unsigned int);
	
	if(*inOutLen < OTA_HV_NUM_MAX+1) return 1;
	if(vh == 0) return 2;

	memset(vh,0,(OTA_HV_NUM_MAX+1)*sizeof(unsigned int));
	if(OriginalVersinGet(&vh[0]) != 0){ 
		OriginalVersinGet(&vh[0]);
	}
	if(AppParaRead(APS_PARA_MODULE_APS_OTA,OTA_PARA_INDEX_HV,(unsigned char*)(&vh[1]),&len) != 0)
	{
		AppParaRead(APS_PARA_MODULE_APS_OTA,OTA_PARA_INDEX_HV,(unsigned char*)(&vh[1]),&len);
	}
	*inOutLen = OTA_HV_NUM_MAX+1;
	return 0;
}

/****************************************************************************/

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
	char *ptr;
	int ret;
	
	printf("read_past_http_header\r\n");
	printf("%s",text);
	ptr = strstr(text,"Content-Length: ");
	if(ptr){
		printf("%s",ptr);
		ret = sscanf(ptr,"Content-Length: %d%*[\r\n]", &g_iOtaFileLen);
		if(1==ret)
			printf("ota file len:%d\r\n",g_iOtaFileLen);
	}
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

int OtaProg(void)
{
	return g_iOtaFileLen==0?0:binary_file_length*100/g_iOtaFileLen;
}

int OtaState(void)
{
	return g_stOtaProcess.state;
}

static bool connect_to_http_server()
{
    ESP_LOGI(TAG, "Server IP: %s Server Port:%s", g_aucOtaSvrIp, g_aucOtaSvrPort);

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
    sock_info.sin_addr.s_addr = inet_addr(g_aucOtaSvrIp);
    sock_info.sin_port = htons(atoi(g_aucOtaSvrPort));

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
#define task_fatal_error(err) \
{\
    close(socket_id);\
    g_aucOtaStart = 0;\    
	g_stOtaProcess.type = OTA_REPORT; \
	g_stOtaProcess.state = OTA_FAIL; \
	g_stOtaProcess.error = err; \
	g_stOtaProcess.process = OtaProg(); \
	ApsCoapOtaProcessRsp(&g_stOtaProcess); \
    goto OTA_START;\
}

int RecordVersion(void)
{
	int err;
	unsigned int vh[OTA_HV_NUM_MAX+1],inOutLen = OTA_HV_NUM_MAX+1;
	
	err = HistoryVersionGet(vh,&inOutLen);
	if(err == 0)
	{
		if(vh[OTA_HV_NUM_MAX] != OPP_LAMP_CTRL_CFG_DATA_VER)
		{
			err = HistoryVersionPush(OPP_LAMP_CTRL_CFG_DATA_VER);
			return err;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return err;
	}
}

void OtaThread(void *pvParameter)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    esp_partition_t *configured;
    esp_partition_t *running;
    int rvErr;

    strcpy(g_aucOtaSvrIp, OTA_SERVER_IP);
    strcpy(g_aucOtaSvrPort, OTA_SERVER_PORT);
    strcpy(g_aucOtaFile, OTA_FILENAME);

	rvErr = RecordVersion();
	if(rvErr != 0)
	{
		printf("RecordVersion fail,err=%d!\r\n",rvErr);
	}
    
    while(1)
    {
OTA_START:
        DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Ota wating...\r\n");

        // sempWait(g_aucOtaStart)
        while(!g_aucOtaStart){
           vTaskDelay(1000 / portTICK_PERIOD_MS); 
        }
        
        DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Start ota!\r\n");
        if(0 == strlen(g_aucOtaSvrIp))
    		strcpy(g_aucOtaSvrIp, OTA_SERVER_IP);
      	if(0 == strlen(g_aucOtaSvrPort))
    		strcpy(g_aucOtaSvrPort, OTA_SERVER_PORT);
    	if(0 == strlen(g_aucOtaFile))
    		strcpy(g_aucOtaFile, OTA_FILENAME);
		g_iOtaFileLen = 0;
		binary_file_length = 0;
        ESP_LOGI(TAG,"ip:%s,port:%s,file:%s\r\n",g_aucOtaSvrIp,g_aucOtaSvrPort,g_aucOtaFile);
    
        ESP_LOGI(TAG, "Starting OTA example...");
        
        DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Connect wifi...\r\n");
        //xEventGroupWaitBits(g_eventWifi, 1,false, true, portMAX_DELAY);
        OS_EVENTGROUP_WAITBIT(g_eventWifi,1,OS_EVENTGROUP_WAIT_ALWAYS);
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
        
        DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Connect http server...\r\n");
        /*connect to http server*/
        if (connect_to_http_server()) {
            ESP_LOGI(TAG, "Connected to http server");
        } else {
            ESP_LOGE(TAG, "Connect to http server failed!");
            task_fatal_error(OTA_CONNECT_SVR_ERR);
        }
    
        /*send GET request to http server*/
        const char *GET_FORMAT =
            "GET %s HTTP/1.0\r\n"
            "Host: %s:%s\r\n"
            "User-Agent: esp-idf/1.0 esp32\r\n\r\n";
    
        char *http_request = NULL;
        //int get_len = asprintf(&http_request, GET_FORMAT, OTA_FILENAME, OTA_SERVER_IP, OTA_SERVER_PORT);
        
        int get_len = asprintf(&http_request, GET_FORMAT, g_aucOtaFile, g_aucOtaSvrIp, g_aucOtaSvrPort);
        if (get_len < 0) {
            ESP_LOGE(TAG, "Failed to allocate memory for GET request buffer");
            task_fatal_error(OTA_MALLOC_ERR);
        }
        int res = send(socket_id, http_request, get_len, 0);
        free(http_request);
    
        if (res < 0) {
            ESP_LOGE(TAG, "Send GET request to server failed");
            task_fatal_error(OTA_SEND_ERR);
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
            task_fatal_error(OTA_BEGIN_ERR);
        }
       
        ESP_LOGI(TAG, "esp_ota_begin succeeded");
        DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Start ota...\r\n");
    
        bool resp_body_start = false, flag = true;
		struct timeval ti; 
		
		ti.tv_sec=30;
		ti.tv_usec=0;
		if(setsockopt(socket_id,SOL_SOCKET,SO_RCVTIMEO,&ti,sizeof(struct timeval)) == -1){
			DEBUG_LOG(DEBUG_MODULE_AUS, DLL_ERROR, "connect http server socket set SO_RCVTIMEO fail\r\n");
			task_fatal_error(OTA_SOL_SOCKET_ERR);
		}
		
        /*deal with all receive packet*/
        while (flag) {
            memset(text, 0, TEXT_BUFFSIZE);
            memset(ota_write_data, 0, BUFFSIZE);
            int buff_len = recv(socket_id, text, TEXT_BUFFSIZE, 0);
    		//printf("%s\r\n",text);
            if (buff_len < 0) { /*receive error*/
                ESP_LOGE(TAG, "Error: receive data error! errno=%d", errno);
                task_fatal_error(OTA_RECV_TO_ERR);
            } else if (buff_len > 0 && !resp_body_start) { /*deal with response header*/
                memcpy(ota_write_data, text, buff_len);
                resp_body_start = read_past_http_header(text, buff_len, update_handle);
            } else if (buff_len > 0 && resp_body_start) { /*deal with response body*/
                memcpy(ota_write_data, text, buff_len);
    
    			err = esp_ota_write( update_handle, (const void *)ota_write_data, buff_len);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Error: esp_ota_write failed (%s)!", esp_err_to_name(err));
                    task_fatal_error(OTA_WRITE_ERR);
                }
                
                binary_file_length += buff_len;
                //ESP_LOGI(TAG, "Have written image length %d", binary_file_length);
                //DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Have written image length %d\r\n", binary_file_length);
                
            } else if (buff_len == 0) {  /*packet over*/
                flag = false;			
				g_stOtaProcess.state = OTA_DOWNLOADED;
                ESP_LOGI(TAG, "Connection closed, all packets received");
                close(socket_id);
            } else {
                ESP_LOGE(TAG, "Unexpected recv result");
            }
        }
    
        //ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);
        //DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Total Write binary data length : %d\r\n", binary_file_length);
    
        if (esp_ota_end(update_handle) != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_end failed!");
            DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"esp_ota_end failed!\r\n");
            task_fatal_error(OTA_END_ERR);
        }		
		g_stOtaProcess.state = OTA_UPGRADING;
        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
            DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"esp_ota_set_boot_partition failed!\r\n");
            task_fatal_error(OTA_SET_BOOT_PARTITION);
        }
    	
        ESP_LOGI(TAG, "Prepare to restart system!");
        DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Ota success!\r\n");
		
		g_stOtaProcess.type = OTA_REPORT;
		g_stOtaProcess.process = OtaProg();
		g_stOtaProcess.state = OTA_SUCCESS;
		g_stOtaProcess.error = OTA_NO_ERR;
		strcpy(g_stOtaProcess.version,OPP_LAMP_CTRL_CFG_DATA_VER_STR);
		ApsCoapOtaProcessRsp(&g_stOtaProcess);

		DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"Rebooting...\r\n");
		
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        
		sysModeDecreaseSet();
		ApsCoapWriteExepInfo(OTA_RST);
		ApsCoapDoReboot();
        esp_restart();
    }
    return ;
}

void OtaStart(void)
{
    g_aucOtaStart = 1;
}
/********NB OTA******************/
unsigned short g_usCurPktReqFragNum = 0;
unsigned short g_usCurPktAckFragNum = 0;
unsigned short g_usPktRetryTimes = 0;
unsigned char g_usCurPktAck = 0;
unsigned char g_ucOtaPsm = JUDGE;
unsigned int g_uiSendTick = 0;
T_MUTEX g_stNbOtaMutex;
ST_NB_OTA_STATE g_stNbOtaState={
	{
		.dstVersion = "522",
		.fragSize = 0,
		.fragNum = 0,
		.checkCode = 0
	},
	.curFragNum = 0
};

extern ST_COAP_PKT g_stCoapPkt;

//※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※
// 函数名称  : U16 IOT_CRC16()
// 功能描述  : CRC16_CCITT
// 输入参数  : None.
// 输出参数  : None
// 返回参数  : None
//※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※
u16  crc_table1[ 256 ] = { 
          0x0000 ,  0x1021 ,  0x2042 ,  0x3063 ,  0x4084 ,  0x50a5 ,  0x60c6 ,  0x70e7 , 
          0x8108 ,  0x9129 ,  0xa14a ,  0xb16b ,  0xc18c ,  0xd1ad ,  0xe1ce ,  0xf1ef , 
          0x1231 ,  0x0210 ,  0x3273 ,  0x2252 ,  0x52b5 ,  0x4294 ,  0x72f7 ,  0x62d6 , 
          0x9339 ,  0x8318 ,  0xb37b ,  0xa35a ,  0xd3bd ,  0xc39c ,  0xf3ff ,  0xe3de , 
          0x2462 ,  0x3443 ,  0x0420 ,  0x1401 ,  0x64e6 ,  0x74c7 ,  0x44a4 ,  0x5485 , 
          0xa56a ,  0xb54b ,  0x8528 ,  0x9509 ,  0xe5ee ,  0xf5cf ,  0xc5ac ,  0xd58d , 
          0x3653 ,  0x2672 ,  0x1611 ,  0x0630 ,  0x76d7 ,  0x66f6 ,  0x5695 ,  0x46b4 , 
          0xb75b ,  0xa77a ,  0x9719 ,  0x8738 ,  0xf7df ,  0xe7fe ,  0xd79d ,  0xc7bc , 
          0x48c4 ,  0x58e5 ,  0x6886 ,  0x78a7 ,  0x0840 ,  0x1861 ,  0x2802 ,  0x3823 , 
          0xc9cc ,  0xd9ed ,  0xe98e ,  0xf9af ,  0x8948 ,  0x9969 ,  0xa90a ,  0xb92b , 
          0x5af5 ,  0x4ad4 ,  0x7ab7 ,  0x6a96 ,  0x1a71 ,  0x0a50 ,  0x3a33 ,  0x2a12 , 
          0xdbfd ,  0xcbdc ,  0xfbbf ,  0xeb9e ,  0x9b79 ,  0x8b58 ,  0xbb3b ,  0xab1a , 
          0x6ca6 ,  0x7c87 ,  0x4ce4 ,  0x5cc5 ,  0x2c22 ,  0x3c03 ,  0x0c60 ,  0x1c41 , 
          0xedae ,  0xfd8f ,  0xcdec ,  0xddcd ,  0xad2a ,  0xbd0b ,  0x8d68 ,  0x9d49 , 
          0x7e97 ,  0x6eb6 ,  0x5ed5 ,  0x4ef4 ,  0x3e13 ,  0x2e32 ,  0x1e51 ,  0x0e70 , 
          0xff9f ,  0xefbe ,  0xdfdd ,  0xcffc ,  0xbf1b ,  0xaf3a ,  0x9f59 ,  0x8f78 , 
          0x9188 ,  0x81a9 ,  0xb1ca ,  0xa1eb ,  0xd10c ,  0xc12d ,  0xf14e ,  0xe16f , 
          0x1080 ,  0x00a1 ,  0x30c2 ,  0x20e3 ,  0x5004 ,  0x4025 ,  0x7046 ,  0x6067 , 
          0x83b9 ,  0x9398 ,  0xa3fb ,  0xb3da ,  0xc33d ,  0xd31c ,  0xe37f ,  0xf35e , 
          0x02b1 ,  0x1290 ,  0x22f3 ,  0x32d2 ,  0x4235 ,  0x5214 ,  0x6277 ,  0x7256 , 
          0xb5ea ,  0xa5cb ,  0x95a8 ,  0x8589 ,  0xf56e ,  0xe54f ,  0xd52c ,  0xc50d , 
          0x34e2 ,  0x24c3 ,  0x14a0 ,  0x0481 ,  0x7466 ,  0x6447 ,  0x5424 ,  0x4405 , 
          0xa7db ,  0xb7fa ,  0x8799 ,  0x97b8 ,  0xe75f ,  0xf77e ,  0xc71d ,  0xd73c , 
          0x26d3 ,  0x36f2 ,  0x0691 ,  0x16b0 ,  0x6657 ,  0x7676 ,  0x4615 ,  0x5634 , 
          0xd94c ,  0xc96d ,  0xf90e ,  0xe92f ,  0x99c8 ,  0x89e9 ,  0xb98a ,  0xa9ab , 
          0x5844 ,  0x4865 ,  0x7806 ,  0x6827 ,  0x18c0 ,  0x08e1 ,  0x3882 ,  0x28a3 , 
          0xcb7d ,  0xdb5c ,  0xeb3f ,  0xfb1e ,  0x8bf9 ,  0x9bd8 ,  0xabbb ,  0xbb9a , 
          0x4a75 ,  0x5a54 ,  0x6a37 ,  0x7a16 ,  0x0af1 ,  0x1ad0 ,  0x2ab3 ,  0x3a92 , 
          0xfd2e ,  0xed0f ,  0xdd6c ,  0xcd4d ,  0xbdaa ,  0xad8b ,  0x9de8 ,  0x8dc9 , 
          0x7c26 ,  0x6c07 ,  0x5c64 ,  0x4c45 ,  0x3ca2 ,  0x2c83 ,  0x1ce0 ,  0x0cc1 , 
          0xef1f ,  0xff3e ,  0xcf5d ,  0xdf7c ,  0xaf9b ,  0xbfba ,  0x8fd9 ,  0x9ff8 , 
          0x6e17 ,  0x7e36 ,  0x4e55 ,  0x5e74 ,  0x2e93 ,  0x3eb2 ,  0x0ed1 ,  0x1ef0  
 } ; 

u16 do_crc(u16 reg_init, u8 *message, u16 length)
{
	u16 crc_reg = reg_init;
	u16  i;

	for (i = 0 ; i < length; i ++ )
	{ 
		crc_reg = (crc_reg >> 8) ^ crc_table1[(crc_reg ^ message[i]) & 0xff]; 						
	} 

	return crc_reg;
}

u16 Get_Crc16(u8 *d, u16 len)
{
	u16 crc_reg = 0x0000;  
    return do_crc(crc_reg, d,len);  
}

int NbOtaWriteFlash(ST_NB_OTA_STATE * pstNbOtaState)
{
	int ret = OPP_SUCCESS;

	DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "NbOtaWriteFlash\r\n");
	ret = AppParaWrite(APS_PARA_MODULE_APS_NBOTA, NB_OTA_ID, (unsigned char *)pstNbOtaState, sizeof(ST_NB_OTA_STATE));
	if(OPP_SUCCESS != ret){
		return OPP_FAILURE;
	}
	
	return OPP_SUCCESS;
}

int NbOtaReadFromFlash(ST_NB_OTA_STATE *pstNbOtaState)
{
	int ret = OPP_SUCCESS;
	unsigned int len = sizeof(ST_NB_OTA_STATE);
	ST_NB_OTA_STATE stNbOtaState;

	DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "NbOtaReadFromFlash\r\n");
	ret = AppParaRead(APS_PARA_MODULE_APS_NBOTA, NB_OTA_ID, (unsigned char *)&stNbOtaState, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "NbOtaReadFromFlash fail ret %d\r\n",ret);
		return OPP_FAILURE;
	}
	
	DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "NbOtaReadFromFlash succ\r\n");
	memcpy(pstNbOtaState,&stNbOtaState,sizeof(ST_NB_OTA_STATE));
	return OPP_SUCCESS;
	
}

int NbOtaInit()
{
	ST_NB_OTA_STATE stNbOtaState;
	int ret;
	
	MUTEX_CREATE(g_stNbOtaMutex);
	//memset(&g_stNbOtaState, 0, sizeof(ST_NB_OTA_STATE));
	ret = NbOtaReadFromFlash(&stNbOtaState);
	if(OPP_SUCCESS == ret){
		DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "NbOtaInit NbOtaReadFromFlash fail ret %d\r\n", ret);	
		memcpy(&g_stNbOtaState, &stNbOtaState, sizeof(ST_NB_OTA_STATE));
	}else{
		strcpy(g_stNbOtaState.stVerNoty.dstVersion, OPP_LAMP_CTRL_CFG_DATA_VER_STR);
	}
	//DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "NbOtaInit NbOtaReadFromFlash fail ret %d\r\n", ret);
	return OPP_SUCCESS;
}


/*
**0.update
**1.no update
*/
int NbOtaVerJudgement(ST_VER_NOTY * pstVerNoty)
{
	char buffer[VER_LEN] = {0};
	
	snprintf(buffer, VER_LEN, "%u%u%u", OPP_HUNDRED_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER), 
		OPP_DECADE_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER),
		OPP_UNIT_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER));
	if(1 == strncmp(pstVerNoty->dstVersion, buffer, VER_LEN)){
		return 1;
	}

	return 0;
}
int NbOtaRsp(unsigned char version, unsigned char msgCode, unsigned char retCode, char * append, unsigned short appendLen)
{
	unsigned char sendBuf[100] = {0};
	ST_NB_OTA *pOtaHdr;
	unsigned char *ptr;
	int len = 0, payloadLen = 0;
	MSG_ITEM_T mitem;
	int ret;

	printf("NbOtaRsp %d %d %d %s %d\r\n", version, msgCode, retCode, append, appendLen);
	
	pOtaHdr = (ST_NB_OTA *)sendBuf;
	pOtaHdr->hdr = OPP_HTONS(OTA_HDR);
	pOtaHdr->protVersion = version;
	pOtaHdr->msg = msgCode;
	len += sizeof(ST_NB_OTA);
	if(appendLen > 0){
		ptr = (unsigned char *)(sendBuf + len);
		ptr[payloadLen++] = retCode;		
		memcpy(&ptr[payloadLen], append, appendLen);
		payloadLen += appendLen;
		len += payloadLen;
		pOtaHdr->length = OPP_HTONS(payloadLen);
		pOtaHdr->crc = OPP_HTONS(Get_Crc16(sendBuf, len));		
	}else{
		pOtaHdr->length = 0;
		pOtaHdr->crc = OPP_HTONS(Get_Crc16(sendBuf, len));		
	}

	RoadLamp_Dump((char *)sendBuf,len);
	
	MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
	g_stCoapPkt.g_iCoapNbOtaTx++;
	MUTEX_UNLOCK(g_stCoapPkt.mutex);
	
	mitem.type = MSG_TYPE_HUAWEI;
	mitem.len = len;
	mitem.data = sendBuf;
	//DEBUG_LOG(DEBUG_MODULE_AUS, DLL_ERROR,"NbOtaRsp AppMessageSend chl %d\r\n", CHL_NB);	
	ret = AppMessageSend(CHL_NB,MSG_QUEUE_TYPE_TX,&mitem,PRIORITY_LOW);
	if(OPP_SUCCESS != ret){		
		MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
		g_stCoapPkt.g_iCoapNbOtaTxFail++;
		MUTEX_UNLOCK(g_stCoapPkt.mutex);
		DEBUG_LOG(DEBUG_MODULE_AUS, DLL_ERROR,"NbOtaRsp AppMessageSend fail ret %d\r\n", ret);
		return ret;
	}else{
		MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
		g_stCoapPkt.g_iCoapNbOtaTxSucc++;
		MUTEX_UNLOCK(g_stCoapPkt.mutex);
	}
	//DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"NbOtaRsp AppMessageSend succ\r\n");

	return OPP_SUCCESS;
}

int NbOtaTest(void)
{
	char sendBuf[100] = {0};
	//int len = 0;
	//MSG_ITEM_T mitem;
	//int ret;

	
	snprintf(sendBuf,VER_LEN,"%u%u%u",OPP_HUNDRED_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER),
		OPP_DECADE_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER),
		OPP_UNIT_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER));
	NbOtaRsp(1,QRY_VER_MSG,0,sendBuf,VER_LEN);
	/*
	sendBuf[len++] = 0x00;
	sendBuf[len++] = 0x02;
	sendBuf[len++] = 0x04;
	mitem.chl = CHL_NB;
	mitem.qtype = MSG_QUEUE_TYPE_TX;
	mitem.type = MSG_TYPE_HUAWEI;
	mitem.len = len;
	mitem.data = sendBuf;
	DEBUG_LOG(DEBUG_MODULE_AUS, DLL_ERROR,"NbOtaPktReq AppMessageSend chl %d\r\n", mitem.chl);	
	ret = AppMessageSend(&mitem,PRIORITY_LOW);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_AUS, DLL_ERROR,"NbOtaPktReq AppMessageSend fail ret %d\r\n", ret);
		return ret;
	}
	DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"NbOtaPktReq AppMessageSend succ\r\n");
	*/
	return OPP_SUCCESS;
}

int NbOtaPktReq(unsigned char protVersion, char * dstVersion, unsigned short fragNum)
{
	unsigned char sendBuf[100] = {0};
	ST_NB_OTA *pOtaHdr;
	ST_PKT_REQ *pPktReq;
	int len = 0/*, payloadLen = 0*/;
	MSG_ITEM_T mitem;
	int ret;

	DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"NbOtaPktReq %d %s %d\r\n", protVersion, dstVersion, fragNum);
	pOtaHdr = (ST_NB_OTA *)sendBuf;
	pOtaHdr->hdr = OPP_HTONS(OTA_HDR);
	pOtaHdr->protVersion = protVersion;
	pOtaHdr->msg = PKT_REQ_MSG;

	len += sizeof(ST_NB_OTA);
	pPktReq = (ST_PKT_REQ *)(sendBuf + len);
	memcpy(pPktReq->dstVersion, dstVersion, VER_LEN);
	pPktReq->fragNum = OPP_HTONS(fragNum);
	len += sizeof(ST_PKT_REQ);	
	pOtaHdr->length = OPP_HTONS(sizeof(ST_PKT_REQ));
	pOtaHdr->crc = Get_Crc16(sendBuf, len);

	RoadLamp_Dump((char *)sendBuf,len);
	
	mitem.type = MSG_TYPE_HUAWEI;
	mitem.len = len;
	mitem.data = sendBuf;
	DEBUG_LOG(DEBUG_MODULE_AUS, DLL_ERROR,"NbOtaPktReq AppMessageSend chl %d\r\n", CHL_NB);	
	ret = AppMessageSend(CHL_NB,MSG_QUEUE_TYPE_TX,&mitem,PRIORITY_LOW);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_AUS, DLL_ERROR,"NbOtaPktReq AppMessageSend fail ret %d\r\n", ret);
		return ret;
	}
	DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"NbOtaPktReq AppMessageSend succ\r\n");

	return OPP_SUCCESS;
	
}

int NbOtaDLStReport(unsigned char retCode)
{
	NbOtaRsp(PRO_VER,DL_ST_REPORT_MSG,retCode,NULL,0);

	return OPP_SUCCESS;
}

int NbOtaReport(unsigned char retCode, unsigned char msgType)
{
	char append[512] = {0};
	unsigned short len = 0;
	
	if(msgType == UPGRADE_RESULT_REPORT_MSG){
		snprintf(append,VER_LEN,"%u%u%u",OPP_HUNDRED_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER),
			OPP_DECADE_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER),
			OPP_UNIT_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER));
		len += VER_LEN;
	}
	NbOtaRsp(PRO_VER,msgType,retCode,append,len);
	
	return OPP_SUCCESS;
}
int NbOtaMsgProcess(unsigned char * data, unsigned short length)
{
	ST_NB_OTA *ptr;
	ST_VER_NOTY *pstVerNoty;
	ST_PKT_RSP *pstPktRsp;
	int len = 0, pktLen = 0;
	char append[512] = {0};
	int ret;
	//ST_NB_OTA_STATE stNbOtaState;
	ST_VER_NOTY stVerNoty;

	//DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "NbOtaMsgProcess\r\n");
	if(length < sizeof(ST_NB_OTA)){
		DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "NbOtaMsgProcess recv msg too short...\r\n");
		return OPP_FAILURE;
	}

	///< [Code review][ERROR](2018-6-29,Liqi) MUTEX 淇濇姢绮掑害澶ぇ
	
	MUTEX_LOCK(g_stNbOtaMutex, MUTEX_WAIT_ALWAYS);
	ptr = (ST_NB_OTA *)data;
	len += sizeof(ST_NB_OTA);
	
	//DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "NbOtaMsgProcess msg %d\r\n", ptr->msg);
	if(QRY_VER_MSG == ptr->msg){
		/*snprintf(append,VER_LEN,"%u%u%u",OPP_HUNDRED_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER),
			OPP_DECADE_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER),
			OPP_UNIT_OF_NUMBER(OPP_LAMP_CTRL_CFG_DATA_VER));*/
		snprintf(append,VER_LEN, "%02X%02X",OPP_LAMP_CTRL_CFG_DATA_MAX_VER>>8&0xFF,OPP_LAMP_CTRL_CFG_DATA_MAX_VER&0xFF);
		NbOtaRsp(ptr->protVersion,ptr->msg,0,append,VER_LEN);
	}else if(NEW_VER_NTY_MSG == ptr->msg){
		pstVerNoty = (ST_VER_NOTY *)&data[len];
		strncpy(stVerNoty.dstVersion,pstVerNoty->dstVersion,VER_LEN);
		stVerNoty.dstVersion[VER_LEN-1] = '\0';
		stVerNoty.fragSize = OPP_NTOHS(pstVerNoty->fragSize);
		stVerNoty.fragNum = OPP_NTOHS(pstVerNoty->fragNum);
		stVerNoty.checkCode = OPP_NTOHS(pstVerNoty->checkCode);
		//to judge ver
		ret = NbOtaVerJudgement(&stVerNoty);
		if(0 != ret){
			NbOtaRsp(PRO_VER,ptr->msg,RETCODE_NEWEST_VER,NULL,0);
			MUTEX_UNLOCK(g_stNbOtaMutex);
			return OPP_FAILURE;
		}
		ret = OppOtaStartHandle();
		if(OPP_SUCCESS != ret){
			NbOtaRsp(PRO_VER,ptr->msg,RETCODE_NOT_ENOUGH_ROM,NULL,0);
		}
		NbOtaRsp(PRO_VER,ptr->msg,RETCODE_SUCC,NULL,0);
		memcpy(&g_stNbOtaState.stVerNoty, &stVerNoty, sizeof(ST_VER_NOTY));
		g_stNbOtaState.curFragNum = 0;
		NbOtaWriteFlash(&g_stNbOtaState);
		//NBOTA_START();
	}else if(PKT_REQ_MSG == ptr->msg){
		pstPktRsp = (ST_PKT_RSP *)&data[len];
		g_usCurPktAck = 1;
		g_usCurPktAckFragNum = pstPktRsp->fragNum;
		if(OPP_SUCCESS == pstPktRsp->retCode){
			len += sizeof(ST_PKT_RSP);
			pktLen = OPP_NTOHS(ptr->length)-sizeof(ST_PKT_RSP);
			ret = OppOtaReceivingHandle(OPP_NTOHS(pstPktRsp->fragNum),&data[len],pktLen);
			if(OPP_SUCCESS != ret){
				NbOtaReport(RETCODE_NOT_ENOUGH_ROM, DL_ST_REPORT_MSG);
				NbOtaWriteFlash(&g_stNbOtaState);
				g_ucOtaPsm = EXCEPTION;
			}
		}else{
			NbOtaWriteFlash(&g_stNbOtaState);
			g_ucOtaPsm = EXCEPTION;
		}
	}else if(EXC_UPGRADE_MSG == ptr->msg){
		NbOtaRsp(ptr->protVersion,ptr->msg,0,NULL,0);
		ret = OppOtaReceiveEndHandle();
		if(OPP_SUCCESS != ret){
			NbOtaReport(RETCODE_NOT_ENOUGH_ROM, EXC_UPGRADE_MSG);
		}
		NbOtaReport(RETCODE_SUCC, EXC_UPGRADE_MSG);
	}else if(DL_ST_REPORT_MSG == ptr->msg){

	}else if(UPGRADE_RESULT_REPORT_MSG == ptr->msg){
		if(0 == data[len]){
			DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "NbOtaMsgProcess update succ...\r\n");
		}else{
			DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "NbOtaMsgProcess update fail...\r\n");
		}
	}
	
	MUTEX_UNLOCK(g_stNbOtaMutex);
	return OPP_SUCCESS;
}

void NbOtaThread(void *pvParameter)
{
	int ret;
	static unsigned char isFirst = 1;

	NbOtaInit();
	DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"NbOtaThread...\r\n");

	while(1){
		/*if(BC28_READY != NeulBc28GetDevState()){
			continue;
		}*/

		///< [code review][error][liqi,2018-6-29]mutex淇濇姢绮掑害澶ぇ
		MUTEX_LOCK(g_stNbOtaMutex, MUTEX_WAIT_ALWAYS);
		switch(g_ucOtaPsm){
			case JUDGE:
				DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "version judge\r\n");
				ret = NbOtaVerJudgement(&g_stNbOtaState.stVerNoty);
				if(1 == ret){
					printf("version update\r\n");
					g_ucOtaPsm = UPDATE;
				}
				break;
			case UPDATE:
				DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "UPDATE\r\n");
				if(isFirst){
					NbOtaPktReq(PRO_VER,g_stNbOtaState.stVerNoty.dstVersion,g_stNbOtaState.curFragNum);
					g_uiSendTick = OppTickCountGet();
					isFirst = 0;
				}
				if(g_usCurPktAck){
					if(g_usCurPktAckFragNum == g_stNbOtaState.curFragNum){
						if(g_usCurPktAckFragNum == g_stNbOtaState.stVerNoty.fragNum){
							g_ucOtaPsm = FINISH;
						}else{
							g_stNbOtaState.curFragNum++;
							NbOtaPktReq(PRO_VER,g_stNbOtaState.stVerNoty.dstVersion,g_stNbOtaState.curFragNum);
						}
					}
					g_usCurPktAck = 0;
					g_uiSendTick = OppTickCountGet();
				}
				
				if(OppTickCountGet() - g_uiSendTick > 30000){
					if(g_usPktRetryTimes >= g_usPktRetryTimes-1){
						isFirst = 0;
						DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"NbOtaThread update timeout\r\n");
						NBOTA_STOP();
					}else{
						NbOtaPktReq(PRO_VER,g_stNbOtaState.stVerNoty.dstVersion,g_stNbOtaState.curFragNum);
						g_stNbOtaState.curFragNum++;
					}
					g_usCurPktAck = 0;
					g_usCurPktAckFragNum = 0;
					g_uiSendTick = OppTickCountGet();
				}
				break;
		case FINISH:
			DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO, "FINISH\r\n");		
			isFirst = 0;
			NbOtaReport(RETCODE_SUCC, DL_ST_REPORT_MSG);
			g_stNbOtaState.curFragNum = 0;
			NbOtaWriteFlash(&g_stNbOtaState);
			DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"NbOtaThread pkt finish\r\n");
			break;
		case EXCEPTION:
			DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"EXCEPTION\r\n");		
			DEBUG_LOG(DEBUG_MODULE_AUS, DLL_INFO,"NbOtaThread EXCEPTION\r\n");
			g_ucOtaPsm = JUDGE;
			break;
		}
		MUTEX_UNLOCK(g_stNbOtaMutex);
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

int NbOtaGetState()
{
	return g_ucOtaPsm;
}

void NbOtaSetState(unsigned char state)
{
	g_ucOtaPsm = state;
}

