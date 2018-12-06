
#include <Includes.h>
#include "stdio.h"
#include <Typedef.h>

#include "DRV_Nb.h"
#include "DRV_Bc28.h"
#include "OPP_Debug.h"
#include <SVS_MsgMmt.h>
#include <APP_Communication.h>
#include <DEV_Utility.h>
#include <SVS_Opple.h>
#include <APP_Main.h>
#include "stdio.h"
#include "SVS_Aus.h"


/************update function************/
char tbuffer[100];
char rbuffer[500] = {0};
u16 g_usSeqNo = 0;
u16 g_usPlMax = 100;
//u8  g_aucFirmware[14*1024] = {0};
u32 g_ulFwSize = 0;
static UINT32 tick_start = 0;
//static int g_ota_prepare = 0;
static U8 m_ucOtaStart = 0;

static const u16 crc16_tab[] = 
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d, 
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4, 
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823, 
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49, 
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78, 
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067, 
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d, 
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c, 
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634, 
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab, 
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92, 
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1, 
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0, 
};


u16 OppCalcCRC16(u8 *buf , u32 len)
{
    u16 crc = 0;
    u16 i;
    u16 tabval;
    int idx = 0;

    for (i = 0; i < len; i++)
    {
        tabval = buf[i];
        idx = ((crc >> 8) ^ tabval) & 0xFF;
        crc = (u16)(crc16_tab[idx] ^ (crc << 8));
    }
    return crc;
}


//INT32U* pCRCTable is crc32table
//crc is 0xFFFFFFFF
u32 CalculateImageCrc(u32* pCRCTable, const u8* pImage, u32 imageSize)
{
	u32 crc = 0;
    u32 i;
	
    for (i = 0; i < imageSize; i++)
    {
        crc = pCRCTable[(crc ^ pImage[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc;
}

void OppOtaUpdateReq(unsigned char cmd, unsigned short seqNo)
{
	u16 len = 0;
	u16 crc16 = 0;
	ST_NEUL_DEV stNeulDev;
	MSG_ITEM_T item;

	//NeulBc28QueryUestats(&stNeulDev);
	memset(tbuffer, 0, sizeof(tbuffer));
	strncpy(tbuffer, (char *)stNeulDev.imei, strlen((char *)stNeulDev.imei));
	len += strlen((char *)stNeulDev.imei);
	tbuffer[len++] = (seqNo >> 8) & 0xFF;
	tbuffer[len++] = seqNo & 0xFF;	
	tbuffer[len++] = UPDATE_REQ;
	tbuffer[len++] = (2>>8) & 0xFF;	
	tbuffer[len++] = 2 & 0xFF;	
	tbuffer[len++] = (g_usPlMax >> 8) & 0xFF;
	tbuffer[len++] = g_usPlMax & 0xFF;
	//crc
	crc16 = OppCalcCRC16((U8 *)tbuffer, len);
	tbuffer[len++] = (crc16 >> 8) & 0xFF;
	tbuffer[len++] = crc16 & 0xFF;

	//AINPUT(UPDATE_REQ, seqno, 3);
	
	//NeulBc28UdpSend(socket, tbuffer, len);
	
	item.type = MSG_TYPE_OPPLE;
	item.len = len;
	item.data = (U8 *)tbuffer;
	AppMessageSend(CHL_NB,MSG_QUEUE_TYPE_TX,&item,PRIORITY_LOW);

	DEBUG_LOG(DEBUG_MODULE_OTA, DLL_INFO, "OppOtaUpdateReq send success\r\n");

	return;
}

void OppOtaSendAck(unsigned char cmd, unsigned short seqNo, unsigned char retCode)
{
	u16 len = 0;
	u16 crc16 = 0;
	ST_NEUL_DEV stNeulDev;
	MSG_ITEM_T item;

	//NeulBc28QueryUestats(&stNeulDev);
	memset(tbuffer, 0, sizeof(tbuffer));
	strncpy(tbuffer, (char *)stNeulDev.imei, strlen((char *)stNeulDev.imei));
	len += strlen((char *)stNeulDev.imei);
	tbuffer[len++] = (seqNo >> 8) & 0xFF;
	tbuffer[len++] = seqNo & 0xFF;
	tbuffer[len++] = cmd;
	tbuffer[len++] = (1>>8) & 0xFF;	
	tbuffer[len++] = 1 & 0xFF;
	tbuffer[len++] = retCode;
	//crc
	crc16 = OppCalcCRC16((U8 *)tbuffer, len);	
	tbuffer[len++] = (crc16 >> 8) & 0xFF;
	tbuffer[len++] = crc16 & 0xFF;

	//NeulBc28UdpSend(socket, tbuffer, len);

	item.type = MSG_TYPE_OPPLE;
	item.len = len;
	item.data = (U8 *)tbuffer;
	AppMessageSend(CHL_NB,MSG_QUEUE_TYPE_TX,&item,PRIORITY_LOW);

	DEBUG_LOG(DEBUG_MODULE_OTA, DLL_INFO, "OppOtaSendAck success\r\n");
	return;
}

void OppOtaRecvUdpUpdate(U8 *pucVal, U16 nLen)
{
	OPP_OTA_HDR * pstOppHdr = NULL;
	int error = 0;
	static unsigned char* ota_data;
	static int time_start,time_use = 0;
	//static int t0,t1;
	ST_NEUL_DEV stNeulDev;

	//NeulBc28QueryUestats(&stNeulDev);
		
	if(pucVal == NULL)
	{
		return;
	}
	/*
	printf("\r\n*****nLen %d********\r\n", nLen);
	for(i = 0; i<nLen; i++)
	{
		printf("%x ", pucVal[i]);
		if(i%16 == 0)
		{
			printf("\r\n");
		}
	}
	printf("\r\n*************\r\n");
	*/
	pstOppHdr = (OPP_OTA_HDR *)pucVal;
/*
	for(i = 0; i<16; i++)
		printf("%x", imei[i]);
	printf("\r\n*************\r\n");
	for(i = 0; i<16; i++)
		printf("%x", pstOppHdr->imei[i]);
*/	
	if(strncmp((char *)stNeulDev.imei, (char *)pstOppHdr->imei,strlen((char *)stNeulDev.imei)) != 0)
	{
		DEBUG_LOG(DEBUG_MODULE_OTA, DLL_ERROR, "%s imei is not me\r\n", __FUNCTION__);
		return;
	}

/*
	len = ntohs(pstOppHdr->len) + sizeof(OPP_OTA_HDR);
	//memcpy((u8 *)&crc16, pucVal+len, sizeof(crc16));
	printf("~~~~hdr %d len %d\r\n", sizeof(OPP_OTA_HDR), len);
	crc16 = (u16)(pucVal[len]<<8|pucVal[len+1]);
	calc_crc16 = OppCalcCRC16(pucVal, len);

	if(crc16 != calc_crc16)
	{
		printf("crc 0x%04x calc_crc 0x%04x error\r\n", crc16, calc_crc16);
		return;
	}
	*/
	
	//更新定时器
	if((pstOppHdr->cmd == UPDATE_ACK) 
		||(pstOppHdr->cmd == CONTENT)
		||(pstOppHdr->cmd == FIN_ACK))
	{
//		g_llLastTick1 = 0;
		tick_start = (UINT32)OppTickCountGet();

	}
	
	switch(pstOppHdr->cmd)
	{
		case UPDATE_ACK:
			DEBUG_LOG(DEBUG_MODULE_OTA, DLL_INFO, "update_ack %d\r\n", OPP_NTOHS(pstOppHdr->seqno));
			AOUTPUT(OPP_NTOHS(pstOppHdr->seqno));
		  OppOtaStartHandle();
		  time_start = OppTickCountGet();
			break;
		case CONTENT:
			DEBUG_LOG(DEBUG_MODULE_OTA, DLL_INFO, "content pstOppHdr->len %d~~\r\n", OPP_NTOHS(pstOppHdr->len));
		  //DEBUG_LOG(DEBUG_MODULE_NB,DLL_INFO,"Receiving data...");
		  //t0 = OppTickCountGet();
		  ota_data = (unsigned char*)(pucVal+sizeof(OPP_OTA_HDR));
		  
		  error = OppOtaReceivingHandle(OPP_NTOHS(pstOppHdr->seqno),ota_data,OPP_NTOHS(pstOppHdr->len));
			OppOtaSendAck(CONTENT_ACK, OPP_NTOHS(pstOppHdr->seqno), error);
		  if(error != 0)
			{
			}
		  //DEBUG_LOG(DEBUG_MODULE_NB,DLL_INFO,"Receiving ack...");
		   //printf("Task time use: %lld\r\n",OppTickCountGet()-t0);
			//memcpy(&g_aucFirmware[g_ulFwSize], pstOppHdr+sizeof(OPP_OTA_HDR), ntohs(pstOppHdr->len));
			//g_ulFwSize += ntohs(pstOppHdr->len);
			break;
		case FIN:
			DEBUG_LOG(DEBUG_MODULE_OTA, DLL_INFO, "Fin ok\r\n");
			//usLastSeqNo = ntohs(pstOppHdr->seqno);
			//memcpy((u8 *)&crc32, pstOppHdr+sizeof(OPP_OTA_HDR),sizeof(crc32));
			//calc_crc32 = CalculateImageCrc(crc32table, g_aucFirmware, g_ulFwSize);
			//if(crc32 != calc_crc32)
			//{
			//	printf("firmware crc32 0x%08x calc_crc32 0x%08x error\r\n", crc32, calc_crc32);
			//	error = 1;
			//}
		  error = OppOtaReceiveEndHandle();
			OppOtaSendAck(FIN_ACK, OPP_NTOHS(pstOppHdr->seqno), error);
			//g_ulFwSize = 0;
			AINPUT(UPDATE_REQ, OppOtaUpdateReq, g_usSeqNo++, 3);
		  
      		time_use = OppTickCountGet();
			time_use -= time_start;
      		DEBUG_LOG(DEBUG_MODULE_OTA, DLL_INFO, "******************* Time use %d ms !\r\n",time_use);		
		  
		  	TASK_DELAY(3000);
		  //NVIC_SystemReset();
			break;
		default:
			DEBUG_LOG(DEBUG_MODULE_OTA, DLL_INFO, "ota unknow cmd 0x%x\r\n", pstOppHdr->cmd);
			break;
	}
}

void OppOtaInit(void)
{
	int socket = 0;
	int i = 0;
	
	NeulBc28ProtoInit(UDP_PRO);
	socket = NeulBc28CreateUdpSocket(3005);
	NeulBc28SocketConfigRemoteInfo(socket,"58.211.235.246", 10002);
	m_ucOtaStart = 1;
	
	for(i = 0; i < MAX; i++)
	{
		memset((u8 *)&g_astRetry[i], 0, sizeof(ST_REQID));
	}
	
	AINPUT(UPDATE_REQ, OppOtaUpdateReq, g_usSeqNo++, 3);
	
	tick_start = (UINT32)OppTickCountGet();
}

void OppOtaLoop(void)
{
	static u8 idx = 0;

	if((BC28_READY != NeulBc28GetDevState()) || (0 == m_ucOtaStart))
	{
		return;
	}
	
	if(((UINT32)OppTickCountGet() - tick_start) >= 1000)
	{
		//NeulBc28QueryClock();
		//NeulBc28QueryUestats();
		if(idx >= MAX)
		{
			idx = 0;
		}
		
		ACALL(idx);
		idx++;
		tick_start = (UINT32)OppTickCountGet();
	}
	
	//60s没有数据重新开始
	if(((UINT32)OppTickCountGet() - tick_start) >= 60000)
	{			
		//NeulBc28QueryUestats();
		
		AINPUT(UPDATE_REQ, OppOtaUpdateReq, g_usSeqNo++, 3);
		tick_start = (UINT32)OppTickCountGet();
	}
}

