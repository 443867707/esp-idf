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

#include "SVS_Coap.h"
#include "SVS_Opple.h"
#include "SVS_MsgMmt.h"
#include "OPP_Debug.h"
#include "SVS_WiFi.h"
#include "DEV_Utility.h"
#include "opple_main.h"
#include "SVS_Udp.h"


#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024

/*an packet receive buffer*/
static char text[BUFFSIZE + 1] = { 0 };
/*socket id*/
static int udpSocket = -1;
//struct sockaddr_in addr;
//unsigned int addr_len = sizeof(struct sockaddr_in);
LIST_HEAD(listu,_list_udp_client_entry) list_udp;
T_MUTEX g_stUdpMutex;

extern OS_EVENTGROUP_T g_eventWifi;


static int udp_server_init(void)  
{  
    int server_udp;  
    //unsigned char one = 1;  
    int sock_opt = 1;  
	struct sockaddr_in local_addr;	
	struct timeval ti; 
      
    server_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
    if (server_udp == -1) {  
        //printf("unable to create socket\n");  
    }  
  
	ti.tv_sec=5;
	ti.tv_usec=0;
	if(setsockopt(server_udp,SOL_SOCKET,SO_RCVTIMEO,&ti,sizeof(ti)) == -1){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "UdpSend socket set SO_RCVTIMEO fail\r\n");
	}
    /* reuse socket addr, SO_REUSERADDR 允许重用本地地址和端口 int充许绑定已被使用的地址（或端口号）*/  
    if ((setsockopt(server_udp, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,sizeof (sock_opt))) == -1) {
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "UdpSend socket SO_REUSEADDR fail\r\n");
	}
	
	memset(&local_addr,0,sizeof(local_addr));//set 0 for everything  
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);//INADDR_ANY就是指定地址为0.0.0.0的地址,这个地址事实上表示不确定地址,或“所有地址”、“任意地址”  
	local_addr.sin_port = htons(59876);	  
	bind(server_udp,(struct sockaddr*)&local_addr,sizeof(local_addr)); 

    return server_udp;  
}  

void UdpSend(const MSG_ITEM_T *item)
{
	struct sockaddr_in addr;
	unsigned int addr_len = sizeof(struct sockaddr_in);
	int ret;
	int sendFail = 0;
	unsigned char * data = item->data;
	unsigned int len = item->len;
	unsigned char type = item->type;
	// ip,port
	
	if(-1 != udpSocket){
		RoadLamp_Dump((char *)data,len);
		//addr.sin_port = htons(59876);
		//printf("ip:%08x, port:%d\r\n", *(unsigned int *)item->info, *(unsigned short *)&item->info[4]);
		//printf("trans ip:%08x, port:%d\r\n", htonl(*(unsigned int *)item->info), htons(*(unsigned short *)&item->info[4]));

		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr =  htonl(*(unsigned int *)item->info);
		addr.sin_port = htons(*(unsigned short *)&item->info[4]);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "UdpSend socket %d ip %s port %d len %d\r\n",udpSocket,inet_ntoa(addr.sin_addr),ntohs(addr.sin_port), len);
		ret = sendto(udpSocket, data, len, 0, (struct sockaddr*)&addr,addr_len);
		if(ret < 0){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"UdpSend fail \r\n");
			sendFail = 1;
		}
		UdpClientListTxUpdate(&addr,sendFail);
	}
}

unsigned char UdpIsBusy(void)
{
	return 0;
}

void UdpPrintInfoDelay(int timeout)
{
	static U32 tick = 0;

	if(0 == tick){
		tick = OppTickCountGet();
	}else if(OppTickCountGet() - tick > timeout){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "UdpThread run..........\r\n");
		tick = 0;
	}
}
/*pushState:0 success, !0 fail*/
void UdpClientListRxUpdate(struct sockaddr_in *addr, int pushFail)
{
	ST_UDP_CLIENT_ENTRY *pstUdpClient;

	MUTEX_LOCK(g_stUdpMutex,MUTEX_WAIT_ALWAYS);
	LIST_FOREACH(pstUdpClient, &list_udp, elements){
		if(pstUdpClient->addr.sin_addr.s_addr == addr->sin_addr.s_addr
			&& pstUdpClient->addr.sin_port == addr->sin_port){
			pstUdpClient->tick = OppTickCountGet();
			pstUdpClient->rx++;
			if(pushFail)
				pstUdpClient->pushFail++;
			MUTEX_UNLOCK(g_stUdpMutex);
			return;
		}
	}
	MUTEX_UNLOCK(g_stUdpMutex);
	
	pstUdpClient = (ST_UDP_CLIENT_ENTRY *)malloc(sizeof(ST_UDP_CLIENT_ENTRY));
	if(!pstUdpClient){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "UdpClientListRxUpdate malloc error..........\r\n");
		return;
	}
	memset(pstUdpClient,0,sizeof(ST_UDP_CLIENT_ENTRY));	
	memcpy(&pstUdpClient->addr,addr,sizeof(struct sockaddr_in));
	pstUdpClient->tick = OppTickCountGet();
	pstUdpClient->rx++;
	if(pushFail)
		pstUdpClient->pushFail++;
	MUTEX_LOCK(g_stUdpMutex,MUTEX_WAIT_ALWAYS);
	LIST_INSERT_HEAD(&list_udp,pstUdpClient,elements);
	MUTEX_UNLOCK(g_stUdpMutex);
}

void UdpClientListTxUpdate(struct sockaddr_in *addr, int sendFail)
{
	ST_UDP_CLIENT_ENTRY *pstUdpClient;

	MUTEX_LOCK(g_stUdpMutex,MUTEX_WAIT_ALWAYS);
	LIST_FOREACH(pstUdpClient, &list_udp, elements){
		if(pstUdpClient->addr.sin_addr.s_addr == addr->sin_addr.s_addr
			&& pstUdpClient->addr.sin_port == addr->sin_port){
			pstUdpClient->txTick = OppTickCountGet();
			pstUdpClient->tx++;
			if(sendFail)
				pstUdpClient->txFail++;
			MUTEX_UNLOCK(g_stUdpMutex);
			return;
		}
	}
	MUTEX_UNLOCK(g_stUdpMutex);
}

void UdpClientTimeout()
{
	
	ST_UDP_CLIENT_ENTRY *pstUdpClient;
	
	MUTEX_LOCK(g_stUdpMutex,MUTEX_WAIT_ALWAYS);
	LIST_FOREACH(pstUdpClient, &list_udp, elements){
		if(OppTickCountGet() - pstUdpClient->tick > UDP_TO){
			LIST_REMOVE(pstUdpClient,elements);
		}
	}
	MUTEX_UNLOCK(g_stUdpMutex);
}

void UdpClientListShow(myprint print)
{	
	ST_UDP_CLIENT_ENTRY *pstUdpClient;
	print("%15s\t%5s\t%10s\t%10s\t%10s\t\%10s\t%10s\t%10s\r\n","Ip","Port","RxTick(ms)","Rx","PushFail","TxTick(ms)","Tx","TxFail");
	LIST_FOREACH(pstUdpClient, &list_udp, elements){
		print("%15s\t%5u\t%10u\t%10u\t%10u\t%10u\t%10u\t%10u\r\n",inet_ntoa(pstUdpClient->addr.sin_addr.s_addr),
			ntohs(pstUdpClient->addr.sin_port),pstUdpClient->tick,pstUdpClient->rx,
			pstUdpClient->pushFail,pstUdpClient->txTick,pstUdpClient->tx,pstUdpClient->txFail);
	}
}

void UdpThread(void *pvParameter)
{
	MSG_ITEM_T item;
	int buff_len;
  	int ret;
	struct sockaddr_in addr;
	unsigned int addr_len = sizeof(struct sockaddr_in);

	MUTEX_CREATE(g_stUdpMutex);
	LIST_INIT(&list_udp);
	udpSocket = udp_server_init();
	
	while (1) {
		// taskWdtReset();
		
		//xEventGroupWaitBits(g_eventWifi, 1,false, true, portMAX_DELAY);
		OS_EVENTGROUP_WAITBIT(g_eventWifi,1,OS_EVENTGROUP_WAIT_ALWAYS);
		
		memset(text, 0, TEXT_BUFFSIZE);
		//int buff_len = recv(udpSocket, text, TEXT_BUFFSIZE, 0);
		buff_len = recvfrom(udpSocket, text, TEXT_BUFFSIZE, 0, (struct sockaddr*)&addr, (unsigned int *)&addr_len);
		if(buff_len > 0){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"~~~~~~~udp len %d~~~~~~~~~~~~\r\n", buff_len);			
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"src address %s\r\n",inet_ntoa(addr.sin_addr.s_addr));
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%d\r\n",ntohs(addr.sin_port));   

			item.type = MSG_TYPE_HUAWEI;
			item.len = buff_len;
			item.data = (unsigned char *)text;
			*(unsigned int *)item.info = ntohl(addr.sin_addr.s_addr);
			*(unsigned short *)&item.info[4] = ntohs(addr.sin_port);
			//printf("ip:%08x, port:%d\r\n", *(unsigned int *)item.info, *(unsigned short *)&item.info[4]);
			ret = MsgPush(CHL_WIFI,MSG_QUEUE_TYPE_RX,&item, 0,PRIORITY_LOW);  // Mutex need
			if(0 != ret)
			{
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"MsgPush fail ret %d\r\n", ret);			
			}
			UdpClientListRxUpdate(&addr,ret);
		}
		UdpClientTimeout();
		UdpPrintInfoDelay(60000);
	}
}
