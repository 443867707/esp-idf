
#include <Includes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "DRV_Nb.h"
#include <DRV_Gps.h>
#include "DRV_Bc28.h"
#include <SVS_MsgMmt.h>
#include <SVS_Opple.h>
#include <APP_Communication.h>
#include <APP_LampCtrl.h>
#include <SVS_Config.h>
#include <SVS_Para.h>
#include <SVS_Coap.h>
#include <SVS_Log.h>
#include <APP_Main.h>
#include <APP_Daemon.h>
#include <DEV_Utility.h>
#include <DEV_Queue.h>
#include <Opp_Module.h>
#include <OPP_Debug.h>
#include <SVS_Lamp.h>
#include <SVS_Ota.h>
#include <SVS_Time.h>
#include "cJSON.h"
#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "SVS_WiFi.h"
#include "SVS_Location.h"
#include "DRV_LightSensor.h"
#include "DRV_SpiMeter.h"
#include "DRV_DimCtrl.h"
#include "opple_main.h"
#include <esp_system.h>
#include "rom/rtc.h"
#include "SVS_Test.h"
#include "LightBri.h"

#define mymalloc  malloc
#define myfree    free

#define QLOG_QUEUE_MAX    100

T_MUTEX m_ulCoapMutex = 0;
static T_MUTEX m_stCoapReportMutex = 0;
static T_MUTEX m_stLogReportMutex = 0;
T_MUTEX m_ulCoapRHB = NULL;
static T_MUTEX m_stMidMutex = 0;
static U16 m_usMid = 0;
static int m_iReqid = 0;
static cJSON *g_pstLogRoot = NULL;
static cJSON *g_pstLogArray = NULL;

static ST_HEART_PARA g_stHeartPara = {
	.udwHeartTick = 0,
	.udwRandHeartPeriod = 0
};
static ST_RX_PROTECT g_stRxProtect = {
	.enableH = RX_PROTECT_ENABLE_H,
	.enableL = RX_PROTECT_ENABLE_L
};
static ST_DIS_HEART_SAVE_PARA g_stHeartDisParaDefault = {
	.udwDiscreteWin = HEART_DIS_WIN_DEFAULT,
	.udwDiscreteInterval = HEART_DIS_INTERVAL_DEFAULT
};
static ST_DIS_HEART_PARA g_stHeartDisPara;

ST_LOG_STATUS m_stLogStatus;

/*code review reboot notify*/
static U32 g_iReTick = 0;
static U32 g_iRebootTick = 0;
static U32 g_iAttachTick = 0;
static U32 g_iWifiConTick = 0;
ST_OPP_OTA_PROC g_stOtaProcess;
static t_queue queue_qlog;
static U8 queue_qlog_buf[sizeof(ST_LOG_QUERY_REPORT)*QLOG_QUEUE_MAX];
static U8 g_isSupportRetry = 1;
static U8 startReboot = 0;
static U8 startAttach = 0;
ST_COAP_PKT g_stCoapPkt = {
	.g_iCoapNbTxSuc = 0,
	.g_iCoapNbTxFail = 0,
	.g_iCoapNbTxRetry = 0,
	.g_iCoapNbTxReqRetry = 0,
	.g_iCoapNbTxRspRetry = 0,
	.g_iCoapNbRx = 0,
	.g_iCoapNbRxRsp = 0,
	.g_iCoapNbRxReq = 0,
	.g_iCoapNbDevReq = 0,
	.g_iCoapNbDevRsp = 0,
	.g_iCoapNbUnknow = 0,
	.g_iCoapNbOtaRx = 0,
	.g_iCoapNbOtaTx = 0,
	.g_iCoapNbOtaTxSucc = 0,
	.g_iCoapNbOtaTxFail = 0,
	.g_iCoapBusy = 0,
};
static U8 retryHeartbeat = 0;
static U8 recvHeartAck = 0;
	
const ST_RES_MAP aucResMap[] = {
	{RSC_NB_SIGNAL, U32_T, R|W|M|D},
	{RSC_NB_NETWORK, U8_T, R|W|M|D},
	{RSC_LAMP_ONOFF, U8_T, R|W|M|D},
	{RSC_LAMP_BRI, U16_T, R|W|M|D},
	{RSC_RUNTIME, U32_T, R|W|M|D},
	{RSC_HISTIME, U32_T, R|W|M|D},
	{RSC_VOLT, U8_T, R|W|M|D},
	{RSC_CURRENT, U32_T, R|W|M|D},
	{RSC_HEART, U8_T, R|M|D},
	{RSC_LOC, MAX_T, R},	
	{RSC_TEST, MAX_T, R},
};

const ST_PROP_MAP aucPropMap[] = {
	{RSC_NB_SIGNAL, STR_T, "rsrp", "nbSignal"},
	{RSC_NB_NETWORK, INT_T, "reg", "nbCon"},
	{RSC_LAMP_ONOFF, INT_T, "switch", ""},
	{RSC_LAMP_BRI, INT_T, "bri", ""},
	{RSC_RUNTIME, INT_T, "rTime", "runtime"},
	{RSC_HISTIME, INT_T, "hTime", "runtime"},
	{RSC_VOLT, INT_T, "voltage", "elec"},
	{RSC_CURRENT, INT_T, "current", "elec"},
	{RSC_LOC, INT_T, "", "loc"},
};

const ST_CMDID_MAP aucCmdIdMap[] = {
	{RSC_HEART,"heart",ApsCoapOceanconHeart},
};

const ST_PROP aucProperty[] = {
	{PROP_SWITCH, "switch",INT_T, R|W|RE,OppCoapPackSwitchState},
	{PROP_BRI, "bri",INT_T, R|W|RE,OppCoapPackBriState},
	{PROP_RUNTIME, "runtime", OBJ_T, R|W|RE,OppCoapPackRuntimeState},
	{PROP_ELEC, "elec", OBJ_T, R|RE,OppCoapPackElecState},
	{PROP_ECINFO, "ecInfo", OBJ_T, R|W|RE,NULL},
	{PROP_LOC, "loc", OBJ_T, R|RE,OppCoapPackLoctionState},
	{PROP_NBBASE, "nbBase", OBJ_T, R|RE,OppCoapPackNbBaseState},
	{PROP_NBSIGNAL, "nbSignal", OBJ_T, R|RE,OppCoapPackNbSignalState},
	{PROP_NBCON, "nbCon", OBJ_T, R|RE,OppCoapPackNbConState},
	{PROP_PRODUCT, "product", STR_T, R,NULL},
	{PROP_SN, "sn", STR_T, R,NULL},
	{PROP_PROTID, "protId", STR_T, R,NULL},
	{PROP_MODEL, "model", STR_T, R,NULL},
	{PROP_MANU, "manu", STR_T, R,NULL},
	{PROP_SKU, "sku", STR_T, R,NULL},
	{PROP_CLAS, "clas", STR_T, R,NULL},
	{PROP_FWV, "fwv", INT_T, R,NULL},
	{PROP_OCIP, "ocIp", STR_T, R|W,NULL},
	{PROP_OCPORT, "ocPort", INT_T, R|W,NULL},
	{PROP_BAND, "band", INT_T, R|W,NULL},
	{PROP_DIMTYPE, "dimType", INT_T, R|W,NULL},
	{PROP_APNADDRESS, "apnAddres", STR_T, R|W,NULL},
	{PROP_PERIOD, "period", INT_T, R|W,NULL},
	{PROP_SCRAM, "scram", INT_T, R|W,NULL},
	{PROP_PLANSNUM, "plansNum", INT_T, R,NULL},
	{PROP_HBPERIOD, "hbPeriod", INT_T, R,NULL},
	{PROP_WIFI, "wifiInfo", OBJ_T, R,NULL},
	{PROP_AP, "apInfo", OBJ_T, R,NULL},
	{PROP_WIFICFG, "wifiCfg", OBJ_T, R|W,NULL},
	{PROP_LOCCFG, "locCfg", OBJ_T, R|W,NULL},
	{PROP_LOCSRC, "locSrc", INT_T, R|W,NULL},
	{PROP_CLKSRC, "clkSrc", INT_T, R|W,NULL},
	{PROP_DEVCAP, "devCap", OBJ_T, R|W,NULL},
	{PROP_NBVER, "nbVer", STR_T, R,NULL},
	{PROP_NBINFO, "nbInfo", STR_T, R,NULL},
	{PROP_SYSINFO, "sysInfo", STR_T, R,NULL},
	{PROP_NBVER, "nbPid", STR_T, R,NULL},
	{PROP_LUX, "lux", OBJ_T, R,NULL},
	{PROP_EARFCN, "earfcn", INT_T, R|W,NULL},
	{PROP_ADJELEC, "adjElec", OBJ_T, W,NULL},
	{PROP_ADJPHASE, "adjPhase", INT_T, W,NULL},
	{PROP_ADJADC64, "adjAdc64", INT_T, R|W,NULL},
	{PROP_ADJADC192, "adjAdc192", INT_T, R|W,NULL},
	{PROP_WDT, "watchdog", OBJ_T, R|W, NULL},
	{PROP_SDKVER, "sdkVer", INT_T, R, NULL},
	{PROP_BLINK, "blink", INT_T, W, NULL},
	{PROP_TDIMTYPE, "tDimType", INT_T, R|W, NULL},
	{PROP_HARDINFO, "hwInfo", OBJ_T, R, NULL},
	{PROP_RXPROTECT, "rxProtect", INT_T, R|W, NULL},
	{PROP_TELEC, "tElec", OBJ_T, R|RE, NULL},
	{PROP_ELECPARA, "elecPara", OBJ_T, R|W, NULL},
	{PROP_ATTACHDISPARA, "attachDisPara", OBJ_T, R|W, NULL},
	{PROP_ATTACHDISOFFSET, "attachDisOffset", INT_T, R, NULL},
	{PROP_ATTACHDISENABLE, "attachDisEnable", INT_T, R|W, NULL},
	{PROP_HEARTDISPARA, "heartDisPara", OBJ_T, R|W, NULL},
	{PROP_HEARTDISOFFSET, "heartDisOffset", INT_T, R, NULL},
	{PROP_TCUR, "testCur", INT_T, R|W, NULL},
	{PROP_T1, "test1", INT_T, R, NULL},
	{PROP_T2, "test2", INT_T, R|W, NULL},
	{PROP_T3, "test3", INT_T, R|W, NULL},
	{PROP_T4, "test4", INT_T, R|W, NULL},
	{PROP_T5, "test5", INT_T, R|W, NULL},
	{PROP_T6, "test6", INT_T, R|W, NULL},
	{PROP_T7, "test7", INT_T, R|W, NULL},
	{PROP_T8, "test8", INT_T, R|W, NULL},
	{PROP_NBDEVSTATETO, "nbDevStTo", INT_T, R, NULL},
	{PROP_NBREGSTATETO, "nbRegStTo", INT_T, R, NULL},
	{PROP_NBIOTSTATETO, "nbIotStTo", INT_T, R, NULL},
	{PROP_NBHARDRSTTO, "nbHardRstTo", INT_T, R, NULL},
	{PROP_NBOTATO, "nbOtaTo", INT_T, R, NULL},
	{PROP_TIME0, "time0", INT_T, R, NULL},
	{PROP_TIME1, "time1", INT_T, R, NULL},
	{PROP_SAVEHTIMETO, "saveHtimeTo", INT_T, R, NULL},
	{PROP_SCANECTO, "scanEcTo", INT_T, R, NULL},
	{PROP_SAVEHECTO, "saveHecTo", INT_T, R, NULL},
	{PROP_ONLINETO, "onlineTo", INT_T, R, NULL},
	{PROP_BC28PARA, "nbPara", OBJ_T, R|W, NULL},
};

const ST_REBOOT_FUNC g_astRebootFunc[] = {
	{REBOOT_MODULE_TEST, "test", TestReboot},
	{REBOOT_MODULE_ELEC, "elec", ElecDoReboot},	
};

const ST_FASTPROP_PARA g_stFastConfigPara[] = {
	{.resId = RSC_VOLT,.defaultIdx=VOL_PROPCFG_IDX, .resName="voltage"},
	{.resId = RSC_CURRENT,.defaultIdx=CUR_PROPCFG_IDX, .resName="current"},
	{.resId = RSC_NB_NETWORK,.defaultIdx=NET_PROPCFG_IDX, .resName="nb network"},
	{.resId = RSC_LAMP_ONOFF,.defaultIdx=SWITCH_PROPCFG_IDX, .resName="lamp onoff"},
	{.resId = RSC_LAMP_BRI,.defaultIdx=BRI_PROPCFG_IDX, .resName="bri"},
};

const ST_FASTALARM_PARA g_stFastAlarmPara[] = {
	{.alarmId = OVER_VOLALARMID, .resId = RSC_VOLT,.defaultIdx=OVER_VOL_ALARM_IDX, .alarmDesc="over voltage"},
	{.alarmId = UNDER_VOLALARMID, .resId = RSC_VOLT,.defaultIdx=UNDER_VOL_ALARM_IDX, .alarmDesc="under voltage"},
	{.alarmId = OVER_CURALARMID, .resId = RSC_CURRENT,.defaultIdx=OVER_CUR_ALARM_IDX, .alarmDesc="over current"},
	{.alarmId = UNDER_CURALARMID, .resId = RSC_CURRENT,.defaultIdx=UNDER_CUR_ALARM_IDX, .alarmDesc="under current"},
};

LIST_HEAD(listc,_list_entry) list_coap;
LIST_HEAD(listr,_list_report_entry) list_report;
LIST_HEAD(listh,_list_retry_hb_entry) list_rhb;
int TestReboot()
{
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"test reboot\r\n");
	return 0;
}

static void STRNCPY(char * dst, char * src, int len)
{
	strncpy(dst, src, len);
	if(strlen(src) > len-1){
		dst[len-1] = '\0';
	}
}

/*
*json内存已经释放，不需要再次释放内存
*/
int JsonCompres(cJSON * root, char * outBuf, U16 * outLength)
{
	char *p, *q;
	int i = 0, j = 0;

	if(NULL == outBuf)
		return OPP_FAILURE;
		
	p = cJSON_Print(root);
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%s",p);
	cJSON_Delete(root);
	q = (char *)outBuf;
	for(i=0; i<strlen(p);i++){
		if((p[i] != '\n') /*&& (p[i] != ' ') */&& (p[i] != '\t') && (p[i] != '\r')){
			q[j++]=p[i];
			if(j >= JSON_S_MAX_LEN)
				return NBIOT_SEND_BIG_ERR;
		}
	}
	myfree(p);
	*outLength = j;
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"~~~outLength is %d\r\n", *outLength);
	return OPP_SUCCESS;
}

void OppCoapMidIncrease(void)
{
	MUTEX_LOCK(m_stMidMutex,MUTEX_WAIT_ALWAYS);
	m_usMid++;
	MUTEX_UNLOCK(m_stMidMutex);
	return;
}

int OppCoapReqIdGen(void)
{
	return (++m_iReqid == 0) ? ++m_iReqid : m_iReqid; 
}

unsigned short OppCoapMidGet(void)
{
	unsigned short mid;
	MUTEX_LOCK(m_stMidMutex,MUTEX_WAIT_ALWAYS);
	mid = m_usMid;
	MUTEX_UNLOCK(m_stMidMutex);

	return mid;
}

int OppCoapSend(unsigned char dstChl, unsigned char *dstInfo, unsigned char msgType, unsigned char type, unsigned char serviceId, int errHdr, unsigned char * data, unsigned short length, unsigned int mid)
{
	MSG_ITEM_T mitem;
	ST_COAP_ENTRY *pstCoapEntry = NULL;
	int ret;
	ST_COMDIR_RSQ_HDR * pstRsqHdr = NULL;	
	ST_COMDIR_REQ_HDR * pstReqHdr = NULL;
	ST_LOC_HDR * pstLocHdr = NULL;
	U8 sendTx[JSON_S_MAX_LEN] ={0};
	int len = 0;
	U8 zeroInfo[MSG_INFO_SIZE] = {0};
	
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"OppCoapSend ch %d msgtype %d type %d service %d length %d mid %d\r\n", dstChl, msgType, type, serviceId,length,mid);
	
	APS_COAP_NULL_POINT_CHECK(data);
	
	if(CHL_NB == dstChl){
		if(RSP_T == type){
			len = length + sizeof(ST_COMDIR_RSQ_HDR)+2;/*add tail length*/
		}else if(REPORT_T == type){
			len = length + sizeof(ST_COMDIR_REQ_HDR)+2;/*add tail length*/
		}
	}else if(CHL_WIFI == dstChl){
		len = length + sizeof(ST_LOC_HDR)+2;/*add tail length*/
	}

	if(len > JSON_S_MAX_LEN){
		return NBIOT_SEND_BIG_ERR;
	}

	len = 0;
	
	if(CHL_NB == dstChl){
		if(RSP_T == type){
			pstRsqHdr = (ST_COMDIR_RSQ_HDR *)sendTx;
			pstRsqHdr->hdr = OPP_HTONS(0xFBFB);
			pstRsqHdr->comDir = DEVICE_RSP_MSG;
			pstRsqHdr->hasMore = 0;
			pstRsqHdr->serviceId = serviceId;
			pstRsqHdr->error= errHdr;
			len += sizeof(ST_COMDIR_RSQ_HDR);
		}else if(REPORT_T == type){
			pstReqHdr = (ST_COMDIR_REQ_HDR *)sendTx;
			pstReqHdr->hdr = OPP_HTONS(0xFBFB);
			pstReqHdr->comDir = DEVICE_REQ_MSG;
			pstReqHdr->hasMore = 0;
			pstReqHdr->serviceId = serviceId;
			len += sizeof(ST_COMDIR_REQ_HDR);
		}
	}else if(CHL_WIFI == dstChl){
		pstLocHdr = (ST_LOC_HDR *)sendTx;
		pstLocHdr->hdr = OPP_HTONL(0xbebebebe);
		pstLocHdr->protId = 0x01;
		pstLocHdr->protVer = 0x00;
		pstLocHdr->seqNum = OPP_HTONL(mid); /*wifi 4 bytes*/
		pstLocHdr->isRsp = 0x01;
		if(PROP_SERVICE == serviceId)
			pstLocHdr->conType = 0x01;
		else if(FUNC_SERVICE == serviceId)
			pstLocHdr->conType = 0x02;
		else if(LOG_SERVICE == serviceId)
			pstLocHdr->conType = 0x03;
		len += sizeof(ST_LOC_HDR);
	}
	memcpy(&sendTx[len],data,length);
	len += length;
	if(CHL_NB == dstChl){
		if(RSP_T == type){
			pstRsqHdr->length = OPP_HTONS(length);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"pstRsqHdr->length 0x%x\r\n", OPP_HTONS(pstRsqHdr->length));		
		}else if(REPORT_T == type){
			pstReqHdr->length = OPP_HTONS(length);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"pstReqHdr->length 0x%x\r\n", OPP_HTONS(pstReqHdr->length));		
		}
		sendTx[len++] = mid>>8 & 0xFF;
		sendTx[len++] = mid & 0xFF;
		
	}else if(CHL_WIFI == dstChl){
		pstLocHdr->crc = 0;
		pstLocHdr->dataLen = OPP_HTONS(length);		
		sendTx[len++] = 0xed;
		sendTx[len++] = 0xed;
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"pstLocHdr->dataLen 0x%x\r\n", OPP_HTONS(pstLocHdr->dataLen));		
	}
	
	mitem.type = MSG_TYPE_HUAWEI;
	mitem.len = len;
	mitem.data = sendTx;
	
	if(dstInfo && memcmp(dstInfo,zeroInfo,sizeof(zeroInfo))){
		memcpy(mitem.info,dstInfo,sizeof(mitem.info));
		//printf("send coap ip:%08x, port:%d\r\n", *(unsigned int *)mitem.info, *(unsigned short *)&mitem.info[4]);
	}
	//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"OppCoapSend AppMessageSend chl %d\r\n", mitem.chl);	
	if(ALARM_MSG == msgType)
		ret = AppMessageSend(dstChl,MSG_QUEUE_TYPE_TX,&mitem,PRIORITY_HIGH);
	else
		ret = AppMessageSend(dstChl,MSG_QUEUE_TYPE_TX,&mitem,PRIORITY_LOW);
	if(OPP_SUCCESS != ret){
		if(CHL_NB == dstChl){
			MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
			g_stCoapPkt.g_iCoapNbTxFail++;
			MUTEX_UNLOCK(g_stCoapPkt.mutex);
		}
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"OppCoapSend AppMessageSend fail ret %d\r\n", ret);
		return ret;
	}
	if(CHL_NB == dstChl){
		MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
		g_stCoapPkt.g_iCoapNbTxSuc++;
		MUTEX_UNLOCK(g_stCoapPkt.mutex);
	}	
	//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"OppCoapSend AppMessageSend succ\r\n");
	//wifi not retry send
	if(CHL_WIFI == dstChl){
		return OPP_SUCCESS;
	}

	if(g_isSupportRetry){
		//add list
		pstCoapEntry = (ST_COAP_ENTRY *)mymalloc(sizeof(ST_COAP_ENTRY));
		if(NULL == pstCoapEntry)
		{
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"malloc ST_COAP_ENTRY error\r\n");
			return OPP_FAILURE;
		}
		pstCoapEntry->tick = OppTickCountGet();
		pstCoapEntry->type = type;
		pstCoapEntry->chl = dstChl;
		pstCoapEntry->mid = mid;
		pstCoapEntry->times = RETRY_TIMES_MAX;
		pstCoapEntry->msgType = msgType;
		pstCoapEntry->length = len;
		pstCoapEntry->data = (U8 *)mymalloc(len);
		if(NULL == pstCoapEntry->data)
		{
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"malloc pstCoapEntry->data error\r\n");
			myfree(pstCoapEntry);
			return OPP_FAILURE;
		}
		//memcpy(pstCoapEntry->data, sendTx, length);
		memcpy(pstCoapEntry->data, sendTx, pstCoapEntry->length);
		if(dstInfo && memcmp(dstInfo,zeroInfo,sizeof(zeroInfo))){
			memcpy(pstCoapEntry->dstInfo,dstInfo,sizeof(pstCoapEntry->dstInfo));
		}
		MUTEX_LOCK(m_ulCoapMutex, MUTEX_WAIT_ALWAYS);	
		LIST_INSERT_HEAD(&list_coap,pstCoapEntry,elements);
		MUTEX_UNLOCK(m_ulCoapMutex);	;
	}
	
	return OPP_SUCCESS;
}


int ApsCoapRsp(unsigned char dstChl, unsigned char *dstInfo, unsigned char msgType, unsigned char isNeedRsp, int reqId, unsigned char serviceId, int errHdr, int error, char * errDesc, cJSON * prop, U32 mid)
{
    cJSON * root;
	U8 coapmsgTx[JSON_S_MAX_LEN] ={0};
	U16 length = 0;
	int ret = OPP_SUCCESS;

	//code review  wifi not need rsp, so do this here
	if(0 == isNeedRsp){
		return OPP_SUCCESS;
	}

    root =  cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_BC28,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "ApsCoapRsp create root object error\r\n");
		return OPP_FAILURE;
	}
	
	if(0 != reqId)
    	cJSON_AddItemToObject(root, "reqId", cJSON_CreateNumber(reqId));//根节点下添加
    //itoa(error,buf,10);
    //cJSON_AddItemToObject(root, "err", cJSON_CreateString(buf));
    cJSON_AddItemToObject(root, "err", cJSON_CreateNumber(error));
	if(NULL != errDesc)
		cJSON_AddItemToObject(root, "errDesc", cJSON_CreateString(errDesc));


	if(NULL != prop)
		cJSON_AddItemToObject(root,"prop", prop);

	ret = JsonCompres(root, (char *)coapmsgTx, &length);
	if(OPP_SUCCESS != ret){
		return ret;
	}
	ret = OppCoapSend(dstChl, dstInfo, msgType, RSP_T, serviceId, 0, coapmsgTx,length,mid);
	if(OPP_SUCCESS != ret)
		return ret;
	
    return OPP_SUCCESS;
}

	
int ApsCoapInit(void)
{
	int ret;
	ST_DIS_HEART_SAVE_PARA stDisHeartPara;

	MUTEX_CREATE(m_ulCoapMutex);
	MUTEX_CREATE(m_stCoapReportMutex);
	MUTEX_CREATE(m_stLogReportMutex);
	MUTEX_CREATE(m_stMidMutex);
	MUTEX_CREATE(g_stCoapPkt.mutex);
	MUTEX_CREATE(g_stHeartPara.mutex);
	MUTEX_CREATE(m_ulCoapRHB);
	MUTEX_CREATE(m_stLogStatus.mutex);
	MUTEX_CREATE(g_stRxProtect.mutex);
	MUTEX_CREATE(g_stHeartDisPara.mutex);
	
	LIST_INIT(&list_coap);
	LIST_INIT(&list_report);
	LIST_INIT(&list_rhb);
	initQueue(&queue_qlog,queue_qlog_buf,sizeof(ST_LOG_QUERY_REPORT)*QLOG_QUEUE_MAX,QLOG_QUEUE_MAX,sizeof(ST_LOG_QUERY_REPORT));

	ret = ApsCoapHeartDisParaGetFromFlash(&stDisHeartPara);
	if(OPP_SUCCESS == ret){
		ApsCoapHeartDisParaSet(&stDisHeartPara);
	}else{
		ApsCoapHeartDisParaSet(&g_stHeartDisParaDefault);
	}
	return OPP_SUCCESS;
}
/*only report to nb server, so no need dstInfo*/
int ApsCoapAlarmReport(ST_ALARM_PARA *pstAlarmPara)
{
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
    cJSON * root;
	cJSON * cmdData;
	U16 outLength = 0;
	char buffer[64]={0};
	int ret = OPP_SUCCESS;
	ST_TIME time;

	APS_COAP_NULL_POINT_CHECK(pstAlarmPara);

    root = cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_BC28,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "ApsCoapAlarmReport create root object error\r\n");
		return OPP_FAILURE;
	}
	
    cJSON_AddItemToObject(root, "cmdId", cJSON_CreateString(ALARM_CMDID));
    cmdData = cJSON_CreateObject();
	if(NULL == cmdData){
		MakeErrLog(DEBUG_MODULE_BC28,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "ApsCoapAlarmReport create cmdData object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}
    cJSON_AddItemToObject(root, "cmdData", cmdData);

    cJSON_AddItemToObject(cmdData, "id", cJSON_CreateNumber(pstAlarmPara->id));	
	Opple_Get_RTC(&time);
	sprintf(buffer, "%02hhu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",(time.Year-2000),time.Month,time.Day,time.Hour,time.Min,time.Sencond);
    cJSON_AddItemToObject(cmdData, "time", cJSON_CreateString(buffer));
    cJSON_AddItemToObject(cmdData, "alarmId", cJSON_CreateNumber(pstAlarmPara->alarmId));
    cJSON_AddItemToObject(cmdData, "status", cJSON_CreateNumber(pstAlarmPara->status));
	itoa(pstAlarmPara->value, buffer, 10);
	cJSON_AddItemToObject(cmdData, "val", cJSON_CreateString(buffer));

	ret = JsonCompres(root, (char *)coapmsgTx, &outLength);
	if(OPP_SUCCESS != ret){
		return ret;
	}
	/*only report to nb server, so no need dstInfo*/
	ret = OppCoapSend(pstAlarmPara->dstChl, pstAlarmPara->dstInfo, ALARM_MSG, REPORT_T, FUNC_SERVICE, 0, coapmsgTx, outLength, OppCoapMidGet());
	if(OPP_SUCCESS != ret)
		return ret;
	
	OppCoapMidIncrease();
	return OPP_SUCCESS;
}

/*
*type=0 主动上报，type=1查询上报
*/

int ApsCoapLogReport(ST_LOG_PARA *pstLogPara)
{
	U8 coapmsgTx[JSON_S_MAX_LEN] ={0};
	cJSON *root, *arrayPtr, *object;
	char buf[64] ={0};
	U16 outLength = 0;
	int ret = OPP_SUCCESS;
	ST_LOG_DESC logDesc;
	
	APS_COAP_NULL_POINT_CHECK(pstLogPara);
	
    root =  cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_BC28,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "ApsCoapLogReport create root object error\r\n");
		return OPP_FAILURE;
	}

	if(QUERY_REPORT == pstLogPara->type){
		cJSON_AddItemToObject(root,"reqId",cJSON_CreateNumber(pstLogPara->reqId));
		cJSON_AddItemToObject(root,"leftItems",cJSON_CreateNumber(pstLogPara->leftItems));
		cJSON_AddItemToObject(root,"err",cJSON_CreateNumber(pstLogPara->err));
		//if some err send directly
		if(pstLogPara->err){
			ret = JsonCompres(root,(char *)coapmsgTx,&outLength);
			if(OPP_SUCCESS != ret){
				return ret;
			}
			
			ret = OppCoapSend(pstLogPara->chl, pstLogPara->dstInfo, CMD_MSG, REPORT_T, LOG_SERVICE, 0, coapmsgTx, outLength, OppCoapMidGet());
			if(OPP_SUCCESS != ret){
				return ret;
			}
			if(pstLogPara->chl == CHL_NB){
				ApsCoapSetLogReportStatus(LOG_SEND_ING,OppCoapMidGet());
			}
			OppCoapMidIncrease();
			return OPP_SUCCESS;
		}
	}

	arrayPtr = cJSON_CreateArray();
	if(NULL == arrayPtr){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapLogReport create <items> array error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}
	
	cJSON_AddItemToObject(root,"items",arrayPtr);
    object =  cJSON_CreateObject();
	if(NULL == object){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapLogReport create <object> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}
	cJSON_AddItemToArray(arrayPtr, object);
	/*"logSeq": 1000,
			"logSaveId": 1,
			"logId": 1,
			"time": "YY:MM:DD HH-MM-SS",
			"module": 1,
			"level": 1,
			"logDescId": "xxx",
			"log": "v=2"*/
			
	cJSON_AddItemToObject(object, "logSeq", cJSON_CreateNumber(pstLogPara->log.seqnum));
	cJSON_AddItemToObject(object, "logSaveId", cJSON_CreateNumber(pstLogPara->logSaveId));
	cJSON_AddItemToObject(object, "logId", cJSON_CreateNumber(pstLogPara->log.id));
	sprintf(buf,"%02d-%02d-%02d %02d:%02d:%02d", 
		pstLogPara->log.time[0], pstLogPara->log.time[1], pstLogPara->log.time[2], pstLogPara->log.time[3], pstLogPara->log.time[4], pstLogPara->log.time[5]);
	cJSON_AddItemToObject(object, "time", cJSON_CreateString(buf));
	ret = ApsDaemonLogModuleNameGet(pstLogPara->log.module, buf);
	if(OPP_SUCCESS == ret){
		cJSON_AddItemToObject(object, "module", cJSON_CreateString(buf));
	}
	ret = ApsDaemonLogLevelGet(pstLogPara->log.level,buf);
	if(OPP_SUCCESS == ret){
		cJSON_AddItemToObject(object, "level", cJSON_CreateString(buf));
	}
	//获取logDescId
	ret = ApsLogAction(&pstLogPara->log,&logDesc);
	if(OPP_SUCCESS == ret){
		cJSON_AddItemToObject(object, "logDescId", cJSON_CreateNumber(logDesc.logDescId));
		cJSON_AddItemToObject(object, "log", cJSON_CreateString(logDesc.log));
	}
	ret = JsonCompres(root,(char *)coapmsgTx,&outLength);
	if(OPP_SUCCESS != ret){
		return ret;
	}

	ret = OppCoapSend(pstLogPara->chl, pstLogPara->dstInfo, CMD_MSG, REPORT_T, LOG_SERVICE, 0, coapmsgTx, outLength, OppCoapMidGet());
	if(OPP_SUCCESS != ret){
		return ret;
	}
	if(pstLogPara->chl == CHL_NB){
		ApsCoapSetLogReportStatus(LOG_SEND_ING, OppCoapMidGet());
	}
	OppCoapMidIncrease();

	return OPP_SUCCESS;
}

void ApsCoapLogBatchStart()
{
#if 0
	memset(logTx, 0, sizeof(logTx));

	g_ucLogItems = 0;
	//g_usLogLength += sizeof(ST_COMDIR_REQ_HDR);
	g_usLogLength = 0;
#else
	if(NULL == g_pstLogArray){
		g_pstLogArray =	cJSON_CreateArray();
		if(NULL == g_pstLogArray){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapLogBatchStart create <g_pstLogArray> array error\r\n");
			return;
		}
	}
#endif
	return;
}

int ApsCoapJudgeJsonLength(cJSON *json, unsigned int * length)
{
	char *p;
	unsigned int i=0,j=0;
	
	p = cJSON_Print(json);
	for(i=0; i<strlen(p);i++){
		if((p[i] != '\n') /*&& (p[i] != ' ') */&& (p[i] != '\t') && (p[i] != '\r'))
			j++;
	}
	*length = j;
	myfree(p);

	return OPP_SUCCESS;
}

// 0:success,1:full,2:fail
int ApsCoapLogBatchAppend(ST_LOG_APPEND_PARA * para) // 0:success,1:full,2:fail
{
	char buf[64] = {0};
	cJSON *object;
	int len = 0;
	ST_LOG_DESC logDesc;
	int ret = OPP_SUCCESS;
	unsigned int length = 0, length1;
	ST_LOG_STOP_PARA stopPara;
	
	if(NULL == para)
	{
		return 2;
	}

	MUTEX_LOCK(m_stLogReportMutex, MUTEX_WAIT_ALWAYS);
	if(NULL != g_pstLogRoot){
		ApsCoapJudgeJsonLength(g_pstLogRoot, &length);
		if(CHL_WIFI == para->dstChl){
			len = length+sizeof(ST_LOC_HDR);
		}
		if(CHL_NB == para->dstChl){
			len = length+sizeof(ST_COMDIR_REQ_HDR);
		}
		if(len > JSON_S_MAX_LEN-2){/*新加json节点格式长度,\r*/
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n----ApsCoapLogBatchAppend already len %d too big-----\r\n", len);			
			MUTEX_UNLOCK(m_stLogReportMutex);
			
			//if full then send
			stopPara.dstChl = CHL_NB;
			stopPara.leftItems = 0;
			stopPara.reqId = 0;
			stopPara.type = ACTIVE_REPROT;
			ApsCoapLogBatchStop(&stopPara);
			return 1;
		}
	}
	
	if(NULL == g_pstLogRoot){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n----ApsCoapLogBatchAppend g_pstLogRoot-----\r\n");	
		g_pstLogRoot= cJSON_CreateObject();
		if(NULL == g_pstLogRoot){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n----ApsCoapLogBatchAppend g_pstLogRoot= cJSON_CreateObject()-----\r\n");	
			MUTEX_UNLOCK(m_stLogReportMutex);
			return 2;
		}
		if(NULL == g_pstLogArray){
			g_pstLogArray = cJSON_CreateArray();
			if(NULL == g_pstLogArray){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n----ApsCoapLogBatchAppend g_pstLogArray = cJSON_CreateArray()-----\r\n");	
				MUTEX_UNLOCK(m_stLogReportMutex);
				return 2;
			}
		}
		cJSON_AddItemToObject(g_pstLogRoot, "items", g_pstLogArray);
	}

	object = cJSON_CreateObject();
	if(NULL == object){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n----ApsCoapLogBatchAppend object=cJSON_CreateObject()-----\r\n");	
		MUTEX_UNLOCK(m_stLogReportMutex);
		return 2;
	}

	//cJSON_AddItemToArray(g_pstLogArray, object);
	cJSON_AddItemToObject(object, "logSeq", cJSON_CreateNumber(para->log.seqnum));
	cJSON_AddItemToObject(object, "logSaveId", cJSON_CreateNumber(para->logSaveId));
	cJSON_AddItemToObject(object, "logId", cJSON_CreateNumber(para->log.id));	
	sprintf(buf,"%02d-%02d-%02d %02d:%02d:%02d", 
		para->log.time[0], para->log.time[1], para->log.time[2], para->log.time[3], para->log.time[4], para->log.time[5]);
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n----%s-----\r\n", buf);	
	cJSON_AddItemToObject(object, "time", cJSON_CreateString(buf));
	ret = ApsDaemonLogModuleNameGet(para->log.module, buf);
	if(OPP_SUCCESS == ret){
		cJSON_AddItemToObject(object, "module", cJSON_CreateString(buf));
	}
	ret = ApsDaemonLogLevelGet(para->log.level,buf);
	if(OPP_SUCCESS == ret){
		cJSON_AddItemToObject(object, "level", cJSON_CreateString(buf));
	}
	ret = ApsLogAction(&para->log, &logDesc);
	if(OPP_SUCCESS == ret){
		cJSON_AddItemToObject(object, "logDescId", cJSON_CreateNumber(logDesc.logDescId));
		cJSON_AddItemToObject(object, "log", cJSON_CreateString(logDesc.log));
	}
	ApsCoapJudgeJsonLength(object,&length1);
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n----ApsCoapLogBatchAppend already len %d len1 %d-----\r\n", length, length1);			

	if(length+length1 > JSON_S_MAX_LEN-2){/*新加json节点格式长度,\r*/
		cJSON_Delete(object);
		MUTEX_UNLOCK(m_stLogReportMutex);
		//if full then send
		stopPara.dstChl = CHL_NB;
		stopPara.leftItems = 0;
		stopPara.reqId = 0;
		stopPara.type = ACTIVE_REPROT;
		ApsCoapLogBatchStop(&stopPara);
		return 1;
	}else{
		cJSON_AddItemToArray(g_pstLogArray, object);
	}
	
	MUTEX_UNLOCK(m_stLogReportMutex);
	return OPP_SUCCESS;
}

/*
*type=0 主动上报，type=1查询上报
*/

int ApsCoapLogBatchStop(ST_LOG_STOP_PARA * para)
{
	int ret;
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
	U16 usOutLength = 0;
	U32 ulOutLength = 0;
	int len = 0;

	APS_COAP_NULL_POINT_CHECK(para);
	
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ApsCoapLogBatchStop");
	if(NULL == g_pstLogRoot){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ApsCoapLogBatchStop g_pstLogRoot error\r\n");
		return OPP_FAILURE;
	}
	
	if(para->type == QUERY_REPORT){
		cJSON_AddItemToObject(g_pstLogRoot, "reqId", cJSON_CreateNumber(para->reqId));
		cJSON_AddItemToObject(g_pstLogRoot, "leftItems", cJSON_CreateNumber(para->leftItems));
	}
	MUTEX_LOCK(m_stLogReportMutex, MUTEX_WAIT_ALWAYS);
	ApsCoapJudgeJsonLength(g_pstLogRoot,&ulOutLength);
	
	if(CHL_WIFI == para->dstChl){
		len = ulOutLength+sizeof(ST_LOC_HDR);
	}
	if(CHL_NB == para->dstChl){
		len = ulOutLength+sizeof(ST_COMDIR_REQ_HDR);
	}
	if(len > JSON_S_MAX_LEN){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n----ApsCoapLogBatchStop len %d too big-----\r\n", len);			
		cJSON_Delete(g_pstLogRoot);
		g_pstLogRoot = NULL;
		g_pstLogArray = NULL;
		ApsCoapFuncRsp(para->dstChl,para->dstInfo,1,LOG_SERVICE,LOG_CMDID,0,0,NBIOT_SEND_BIG_ERR,NBIOT_SEND_BIG_ERR_DESC,NULL,OppCoapMidGet());
		MUTEX_UNLOCK(m_stLogReportMutex);
		return OPP_FAILURE;
	}
	
	ret = JsonCompres(g_pstLogRoot,(char *)coapmsgTx,&usOutLength);
	if(OPP_SUCCESS != ret){
		MUTEX_UNLOCK(m_stLogReportMutex);
		return ret;
	}

	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n----ApsCoapLogBatchStop outLength %d-----\r\n", usOutLength);			

	
	ret = OppCoapSend(para->dstChl, para->dstInfo, CMD_MSG, REPORT_T, LOG_SERVICE, 0, coapmsgTx, usOutLength, OppCoapMidGet());
	if(OPP_SUCCESS != ret){
		g_pstLogRoot = NULL;
		g_pstLogArray = NULL;
		MUTEX_UNLOCK(m_stLogReportMutex);
		return ret;
	}
	if(para->dstChl == CHL_NB){
		ApsCoapSetLogReportStatus(LOG_SEND_ING, OppCoapMidGet());
	}	
	OppCoapMidIncrease();

	g_pstLogRoot = NULL;
	g_pstLogArray = NULL;
	MUTEX_UNLOCK(m_stLogReportMutex);
	return OPP_SUCCESS;
}

int ApsCopaLogReporterIsBusy(void) // 0:idle,1:busy
{
	U16 status, mid;
	
	//cmd line control busy or not busy
	MUTEX_LOCK(g_stCoapPkt.mutex,MUTEX_WAIT_ALWAYS);
	if(g_stCoapPkt.g_iCoapBusy){
		MUTEX_UNLOCK(g_stCoapPkt.mutex);
		return 1;
	}
	MUTEX_UNLOCK(g_stCoapPkt.mutex);
	
	//return 0;
	ApsCoapGetLogReportStatus(&status,&mid);
	if(status == LOG_SEND_INIT)
		return 0;
	else
		return 1;

	return 0;
}


int ApsCoapOtaProcessRsp(ST_OPP_OTA_PROC * pstOtaProcess)
{
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
    cJSON * root;
	cJSON * cmdData;
	U16 outLength = 0;
	int ret = OPP_SUCCESS;
	
	APS_COAP_NULL_POINT_CHECK(pstOtaProcess);

    root =  cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapOtaProcessRsp create root object error\r\n");
		return OPP_FAILURE;
	}
	
	if(0 != pstOtaProcess->reqId)
    	cJSON_AddItemToObject(root, "reqId", cJSON_CreateNumber(pstOtaProcess->reqId));
    cJSON_AddItemToObject(root, "cmdId", cJSON_CreateString(OTAUDP_CMDID));
	if(OTA_REPORT == pstOtaProcess->type){
		cJSON_AddItemToObject(root, "op", cJSON_CreateString("RE"));
	}
    cmdData =  cJSON_CreateObject();
	if(NULL == cmdData){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapOtaProcessRsp create <cmdData> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}	
    cJSON_AddItemToObject(root, "cmdData", cmdData);
    cJSON_AddItemToObject(cmdData, "err", cJSON_CreateNumber(pstOtaProcess->error));
    cJSON_AddItemToObject(cmdData, "status", cJSON_CreateNumber(pstOtaProcess->state));
    cJSON_AddItemToObject(cmdData, "process", cJSON_CreateNumber(pstOtaProcess->process));
	cJSON_AddItemToObject(cmdData, "version", cJSON_CreateString(pstOtaProcess->version));

	ret = JsonCompres(root, (char *)coapmsgTx, &outLength);
	if(OPP_SUCCESS != ret){
		return ret;
	}
	
	if(OTA_RSP == pstOtaProcess->type){
		ret = OppCoapSend(pstOtaProcess->dstChl, pstOtaProcess->dstInfo, CMD_MSG, RSP_T, FUNC_SERVICE, 0, coapmsgTx, outLength, pstOtaProcess->mid);
		if(OPP_SUCCESS != ret)
			return ret;
	}
	if(OTA_REPORT == pstOtaProcess->type){
		ret = OppCoapSend(pstOtaProcess->dstChl, pstOtaProcess->dstInfo, CMD_MSG, REPORT_T, FUNC_SERVICE, 0, coapmsgTx, outLength, OppCoapMidGet());
		if(OPP_SUCCESS != ret)
			return ret;
		OppCoapMidIncrease();
	}
	return OPP_SUCCESS;
}

/*
int ApsCoapPropReport(U8 dstChl, U8 *data, U16 length)
{
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
	int ret = OPP_SUCCESS;

	APS_COAP_NULL_POINT_CHECK(data);
	
	memcpy((char *)coapmsgTx, (char *)data, length);

	ret = OppCoapSend(dstChl, CMD_MSG, REPORT_T, PROP_SERVICE, 0, coapmsgTx, length, OppCoapMidGet());
	if(OPP_SUCCESS != ret)
		return ret;
	
	OppCoapMidIncrease();
	
	return OPP_SUCCESS;
}
*/

int ApsCoapRIRsp(U8 dstChl, unsigned char *dstInfo, U8 isNeedRsp, U32 reqId, U8 errHdr, U8 error, char * errDesc, U32 mid)
{
    cJSON * root =  NULL;
	cJSON * cmdData, *comObj;	
	char buffer[64] = {0};
	U16 outLength = 0;
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
	int ret = OPP_SUCCESS;
	ST_OPP_LAMP_CURR_ELECTRIC_INFO stElecInfo;
	U8 lampSwitch;
	U16 usBri;
	U32 rTime, hTime;
	long double lat,lng;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	ST_NEUL_DEV *pstNeulDev = NULL;

	if(0 == isNeedRsp){
		return OPP_SUCCESS;
	}
	
    root =  cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapRIRsp create <root> object error\r\n");
		return OPP_FAILURE;
	}

	if(0 != reqId)
    	cJSON_AddItemToObject(root, "reqId", cJSON_CreateNumber(reqId));
    cJSON_AddItemToObject(root, "err", cJSON_CreateNumber(error));
	if(NULL != errDesc)
		cJSON_AddItemToObject(root, "errDesc", cJSON_CreateString(errDesc));
    cJSON_AddItemToObject(root, "cmdId", cJSON_CreateString(PATROL_CMDID));
	
	
	cmdData = cJSON_CreateObject();
	if(NULL == cmdData){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapRIRsp create <cmdData> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}

    cJSON_AddItemToObject(root, "cmdData", cmdData);
	OppLampCtrlGetSwitch(0, &lampSwitch);
    cJSON_AddItemToObject(cmdData, "switch", cJSON_CreateNumber(lampSwitch));
	OppLampCtrlGetBri(0,&usBri);
    cJSON_AddItemToObject(cmdData, "bri", cJSON_CreateNumber(usBri));

	/*
	//loc	
	LocGetLat(&lat);
	LocGetLng(&lng);
	//LocationRead(GPS_LOC, &stLocation);
	//ftoa(buffer,lat,6);	
	snprintf(buffer,64,"%Lf",lat);
	
    comObj =  cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapRIRsp create <comObj> object error\r\n");
		cJSON_Delete(root);		
		return OPP_FAILURE;
	}
	
	cJSON_AddItemToObject(cmdData, "loc", comObj);
	cJSON_AddItemToObject(comObj, "lat", cJSON_CreateString(buffer));
	//ftoa(buffer,lng,6);
	snprintf(buffer,64,"%Lf",lng);
	cJSON_AddItemToObject(comObj, "lng", cJSON_CreateString(buffer));
	*/
	//elec
    comObj =  cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapRIRsp create <comObj> object error\r\n");
		cJSON_Delete(root);		
		return OPP_FAILURE;
	}
	ElecGetElectricInfo(&stElecInfo);
	cJSON_AddItemToObject(cmdData, "elec", comObj);
	cJSON_AddItemToObject(comObj, "current", cJSON_CreateNumber(stElecInfo.current));							
	cJSON_AddItemToObject(comObj, "voltage", cJSON_CreateNumber(stElecInfo.voltage));							
	cJSON_AddItemToObject(comObj, "factor", cJSON_CreateNumber(stElecInfo.factor));							
	cJSON_AddItemToObject(comObj, "power", cJSON_CreateNumber(stElecInfo.power));							
	//ecInfo
    comObj =  cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapRIRsp create <comObj> object error\r\n");
		cJSON_Delete(root);		
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(cmdData, "ecInfo", comObj);
	cJSON_AddItemToObject(comObj, "EC", cJSON_CreateNumber(stElecInfo.consumption));							
	cJSON_AddItemToObject(comObj, "HEC", cJSON_CreateNumber(stElecInfo.hisConsumption));

	//查询信号强度
	//NeulBc28QueryUestats(&g_stThisLampInfo.stNeulSignal);
	//ret = sendEvent(UESTATE_EVENT,RISE_STATE,sArgc,sArgv);
	//ret = recvEvent(UESTATE_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	/*if(0 != ret){
		pstNeulDev = &g_stThisLampInfo.stNeulSignal;
		
	}else{
		pstNeulDev = (ST_NEUL_DEV *)rArgv[0];
	}*/
	//pstNeulDev = (ST_NEUL_DEV *)rArgv[0];
	//nbBase
	//cJSON_AddItemToObject(cmdData, "nbBase", comObj = cJSON_CreateObject());
	//cJSON_AddItemToObject(comObj, "imei", cJSON_CreateString((char *)pstNeulDev->imei));
	//cJSON_AddItemToObject(comObj, "imsi", cJSON_CreateString((char *)pstNeulDev->imsi));
	//cJSON_AddItemToObject(comObj, "iccid", cJSON_CreateString((char *)pstNeulDev->nccid));
	//if(strlen((char *)pstNeulDev->pdpAddr0))
	//	cJSON_AddItemToObject(comObj, "nbip", cJSON_CreateString((char *)pstNeulDev->pdpAddr0));
	//else
	//	cJSON_AddItemToObject(comObj, "nbip", cJSON_CreateString((char *)pstNeulDev->pdpAddr1));
	//nbSignal
	/*cJSON_AddItemToObject(cmdData, "nbSignal", comObj = cJSON_CreateObject());
	itoa(pstNeulDev->rsrp, buffer, 10);
	cJSON_AddItemToObject(comObj, "rsrp", cJSON_CreateString(buffer));
	itoa(pstNeulDev->rsrq, buffer, 10);						
	cJSON_AddItemToObject(comObj, "rsrq", cJSON_CreateString(buffer));
	cJSON_AddItemToObject(comObj, "ecl", cJSON_CreateNumber(pstNeulDev->signalEcl));
	cJSON_AddItemToObject(comObj, "cellid", cJSON_CreateNumber(pstNeulDev->cellId));
	cJSON_AddItemToObject(comObj, "pci", cJSON_CreateNumber(pstNeulDev->signalPci));
	itoa(pstNeulDev->snr, buffer, 10);						
	cJSON_AddItemToObject(comObj, "snr", cJSON_CreateString(buffer));
	cJSON_AddItemToObject(comObj, "earfcn", cJSON_CreateNumber(pstNeulDev->earfcn));
	ARGC_FREE(rArgc,rArgv);*/

	//nbCob
	/*cJSON_AddItemToObject(cmdData, "nbCon", comObj = cJSON_CreateObject());
	NeulBc28QueryNetstats(&regStatus, &conStatus);
	cJSON_AddItemToObject(comObj, "con", cJSON_CreateNumber(conStatus));
	cJSON_AddItemToObject(comObj, "reg", cJSON_CreateNumber(regStatus));*/
	
	//runtime
	OppLampCtrlGetRtime(0,&rTime);
	OppLampCtrlGetHtime(0,&hTime);
    comObj =  cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapRIRsp create <comObj> object error\r\n");
		cJSON_Delete(root);		
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(cmdData, "runtime",comObj);
	cJSON_AddItemToObject(comObj, "hTime", cJSON_CreateNumber(hTime)); 						
	cJSON_AddItemToObject(comObj, "rTime", cJSON_CreateNumber(rTime));							
	//nb signal
	ret = sendEvent(UESTATE_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(UESTATE_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 == ret){
		pstNeulDev = (ST_NEUL_DEV *)rArgv[0];

		comObj =  cJSON_CreateObject();
		if(NULL == comObj){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapRIRsp create <comObj> object error\r\n");
			cJSON_Delete(root); 	
			return OPP_FAILURE;
		}		
		cJSON_AddItemToObject(cmdData, "nbSignalCore", comObj);
		cJSON_AddItemToObject(comObj, "rsrp", cJSON_CreateNumber(pstNeulDev->rsrp));							
		cJSON_AddItemToObject(comObj, "snr", cJSON_CreateNumber(pstNeulDev->snr));
		cJSON_AddItemToObject(comObj, "cellid", cJSON_CreateNumber(pstNeulDev->cellId));	
	}
	ARGC_FREE(rArgc,rArgv);
	
	ret = JsonCompres(root, (char *)coapmsgTx, &outLength);
	if(OPP_SUCCESS != ret){
		return ret;
	}	
	ret = OppCoapSend(dstChl,dstInfo,CMD_MSG,RSP_T,FUNC_SERVICE, 0, coapmsgTx,outLength,mid);
	if(OPP_SUCCESS != ret)
		return ret;
	
	return OPP_SUCCESS;
}

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
int ApsCoapFuncRspBatchStart(unsigned char dstChl, U32 reqId, U8 errHdr, U8 error, char *errDesc, char * cmdId, cJSON ** root, cJSON ** cmdData)
{
    *root =  cJSON_CreateObject();
	if(NULL == *root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapFuncRspBatchStart create <root> object error\r\n");
		return OPP_FAILURE;
	}

	if(0 != reqId)
    	cJSON_AddItemToObject(*root, "reqId", cJSON_CreateNumber(reqId));//根节点下添加
    cJSON_AddItemToObject(*root, "cmdId", cJSON_CreateString(cmdId));
    cJSON_AddItemToObject(*root, "err", cJSON_CreateNumber(error));
	if(NULL != errDesc)
    	cJSON_AddItemToObject(*root, "errDesc", cJSON_CreateString(errDesc));
    *cmdData =  cJSON_CreateObject();
	if(NULL == *cmdData){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapFuncRspBatchStart create <cmdData> object error\r\n");
		cJSON_Delete(*root);
		return OPP_FAILURE;
	}
    cJSON_AddItemToObject(*root, "cmdData", *cmdData);

	return OPP_SUCCESS;
}

int ApsCoapWorkPlanRspBatchAppend(cJSON * root, cJSON * cmdData, cJSON * planArray,ST_WORK_SCHEME *pstPlanScheme)
{
	int i = 0;
	char buffer[64] = {0};
	char week[8] ={0};	
	cJSON *jobs, *row, *plans, *valide, *object;

	if(NULL == pstPlanScheme){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapWorkPlanRspBatchAppend pstPlanScheme is NULL\r\n");
		return OPP_NULL_POINTER;
	}
	
    object =  cJSON_CreateObject();
	if(NULL == object){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapWorkPlanRspBatchAppend create <object> object error\r\n");
		return OPP_FAILURE;
	}
	cJSON_AddItemToArray(planArray, object);
    cJSON_AddItemToObject(object, "id", cJSON_CreateNumber(pstPlanScheme->hdr.index));
    cJSON_AddItemToObject(object, "type", cJSON_CreateNumber(pstPlanScheme->hdr.mode));
    cJSON_AddItemToObject(object, "level", cJSON_CreateNumber(pstPlanScheme->hdr.priority));
    cJSON_AddItemToObject(object, "valid", cJSON_CreateNumber(pstPlanScheme->hdr.valid));
    plans =  cJSON_CreateObject();
	if(NULL == plans){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapWorkPlanRspBatchAppend create <plans > object error\r\n");
		cJSON_Delete(object);
		return OPP_FAILURE;
	}	
    cJSON_AddItemToObject(object, "plans", plans);

	if(WEEK_MODE == pstPlanScheme->hdr.mode){
		if(pstPlanScheme->u.stWeekPlan.dateValide){
			valide =  cJSON_CreateObject();
			if(NULL == valide){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapWorkPlanRspBatchAppend create <valide > object error\r\n");
				cJSON_Delete(object);
				return OPP_FAILURE;
			}	
			cJSON_AddItemToObject(plans,"validDate", valide);
			sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
				pstPlanScheme->u.stWeekPlan.sDate.Year,
				pstPlanScheme->u.stWeekPlan.sDate.Month,
				pstPlanScheme->u.stWeekPlan.sDate.Day,
				pstPlanScheme->u.stWeekPlan.sDate.Hour,
				pstPlanScheme->u.stWeekPlan.sDate.Min,
				pstPlanScheme->u.stWeekPlan.sDate.Sencond);
			cJSON_AddItemToObject(valide, "sDate", cJSON_CreateString(buffer));
			sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
				pstPlanScheme->u.stWeekPlan.eDate.Year,
				pstPlanScheme->u.stWeekPlan.eDate.Month,
				pstPlanScheme->u.stWeekPlan.eDate.Day,
				pstPlanScheme->u.stWeekPlan.eDate.Hour,
				pstPlanScheme->u.stWeekPlan.eDate.Min,
				pstPlanScheme->u.stWeekPlan.eDate.Sencond);
			cJSON_AddItemToObject(valide, "eDate", cJSON_CreateString(buffer));

		}
		for(i=0;i<7;i++){
			if(pstPlanScheme->u.stWeekPlan.bitmapWeek & 1<<i){
				//cnt++;
				week[i] = '1';
			}
			else{
				week[i] = '0';
			}
		}
		week[i] = '\0';
		//cJSON_AddItemToObject(plans,"week", cJSON_CreateIntArray(week,7));
		cJSON_AddItemToObject(plans,"week", cJSON_CreateString(week));
	}
	else if(YMD_MODE == pstPlanScheme->hdr.mode
		||AST_MODE == pstPlanScheme->hdr.mode){	
		sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
			pstPlanScheme->u.stYmdPlan.sDate.Year,
			pstPlanScheme->u.stYmdPlan.sDate.Month,
			pstPlanScheme->u.stYmdPlan.sDate.Day,
			pstPlanScheme->u.stYmdPlan.sDate.Hour,
			pstPlanScheme->u.stYmdPlan.sDate.Min,
			pstPlanScheme->u.stYmdPlan.sDate.Sencond);
		cJSON_AddItemToObject(plans, "sDate", cJSON_CreateString(buffer));
		sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
			pstPlanScheme->u.stYmdPlan.eDate.Year,
			pstPlanScheme->u.stYmdPlan.eDate.Month,
			pstPlanScheme->u.stYmdPlan.eDate.Day,
			pstPlanScheme->u.stYmdPlan.eDate.Hour,
			pstPlanScheme->u.stYmdPlan.eDate.Min,
			pstPlanScheme->u.stYmdPlan.eDate.Sencond);
		cJSON_AddItemToObject(plans, "eDate", cJSON_CreateString(buffer));
	}

	if(pstPlanScheme->jobNum > 0){
		jobs=cJSON_CreateArray();
		if(NULL == jobs){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapWorkPlanRspBatchAppend create <jobs> array error\r\n");
			return OPP_FAILURE;
		}
		cJSON_AddItemToObject(plans,"jobs",jobs);
		for(i=0;i<JOB_MAX;i++)
		{
			if(1 != pstPlanScheme->jobs[i].used){
				continue;
			}
			if(AST_MODE == pstPlanScheme->hdr.mode){
					row = cJSON_CreateObject();
					if(NULL == row){
						MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
						DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapWorkPlanRspBatchAppend create <row> object error\r\n");		
						cJSON_Delete(object);
						return OPP_FAILURE;
					}
					cJSON_AddItemToArray(jobs, row); 
					itoa(pstPlanScheme->jobs[i].intervTime,buffer,10);
					cJSON_AddItemToObject(row, "min", cJSON_CreateString(buffer));
					itoa(pstPlanScheme->jobs[i].dimLevel,buffer,10);
					cJSON_AddItemToObject(row, "bri", cJSON_CreateString(buffer));
			}
			else{
				row = cJSON_CreateObject();
				if(NULL == row){
					MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapWorkPlanRspBatchAppend create <row> object error\r\n");		
					cJSON_Delete(object);
					return OPP_FAILURE;
				}
				cJSON_AddItemToArray(jobs, row); 
				sprintf(buffer, "%02d:%02d", pstPlanScheme->jobs[i].startHour,pstPlanScheme->jobs[i].startMin);
				cJSON_AddItemToObject(row, "sTime", cJSON_CreateString(buffer));
				sprintf(buffer, "%02d:%02d", pstPlanScheme->jobs[i].endHour,pstPlanScheme->jobs[i].endMin);
				cJSON_AddItemToObject(row, "eTime", cJSON_CreateString(buffer));
				itoa(pstPlanScheme->jobs[i].dimLevel,buffer,10);
				cJSON_AddItemToObject(row, "bri", cJSON_CreateString(buffer));
			}
		}
	}
	return OPP_SUCCESS;
}
int ApsCoapFindPropMapIdx(int resId, int *index)
{
	int i = 0;

	for(i=0;i<sizeof(aucPropMap)/sizeof(ST_PROP_MAP);i++){
		if(aucPropMap[i].id == resId){
			*index = i;
			return OPP_SUCCESS;
		}
	}

	return OPP_FAILURE;
}

int ApsCoapFindResMapIdx(int resId, int *index)
{
	int i = 0;

	for(i=0;i<sizeof(aucResMap)/sizeof(ST_RES_MAP);i++){
		if(aucResMap[i].id == resId){
			*index = i;
			return OPP_SUCCESS;
		}
	}

	return OPP_FAILURE;
}
int ApsCoapThresholdRspBatchAppend(cJSON * root, cJSON * cmdData, cJSON * batArray, unsigned int index, unsigned int type, AD_CONFIG_T *pstConfig)
{
	cJSON *item, *para;
	cJSON *alarm, *update;
	int ret, idx;
	
	if(NULL == pstConfig){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_NULL_POINTER);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapThresholdRspBatchAppend pstConfig is NULL\r\n");
		return OPP_FAILURE;
	}
	
	item = cJSON_CreateObject();
	if(NULL == item){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapThresholdRspBatchAppend create <item> object error\r\n");		
		return OPP_FAILURE;
	}
	cJSON_AddItemToArray(batArray,item);
    cJSON_AddItemToObject(item, "id", cJSON_CreateNumber(index));
    cJSON_AddItemToObject(item, "target", cJSON_CreateNumber(pstConfig->targetid));
    cJSON_AddItemToObject(item, "resId", cJSON_CreateNumber(pstConfig->resource));
	ret = ApsCoapFindPropMapIdx(pstConfig->resource, &idx);
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"pstConfig resId %d idx %d\r\n", pstConfig->resource, idx);
	
	if(OPP_SUCCESS == ret){
		if(0 != strcmp((aucPropMap[idx].comPropName), ""))
			cJSON_AddItemToObject(item, "propName", cJSON_CreateString(aucPropMap[idx].comPropName));
		else 
			cJSON_AddItemToObject(item, "propName", cJSON_CreateString(aucPropMap[idx].propName));
	}else{
		cJSON_AddItemToObject(item, "propName", cJSON_CreateString(""));
	}
	
	if(CUSTOM_ALARM == type || CUSTOM_ALL == type){
		alarm = cJSON_CreateObject();
		if(NULL == alarm){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapThresholdRspBatchAppend create <alarm> object error\r\n");		
			cJSON_Delete(item);
			return OPP_FAILURE;
		}
		cJSON_AddItemToObject(item, "alarm", alarm);
		cJSON_AddItemToObject(alarm, "alarmId", cJSON_CreateNumber(pstConfig->alarmId));
	    cJSON_AddItemToObject(alarm, "condition", cJSON_CreateNumber(1));
		para = cJSON_CreateObject();
		if(NULL == para){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapThresholdRspBatchAppend create <para> object error\r\n");		
			cJSON_Delete(item);
			return OPP_FAILURE;
		}
	    cJSON_AddItemToObject(alarm, "para", para);		
		ret = ApsCoapFindResMapIdx(pstConfig->resource, &idx);
		if(OPP_SUCCESS == ret){
	    	cJSON_AddItemToObject(para, "paraType", cJSON_CreateNumber(aucResMap[idx].type));
		}else{
			cJSON_AddItemToObject(para, "paraType", cJSON_CreateNumber(4));
		}
	    cJSON_AddItemToObject(para, "paraOp", cJSON_CreateNumber(pstConfig->alarmIf));
	    cJSON_AddItemToObject(para, "value", cJSON_CreateNumber(pstConfig->alarmIfPara1));
		if(pstConfig->alarmIf == 4){
			cJSON_AddItemToObject(para, "value2", cJSON_CreateNumber(pstConfig->alarmIfPara2));
		}
    	cJSON_AddItemToObject(para, "isAlarmReport", cJSON_CreateNumber(pstConfig->enable&MCA_ALARM?1:0));
    	cJSON_AddItemToObject(para, "isLogEnable", cJSON_CreateNumber(pstConfig->enable&MCA_LOG?1:0));
	}
	if(CUSTOM_REPORT == type || CUSTOM_ALL == type){
		update=cJSON_CreateObject();
		if(NULL == update){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapThresholdRspBatchAppend create <update> object error\r\n");		
			cJSON_Delete(item);
			return OPP_FAILURE;
		}
		cJSON_AddItemToObject(item, "propConfig", update);
	    cJSON_AddItemToObject(update, "isReportEnable", cJSON_CreateNumber(pstConfig->enable&MCA_REPORT?1:0));
	    cJSON_AddItemToObject(update, "updateType", cJSON_CreateNumber(pstConfig->reportIf));
		ret = ApsCoapFindResMapIdx(pstConfig->resource, &idx);
		if(OPP_SUCCESS == ret){
	    	cJSON_AddItemToObject(update, "paraType", cJSON_CreateNumber(aucResMap[idx].type));
		}else{
			cJSON_AddItemToObject(update, "paraType", cJSON_CreateNumber(4));
		}
	    cJSON_AddItemToObject(update, "value", cJSON_CreateNumber(pstConfig->reportIfPara));
	}
	return OPP_SUCCESS;
}

int ApsCoapFuncRspBatchStop(unsigned char dstChl, unsigned char *dstInfo, cJSON * root, U32 mid)
{
	U16 outLength = 0;
	int ret = OPP_SUCCESS;
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};

	APS_COAP_NULL_POINT_CHECK(root);
	
	ret = JsonCompres(root, (char *)&coapmsgTx, &outLength);
	if(OPP_SUCCESS != ret){
		return ret;
	}	
	
	ret = OppCoapSend(dstChl, dstInfo, CMD_MSG, RSP_T, FUNC_SERVICE,0,coapmsgTx,outLength,mid);
	if(OPP_SUCCESS != ret)
		return ret;
	return OPP_SUCCESS;
}

int ApsCoapFuncRsp(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, unsigned char serviceId, char * cmdId, U32 reqId, U8 errHdr, int error, char * errDesc, cJSON *cmdData, U32 mid)
{
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
    cJSON * root;
	U16 outLength = 0;
	int ret = OPP_SUCCESS;

	if(0 == isNeedRsp){
		return OPP_SUCCESS;
	}

	root=cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create <root> object error\r\n");		
		ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	
	if(0 != reqId)
    	cJSON_AddItemToObject(root, "reqId", cJSON_CreateNumber(reqId));
	//itoa(error, buffer, 10);
    //cJSON_AddItemToObject(root, "err", cJSON_CreateString(buffer));
    cJSON_AddItemToObject(root, "err", cJSON_CreateNumber(error));
	if(NULL != errDesc)
		cJSON_AddItemToObject(root, "errDesc", cJSON_CreateString(errDesc));
    cJSON_AddItemToObject(root, "cmdId", cJSON_CreateString(cmdId));
	if(NULL != cmdData){
		cJSON_AddItemToObject(root, "cmdData", cmdData);
	}
	ret = JsonCompres(root, (char *)coapmsgTx, &outLength);
	if(OPP_SUCCESS != ret){
		return ret;
	}

	ret = OppCoapSend(dstChl, dstInfo, CMD_MSG, RSP_T, serviceId,0,coapmsgTx,outLength,mid);
	if(OPP_SUCCESS != ret)
		return ret;

	return OPP_SUCCESS;
}

int ApsCoapOpIsValid(char *op)
{
	/*if((0 == strcmp(op,"R")) || (0 == strcmp(op,"W")) 
		|| (0 == strcmp(op,"A")) || (0 == strcmp(op,"D"))
		|| (0 == strcmp(op,"M")) || (0 == strcmp(op,"RE")))*/
	if((0 == strcmp(op,"R")) || (0 == strcmp(op,"W")) || (0 == strcmp(op,"RE")))
		return OPP_SUCCESS;

	return OPP_FAILURE;
	
}
int ApsCoapPropIsValid(char *property)
{
	int i = 0;
	
	for(i=0; i<sizeof(aucProperty)/sizeof(ST_PROP);i++)
	{
		if(0 == strcmp(property,aucProperty[i].name))
		{
			if(aucProperty[i].auth & R){
				return OPP_SUCCESS;
			}else{
				break;
			}
		}
	}

	return OPP_FAILURE;
}

int ApsCoapPropIsWriteValid(char *property)
{
	int i = 0;
	
	for(i=0; i<sizeof(aucProperty)/sizeof(ST_PROP);i++)
	{
		if(0 == strcmp(property,aucProperty[i].name))
		{
			if(aucProperty[i].auth & W){
				return OPP_SUCCESS;
			}else{
				break;
			}
		}
	}

	return OPP_FAILURE;
}

int ApsCoapFindPropIdxByName(char *property)
{
	int i = 0;
	
	for(i=0; i<sizeof(aucProperty)/sizeof(ST_PROP);i++)
	{
		if(0 == strcmp(property,aucProperty[i].name))
		{
			return i;
		}
	}

	return i;
}

void ApsCoapPropProcR(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, int reqId, cJSON *prop, U32 mid)
{
	cJSON *propRe, *object, *comObj;
	int arraySize = -1;
	int i = 0;
	int ret = OPP_SUCCESS;
	U8 regStatus, conStatus;
	char buffer[32] = {0};
	ST_CONFIG_PARA stConfigPara;
	ST_IOT_CONFIG stIotConfigPara;
	ST_NB_CONFIG stNbConfigPara;
	ST_OPP_LAMP_CURR_ELECTRIC_INFO stElecInfo;
	//ST_OPP_LAMP_LOCATION stLocation;
	U16 usBri;
	U8 lampSwitch;
	U32 hTime = 0, rTime = 0;
	long double lat, lng;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	ST_NEUL_DEV *pstNeulDev;

	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n-------op R-----\r\n"); 				
	arraySize = cJSON_GetArraySize(prop);
	if(arraySize <=0)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n op R array is zero\r\n");
		ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,PROP_R_E_ERR,PROP_R_E_ERR_DESC,NULL,mid);
		return;
	}
	/*one time one item*/
	/*
	if((arraySize != 0) && (arraySize > R_MAX_ITEMS))
	{
		ApsCoapRsp(dstChl, reqId,PROP_SERVICE,0,PROP_R_ONE_ERR,PROP_R_ONE_ERR_DESC,NULL,mid);
		return;
	}*/
	
	for(i=0; i < arraySize; i++)
	{
		object=cJSON_GetArrayItem(prop,i);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"arraySize %d valide %s\r\n", arraySize, object->valuestring);
		ret = ApsCoapPropIsValid(object->valuestring);
		if(OPP_SUCCESS != ret)
		{
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n op R unknow property %s\n", object->valuestring);
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,PROP_R_ELE_ERR,PROP_R_ELE_ERR_DESC,NULL,mid);
			//cJSON_Delete(json);
			return;
		}
	}
	
	// no error
	propRe=cJSON_CreateObject();
	if(NULL == propRe){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create <propRe> object error\r\n");		
		ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return;
	}
	for(i=0; i < arraySize; i++)
	{
		object=cJSON_GetArrayItem(prop,i);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n------%s----------\r\n", object->valuestring);
		//////////设备状态///////////
		if(0 == strcmp(object->valuestring, "switch"))
		{
			OppLampCtrlGetSwitch(0, &lampSwitch);
			cJSON_AddItemToObject(propRe, "switch", cJSON_CreateNumber(lampSwitch));
		}
		else if(0 == strcmp(object->valuestring, "bri"))
		{
			OppLampCtrlGetBri(0, &usBri);
			cJSON_AddItemToObject(propRe, "bri", cJSON_CreateNumber(usBri));
		}
		else if(0 == strcmp(object->valuestring, "runtime"))
		{
			/*code review*/
			if(OPP_SUCCESS == OppLampCtrlGetHtime(0,&hTime) && OPP_SUCCESS == OppLampCtrlGetRtime(0,&rTime)){
				comObj = cJSON_CreateObject();
				if(NULL == comObj){
					MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create runtime <comObj> object error\r\n");				
					continue;
				}				
				cJSON_AddItemToObject(propRe, "runtime",comObj);
				cJSON_AddItemToObject(comObj, "hTime", cJSON_CreateNumber(hTime));
				cJSON_AddItemToObject(comObj, "rTime", cJSON_CreateNumber(rTime));
			}
		}				
		else if(0 == strcmp(object->valuestring, "tElec"))
		{
			/*code review*/
			ret = ElecGetElectricInfoNow(&stElecInfo);
			if(OPP_SUCCESS == ret){
				comObj = cJSON_CreateObject();
				if(NULL == comObj){
					MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create tElec <comObj> object error\r\n");				
					continue;
				}				
				cJSON_AddItemToObject(propRe, "tElec", comObj);
				cJSON_AddItemToObject(comObj, "current", cJSON_CreateNumber(stElecInfo.current));							
				cJSON_AddItemToObject(comObj, "voltage", cJSON_CreateNumber(stElecInfo.voltage));							
				cJSON_AddItemToObject(comObj, "factor", cJSON_CreateNumber(stElecInfo.factor));	
				cJSON_AddItemToObject(comObj, "power", cJSON_CreateNumber(stElecInfo.power));
			}
		}		
		else if(0 == strcmp(object->valuestring, "elec"))
		{
			/*code review*/
			ret = ElecGetElectricInfo(&stElecInfo);
			if(OPP_SUCCESS == ret){
				comObj = cJSON_CreateObject();
				if(NULL == comObj){
					MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create elec <comObj> object error\r\n");				
					continue;
				}
				cJSON_AddItemToObject(propRe, "elec", comObj);
				cJSON_AddItemToObject(comObj, "current", cJSON_CreateNumber(stElecInfo.current));							
				cJSON_AddItemToObject(comObj, "voltage", cJSON_CreateNumber(stElecInfo.voltage));							
				cJSON_AddItemToObject(comObj, "factor", cJSON_CreateNumber(stElecInfo.factor));	
				cJSON_AddItemToObject(comObj, "power", cJSON_CreateNumber(stElecInfo.power));
			}
		}
		else if(0 == strcmp(object->valuestring, "ecInfo"))
		{
			ret = ElecGetElectricInfo(&stElecInfo);
			if(OPP_SUCCESS == ret){				
				comObj = cJSON_CreateObject();
				if(NULL == comObj){
					MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create ecInfo <comObj> object error\r\n"); 			
					continue;
				}
				cJSON_AddItemToObject(propRe, "ecInfo", comObj);
				cJSON_AddItemToObject(comObj, "EC", cJSON_CreateNumber(stElecInfo.consumption));							
				cJSON_AddItemToObject(comObj, "HEC", cJSON_CreateNumber(stElecInfo.hisConsumption));
			}
		}
		else if(0 == strcmp(object->valuestring, "loc"))
		{
			//LocationRead(GPS_LOC, &stLocation);	
			if(OPP_SUCCESS == LocGetLat(&lat) && OPP_SUCCESS == LocGetLng(&lng)){
				comObj = cJSON_CreateObject();
				if(NULL == comObj){
					MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create loc <comObj> object error\r\n");				
					continue;
				}	
				cJSON_AddItemToObject(propRe, "loc", comObj);
				//ftoa(buffer,lat,6);			
				snprintf(buffer,32,"%Lf",lat);
				cJSON_AddItemToObject(comObj, "lat", cJSON_CreateString(buffer));
				//ftoa(buffer,lng,6);			
				snprintf(buffer,32,"%Lf",lng);
				cJSON_AddItemToObject(comObj, "lng", cJSON_CreateString(buffer));
			}
		}
		else if(0 == strcmp(object->valuestring, "nbBase"))
		{
			ret = sendEvent(UESTATE_EVENT,RISE_STATE,sArgc,sArgv);
			ret = recvEvent(UESTATE_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
			if(ret < -5 || ret == -1){
				//pstNeulDev = &g_stThisLampInfo.stNeulSignal;
				ARGC_FREE(rArgc,rArgv);
				continue;
			}else{
				pstNeulDev = (ST_NEUL_DEV *)rArgv[0];
			}
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create nbBase <comObj> object error\r\n");				
				continue;
			}			
			cJSON_AddItemToObject(propRe, "nbBase", comObj);
			cJSON_AddItemToObject(comObj, "imei", cJSON_CreateString((char *)pstNeulDev->imei));
			cJSON_AddItemToObject(comObj, "imsi", cJSON_CreateString((char *)pstNeulDev->imsi));
			cJSON_AddItemToObject(comObj, "iccid", cJSON_CreateString((char *)pstNeulDev->nccid));
			if(strlen((char *)pstNeulDev->pdpAddr0))
				cJSON_AddItemToObject(comObj, "nbip", cJSON_CreateString((char *)pstNeulDev->pdpAddr0));
			else
				cJSON_AddItemToObject(comObj, "nbip", cJSON_CreateString((char *)pstNeulDev->pdpAddr1));
			ARGC_FREE(rArgc,rArgv);
		}
		else if(0 == strcmp(object->valuestring, "nbSignal"))
		{			
			ret = sendEvent(UESTATE_EVENT,RISE_STATE,sArgc,sArgv);
			ret = recvEvent(UESTATE_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
			if(OPP_SUCCESS != ret){
				//pstNeulDev = &g_stThisLampInfo.stNeulSignal;
				ARGC_FREE(rArgc,rArgv);
				continue;
			}
			pstNeulDev = (ST_NEUL_DEV *)rArgv[0];
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create nbSignal <comObj> object error\r\n");				
				continue;
			}
			cJSON_AddItemToObject(propRe, "nbSignal", comObj);
			itoa(pstNeulDev->rsrp, buffer, 10);
			cJSON_AddItemToObject(comObj, "rsrp", cJSON_CreateString(buffer));
			itoa(pstNeulDev->rsrq, buffer, 10);						
			cJSON_AddItemToObject(comObj, "rsrq", cJSON_CreateString(buffer));
			cJSON_AddItemToObject(comObj, "ecl", cJSON_CreateNumber(pstNeulDev->signalEcl));
			cJSON_AddItemToObject(comObj, "cellid", cJSON_CreateNumber(pstNeulDev->cellId));
			cJSON_AddItemToObject(comObj, "pci", cJSON_CreateNumber(pstNeulDev->signalPci));
			itoa(pstNeulDev->snr, buffer, 10);						
			cJSON_AddItemToObject(comObj, "snr", cJSON_CreateString(buffer));
			cJSON_AddItemToObject(comObj, "earfcn", cJSON_CreateNumber(pstNeulDev->earfcn));
			cJSON_AddItemToObject(comObj, "txPower", cJSON_CreateNumber(pstNeulDev->txPower));
			ARGC_FREE(rArgc,rArgv);
		}
		else if(0 == strcmp(object->valuestring, "nbCon"))
		{
			NeulBc28QueryNetstats(&regStatus, &conStatus);
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create nbCon <comObj> object error\r\n");				
				continue;
			}				
			cJSON_AddItemToObject(propRe, "nbCon", comObj);			
			cJSON_AddItemToObject(comObj, "con", cJSON_CreateNumber(conStatus));
			cJSON_AddItemToObject(comObj, "reg", cJSON_CreateNumber(regStatus));
		}
		else if(0 == strcmp(object->valuestring, "nbInfo"))
		{
			//ST_NEUL_DEV stNeulDev;
			signed char iotStatus;
			
			ret = sendEvent(UESTATE_EVENT,RISE_STATE,sArgc,sArgv);
			ret = recvEvent(UESTATE_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
			if(0 != ret){
				ARGC_FREE(rArgc,rArgv);
				continue;
			}
			
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create nbInfo <comObj> object error\r\n");				
				continue;
			}
			cJSON_AddItemToObject(propRe, "nbInfo", comObj);			
			pstNeulDev = (ST_NEUL_DEV *)rArgv[0];
			cJSON_AddItemToObject(comObj, "imei", cJSON_CreateString((char *)pstNeulDev->imei));
			cJSON_AddItemToObject(comObj, "imsi", cJSON_CreateString((char *)pstNeulDev->imsi));
			cJSON_AddItemToObject(comObj, "iccid", cJSON_CreateString((char *)pstNeulDev->nccid));
			if(strlen((char *)pstNeulDev->pdpAddr0))
				cJSON_AddItemToObject(comObj, "nbip", cJSON_CreateString((char *)pstNeulDev->pdpAddr0));
			else
				cJSON_AddItemToObject(comObj, "nbip", cJSON_CreateString((char *)pstNeulDev->pdpAddr1));
			cJSON_AddItemToObject(comObj, "rsrp", cJSON_CreateNumber(pstNeulDev->rsrp));
			cJSON_AddItemToObject(comObj, "totalPower", cJSON_CreateNumber(pstNeulDev->totalPower));
			cJSON_AddItemToObject(comObj, "txPower", cJSON_CreateNumber(pstNeulDev->txPower));
			cJSON_AddItemToObject(comObj, "txTime", cJSON_CreateNumber(pstNeulDev->txTime));
			cJSON_AddItemToObject(comObj, "rxTime", cJSON_CreateNumber(pstNeulDev->rxTime));
			cJSON_AddItemToObject(comObj, "cellid", cJSON_CreateNumber(pstNeulDev->cellId));
			cJSON_AddItemToObject(comObj, "ecl", cJSON_CreateNumber(pstNeulDev->signalEcl));
			cJSON_AddItemToObject(comObj, "snr", cJSON_CreateNumber(pstNeulDev->snr));
			cJSON_AddItemToObject(comObj, "earfcn", cJSON_CreateNumber(pstNeulDev->earfcn));
			cJSON_AddItemToObject(comObj, "pci", cJSON_CreateNumber(pstNeulDev->signalPci));				
			cJSON_AddItemToObject(comObj, "rsrq", cJSON_CreateNumber(pstNeulDev->rsrq));				
			cJSON_AddItemToObject(comObj, "opmode", cJSON_CreateNumber(pstNeulDev->opmode));
			ARGC_FREE(rArgc,rArgv);
			
			NeulBc28QueryNetstats(&regStatus, &conStatus);
			cJSON_AddItemToObject(comObj, "con",cJSON_CreateNumber(conStatus));
			cJSON_AddItemToObject(comObj, "reg",cJSON_CreateNumber(regStatus));
			
			NeulBc28QueryIOTstats(&iotStatus);
			cJSON_AddItemToObject(comObj, "iotState",cJSON_CreateNumber(iotStatus));
			cJSON_AddItemToObject(comObj, "devState",cJSON_CreateNumber(NeulBc28GetDevState()));
			
		}		
		else if(0 == strcmp(object->valuestring, "sysInfo"))
		{
			cJSON *subObj;
			//unsigned int Temp_ProductClass = PRODUCTCLASS;
			//unsigned int Temp_ProductSku = PRODUCTSKU;
			//unsigned int Temp_Ver = OPP_LAMP_CTRL_CFG_DATA_VER;
				
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create sysInfo <comObj> object error\r\n");				
				continue;
			}
			cJSON_AddItemToObject(propRe, "sysInfo", comObj);
			//version
			subObj=cJSON_CreateObject();
			if(NULL == subObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create sysInfo.version <subObj> object error\r\n");				
				continue;
			}
			cJSON_AddItemToObject(comObj, "version",subObj);
			snprintf(buffer, 5, "%04x", PRODUCTCLASS);			
			cJSON_AddItemToObject(subObj, "clas", cJSON_CreateString(buffer));
			snprintf(buffer, 5, "%04x", PRODUCTSKU);			
			cJSON_AddItemToObject(subObj, "sku", cJSON_CreateString(buffer));
			cJSON_AddItemToObject(subObj, "fwv", cJSON_CreateNumber(g_stThisLampInfo.stObjInfo.ulVersion));	
			cJSON_AddItemToObject(subObj, "data", cJSON_CreateString(__DATE__));
			cJSON_AddItemToObject(subObj, "time", cJSON_CreateString(__TIME__));
			//runtime
			OppLampCtrlGetHtime(0,&hTime);
			OppLampCtrlGetRtime(0,&rTime);
			subObj=cJSON_CreateObject();
			if(NULL == subObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create sysInfo.runtime <subObj> object error\r\n");				
				continue;
			}
			cJSON_AddItemToObject(comObj, "runtime",subObj);
			cJSON_AddItemToObject(subObj, "hTime", cJSON_CreateNumber(hTime));			
			cJSON_AddItemToObject(subObj, "rTime", cJSON_CreateNumber(rTime));
			//elec
			ElecGetElectricInfo(&stElecInfo);
			subObj=cJSON_CreateObject();
			if(NULL == subObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create sysInfo.elec <subObj> object error\r\n");				
				continue;
			}
			cJSON_AddItemToObject(comObj, "elec", subObj);
			cJSON_AddItemToObject(subObj, "current", cJSON_CreateNumber(stElecInfo.current));							
			cJSON_AddItemToObject(subObj, "voltage", cJSON_CreateNumber(stElecInfo.voltage));							
			cJSON_AddItemToObject(subObj, "factor", cJSON_CreateNumber(stElecInfo.factor));	
			cJSON_AddItemToObject(subObj, "power", cJSON_CreateNumber(stElecInfo.power));
			//EC
			subObj=cJSON_CreateObject();
			if(NULL == subObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create sysInfo.ecInfo <subObj> object error\r\n");				
				continue;
			}
			cJSON_AddItemToObject(comObj, "ecInfo", subObj);
			cJSON_AddItemToObject(subObj, "EC", cJSON_CreateNumber(stElecInfo.consumption));							
			cJSON_AddItemToObject(subObj, "HEC", cJSON_CreateNumber(stElecInfo.hisConsumption));
			
		}
		else if(0 == strcmp(object->valuestring, "wifiInfo"))
		{
			bool aucon_en;
			unsigned char mac[6];
			tcpip_adapter_ip_info_t local_ip;
			unsigned char primary;
			wifi_second_chan_t second;
			wifi_mode_t mode;
			float power;
			uint8_t protocol_bitmap;
			int len = 0;
			wifi_bandwidth_t bw;
			wifi_country_t country;
			
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create wifiInfo <comObj> object error\r\n");								
				continue;
			}			
			cJSON_AddItemToObject(propRe, "wifiInfo", comObj);
			esp_wifi_get_auto_connect(&aucon_en);
			cJSON_AddItemToObject(comObj, "aucon", cJSON_CreateNumber(aucon_en));
			esp_wifi_get_mac(WIFI_IF_STA, mac);
			snprintf(buffer,18,"%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
			cJSON_AddItemToObject(comObj, "mac", cJSON_CreateString(buffer));
			tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);	
			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer,16,IPSTR,IP2STR(&local_ip.ip));			
			cJSON_AddItemToObject(comObj, "ip", cJSON_CreateString(buffer));
			snprintf(buffer,16,IPSTR,IP2STR(&local_ip.netmask));
			cJSON_AddItemToObject(comObj, "mask", cJSON_CreateString(buffer));
			snprintf(buffer,16,IPSTR,IP2STR(&local_ip.gw));
			cJSON_AddItemToObject(comObj, "gw", cJSON_CreateString(buffer));

			esp_wifi_get_channel(&primary,&second);
			cJSON_AddItemToObject(comObj, "primary", cJSON_CreateNumber(primary));
			cJSON_AddItemToObject(comObj, "second", cJSON_CreateNumber(second));
			esp_wifi_get_mode(&mode);
			cJSON_AddItemToObject(comObj, "mode", cJSON_CreateNumber(mode));
			ApsGetWifiPower(&power);
			memset(buffer, 0, sizeof(buffer));
			ftoa(buffer, (long double)power, 1);
			cJSON_AddItemToObject(comObj, "power", cJSON_CreateString(buffer));
			esp_wifi_get_protocol(WIFI_IF_STA,&protocol_bitmap);
			memset(buffer, 0, sizeof(buffer));
			if(protocol_bitmap|WIFI_PROTOCOL_11B){
				len += snprintf(buffer+len,2,"%s","b");
			}
			if(protocol_bitmap|WIFI_PROTOCOL_11G){
				len += snprintf(buffer+len,2,"%s","g");
			}
			if(protocol_bitmap|WIFI_PROTOCOL_11N){
				len += snprintf(buffer+len,2,"%s","n");
			}
			cJSON_AddItemToObject(comObj, "protocol", cJSON_CreateString(buffer));
			esp_wifi_get_bandwidth(WIFI_IF_STA,&bw);
			cJSON_AddItemToObject(comObj, "bw", cJSON_CreateNumber(bw));
			esp_wifi_get_country(&country);
			country.cc[2] = '\0';
			cJSON_AddItemToObject(comObj, "cc", cJSON_CreateString(country.cc));
		}
		else if(0 == strcmp(object->valuestring, "apInfo")){
				int ret;
				int len = 0;
				wifi_ap_record_t ap_info;

				comObj = cJSON_CreateObject();
				if(NULL == comObj){
					MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create apInfo <comObj> object error\r\n");					
					continue;
				}			
				cJSON_AddItemToObject(propRe, "apInfo", comObj);
				
				ret = esp_wifi_sta_get_ap_info(&ap_info);
				if(0 != ret){
					continue;
				}
				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer,18,"%02x:%02x:%02x:%02x:%02x:%02x",ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2], ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
				cJSON_AddItemToObject(comObj, "mac", cJSON_CreateString(buffer));
				cJSON_AddItemToObject(comObj, "ssid", cJSON_CreateString((char *)ap_info.ssid));
				cJSON_AddItemToObject(comObj, "primary", cJSON_CreateNumber(ap_info.primary));
				cJSON_AddItemToObject(comObj, "second", cJSON_CreateNumber(ap_info.second));
				cJSON_AddItemToObject(comObj, "rssi", cJSON_CreateNumber(ap_info.rssi));
				cJSON_AddItemToObject(comObj, "auth", cJSON_CreateNumber(ap_info.authmode));
				cJSON_AddItemToObject(comObj, "pairwise_cipher", cJSON_CreateNumber(ap_info.pairwise_cipher));
				cJSON_AddItemToObject(comObj, "group_cipher", cJSON_CreateNumber(ap_info.group_cipher));
				memset(buffer, 0, sizeof(buffer));
				if(ap_info.phy_11b|WIFI_PROTOCOL_11B)
					len += snprintf(buffer+len,512,"%s","b");
				if(ap_info.phy_11g|WIFI_PROTOCOL_11G)
					len += snprintf(buffer+len,512,"%s","g");
				if(ap_info.phy_11n|WIFI_PROTOCOL_11N)
					len += snprintf(buffer+len,512,"%s","n");
				cJSON_AddItemToObject(comObj, "protocol", cJSON_CreateString(buffer));
				cJSON_AddItemToObject(comObj, "wps", cJSON_CreateNumber(ap_info.wps&(1<<4)));
		}		
		else if(0 == strcmp(object->valuestring, "wifiCfg"))
		{
			ST_WIFI_CONFIG stWifiConfig;

			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create wifiCfg <comObj> object error\r\n");				
				continue;
			}			
			cJSON_AddItemToObject(propRe, "wifiCfg", comObj);
			ApsWifiConfigRead(&stWifiConfig);			
			cJSON_AddItemToObject(comObj, "ssid", cJSON_CreateString((char *)stWifiConfig.ssid));
			cJSON_AddItemToObject(comObj, "password", cJSON_CreateString((char *)stWifiConfig.password));
		}
		else if(0 == strcmp(object->valuestring, "locCfg"))
		{
			long double lat, lng;
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create locCfg <comObj> object error\r\n");				
				continue;
			}			
			cJSON_AddItemToObject(propRe, "locCfg", comObj);
			LocGetCfgLat(&lat);
			LocGetCfgLng(&lng);			
			memset(buffer, 0, sizeof(buffer));
			//ftoa(buffer,lat,6);
			snprintf(buffer,32,"%Lf",lat);
			cJSON_AddItemToObject(comObj, "lat", cJSON_CreateString(buffer));
			memset(buffer, 0, sizeof(buffer));
			//ftoa(buffer,lng,6);
			snprintf(buffer,32,"%Lf",lng);
			cJSON_AddItemToObject(comObj, "lng", cJSON_CreateString(buffer));
			
		}		
		else if(0 == strcmp(object->valuestring, "locSrc"))
		{
			cJSON_AddItemToObject(propRe, "locSrc", cJSON_CreateNumber(LocGetSrc()));
		}
		else if(0 == strcmp(object->valuestring, "clkSrc"))
		{
			cJSON_AddItemToObject(propRe, "clkSrc", cJSON_CreateNumber(TimeGetClockSrc()));
		}
		else if(0 == strcmp(object->valuestring, "devCap"))
		{
			ST_DEV_CONFIG stDevConfig;
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create devCap <comObj> object error\r\n");				
				continue;
			}
			cJSON_AddItemToObject(propRe, "devCap", comObj);
			ApsDevConfigRead(&stDevConfig);
			cJSON_AddItemToObject(comObj, "nb", cJSON_CreateNumber(stDevConfig.nb));
			cJSON_AddItemToObject(comObj, "wifi", cJSON_CreateNumber(stDevConfig.wifi));
			cJSON_AddItemToObject(comObj, "gps", cJSON_CreateNumber(stDevConfig.gps));
			cJSON_AddItemToObject(comObj, "meter", cJSON_CreateNumber(stDevConfig.meter));
			cJSON_AddItemToObject(comObj, "sensor", cJSON_CreateNumber(stDevConfig.sensor));
			cJSON_AddItemToObject(comObj, "rs485", cJSON_CreateNumber(stDevConfig.rs485));
			cJSON_AddItemToObject(comObj, "light", cJSON_CreateNumber(stDevConfig.light));
		}
		else if(0 == strcmp(object->valuestring, "nbVer")){
			char *sArgv[MAX_ARGC];
			char *rArgv[MAX_ARGC];
			int sArgc = 0, rArgc = 0;			
			ret = sendEvent(VER_EVENT,RISE_STATE,sArgc,sArgv);
			ret = recvEvent(VER_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
			if(0 != ret){
				ARGC_FREE(rArgc,rArgv);
				continue;
			}
			cJSON_AddItemToObject(propRe, "nbVer", cJSON_CreateString(rArgv[0]));
			ARGC_FREE(rArgc,rArgv);
		}
		else if(0 == strcmp(object->valuestring, "nbPid")){
			char *sArgv[MAX_ARGC];
			char *rArgv[MAX_ARGC];
			int sArgc = 0, rArgc = 0;			
			ret = sendEvent(PID_EVENT,RISE_STATE,sArgc,sArgv);
			ret = recvEvent(PID_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
			if(0 != ret){
				ARGC_FREE(rArgc,rArgv);
				continue;
			}
			cJSON_AddItemToObject(propRe, "nbPid", cJSON_CreateString(rArgv[0]));
			ARGC_FREE(rArgc,rArgv);
		}
		else if(0 == strcmp(object->valuestring, "lux")){
			U32 vol,lx;
			
			ret = BspLightSensorVoltageGet(&vol);
			if(OPP_SUCCESS != ret){
				continue;
			}
			ret = BspLightSensorLuxGet(&lx);
			if(OPP_SUCCESS != ret){
				continue;
			}
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create lux <comObj> object error\r\n");				
				continue;
			}
			cJSON_AddItemToObject(propRe, "lux", comObj);
			cJSON_AddItemToObject(comObj, "voltage", cJSON_CreateNumber(vol));
			cJSON_AddItemToObject(comObj, "lx", cJSON_CreateNumber(lx));
		}
		/*else if(0 == strcmp(object->valuestring, "adjElec"))
		{
			uint16_t IbGain = 0;
			uint16_t Ugain = 0;
			uint16_t PbGain = 0;
			
			VoltageParamGet(&Ugain);
			CurrentParamGet(&IbGain);
			PowerParamGet(&PbGain);
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create adjElec <comObj> object error\r\n");				
				continue;
			}			
			cJSON_AddItemToObject(propRe, "adjElec", comObj);
			cJSON_AddItemToObject(comObj, "voltage", cJSON_CreateNumber(Ugain));
			cJSON_AddItemToObject(comObj, "current", cJSON_CreateNumber(IbGain));
			cJSON_AddItemToObject(comObj, "power", cJSON_CreateNumber(PbGain));
		}
		else if(0 == strcmp(object->valuestring, "adjPhase"))
		{
			uint8_t u8BPhCal = 0;
			
			BPhCalParamGet(&u8BPhCal);		
			cJSON_AddItemToObject(propRe, "adjPhase", cJSON_CreateNumber(u8BPhCal));
		}*/
		else if(0 == strcmp(object->valuestring, "elecPara")){
			uint16_t IbGain = 0;
			uint16_t Ugain = 0;
			uint16_t PbGain = 0;
			uint8_t u8BPhCal = 0;
			
			if(OPP_SUCCESS == VoltageParamGet(&Ugain) 
				&& OPP_SUCCESS == CurrentParamGet(&IbGain) 
				&& OPP_SUCCESS == PowerParamGet(&PbGain) 
				&& OPP_SUCCESS == BPhCalParamGet(&u8BPhCal)){
				comObj = cJSON_CreateObject();
				if(NULL == comObj){
					MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create elecPara <comObj> object error\r\n");				
					continue;
				}			
				cJSON_AddItemToObject(propRe, "elecPara", comObj);
				cJSON_AddItemToObject(comObj, "voltage", cJSON_CreateNumber(Ugain));
				cJSON_AddItemToObject(comObj, "current", cJSON_CreateNumber(IbGain));
				cJSON_AddItemToObject(comObj, "power", cJSON_CreateNumber(PbGain));
				cJSON_AddItemToObject(comObj, "phase", cJSON_CreateNumber(u8BPhCal));
			}
		}
		else if(0 == strcmp(object->valuestring, "attachDisPara"))
		{
			ST_DISCRETE_SAVE_PARA stDisSavePara;
			NeulBc28GetDiscretePara(&stDisSavePara);
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create attachDisPara <comObj> object error\r\n");				
				continue;
			}			
			cJSON_AddItemToObject(propRe, "attachDisPara", comObj);
			cJSON_AddItemToObject(comObj, "window", cJSON_CreateNumber(stDisSavePara.udwDiscreteWin));
			cJSON_AddItemToObject(comObj, "interval", cJSON_CreateNumber(stDisSavePara.udwDiscreteInterval));
		}
		else if(0 == strcmp(object->valuestring, "heartDisPara"))
		{
			ST_DIS_HEART_SAVE_PARA stDisHeartSavePara;
			ApsCoapHeartDisParaGet(&stDisHeartSavePara);
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create attachDisPara <comObj> object error\r\n");				
				continue;
			}			
			cJSON_AddItemToObject(propRe, "heartDisPara", comObj);
			cJSON_AddItemToObject(comObj, "window", cJSON_CreateNumber(stDisHeartSavePara.udwDiscreteWin));
			cJSON_AddItemToObject(comObj, "interval", cJSON_CreateNumber(stDisHeartSavePara.udwDiscreteInterval));

		}
		else if(0 == strcmp(object->valuestring, "adjAdc64"))
		{
			uint16_t u16DacOutput = 0;

			DacLevel64ParamGet(&u16DacOutput);
			cJSON_AddItemToObject(propRe, "adjAdc64", cJSON_CreateNumber(u16DacOutput));
		}
		else if(0 == strcmp(object->valuestring, "adjAdc192"))
		{
			uint16_t u16DacOutput = 0;
			
			DacLevel192ParamGet(&u16DacOutput);
			cJSON_AddItemToObject(propRe, "adjAdc192", cJSON_CreateNumber(u16DacOutput));
		}
		else if(0 == strcmp(object->valuestring, "watchdog"))
		{
			TaskWdtConfig_EN config;
			taskWdtConfigGet(&config);
			
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create watchdog <comObj> object error\r\n");				
				continue;
			}			
			cJSON_AddItemToObject(propRe, "watchdog", comObj);
			cJSON_AddItemToObject(comObj, "period", cJSON_CreateNumber(config.period));
			cJSON_AddItemToObject(comObj, "reset", cJSON_CreateNumber(config.resetEn));
		}
		else if(0 == strcmp(object->valuestring, "sdkVer"))
		{
			cJSON_AddItemToObject(propRe, "sdkVer", cJSON_CreateString(esp_get_idf_version()));
		}
		else if(0 == strcmp(object->valuestring, "hwInfo"))
		{
			extern uint16_t g_DacLevel;
			extern uint16_t g_PwmLevel;
			extern uint8_t	g_RelayGpioStatus;
			extern uint8_t	g_DimTypeStatus;
			
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create hardInfo <hwInfo> object error\r\n");				
				continue;
			}			
			cJSON_AddItemToObject(propRe, "hwInfo", comObj);
			cJSON_AddItemToObject(comObj, "dacLevel", cJSON_CreateNumber(g_DacLevel));
			cJSON_AddItemToObject(comObj, "pwmLevel", cJSON_CreateNumber(g_PwmLevel));
			cJSON_AddItemToObject(comObj, "relayGpioStatus", cJSON_CreateString(g_RelayGpioStatus == 0 ? "OFF" : "ON"));
			cJSON_AddItemToObject(comObj, "dimTypeStatus", cJSON_CreateString(g_DimTypeStatus == 0 ? "0-10V" : "PWM"));
		}
		else if(0 == strcmp(object->valuestring, "rxProtect")){
			cJSON_AddItemToObject(propRe, "rxProtect", cJSON_CreateNumber(ApsCoapRxProtectGet()));
		}
		else if(0 == strcmp(object->valuestring, "attachDisOffset")){
			cJSON_AddItemToObject(propRe, "attachDisOffset", cJSON_CreateNumber(NeulBc28DisOffsetSecondGet()));
		}
		else if(0 == strcmp(object->valuestring, "heartDisOffset")){
			cJSON_AddItemToObject(propRe, "heartDisOffset", cJSON_CreateNumber(ApsCoapHeartDisOffsetSecondGet()));
		}
		else if(0 == strcmp(object->valuestring, "attachDisEnable")){
			cJSON_AddItemToObject(propRe, "attachDisEnable", cJSON_CreateNumber(NeulBc28GetDisEnable()));
		}
		else if(0 == strcmp(object->valuestring, "testCur")){
			EN_TEST_RESULT result;
			SvsTestCurGet(&result);
			cJSON_AddItemToObject(propRe, "testCur", cJSON_CreateNumber(result));
		}
		else if(0 == strcmp(object->valuestring, "test1")){
			EN_TEST_RESULT result;
			SvsTestInfoGet(TEST_FCT,&result);
			cJSON_AddItemToObject(propRe, "test1", cJSON_CreateNumber(result));
		}
		else if(0 == strcmp(object->valuestring, "test2")){
			EN_TEST_RESULT result;
			SvsTestInfoGet(TEST_C1,&result);
			cJSON_AddItemToObject(propRe, "test2", cJSON_CreateNumber(result));
		}
		else if(0 == strcmp(object->valuestring, "test3")){
			EN_TEST_RESULT result;
			SvsTestInfoGet(TEST_AG,&result);
			cJSON_AddItemToObject(propRe, "test3", cJSON_CreateNumber(result));
		}
		else if(0 == strcmp(object->valuestring, "test4")){
			EN_TEST_RESULT result;
			SvsTestInfoGet(TEST_C2,&result);
			cJSON_AddItemToObject(propRe, "test4", cJSON_CreateNumber(result));
		}
		else if(0 == strcmp(object->valuestring, "test5")){
			EN_TEST_RESULT result;
			SvsTestInfoGet(TEST_REMAIN0,&result);
			cJSON_AddItemToObject(propRe, "test5", cJSON_CreateNumber(result));
		}
		else if(0 == strcmp(object->valuestring, "test6")){
			EN_TEST_RESULT result;
			SvsTestInfoGet(TEST_REMAIN1,&result);
			cJSON_AddItemToObject(propRe, "test6", cJSON_CreateNumber(result));
		}
		else if(0 == strcmp(object->valuestring, "test7")){
			EN_TEST_RESULT result;
			SvsTestInfoGet(TEST_REMAIN2,&result);
			cJSON_AddItemToObject(propRe, "test7", cJSON_CreateNumber(result));
		}
		else if(0 == strcmp(object->valuestring, "test8")){
			EN_TEST_RESULT result;
			SvsTestInfoGet(TEST_REMAIN3,&result);
			cJSON_AddItemToObject(propRe, "test8", cJSON_CreateNumber(result));
		}
		////////////////设备信息/////////////////////////////
		else if(0 == strcmp(object->valuestring, "product"))
		{
			cJSON_AddItemToObject(propRe, "product", cJSON_CreateString(PRODUCTNAME));
		}
		else if(0 == strcmp(object->valuestring, "sn"))
		{
			cJSON_AddItemToObject(propRe, "sn", cJSON_CreateString((char *)g_stThisLampInfo.stObjInfo.aucObjectSN));
		}
		else if(0 == strcmp(object->valuestring, "protId"))
		{
			cJSON_AddItemToObject(propRe, "protId", cJSON_CreateString(PROTID));							
		}
		else if(0 == strcmp(object->valuestring, "model"))
		{
			cJSON_AddItemToObject(propRe, "model", cJSON_CreateString(MODULE)); 						
		}
		else if(0 == strcmp(object->valuestring, "manu"))
		{
			cJSON_AddItemToObject(propRe, "manu", cJSON_CreateString(MANU)); 						
		}
		else if(0 == strcmp(object->valuestring, "sku"))
		{
			snprintf(buffer, 5, "%04x", PRODUCTSKU);
			cJSON_AddItemToObject(propRe, "sku", cJSON_CreateString(buffer));							

		}
		else if(0 == strcmp(object->valuestring, "clas"))
		{
			snprintf(buffer, 5, "%04x", PRODUCTCLASS);
			cJSON_AddItemToObject(propRe, "clas", cJSON_CreateString(buffer)); 						
		}
		else if(0 == strcmp(object->valuestring, "fwv"))
		{
		/*
			snprintf(buffer, 8, "1.%u.%u.%u",
				OPP_HUNDRED_OF_NUMBER(g_stThisLampInfo.stObjInfo.ulVersion), 
				OPP_DECADE_OF_NUMBER(g_stThisLampInfo.stObjInfo.ulVersion), 
				OPP_UNIT_OF_NUMBER(g_stThisLampInfo.stObjInfo.ulVersion));
			cJSON_AddItemToObject(propRe, "fwv", cJSON_CreateString(buffer));
		*/
			cJSON_AddItemToObject(propRe, "fwv", cJSON_CreateNumber(g_stThisLampInfo.stObjInfo.ulVersion));
		}
		/////////////////设备配置////////////////////////////
		else if(0 == strcmp(object->valuestring, "ocIp"))
		{
			OppCoapIOTGetConfig(&stIotConfigPara);		
			cJSON_AddItemToObject(propRe, "ocIp", cJSON_CreateString(stIotConfigPara.ocIp));
		}
		else if(0 == strcmp(object->valuestring, "ocPort"))
		{
			OppCoapIOTGetConfig(&stIotConfigPara);;		
			cJSON_AddItemToObject(propRe, "ocPort", cJSON_CreateNumber(stIotConfigPara.ocPort));
		}
		else if(0 == strcmp(object->valuestring, "band"))
		{
			NeulBc28GetConfig(&stNbConfigPara);		
			cJSON_AddItemToObject(propRe, "band", cJSON_CreateNumber(stNbConfigPara.band));
		}
		else if(0 == strcmp(object->valuestring, "dimType"))
		{
			OppLampCtrlGetConfigPara(&stConfigPara);		
			cJSON_AddItemToObject(propRe, "dimType", cJSON_CreateNumber(stConfigPara.dimType));
		}
		else if(0 == strcmp(object->valuestring, "tDimType"))
		{
			cJSON_AddItemToObject(propRe, "tDimType", cJSON_CreateNumber(LampOuttypeGet()));
		}
		else if(0 == strcmp(object->valuestring, "apnAddres"))
		{
			NeulBc28GetConfig(&stNbConfigPara);	
			cJSON_AddItemToObject(propRe, "apnAddres", cJSON_CreateString(stNbConfigPara.apn));
		}
		else if(0 == strcmp(object->valuestring, "earfcn"))
		{
			U8 enable;
			U16 earfcn;
			
			NeulBc28GetEarfcnFromRam(&enable,&earfcn);
			//NeulBc28GetConfig(&stNbConfigPara);	
			cJSON_AddItemToObject(propRe, "earfcn", cJSON_CreateNumber(earfcn));
		}
		else if(0 == strcmp(object->valuestring, "nbPara"))
		{
			U8 enable;
			U16 earfcn,band;
			
			NeulBc28GetParaFromRam(&enable,&earfcn,&band);
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropProcR create nbPara <comObj> object error\r\n");				
				continue;
			}			
			cJSON_AddItemToObject(propRe, "nbPara", comObj);
			cJSON_AddItemToObject(comObj, "enable", cJSON_CreateNumber(enable));
			cJSON_AddItemToObject(comObj, "earfcn", cJSON_CreateNumber(earfcn));
			cJSON_AddItemToObject(comObj, "band", cJSON_CreateNumber(band));
		}
		else if(0 == strcmp(object->valuestring, "period"))
		{
			AD_CONFIG_T config;
			//OppLampCtrlGetConfigPara(&stConfigPara);	
			ApsDaemonActionItemParaGetCustom(CUSTOM_REPORT,HEARTBEAT_PERIOD_IDX,&config);
			cJSON_AddItemToObject(propRe, "period", cJSON_CreateNumber(config.reportIfPara));
		}
		else if(0 == strcmp(object->valuestring, "scram"))
		{
			NeulBc28GetConfig(&stNbConfigPara);;			
			cJSON_AddItemToObject(propRe, "scram", cJSON_CreateNumber(stNbConfigPara.scram));	
		}
		else if(0 == strcmp(object->valuestring, "plansNum"))
		{
			OppLampCtrlGetConfigPara(&stConfigPara);			
			cJSON_AddItemToObject(propRe, "plansNum", cJSON_CreateNumber(stConfigPara.plansNum));	
		}
		else if(0 == strcmp(object->valuestring, "hbPeriod"))
		{
			AD_CONFIG_T config;	
			ApsDaemonActionItemParaGetCustom(CUSTOM_REPORT,HEARTBEAT_PERIOD_IDX,&config);
			cJSON_AddItemToObject(propRe, "hbPeriod", cJSON_CreateNumber(config.reportIfPara));
		}
		else if(0 == strcmp(object->valuestring, "nbDevStTo"))
		{
			cJSON_AddItemToObject(propRe, "nbDevStTo", cJSON_CreateNumber(DEVSTATE_DEFAULT_TO));
		}
		else if(0 == strcmp(object->valuestring, "nbRegStTo"))
		{
			cJSON_AddItemToObject(propRe, "nbRegStTo", cJSON_CreateNumber(REGSTATE_DEFAULT_TO));
		}
		else if(0 == strcmp(object->valuestring, "nbIotStTo"))
		{
			cJSON_AddItemToObject(propRe, "nbIotStTo", cJSON_CreateNumber(IOTSTATE_DEFAULT_TO));
		}
		else if(0 == strcmp(object->valuestring, "nbHardRstTo"))
		{
			cJSON_AddItemToObject(propRe, "nbHardRstTo", cJSON_CreateNumber(HARD_RESET_TO));
		}
		else if(0 == strcmp(object->valuestring, "nbOtaTo"))
		{
			cJSON_AddItemToObject(propRe, "nbOtaTo", cJSON_CreateNumber(FIRMWARE_DEFAULT_TO));
		}
		else if(0 == strcmp(object->valuestring, "time0"))
		{
			cJSON_AddItemToObject(propRe, "time0", cJSON_CreateNumber(NBIOT_ATCLCK_NOTIME_TO));
		}
		else if(0 == strcmp(object->valuestring, "time1"))
		{
			cJSON_AddItemToObject(propRe, "time1", cJSON_CreateNumber(NBIOT_ATCLCK_TO));
		}
		else if(0 == strcmp(object->valuestring, "saveHtimeTo"))
		{
			cJSON_AddItemToObject(propRe, "saveHtimeTo", cJSON_CreateNumber(RUNTIME_SAVE_TO));
		}
		else if(0 == strcmp(object->valuestring, "scanEcTo"))
		{
			cJSON_AddItemToObject(propRe, "scanEcTo", cJSON_CreateNumber(EC_SCAN_TO));
		}
		else if(0 == strcmp(object->valuestring, "saveHecTo"))
		{
			cJSON_AddItemToObject(propRe, "saveHecTo", cJSON_CreateNumber(EC_SAVE_TO));
		}
		else if(0 == strcmp(object->valuestring, "onlineTo"))
		{
			cJSON_AddItemToObject(propRe, "onlineTo", cJSON_CreateNumber(ONLINE_TO));
		}
	}
	
	ret = ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,SUCCESS,NULL,propRe,mid);
	if(NBIOT_SEND_BIG_ERR == ret){
		ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,NBIOT_SEND_BIG_ERR,NBIOT_SEND_BIG_ERR_DESC,NULL,mid);
	}
	
	return;
}



void ApsCoapPropProcW(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, int reqId, cJSON *prop, U32 mid)
{
	cJSON *object, *subObj;
	char *p;
	int i = 0;
	int temp = -1, dimType = -1, /*period = -1,*/ rTime = -1, consumption = -1;
	ST_CONFIG_PARA para;
	ST_NB_CONFIG stNbConfig;
	ST_IOT_CONFIG stIotConfig;
	//ST_RUNTIME_CONFIG stRuntime;
	ST_ELEC_CONFIG stElecConfig,stElecRConfig;
	int ret = OPP_SUCCESS;
	int updateHisTime = 0, updateHisEc = 0, updateNbConfig = 0, updateIotConfig = 0, updateLampConfig = 0, updateWifiCfg = 0, updateLoc = 0, updateLocSrc = 0, updateClkSrc = 0, updateDevCap = 0;
	int updatePeriod = 0;
	int period = 0;
	int isReadLamp = 0, isReadIot = 0, isReadNb = 0;
	ST_WIFI_CONFIG stWifiConfig;
	ST_OPP_LAMP_LOCATION stLoc;
	ST_LOCSRC stLocSrc;
	ST_CLKSRC stClkSrc;
	ST_DEV_CONFIG stDevConfig;
	
	//for(i=0; i<sizeof(aucProperty)/sizeof(ST_PROP);i++)
	cJSON_ArrayForEach(object,prop)
	{		
		//object = cJSON_GetObjectItem(prop, aucProperty[i].name);
		//if(NULL == object){
		//	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%s is null\r\n", aucProperty[i].name);
		//	continue;
		//}
		i = ApsCoapFindPropIdxByName(object->string);
		if(i >= sizeof(aucProperty)/sizeof(ST_PROP)){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"no find %s\r\n", object->string);
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,PROP_W_ELE_ERR,PROP_W_ELE_ERR_DESC,NULL,mid);
			return;
		}
		
		ret = ApsCoapPropIsWriteValid(aucProperty[i].name);
		if(OPP_FAILURE == ret){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n op W unknow property %s\n", aucProperty[i].name);
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,PROP_W_ERR,PROP_W_ERR_DESC,NULL,mid);
			return;
		}
		
		if(OBJ_T  == aucProperty[i].type){
			if(0 == strcmp(aucProperty[i].name, "runtime")){
				subObj = cJSON_GetObjectItem(object,"rTime");
				if(NULL == subObj){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); \
					return;
				}
				if(cJSON_Number != subObj->type){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_NUMBER_ERR,JSON_TYPE_NUMBER_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valueint < 0){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,VAL_L_ERR,VAL_L_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valuedouble > INT_MAX){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,VAL_G_ERR,VAL_G_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valueint != subObj->valuedouble){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_FLOAT_ERR,JSON_TYPE_FLOAT_ERR_DESC,NULL,mid);
					return;
				}
				rTime = subObj->valueint;
				subObj = cJSON_GetObjectItem(object,"hTime");
				if(NULL == subObj){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); \
					return;
				}
				if(cJSON_Number != subObj->type){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_NUMBER_ERR,JSON_TYPE_NUMBER_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valueint < 0){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,VAL_L_ERR,VAL_L_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valuedouble > INT_MAX){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,VAL_G_ERR,VAL_G_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valueint != subObj->valuedouble){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_FLOAT_ERR,JSON_TYPE_FLOAT_ERR_DESC,NULL,mid);
					return;
				}
				updateHisTime = 1;
				//stRuntime.hisTime = temp;
				stElecRConfig.hisTime = subObj->valueint;
			}
			else if(0 == strcmp(aucProperty[i].name, "ecInfo")){
				subObj = cJSON_GetObjectItem(object,"EC");
				if(NULL == subObj){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); \
					return;
				}
				if(cJSON_Number != subObj->type){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_NUMBER_ERR,JSON_TYPE_NUMBER_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valueint < 0){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,VAL_L_ERR,VAL_L_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valueint > EC_MAX){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,EC_VAL_G_ERR,EC_VAL_G_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valueint != subObj->valuedouble){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_FLOAT_ERR,JSON_TYPE_FLOAT_ERR_DESC,NULL,mid);
					return;
				}
				consumption = subObj->valueint;
				subObj = cJSON_GetObjectItem(object,"HEC");
				if(NULL == subObj){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); \
					return;
				}
				if(cJSON_Number != subObj->type){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_NUMBER_ERR,JSON_TYPE_NUMBER_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valueint < 0){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,VAL_L_ERR,VAL_L_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valueint > EC_MAX){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,EC_VAL_G_ERR,EC_VAL_G_ERR_DESC,NULL,mid);
					return;
				}
				if(subObj->valueint != subObj->valuedouble){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_FLOAT_ERR,JSON_TYPE_FLOAT_ERR_DESC,NULL,mid);
					return;
				}
				updateHisEc = 1;
				stElecConfig.hisConsumption = subObj->valueint;
			}
			else if(0 == strcmp(aucProperty[i].name, "wifiCfg")){
				STRNCPY((char *)stWifiConfig.ssid,cJSON_GetObjectItem(object,"ssid")->valuestring,sizeof(stWifiConfig.ssid));
				STRNCPY((char *)stWifiConfig.password,cJSON_GetObjectItem(object,"password")->valuestring,sizeof(stWifiConfig.password));
				updateWifiCfg = 1;
			}
			else if(0 == strcmp(aucProperty[i].name, "locCfg")){
				subObj = cJSON_GetObjectItem(object,"lat");
				if(NULL == subObj){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					return;
				}
				if(cJSON_String != subObj->type){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_STRING_ERR,JSON_TYPE_STRING_ERR_DESC,NULL,mid);
					return;
				}
				if(0 == strlen(subObj->valuestring)){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_STRING_EMPTY_ERR,JSON_STRING_EMPTY_ERR_DESC,NULL,mid);
					return;
				}
				stLoc.ldLatitude=atof(subObj->valuestring);
				if(atof(subObj->valuestring) < -90 || atof(subObj->valuestring) > 90){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,LAT_VAL_ERR,LAT_VAL_ERR_DESC,NULL,mid);
					return;
				}
				subObj = cJSON_GetObjectItem(object,"lng");
				if(NULL == subObj){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					return;
				}
				if(cJSON_String != subObj->type){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_STRING_ERR,JSON_TYPE_STRING_ERR_DESC,NULL,mid);
					return;
				}
				if(0 == strlen(subObj->valuestring)){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_STRING_EMPTY_ERR,JSON_STRING_EMPTY_ERR_DESC,NULL,mid);
					return;
				}
				stLoc.ldLongitude=atof(subObj->valuestring);
				if(atof(subObj->valuestring) < -180 || atof(subObj->valuestring) > 180){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,LNG_VAL_ERR,LNG_VAL_ERR_DESC,NULL,mid);
					return;
				}
				if(stLoc.ldLatitude == INVALIDE_LAT && stLoc.ldLongitude == INVALIDE_LNG){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,LOC_VAL_ERR,LOC_VAL_ERR_DESC,NULL,mid);
					return;
				}
				updateLoc = 1;
			}
			else if(0 == strcmp(aucProperty[i].name, "devCap")){
				stDevConfig.nb = cJSON_GetObjectItem(object,"nb")->valueint;
				stDevConfig.wifi = cJSON_GetObjectItem(object,"wifi")->valueint;
				stDevConfig.gps = cJSON_GetObjectItem(object,"gps")->valueint;
				stDevConfig.meter = cJSON_GetObjectItem(object,"meter")->valueint;
				stDevConfig.sensor = cJSON_GetObjectItem(object,"sensor")->valueint;
				stDevConfig.rs485 = cJSON_GetObjectItem(object,"rs485")->valueint;
				stDevConfig.light = cJSON_GetObjectItem(object,"light")->valueint;
				updateDevCap = 1;
			}
			else if(0 == strcmp(aucProperty[i].name, "adjElec")){
				ret = VoltageAdjust(cJSON_GetObjectItem(object,"voltage")->valueint);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ADJ_VOL_ERR,ADJ_VOL_ERR_DESC,NULL,mid);
					return;
				}
				ret = CurrentAdjust(cJSON_GetObjectItem(object,"current")->valueint);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ADJ_CUR_ERR,ADJ_CUR_ERR_DESC,NULL,mid);
					return;
				}
				ret = PowerAdjust(cJSON_GetObjectItem(object,"power")->valueint);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ADJ_PWR_ERR,ADJ_PWR_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "watchdog")){
				TaskWdtConfig_EN config;
				config.period = cJSON_GetObjectItem(object,"period")->valueint;
				config.resetEn = cJSON_GetObjectItem(object,"reset")->valueint;
				ret = taskWdtConfigSet(&config);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,SET_WATCHDOG_ERR,SET_WATCHDOG_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "elecPara")){
				ret = VoltageParamSet(cJSON_GetObjectItem(object,"voltage")->valueint);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ELEC_CUR_PARA_SET_ERR,ELEC_CUR_PARA_SET_ERR_DESC,NULL,mid);
					return;
				}
				ret = CurrentParamSet(cJSON_GetObjectItem(object,"current")->valueint);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ELEC_VOL_PARA_SET_ERR,ELEC_VOL_PARA_SET_ERR_DESC,NULL,mid);
					return;
				}
				ret = PowerParamSet(cJSON_GetObjectItem(object,"power")->valueint);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ELEC_PWR_PARA_SET_ERR,ELEC_PWR_PARA_SET_ERR_DESC,NULL,mid);
					return;
				}
				ret = BPhCalParamSet(cJSON_GetObjectItem(object,"phase")->valueint);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ELEC_PHASE_PARA_SET_ERR,ELEC_PHASE_PARA_SET_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "attachDisPara")){
				ST_DISCRETE_SAVE_PARA stDisSavePara;
			
				if(cJSON_GetObjectItem(object,"window")->valueint > DIS_WINDOW_MAX_SECOND){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ATTACH_DISCRETE_WIN_SET_ERR,ATTACH_DISCRETE_WIN_SET_ERR_DESC,NULL,mid);
					return;
				}
				
				stDisSavePara.udwDiscreteWin=cJSON_GetObjectItem(object,"window")->valueint;
				stDisSavePara.udwDiscreteInterval=cJSON_GetObjectItem(object,"interval")->valueint;
				printf("win %d interval %d\r\n",stDisSavePara.udwDiscreteWin,stDisSavePara.udwDiscreteInterval);
				ret = NeulBc28SetDiscreteParaToFlash(&stDisSavePara);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ATTACH_DISCRETE_PARA_SET_ERR,ATTACH_DISCRETE_PARA_SET_ERR_DESC,NULL,mid);
					return;
				}
				printf("write memory\r\n");
				NeulBc28SetDiscretePara(&stDisSavePara);
			}
			else if(0 == strcmp(aucProperty[i].name, "heartDisPara")){
				ST_DIS_HEART_SAVE_PARA stDisHeartSavePara;
			
				if(cJSON_GetObjectItem(object,"window")->valueint > DIS_WINDOW_MAX_SECOND){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ATTACH_DISCRETE_WIN_SET_ERR,ATTACH_DISCRETE_WIN_SET_ERR_DESC,NULL,mid);
					return;
				}
				
				stDisHeartSavePara.udwDiscreteWin=cJSON_GetObjectItem(object,"window")->valueint;
				stDisHeartSavePara.udwDiscreteInterval=cJSON_GetObjectItem(object,"interval")->valueint;
				printf("win %d interval %d\r\n",stDisHeartSavePara.udwDiscreteWin,stDisHeartSavePara.udwDiscreteInterval);
				ret = ApsCoapHeartDisParaSetToFlash(&stDisHeartSavePara);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ATTACH_DISCRETE_PARA_SET_ERR,ATTACH_DISCRETE_PARA_SET_ERR_DESC,NULL,mid);
					return;
				}
				printf("write memory\r\n");
				ApsCoapHeartDisParaSet(&stDisHeartSavePara);
			}
			else if(0 == strcmp(aucProperty[i].name, "nbPara")){
				U8 enable;
				U16 earfcn,band;
				if(CHL_NB == dstChl){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,NOT_SUPPORT_CFG_BY_NB_ERR,NOT_SUPPORT_CFG_BY_NB_ERR_DESC,NULL,mid);
					return;
				}
				subObj = cJSON_GetObjectItem(object,"enable");
				if(NULL == subObj){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					return;
				}
				enable = subObj->valueint;
				if(0 != enable && 1 != enable){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,NBPARA_ENABLE_PARA_ERROR,NBPARA_ENABLE_PARA_ERROR_DESC,NULL,mid);
					return;
				}
				subObj = cJSON_GetObjectItem(object,"earfcn");
				if(NULL == subObj){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					return;
				}
				earfcn = subObj->valueint;
				if(earfcn > SHORT_MAX){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,NBPARA_EARFCN_PARA_ERROR,NBPARA_EARFCN_PARA_ERROR_DESC,NULL,mid);
					return;
				}
				subObj = cJSON_GetObjectItem(object,"band");
				if(NULL == subObj){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					return;
				}
				band = subObj->valueint;
				if(band != 5 && band != 8 && band != 3 && band != 28 && band != 20 && band != 1){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,NBPARA_BAND_PARA_ERROR,NBPARA_BAND_PARA_ERROR_DESC,NULL,mid);
					return;
				}
				NeulBc28SetParaToRam(enable,earfcn,band);
			}
		}
		else if(STR_T == aucProperty[i].type){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%s %d\r\n", aucProperty[i].name, temp);
			p = object->valuestring;
			if(0 != strlen(p))
			{
				if(0 == strcmp(aucProperty[i].name, "ocIp")){
					if(CHL_NB == dstChl){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,NOT_SUPPORT_CFG_BY_NB_ERR,NOT_SUPPORT_CFG_BY_NB_ERR_DESC,NULL,mid);
						return;
					}
					if(!isReadIot){
						OppCoapIOTGetConfig(&stIotConfig);
						isReadIot = 1;
					}
					if(!is_ipv4_address(p)){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,IPV4_ADDR_FORMAT_ERROR,IPV4_ADDR_FORMAT_ERROR_DESC,NULL,mid);
						return;
					}
					STRNCPY(stIotConfig.ocIp, p, sizeof(stIotConfig.ocIp));
					//stIotConfig.ocIp[strlen(p)] = '\0';
					updateIotConfig = 1;
				}
				else if(0 == strcmp(aucProperty[i].name, "apnAddres")){
					if(CHL_NB == dstChl){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,NOT_SUPPORT_CFG_BY_NB_ERR,NOT_SUPPORT_CFG_BY_NB_ERR_DESC,NULL,mid);
						return;
					}
					if(!isReadNb){
						NeulBc28GetConfig(&stNbConfig);
						isReadNb = 1;
					}
					STRNCPY(stNbConfig.apn, p, APN_NAME_LEN);
					//stNbConfig.apn[strlen(p)] = '\0';
					updateNbConfig = 1;
				}
			}
		}else if(INT_T == aucProperty[i].type){
			if(cJSON_Number != object->type){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_NUMBER_ERR,JSON_TYPE_NUMBER_ERR_DESC,NULL,mid);
				return;
			}			
			if(object->valueint < 0){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,VAL_L_ERR,VAL_L_ERR_DESC,NULL,mid);
				return;
			}
			if(object->valuedouble > INT_MAX){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,VAL_G_ERR,VAL_G_ERR_DESC,NULL,mid);
				return;
			}
			if(object->valueint != object->valuedouble){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_TYPE_FLOAT_ERR,JSON_TYPE_FLOAT_ERR_DESC,NULL,mid);
				return;
			}
			temp = object->valueint;			
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%s temp %d\r\n", aucProperty[i].name, temp);
			
			if(0 == strcmp(aucProperty[i].name, "switch")){
				if(-1 != temp){
					if(temp != 0 && temp != 1){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,SWITCH_VAL_ERR,SWITCH_VAL_ERR_DESC,NULL,mid);
						return;
					}
					if(dstChl == CHL_NB)
						OppLampCtrlOnOff(NB_SRC, temp);
					else if(dstChl == CHL_WIFI)
						OppLampCtrlOnOff(WIFI_SRC, temp);
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "bri")){
				if(-1 != temp){
					if(temp < BRI_MIN|| temp > BRI_MAX){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,BRI_VAL_ERR,BRI_VAL_ERR_DESC,NULL,mid);
						return;
					}
					if(dstChl == CHL_NB)
						OppLampCtrlSetDimLevel(NB_SRC, temp);
					else if(dstChl == CHL_WIFI)
						OppLampCtrlSetDimLevel(WIFI_SRC, temp);
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "ocPort")){	
				if(CHL_NB == dstChl){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,NOT_SUPPORT_CFG_BY_NB_ERR,NOT_SUPPORT_CFG_BY_NB_ERR_DESC,NULL,mid);
					return;
				}
				if(-1 != temp){
					if(temp < 0 || temp > 65535){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,OCPORT_VAL_ERR,OCPORT_VAL_ERR_DESC,NULL,mid);
						return;
					}
					if(!isReadIot){
						OppCoapIOTGetConfig(&stIotConfig);
						isReadIot = 1;
					}
					stIotConfig.ocPort= temp;
					updateIotConfig = 1;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "band")){
				if(CHL_NB == dstChl){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,NOT_SUPPORT_CFG_BY_NB_ERR,NOT_SUPPORT_CFG_BY_NB_ERR_DESC,NULL,mid);
					return;
				}
				if(-1 != temp){
					if(temp != 1 && temp != 3 && temp != 5 && temp != 8 && temp != 20){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,BAND_VAL_ERR,BAND_VAL_ERR_DESC,NULL,mid);
						return;
					}
					if(!isReadNb){
						NeulBc28GetConfig(&stNbConfig);
						isReadNb = 1;
					}
					updateNbConfig = 1;
					stNbConfig.band = temp;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "scram")){
				if(CHL_NB == dstChl){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,NOT_SUPPORT_CFG_BY_NB_ERR,NOT_SUPPORT_CFG_BY_NB_ERR_DESC,NULL,mid);
					return;
				}
				if(-1 != temp){
					if(!isReadNb){
						NeulBc28GetConfig(&stNbConfig);
						isReadNb = 1;
					}
					updateNbConfig = 1;
					stNbConfig.scram= temp;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "dimType")){
				if(-1 != temp){
					if(temp != 0 && temp != 1){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,DIMT_VAL_ERR,DIMT_VAL_ERR_DESC,NULL,mid);
						return;
					}
					if(!isReadLamp){
						OppLampCtrlGetConfigPara(&para);
						isReadLamp = 1;
					}
					updateLampConfig = 1;
					dimType = temp;
					para.dimType = temp;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "tDimType")){
				if(-1 != temp){
					if(temp != 0 && temp != 1){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,DIMT_VAL_ERR,DIMT_VAL_ERR_DESC,NULL,mid);
						return;
					}
					LampOuttypeSet(temp);
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "period")){
				if(-1 != temp){
					if(temp < 0 || temp > INT_MAX){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,PERIOD_VAL_ERR,PERIOD_VAL_ERR_DESC,NULL,mid);
						return;
					}
					updatePeriod = 1;
					period = temp;						
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "locSrc")){
				if(-1 != temp){
					if(temp >= UNKNOW_LOC){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,LOC_VAL_ERR,LOC_VAL_ERR_DESC,NULL,mid);
						return;

					}
					stLocSrc.locSrc = temp;
					updateLocSrc = 1;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "clkSrc")){
				if(-1 != temp){
					if(temp >= UNKNOW_CLK){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,CLK_VAL_ERR,CLK_VAL_ERR_DESC,NULL,mid);
						return;

					}
					stClkSrc.clkSrc = temp;
					updateClkSrc = 1;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "earfcn")){
				/*if(!isReadIot){
					NeulBc28GetConfig(&stNbConfig);
					isReadIot = 1;
				}
				stNbConfig.earfcn = temp;
				updateNbConfig = 1;*/
				if(-1 != temp){
					if(temp < 0 || temp > SHORT_MAX){
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,PERIOD_VAL_ERR,PERIOD_VAL_ERR_DESC,NULL,mid);
						return;
					}
				}
				NeulBc28SetEarfcnToRam(1,temp);
			}
			else if(0 == strcmp(aucProperty[i].name, "adjPhase")){
				ret = BPhCalAdjust(temp);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ADJ_PHASE_ERR,ADJ_PHASE_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "adjAdc64")){
				ret = DacLevel64ParamSet(temp);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ADJ_ADC_64_ERR,ADJ_ADC_64_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "adjAdc192")){
				ret = DacLevel192ParamSet(temp);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,ADJ_ADC_192_ERR,ADJ_ADC_192_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "blink")){
				if(0 == temp){
					ret = LampBlink(0,100,10000,2,1,86400);
				}else{
					ret = LampBlink(1,100,10000,2,1,86400);
				}
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,BLINK_ERR,BLINK_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "rxProtect")){
				if(0 == temp){
					ApsCoapRxProtectSet(RX_PROTECT_DISABLE);
				}else{
					ApsCoapRxProtectSet(RX_PROTECT_ENABLE);
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "attachDisEnable")){
				if(0 == temp){
					NeulBc28SetDisEnable(DIS_DISABLE);
				}else{
					NeulBc28SetDisEnable(DIS_ENABLE);
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "testCur")){
				ret = SvsTestCurSet(temp);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,TEST_SET_ERR,TEST_SET_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "test2")){
				ret = SvsTestInfoSet(TEST_C1,temp);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,TEST_SET_ERR,TEST_SET_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "test3")){
				ret = SvsTestInfoSet(TEST_AG,temp);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,TEST_SET_ERR,TEST_SET_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "test4")){
				ret = SvsTestInfoSet(TEST_C2,temp);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,TEST_SET_ERR,TEST_SET_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "test5")){
				ret = SvsTestInfoSet(TEST_REMAIN0,temp);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,TEST_SET_ERR,TEST_SET_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "test6")){
				ret = SvsTestInfoSet(TEST_REMAIN1,temp);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,TEST_SET_ERR,TEST_SET_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "test7")){
				ret = SvsTestInfoSet(TEST_REMAIN2,temp);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,TEST_SET_ERR,TEST_SET_ERR_DESC,NULL,mid);
					return;
				}
			}
			else if(0 == strcmp(aucProperty[i].name, "test8")){
				ret = SvsTestInfoSet(TEST_REMAIN3,temp);
				if(OPP_SUCCESS != ret){
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,TEST_SET_ERR,TEST_SET_ERR_DESC,NULL,mid);
					return;
				}
			}
		}

	}					
		//保存flash
		if(updateHisTime){
			/*ret = OppLampCtrlSetRuntimeParaToFlash(&stRuntime);
			if(OPP_SUCCESS != ret){
				ApsCoapRsp(dstChl,isNeedRsp,reqId,PROP_SERVICE,0,WRITE_FLASH_ERR,WRITE_FLASH_ERR_DESC,NULL,mid);
				return;
			}*/
			stElecRConfig.hisConsumption = ElecHisConsumptionGetInner();
			ret = ElecWriteFlash(&stElecRConfig);
			if(OPP_SUCCESS != ret){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,WRITE_FLASH_ERR,WRITE_FLASH_ERR_DESC,NULL,mid);
				return;
			}
			OppLampCtrlSetHtime(0,stElecRConfig.hisTime);
			OppLampCtrlSetRtime(0,rTime);
		}
		if(updateHisEc){
			OppLampCtrlGetHtime(0,&stElecConfig.hisTime);
			ret = ElecWriteFlash(&stElecConfig);
			if(OPP_SUCCESS != ret){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,WRITE_FLASH_ERR,WRITE_FLASH_ERR_DESC,NULL,mid);
				return;
			}
			ElecConsumptionSet(consumption);
			ElecHisConsumptionSet(stElecConfig.hisConsumption);
		}
		if(updateWifiCfg){
			ret = ApsWifiConfigWithoutStart((char *)stWifiConfig.ssid, (char *)stWifiConfig.password);
			if(OPP_SUCCESS != ret){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,WRITE_FLASH_ERR,WRITE_FLASH_ERR_DESC,NULL,mid);
				return;
			}
			g_iWifiConTick = OppTickCountGet();
		}
		if(updateLoc){
			ret = LocCfgSetToFlash(&stLoc);
			if(OPP_SUCCESS != ret){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,WRITE_FLASH_ERR,WRITE_FLASH_ERR_DESC,NULL,mid);
				return;
			}
			LocSetCfgLat(stLoc.ldLatitude);
			LocSetCfgLng(stLoc.ldLongitude);
			LocSetLat(stLoc.ldLatitude);
			LocSetLng(stLoc.ldLongitude);
		}
		if(updateLocSrc){
			ret = LocSetSrcToFlash(&stLocSrc);
			if(OPP_SUCCESS != ret){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,WRITE_FLASH_ERR,WRITE_FLASH_ERR_DESC,NULL,mid);
				return;
			}
			LocSetSrc(stLocSrc.locSrc);
		}
		if(updateClkSrc){
			ret = TimeSetClkSrcToFlash(&stClkSrc);
			if(OPP_SUCCESS != ret){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,WRITE_FLASH_ERR,WRITE_FLASH_ERR_DESC,NULL,mid);
				return;
			}
			TimeSetClockSrc(stClkSrc.clkSrc);
		}
		if(updateDevCap){
			ret = ApsDevConfigWriteToFlash(&stDevConfig);
			if(OPP_SUCCESS != ret){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,WRITE_FLASH_ERR,WRITE_FLASH_ERR_DESC,NULL,mid);
				return;
			}
			ApsDevConfigWrite(&stDevConfig);
		}
		if(updateLampConfig){
			ret = OppLampCtrlSetConfigPara(&para);
			if(OPP_SUCCESS != ret){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,WRITE_FLASH_ERR,WRITE_FLASH_ERR_DESC,NULL,mid);
				return;
			}

			if(-1 != dimType){
				LampOuttypeSet(dimType);
			}
		}
		if(updatePeriod){
			AD_CONFIG_T config;			
			config.resource = RSC_HEART;
			config.enable |= MCA_REPORT;
			config.targetid = 0;
			config.reportIf = RI_TIMING;
			config.reportIfPara = period;
			ret = ApsDaemonActionItemParaSetCustom(CUSTOM_REPORT, HEARTBEAT_PERIOD_IDX, &config);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,PROPCONFIG_PARA_SET_ERR,PROPCONFIG_PARA_SET_ERR_DESC,NULL,mid);
				return;
			}
		}
		if(updateIotConfig){
			ret = OppCoapIOTSetConfigToFlash(&stIotConfig);
			if(OPP_SUCCESS != ret){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,WRITE_FLASH_ERR,WRITE_FLASH_ERR_DESC,NULL,mid);
				return;
			}
			OppCoapIOTSetConfig(&stIotConfig);
		}
		if(updateNbConfig){
			ret = NeulBc28SetConfigToFlash(&stNbConfig);
			if(OPP_SUCCESS != ret){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,WRITE_FLASH_ERR,WRITE_FLASH_ERR_DESC,NULL,mid);
				return;
			}
			NeulBc28SetConfig(&stNbConfig);
		}
		ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,SUCCESS,NULL,NULL,mid);

		return;
}


void ApsCoapPropProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, char *text, U32 mid)
{
	char *out;
	cJSON *json, *prop;
	int reqId = 0;
	char op[10]={0};
	int ret = OPP_SUCCESS;
	
	json=cJSON_Parse(text);
	if (!json) {
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"Error before: [%s]\n",cJSON_GetErrorPtr());		
		ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,0,PROP_SERVICE,0,JSON_FORMAT_ERR,JSON_FORMAT_ERR_DESC,NULL,mid);
		return;
	}else
	{
		out=cJSON_Print(json);
		if(NULL == out){
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,0,PROP_SERVICE,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			cJSON_Delete(json);
			return;
		}
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%s\n",out);
		myfree(out);

		APS_COAPS_JSON_FORMAT_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,0,PROP_SERVICE,json,"reqId",mid);
		APS_COAPS_JSON_TYPE_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,0,PROP_SERVICE,cJSON_GetObjectItem(json, "reqId"),cJSON_Number,mid);
		reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
		if(reqId <= 0){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n json id is %d\r\n",reqId);
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_ID_ZERO_ERR,JSON_ID_ZERO_ERR_DESC,NULL,mid);
			cJSON_Delete(json);
			return;
		}
		
		APS_COAPS_JSON_FORMAT_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,reqId,PROP_SERVICE,json,"op",mid);
		APS_COAPS_JSON_TYPE_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,0,PROP_SERVICE,cJSON_GetObjectItem(json, "op"),cJSON_String,mid);		
		STRNCPY(op, cJSON_GetObjectItem(json, "op")->valuestring,sizeof(op));
		ret = ApsCoapOpIsValid(op);
		if(OPP_SUCCESS != ret)
		{
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_OP_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"\r\n op action is error %s", op);
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,PROP_OP_ERR,PROP_OP_ERR_DESC,NULL,mid);
			cJSON_Delete(json);
			return;
		}
		
		APS_COAPS_JSON_FORMAT_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,reqId,PROP_SERVICE,json,"prop",mid);
		prop = cJSON_GetObjectItem(json, "prop");
		//debug
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"reqid %d, op %s\r\n", reqId, op);
		
		if(0 == strcmp(op,"R")){
			APS_COAPS_JSON_TYPE_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,0,PROP_SERVICE,cJSON_GetObjectItem(json, "prop"),cJSON_Array,mid);		
			ApsCoapPropProcR(dstChl,dstInfo,isNeedRsp,reqId, prop, mid);
		}else if(0 == strcmp(op,"W")){
			APS_COAPS_JSON_TYPE_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,0,PROP_SERVICE,cJSON_GetObjectItem(json, "prop"),cJSON_Object,mid);		
			ApsCoapPropProcW(dstChl,dstInfo,isNeedRsp,reqId, prop, mid);
		}else if(0 == strcmp(op,"RE")){
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,PROP_RE_OP_ERR,PROP_RE_OP_ERR_DESC,NULL,mid);			
		}else{
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,PROP_UNKONW_OP_ERR,PROP_UNKONW_OP_ERR_DESC,NULL,mid);			
		}
		//释放节点
		cJSON_Delete(json);
	}
}


int ApsCoapWithoutInterNightElapsedTime(ST_WORK_SCHEME *pstPlanScheme, int AInterNightPos)
{
	int i = 0;
	int elapsedTime = 0;


	ST_WORK_SCHEME *ptr;
	
	if(NULL == pstPlanScheme)
		return 1;

	ptr = pstPlanScheme;


	if(AInterNightPos == -1){
		for(i=0;i<ptr->jobNum;i++){
			elapsedTime += (ptr->jobs[i].endHour*60+ptr->jobs[i].endMin) -  (ptr->jobs[i].startHour*60+ptr->jobs[i].startMin);
			if(i+1 != ptr->jobNum)
				elapsedTime += (ptr->jobs[i+1].startHour*60+ptr->jobs[i+1].startMin) -	(ptr->jobs[i].endHour*60+ptr->jobs[i].endMin);
		}
	}else{
		for(i=0;i<ptr->jobNum;i++){
			elapsedTime += (ptr->jobs[i].endHour*60+ptr->jobs[i].endMin) -  (ptr->jobs[i].startHour*60+ptr->jobs[i].startMin);
			if(i+1 == AInterNightPos)
				elapsedTime += ((ptr->jobs[i+1].startHour+24)*60+ptr->jobs[i+1].startMin) -  (ptr->jobs[i].endHour*60+ptr->jobs[i].endMin);
			else{
				if(i+1 != ptr->jobNum)
					elapsedTime += (ptr->jobs[i+1].startHour*60+ptr->jobs[i+1].startMin) -  (ptr->jobs[i].endHour*60+ptr->jobs[i].endMin);
			}
		}
	}

	if(elapsedTime>1440){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"without interNight entry elapsed time is %dh over 1440(Min)\r\n", elapsedTime);
		return 1;
	}


	return 0;

}

int ApsCoapWithInterNightElapsedTime(ST_WORK_SCHEME *pstPlanScheme, int pos)
{
	int i = 0;
	int elapsedTime = 0;
	ST_WORK_SCHEME *ptr = pstPlanScheme;
	
	if(NULL == ptr)
		return 1;


	for(i=0;i<pos;i++){
		elapsedTime+=(ptr->jobs[i].endHour*60+ptr->jobs[i].endMin) -  (ptr->jobs[i].startHour*60+ptr->jobs[i].startMin);
		elapsedTime+=(ptr->jobs[i+1].startHour*60+ptr->jobs[i].startMin) -  (ptr->jobs[i].endHour*60+ptr->jobs[i].endMin);
	}

	for(i=pos;i<ptr->jobNum;i++){
		if(i==pos){
			elapsedTime+=((ptr->jobs[i].endHour+24)*60+ptr->jobs[i].endMin) -  ((ptr->jobs[i].startHour)*60+ptr->jobs[i].startMin);
		}else{
			elapsedTime+=(ptr->jobs[i].endHour*60+ptr->jobs[i].endMin) -  (ptr->jobs[i].startHour*60+ptr->jobs[i].startMin);
		}

		if(i+1 != ptr->jobNum)
			elapsedTime+=(ptr->jobs[i+1].startHour*60+ptr->jobs[i].startMin) -  (ptr->jobs[i].endHour*60+ptr->jobs[i].endMin);

	}
	if(elapsedTime>1440){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"with interNight entry elapsed time is %d over 1440(Min)\r\n", elapsedTime);
		return 1;
	}

	return 0;
}
/*
*0: no error
*1: with inter night > 1
*2: with cross inter night > 1
*3: with inter night and cross inter night > 1
*4: all entry over 24H
*/
int ApsCoapJobsTimeIsValide(ST_WORK_SCHEME *pstPlanScheme)
{
	int i = 0, n1 = 0, interNight = 0, crossInterNight = 0, pos = -1;
	char sTime[6], eTime[6];
	ST_WORK_SCHEME *ptr;
	int ret;
	char buf1[128],buf2[128];
	int len1 = 0, len2 = 0;

	if(NULL == pstPlanScheme)
		return 1;

	ptr = pstPlanScheme;
	
	//粗略判断
	for(i=0;i<ptr->jobNum;i++){
		sprintf(sTime, "%02d:%02d", ptr->jobs[i].startHour, ptr->jobs[i].startMin);
		sprintf(eTime, "%02d:%02d", ptr->jobs[i].endHour, ptr->jobs[i].endMin);
		n1 = CompareHM(sTime,eTime);
		//time over 24H
		if(2 == n1 && ptr->jobNum > 1){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "timer over 24h %d\r\n", i);		
			return 4;
		}
		if(1 == n1){
			len1 = sprintf(buf1+len1, "%d ", i);
			pos = i;
			interNight++;
		}
	}
	//only one inter day
	if(interNight > 1){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "more than one interNight %s\r\n", buf1);		
		return 1;
	}

	for(i=0;i<ptr->jobNum-1;i++){		
		sprintf(eTime, "%02d:%02d", ptr->jobs[i].endHour, ptr->jobs[i].endMin);
		sprintf(sTime, "%02d:%02d", ptr->jobs[i+1].startHour, ptr->jobs[i+1].startMin);
		//午夜分割点
		n1 = CompareHM(eTime,sTime);
		if(1 == n1){
			crossInterNight++;
			pos = i+1;/*after inter night index*/
			len2 = sprintf(buf2+len2,"[%d].eTime:%s>[%d].sTime:%s\r\n",i,sTime,i+1,eTime);
		}
	}
	//only one cross inter day
	if(crossInterNight>1){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"with more than one crossInterNight:\r\n%s", buf2);
		return 2;
	}

	if(interNight+crossInterNight >1){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"with InterNight:%s\r\ncrossInterNight:%s more than one\r\n", buf1, buf2);
		return 3;
	}
	//精细判断
	if(interNight){
		ret = ApsCoapWithInterNightElapsedTime(pstPlanScheme,pos);
	}else{
		ret = ApsCoapWithoutInterNightElapsedTime(pstPlanScheme,pos);
	}
	if(0 != ret){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "all entry over 24H\r\n");
		return 4;
	}

	return 0;
}

int ApsCoapAstJobsTimeIsValide(ST_WORK_SCHEME *pstPlanScheme)
{
	int i = 0;
	int elapseTime = 0;
	
	if(NULL == pstPlanScheme)
		return 1;

	for(i=0;i<pstPlanScheme->jobNum;i++){
		elapseTime += pstPlanScheme->jobs[i].intervTime;
	}

	if(elapseTime > 1440){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ast time over 24H\r\n");
		return 2;
	}

	return 0;
}
int ApsGetPropIdByName(char *propName, int * index)
{
	int i = 0;

	for(i=0;i<sizeof(aucPropMap)/sizeof(ST_PROP_MAP);i++){
		if(0 == strcmp(propName, aucPropMap[i].propName)){
			*index = i;
			return OPP_SUCCESS;
		}
	}
	return OPP_FAILURE;
}

/*
int ApsCoapThresholdProc(unsigned char dstChl, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *root, * cmdData, *para, *object, *alarmObj, *updateObj, *subObj, *cmdDataRe, *batArray, *item;
	char op[10]={0};
	AD_CONFIG_T config;
	int targetId = -1, resId = -1, condition = -1, paraType = -1;
	U32 id;
	int isAlarmEnable = -1, isLogEnable = -1;
	int isReportEnable = -1;
	int reqId = -1;
//	int type;	
	int arraySize = -1;
	int i = 0;
	int ret = OPP_SUCCESS;

	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	strcpy(op, cJSON_GetObjectItem(json, "op")->valuestring);
	if(0 == strcmp(op,"W") || 0 == strcmp(op,"M")){
		memset(&config, 0, sizeof(AD_CONFIG_T));
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		object = cJSON_GetObjectItem(cmdData, "targetId");
		if(NULL != object){
			targetId = object->valueint;
		}
		if(0 == strcmp(op,"M")){
			object = cJSON_GetObjectItem(cmdData, "id");
			if(NULL != object){
				id = object->valueint;
			}
			
		}
		object = cJSON_GetObjectItem(cmdData, "resId");
		if(NULL != object){
			config.resource = object->valueint;
		}
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold resId %d\r\n", config.resource);
		object = cJSON_GetObjectItem(cmdData, "propName");
		if(NULL != object){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold propName %s %d\r\n",object->valuestring);
		}
	
		alarmObj = cJSON_GetObjectItem(cmdData, "alarm");
		if(NULL != alarmObj){
			object = cJSON_GetObjectItem(alarmObj, "alarmId");
			if(NULL != object){
				config.alarmId = object->valueint;
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold alarmId %d\r\n", config.alarmId);
			
			object = cJSON_GetObjectItem(alarmObj, "condition");
			if(NULL != object){
				condition = object->valueint;
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold condition %d\r\n", condition);
	
			para = cJSON_GetObjectItem(alarmObj, "para");
			if(NULL == para){
				ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,THRESHOLD_ALARM_PARA_ERR,THRESHOLD_ALARM_PARA_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			object = cJSON_GetObjectItem(para, "paraOp");
			if(NULL != object){
				config.alarmIf = object->valueint;
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold alarmPara paraOp %d\r\n", config.alarmIf);
			}
			object = cJSON_GetObjectItem(para, "paraType");
			if(NULL != object){
				paraType = object->valueint;
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold alarmPara paraType %d\r\n", paraType);
			}
			object = cJSON_GetObjectItem(para, "value");
			if(NULL != object){
				config.alarmIfPara1 = object->valueint;
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold alarmPara value %d\r\n", config.alarmIfPara1);
			}
			object = cJSON_GetObjectItem(para, "isAlarmReport");
			if(NULL != object){
				isAlarmEnable = object->valueint;
				if(isAlarmEnable)
					config.enable |= MCA_ALARM;
			}
			object = cJSON_GetObjectItem(para, "isLogEnable");
			if(NULL != object){
				isLogEnable = object->valueint;
				if(isLogEnable)
					config.enable |= MCA_LOG;
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold isLogEnable %d\r\n", isLogEnable);
		}
	
		updateObj = cJSON_GetObjectItem(cmdData, "update");
		if(NULL != updateObj){
			object = cJSON_GetObjectItem(updateObj, "isReportEnable");
			if(NULL != object){
				isReportEnable = object->valueint;
				if(isReportEnable)
					config.enable |= MCA_REPORT;
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold isReportEnable %d\r\n", isReportEnable);
			
			object = cJSON_GetObjectItem(updateObj, "updateType");
			if(NULL != object){
				config.reportIf = object->valueint;
			}
			object = cJSON_GetObjectItem(updateObj, "paraType");
			if(NULL != object){
				paraType = object->valueint;
			}
	
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold paraType %d\r\n", paraType);
			
			object = cJSON_GetObjectItem(updateObj, "value");
			if(NULL != object){
				config.reportIfPara = object->valueint;
			}
		}					
		
		if(0 == strcmp(op,"W")){
			ret = ApsDaemonActionItemParaAdd(&id, &config);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,THRESHOLD_PARA_SET_ERR,THRESHOLD_PARA_SET_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			//返回id
			cmdDataRe = cJSON_CreateObject();
			cJSON_AddItemToObject(cmdDataRe, "id", cJSON_CreateNumber(id));
			
			ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,0,NULL,cmdDataRe,mid);			
		}else{
			ret = ApsDaemonActionItemParaSet(id, &config);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,THRESHOLD_PARA_SET_ERR,THRESHOLD_PARA_SET_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,0,NULL,NULL,mid);
		}
	}
	else if(0 == strcmp(op,"R")){
		if(0 == isNeedRsp){
			return OPP_SUCCESS;
		}
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		object = cJSON_GetObjectItem(cmdData, "ids");
		if(NULL == object){
			ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);			
			return OPP_FAILURE;
		}
		arraySize = cJSON_GetArraySize(object);
		if(arraySize == 0){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold no data here\r\n");
			ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,THRESHOLD_IDS_R_EMPTY_ERR,THRESHOLD_IDS_R_EMPTY_ERR_DESC,NULL,mid);
			return OPP_FAILURE; 					
		}
		for(i=0;i<arraySize;i++){
			item = cJSON_GetArrayItem(object,i);
			ret = ApsDaemonActionItemParaGet(item->valueint,&config);
			if(OPP_SUCCESS != ret){
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%d get config ret %d\r\n", i,ret);
				ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,THRESHOLD_IDS_R_CONFIG_ERR,THRESHOLD_IDS_R_CONFIG_ERR_DESC,NULL,mid);
				return OPP_FAILURE; 					
			}
		}
		ApsCoapFuncRspBatchStart(dstChl, reqId, 0, 0, NULL, THRESHOLD_CMDID, &root, &cmdData);
		cJSON_AddItemToObject(cmdData, "thArray", batArray = cJSON_CreateArray());
		for(i=0;i<arraySize;i++){
			item = cJSON_GetArrayItem(object,i);
			ret = ApsDaemonActionItemParaGet(item->valueint,&config);
			if(OPP_SUCCESS == ret){
				ApsCoapThresholdRspBatchAppend(root,cmdData,batArray,item->valueint,CUSTOM_ALL, &config);
			}
		}
		ret = ApsCoapFuncRspBatchStop(dstChl, root, mid);
		if(NBIOT_SEND_BIG_ERR == ret){
			ApsCoapRsp(dstChl,isNeedRsp,reqId,0,PROP_SERVICE,NBIOT_SEND_BIG_ERR,NBIOT_SEND_BIG_ERR_DESC,NULL,mid);
		}
	}				
	else if(0 == strcmp(op,"D")){
		memset(&config, 0, sizeof(AD_CONFIG_T));
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		if(NULL == cmdData){
			ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		object = cJSON_GetObjectItem(cmdData, "ids");
		if(NULL == object){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%d get config ret %d\r\n", i,ret);
			ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,THRESHOLD_IDS_D_ERR,THRESHOLD_IDS_D_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ids object\r\n");
		
		arraySize = cJSON_GetArraySize(object);
		if(arraySize == 0){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"threshold no data here\r\n");
			ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,THRESHOLD_IDS_D_EMPTY_ERR,THRESHOLD_IDS_D_EMPTY_ERR_DESC,NULL,mid);
			return OPP_FAILURE; 					
		}
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ids array size %d\r\n",arraySize);
		
		for(i=0;i<arraySize;i++){
			item = cJSON_GetArrayItem(object,i);
			subObj = cJSON_GetObjectItem(item,"id");
			if(NULL != subObj){
				config.resource= subObj->valueint;
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%d id:%d\r\n",i,config.resource);
	
			//删除
			ret = ApsDaemonActionItemParaDel(item->valueint);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,THRESHOLD_IDS_D_SET_ERR,THRESHOLD_IDS_D_SET_ERR_DESC,NULL,mid);
				return OPP_FAILURE; 					
			}
		}
		ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLD_CMDID,reqId,0,0,NULL,NULL,mid);
		
	}
	return OPP_SUCCESS;
}
*/
int ApsCoapAlarmProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *root, * cmdData, *para, *object, *alarmObj, *cmdDataRe, *batArray, *item;
	char op[10]={0};
	AD_CONFIG_T config;
	int  condition = -1, paraType = -1;
	U32 id;
	int isAlarmEnable = -1, isLogEnable = -1;
	int reqId = 0;
	int arraySize = -1;
	int i = 0;
	int ret = OPP_SUCCESS;
	int idx;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARM_CMDID,json,"reqId",mid);
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARM_CMDID,json,"op",mid);
	STRNCPY(op, cJSON_GetObjectItem(json, "op")->valuestring,sizeof(op));
	if(0 == strcmp(op,"W") || 0 == strcmp(op,"M")){
		memset(&config, 0, sizeof(AD_CONFIG_T));
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		object = cJSON_GetObjectItem(cmdData, "targetId");
		if(NULL != object){
			//targetId = object->valueint;
		}
		if(0 == strcmp(op,"M")){
			object = cJSON_GetObjectItem(cmdData, "id");
			if(NULL != object){
				id = object->valueint;
			}	
		}
		object = cJSON_GetObjectItem(cmdData, "resId");
		if(NULL != object){
			config.resource = object->valueint;
		}
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm resId %d\r\n", config.resource);
		object = cJSON_GetObjectItem(cmdData, "propName");
		if(NULL != object){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm propName %s %d\r\n",object->valuestring);
		}
	
		alarmObj = cJSON_GetObjectItem(cmdData, "alarm");
		if(NULL != alarmObj){
			object = cJSON_GetObjectItem(alarmObj, "alarmId");
			if(NULL != object){
				config.alarmId = object->valueint;
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm alarmId %d\r\n", config.alarmId);
			
			object = cJSON_GetObjectItem(alarmObj, "condition");
			if(NULL != object){
				condition = object->valueint;
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm condition %d\r\n", condition);
	
			para = cJSON_GetObjectItem(alarmObj, "para");
			if(NULL == para){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,ALARM_PARA_ERR,ALARM_PARA_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			object = cJSON_GetObjectItem(para, "paraOp");
			if(NULL != object){
				config.alarmIf = object->valueint;
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm alarmPara paraOp %d\r\n", config.alarmIf);
			}
			object = cJSON_GetObjectItem(para, "paraType");
			if(NULL != object){
				paraType = object->valueint;
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm alarmPara paraType %d\r\n", paraType);
			}
			object = cJSON_GetObjectItem(para, "value");
			if(NULL != object){
				config.alarmIfPara1 = object->valueint;
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm alarmPara value %d\r\n", config.alarmIfPara1);
			}
			object = cJSON_GetObjectItem(para, "isAlarmReport");
			if(NULL != object){
				isAlarmEnable = object->valueint;
				if(isAlarmEnable)
					config.enable |= MCA_ALARM;
				else
					config.enable &= ~MCA_ALARM;
			}
			object = cJSON_GetObjectItem(para, "isLogEnable");
			if(NULL != object){
				isLogEnable = object->valueint;
				if(isLogEnable)
					config.enable |= MCA_LOG;
				else
					config.enable &= ~MCA_LOG;
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm isLogEnable %d\r\n", isLogEnable);
		}

		if(0 == strcmp(op,"W")){
			ret = ApsCoapFindResMapIdx(config.resource,&idx);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_ERR,RESID_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			if(!(aucResMap[idx].auth & W)){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_W_ERR,RESID_W_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			ret = ApsDaemonActionItemParaAddCustom(CUSTOM_ALARM, &id, &config);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,ALARM_PARA_SET_ERR,ALARM_PARA_SET_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			//返回id
			cmdDataRe = cJSON_CreateObject();
			if(NULL == cmdDataRe){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapAlarmProc create <cmdDataRe> object error\r\n");		
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			cJSON_AddItemToObject(cmdDataRe, "id", cJSON_CreateNumber(id));
			
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,0,NULL,cmdDataRe,mid); 		
		}else{
			ret = ApsCoapFindResMapIdx(config.resource,&idx);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_ERR,RESID_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			if(!(aucResMap[idx].auth & M)){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_M_ERR,RESID_M_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
		
			ret = ApsDaemonActionItemParaSetCustom(CUSTOM_ALARM, id, &config);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,ALARM_PARA_SET_ERR,ALARM_PARA_SET_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,0,NULL,NULL,mid);
		}
	}
	else if(0 == strcmp(op,"R")){
		/*no need rsp*/
		if(0 == isNeedRsp){
			return OPP_SUCCESS;
		}
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		object = cJSON_GetObjectItem(cmdData, "ids");
		if(NULL == object){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);			
			return OPP_FAILURE;
		}
		arraySize = cJSON_GetArraySize(object);
		if(arraySize == 0){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm no data here\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,ALARM_IDS_R_EMPTY_ERR,ALARM_IDS_R_EMPTY_ERR_DESC,NULL,mid);
			return OPP_FAILURE; 					
		}

		for(i=0;i<arraySize;i++){
			item = cJSON_GetArrayItem(object,i);
			ret = ApsDaemonActionItemParaGetCustom(CUSTOM_ALARM, item->valueint,&config);
			if(OPP_SUCCESS != ret){
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%d get config ret %d\r\n", i,ret);
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,ALARM_IDS_R_CONFIG_ERR,ALARM_IDS_R_CONFIG_ERR_DESC,NULL,mid);
				return OPP_FAILURE; 					
			}
			ret = ApsCoapFindResMapIdx(config.resource,&idx);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_ERR,RESID_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			if(!(aucResMap[idx].auth & R)){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_R_ERR,RESID_R_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			
		}
		
		ApsCoapFuncRspBatchStart(dstChl, reqId, 0, 0, NULL, ALARM_CMDID, &root, &cmdData);
		batArray=cJSON_CreateArray();
		if(NULL == batArray){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapAlarmProc create <items> array error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			cJSON_Delete(root);
			return OPP_FAILURE;
		}
		cJSON_AddItemToObject(cmdData, "items", batArray);
		for(i=0;i<arraySize;i++){
			item = cJSON_GetArrayItem(object,i);
			ret = ApsDaemonActionItemParaGetCustom(CUSTOM_ALARM,item->valueint,&config);
			if(OPP_SUCCESS == ret){
				ApsCoapThresholdRspBatchAppend(root,cmdData,batArray,item->valueint,CUSTOM_ALARM, &config);
			}
		}
		ret = ApsCoapFuncRspBatchStop(dstChl, dstInfo, root, mid);
		if(NBIOT_SEND_BIG_ERR == ret){
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,0,PROP_SERVICE,NBIOT_SEND_BIG_ERR,NBIOT_SEND_BIG_ERR_DESC,NULL,mid);
		}
	}				
	else if(0 == strcmp(op,"D")){
		memset(&config, 0, sizeof(AD_CONFIG_T));
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		if(NULL == cmdData){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		object = cJSON_GetObjectItem(cmdData, "ids");
		if(NULL == object){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%d get config ret %d\r\n", i,ret);
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,ALARM_IDS_D_ERR,ALARM_IDS_D_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ids object\r\n");
		
		arraySize = cJSON_GetArraySize(object);
		if(arraySize == 0){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm no data here\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,ALARM_IDS_D_EMPTY_ERR,ALARM_IDS_D_EMPTY_ERR_DESC,NULL,mid);
			return OPP_FAILURE; 					
		}
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ids array size %d\r\n",arraySize);

		for(i=0;i<arraySize;i++){
			item = cJSON_GetArrayItem(object,i);
			ret = ApsDaemonActionItemParaGetCustom(CUSTOM_ALARM, item->valueint,&config);
			if(OPP_SUCCESS != ret)
				continue;
			ret = ApsCoapFindResMapIdx(config.resource,&idx);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_ERR,RESID_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			if(!(aucResMap[idx].auth & D)){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_D_ERR,RESID_D_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
		}
		
		for(i=0;i<arraySize;i++){
			item = cJSON_GetArrayItem(object,i);	
			//删除
			ret = ApsDaemonActionItemParaDelCustom(CUSTOM_ALARM, item->valueint);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,ALARM_IDS_D_SET_ERR,ALARM_IDS_D_SET_ERR_DESC,NULL,mid);
				return OPP_FAILURE; 					
			}
		}
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,0,NULL,NULL,mid);
	}
	else{
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,ALARM_CFG_OP_ERR,ALARM_CFG_OP_ERR_DESC,NULL,mid);
	}
	return OPP_SUCCESS;
}

int ApsCoapAlarmFastCfgProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int ret;
	int reqId = 0;
	cJSON *cmdData, *alarms, *item, *items, *subObj, *cmdDataRe;
	int arraySize;
	int i;
	int id, alarmId, value;
	AD_CONFIG_T config;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARMFASTCFG_CMDID,json,"reqId",mid);
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	cmdData = cJSON_GetObjectItem(json, "cmdData");
	if(NULL == cmdData){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFG_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	alarms = cJSON_GetObjectItem(cmdData, "alarms");
	if(NULL == alarms){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFG_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}

	arraySize = cJSON_GetArraySize(alarms);
	if(arraySize <= 0){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFG_CMDID,reqId,0,ALARM_FAST_CFG_NO_ALARM_ERR,ALARM_FAST_CFG_NO_ALARM_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}

	cmdDataRe = cJSON_CreateObject();
	if(NULL == cmdDataRe){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapAlarmFastCfgProc create <cmdDataRe> object error\r\n");		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFG_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	items=cJSON_CreateArray();
	if(NULL == items){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapAlarmFastCfgProc create <item> array error\r\n");
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFG_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		cJSON_Delete(cmdDataRe);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(cmdDataRe, "items", items);

	for(i=0;i<arraySize;i++){
		item = cJSON_GetArrayItem(alarms,i);
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARMFASTCFG_CMDID,item,"alarmId",mid);
		alarmId = cJSON_GetObjectItem(item,"alarmId")->valueint;
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARMFASTCFG_CMDID,item,"value",mid);
		value = cJSON_GetObjectItem(item,"value")->valueint;

		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm id %d, value %d\r\n", alarmId, value);
		if(OVER_VOLALARMID == alarmId){
			id = OVER_VOL_ALARM_IDX;
			config.resource = RSC_VOLT;
			config.alarmIf = AI_GT;
		}else if(UNDER_VOLALARMID == alarmId){
			id = UNDER_VOL_ALARM_IDX;
			config.resource = RSC_VOLT;
			config.alarmIf = AI_LT;
		}else if(OVER_CURALARMID == alarmId){
			id = OVER_CUR_ALARM_IDX;
			config.resource = RSC_CURRENT;
			config.alarmIf = AI_GT;
		}else if(UNDER_CURALARMID == alarmId){
			id = UNDER_CUR_ALARM_IDX;
			config.resource = RSC_CURRENT;
			config.alarmIf = AI_LT;
		}else{
			cJSON_Delete(cmdDataRe);
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFG_CMDID,reqId,0,ALARM_FAST_CFG_INVALID_ALARMID_ERR,ALARM_FAST_CFG_INVALID_ALARMID_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		
		config.alarmId = alarmId;
		config.enable |= MCA_LOG|MCA_ALARM;
		config.targetid = 0;
		config.alarmIfPara1 = value;

		ret = ApsDaemonActionItemParaSetCustom(CUSTOM_ALARM, id, &config);
		if(OPP_SUCCESS != ret){
			cJSON_Delete(cmdDataRe);
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFG_CMDID,reqId,0,ALARM_PARA_SET_ERR,ALARM_PARA_SET_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		subObj = cJSON_CreateObject();
		if(NULL == subObj){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapAlarmFastCfgProc create items.<alarmId> 10000 object error\r\n");		
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFG_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			cJSON_Delete(cmdDataRe);
			return OPP_FAILURE;
		}
		cJSON_AddItemToArray(items,subObj);
		cJSON_AddItemToObject(subObj, "alarmId", cJSON_CreateNumber(alarmId));
		cJSON_AddItemToObject(subObj, "id", cJSON_CreateNumber(id));
	}
	
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFG_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
	return OPP_SUCCESS;
}


int ApsCoapAlarmFastCfgAllProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int ret;
	int reqId = 0;
	cJSON *cmdData, *alarms, *item, *items, *subObj, *cmdDataRe;
	int arraySize;
	int i;
	int id, alarmId, value, logEnable, alarmEnable;
	AD_CONFIG_T config;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,json,"reqId",mid);
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	cmdData = cJSON_GetObjectItem(json, "cmdData");
	if(NULL == cmdData){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	alarms = cJSON_GetObjectItem(cmdData, "alarms");
	if(NULL == alarms){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}

	arraySize = cJSON_GetArraySize(alarms);
	if(arraySize <= 0){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,reqId,0,ALARM_FAST_CFG_NO_ALARM_ERR,ALARM_FAST_CFG_NO_ALARM_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}

	cmdDataRe = cJSON_CreateObject();
	if(NULL == cmdDataRe){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapAlarmFastCfgProc create <cmdDataRe> object error\r\n");		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	items=cJSON_CreateArray();
	if(NULL == items){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapAlarmFastCfgProc create <item> array error\r\n");
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		cJSON_Delete(cmdDataRe);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(cmdDataRe, "items", items);

	for(i=0;i<arraySize;i++){
		item = cJSON_GetArrayItem(alarms,i);
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,item,"alarmId",mid);
		alarmId = cJSON_GetObjectItem(item,"alarmId")->valueint;
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,item,"value",mid);
		value = cJSON_GetObjectItem(item,"value")->valueint;
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARMFASTCFG_CMDID,item,"logEnable",mid);
		logEnable = cJSON_GetObjectItem(item,"logEnable")->valueint;
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARMFASTCFG_CMDID,item,"alarmEnable",mid);
		alarmEnable = cJSON_GetObjectItem(item,"alarmEnable")->valueint;

		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"alarm id %d, value %d\r\n", alarmId, value);
		if(OVER_VOLALARMID == alarmId){
			id = OVER_VOL_ALARM_IDX;
			config.resource = RSC_VOLT;
			config.alarmIf = AI_GT;
		}else if(UNDER_VOLALARMID == alarmId){
			id = UNDER_VOL_ALARM_IDX;
			config.resource = RSC_VOLT;
			config.alarmIf = AI_LT;
		}else if(OVER_CURALARMID == alarmId){
			id = OVER_CUR_ALARM_IDX;
			config.resource = RSC_CURRENT;
			config.alarmIf = AI_GT;
		}else if(UNDER_CURALARMID == alarmId){
			id = UNDER_CUR_ALARM_IDX;
			config.resource = RSC_CURRENT;
			config.alarmIf = AI_LT;
		}else{
			cJSON_Delete(cmdDataRe);
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,reqId,0,ALARM_FAST_CFG_INVALID_ALARMID_ERR,ALARM_FAST_CFG_INVALID_ALARMID_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		
		config.alarmId = alarmId;
		if(0 == logEnable){
			config.enable &= ~MCA_LOG;
		}else{
			config.enable |= MCA_LOG;
		}
		if(0 == alarmEnable){
			config.enable &= ~MCA_ALARM;
		}else{
			config.enable |= MCA_ALARM;
		}
		config.targetid = 0;
		config.alarmIfPara1 = value;

		ret = ApsDaemonActionItemParaSetCustom(CUSTOM_ALARM, id, &config);
		if(OPP_SUCCESS != ret){
			cJSON_Delete(cmdDataRe);
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,reqId,0,ALARM_PARA_SET_ERR,ALARM_PARA_SET_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		subObj = cJSON_CreateObject();
		if(NULL == subObj){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapAlarmFastCfgProc create items.<alarmId> 10000 object error\r\n");		
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			cJSON_Delete(cmdDataRe);
			return OPP_FAILURE;
		}
		cJSON_AddItemToArray(items,subObj);
		cJSON_AddItemToObject(subObj, "alarmId", cJSON_CreateNumber(alarmId));
		cJSON_AddItemToObject(subObj, "id", cJSON_CreateNumber(id));
	}
	
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTCFGALL_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
	return OPP_SUCCESS;
}


int ApsCoapAlarmFastReadProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int ret;
	int reqId = 0;
	cJSON *cmdData, *alarms, *item, *subObj, *cmdDataRe;
	int arraySize;
	int i;
	int id, alarmId, value;
	AD_CONFIG_T config;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARMFASTREAD_CMDID,json,"reqId",mid);
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;

	cmdDataRe = cJSON_CreateObject();
	if(NULL == cmdDataRe){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapAlarmFastReadProc create <cmdDataRe> object error\r\n");		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTREAD_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	item=cJSON_CreateArray();
	if(NULL == item){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapAlarmFastReadProc create <item> array error\r\n");
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		cJSON_Delete(cmdDataRe);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(cmdDataRe, "items", item);

	for(i=0;i<sizeof(g_stFastAlarmPara)/sizeof(ST_FASTALARM_PARA);i++){
		ApsDaemonActionItemParaGetCustom(CUSTOM_ALARM, g_stFastAlarmPara[i].defaultIdx,&config);
		subObj = cJSON_CreateObject();
		if(NULL == subObj){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapAlarmFastReadProc create items.<alarmId> 10000 object error\r\n");		
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTREAD_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			cJSON_Delete(cmdDataRe);
			return OPP_FAILURE;
		}
		cJSON_AddItemToArray(item,subObj);
		cJSON_AddItemToObject(subObj, "alarmId", cJSON_CreateNumber(g_stFastAlarmPara[i].alarmId));
		cJSON_AddItemToObject(subObj, "id", cJSON_CreateNumber(g_stFastAlarmPara[i].defaultIdx));	
		cJSON_AddItemToObject(subObj, "value", cJSON_CreateNumber(config.alarmIfPara1));
		cJSON_AddItemToObject(subObj, "alarmEnable", cJSON_CreateNumber(config.enable & MCA_ALARM?1:0));
		cJSON_AddItemToObject(subObj, "logEnable", cJSON_CreateNumber(config.enable & MCA_LOG?1:0));
	}
	
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMFASTREAD_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
	return OPP_SUCCESS;
}
int ApsCoapFastPropConfig(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int ret;
	int reqId = 0;
	cJSON *cmdData, *items, *object, *cmdDataRe, *subObj;
	int arraySize;
	int i = 0,j = 0,idx = 0;
	int resId, chgRange, isReportEnable;
	AD_CONFIG_T config; 		
    int resIdRec[10];
	int resIdNum = 0;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,json,"reqId",mid);		
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,reqId,0,PROPFASTCFG_NOT_SUPPORT_ERR,PROPFASTCFG_NOT_SUPPORT_ERR_DESC,NULL,mid);
	return OPP_SUCCESS;
	
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,json,"cmdData",mid);		
	cmdData = cJSON_GetObjectItem(json, "cmdData");
	if(NULL == cmdData){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	items = cJSON_GetObjectItem(cmdData, "items");
	if(NULL == items){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	arraySize = cJSON_GetArraySize(items);
	if(arraySize <= 0){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,reqId,0,PROPFASTCFG_EMPTY_ERR,PROPFASTCFG_EMPTY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	for(i=0;i<arraySize;i++){
		object = cJSON_GetArrayItem(items,i);
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,object,"resId",mid);		
		resId = cJSON_GetObjectItem(object,"resId")->valueint;
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,object,"chgRange",mid);		
		chgRange = cJSON_GetObjectItem(object,"chgRange")->valueint;
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,object,"isReportEnable",mid);		
		isReportEnable = cJSON_GetObjectItem(object,"isReportEnable")->valueint;

		resIdRec[resIdNum++] = resId;
		
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"resid id %d, value %d enable %d\r\n", resId, chgRange, isReportEnable);

		memset(&config,0,sizeof(AD_CONFIG_T));
		config.resource = resId;
		if(isReportEnable)
			config.enable |= MCA_REPORT;
		else
			config.enable &= ~MCA_REPORT;
		config.targetid = 0;
		config.reportIf = RI_CHANGE;
		config.reportIfPara = chgRange;

		for(j=0;j<sizeof(g_stFastConfigPara)/sizeof(ST_FASTPROP_PARA);j++){
			if(resId == g_stFastConfigPara[j].resId){
				idx = g_stFastConfigPara[j].defaultIdx;
				break;
			}
		}

		if(j == sizeof(g_stFastConfigPara)/sizeof(ST_FASTPROP_PARA)){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,reqId,0,INNER_ERR,INNER_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		
		ret = ApsDaemonActionItemParaSetCustom(CUSTOM_REPORT, idx, &config);
		if(OPP_SUCCESS != ret){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,reqId,0,PROPCONFIG_PARA_SET_ERR,PROPCONFIG_PARA_SET_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
	}
	
	cmdDataRe = cJSON_CreateObject();
	if(NULL == cmdDataRe){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapFastPropConfig create <cmdDataRe> object error\r\n");		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	
	items=cJSON_CreateArray();
	if(NULL == items){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapFastPropConfig create <items> array error\r\n");
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		cJSON_Delete(cmdDataRe);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(cmdDataRe, "items", items);
	for(i=0;i<resIdNum;i++){
		for(j=0;j<sizeof(g_stFastConfigPara)/sizeof(ST_FASTPROP_PARA);j++){
			if(resIdRec[i] == g_stFastConfigPara[j].resId){
				break;
			}
		}
		if(j == sizeof(g_stFastConfigPara)/sizeof(ST_FASTPROP_PARA)){
			continue;
		}
		subObj = cJSON_CreateObject();
		if(NULL == subObj){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapFastPropConfig create <subObj> object error\r\n");		
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			cJSON_Delete(cmdDataRe);
			return OPP_FAILURE;
		}
		cJSON_AddItemToArray(items,subObj);
		cJSON_AddItemToObject(subObj, "resId", cJSON_CreateNumber(g_stFastConfigPara[j].resId));
		cJSON_AddItemToObject(subObj, "id", cJSON_CreateNumber(g_stFastConfigPara[j].defaultIdx));
	}
	
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTCFG_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
	return OPP_SUCCESS;
}

int ApsCoapFastPropRead(unsigned char dstChl, unsigned char *dstInfo,unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int ret;
	int reqId = 0;
	cJSON *cmdData, *items, *object, *cmdDataRe, *subObj;
	int i = 0,j = 0,idx = 0;
	int resId, chgRange, isReportEnable;
	AD_CONFIG_T config; 		
    int resIdRec[10];
	int resIdNum = 5;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PROPCONFIGFASTREAD_CMDID,json,"reqId",mid);		
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	
	cmdDataRe = cJSON_CreateObject();
	if(NULL == cmdDataRe){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapFastPropConfig create <cmdDataRe> object error\r\n");		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTREAD_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	
	items=cJSON_CreateArray();
	if(NULL == items){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapFastPropConfig create <items> array error\r\n");
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTREAD_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		cJSON_Delete(cmdDataRe);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(cmdDataRe, "items", items);
	for(i=0;i<resIdNum;i++){
		subObj = cJSON_CreateObject();
		if(NULL == subObj){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapFastPropRead create <subObj> object error\r\n");		
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTREAD_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			cJSON_Delete(cmdDataRe);
			return OPP_FAILURE;
		}
		ApsDaemonActionItemParaGetCustom(CUSTOM_REPORT,g_stFastConfigPara[i].defaultIdx,&config);
		cJSON_AddItemToArray(items,subObj);
		cJSON_AddItemToObject(subObj, "resId", cJSON_CreateNumber(g_stFastConfigPara[i].resId));
		cJSON_AddItemToObject(subObj, "id", cJSON_CreateNumber(g_stFastConfigPara[i].defaultIdx));
		cJSON_AddItemToObject(subObj, "isReportEnable", cJSON_CreateNumber(config.enable & MCA_REPORT?1:0));
		cJSON_AddItemToObject(subObj, "chgRange", cJSON_CreateNumber(config.reportIfPara));
	}
	
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGFASTREAD_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
	return OPP_SUCCESS;
}

int ApsCoapPropConfigProc(unsigned char dstChl, unsigned char dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *root, * cmdData, *object, *updateObj, *cmdDataRe, *batArray, *item;
	char op[10]={0};
	AD_CONFIG_T config;
	int paraType = -1;
	U32 id;
	int isReportEnable = -1;
	int reqId = 0;
	int arraySize = -1;
	int i = 0;
	int ret = OPP_SUCCESS;
	int idx;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PROPCONFIG_CMDID,json,"reqId",mid);		
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	STRNCPY(op, cJSON_GetObjectItem(json, "op")->valuestring, sizeof(op));
	if(0 == strcmp(op,"W") || 0 == strcmp(op,"M")){
		memset(&config, 0, sizeof(AD_CONFIG_T));
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PROPCONFIG_CMDID,json,"cmdData",mid);		
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		object = cJSON_GetObjectItem(cmdData, "targetId");
		if(NULL != object){
			//targetId = object->valueint;
		}
		if(0 == strcmp(op,"M")){
			object = cJSON_GetObjectItem(cmdData, "id");
			if(NULL != object){
				id = object->valueint;
			}
			
		}
		object = cJSON_GetObjectItem(cmdData, "resId");
		if(NULL != object){
			config.resource = object->valueint;
		}
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"propconfig resId %d\r\n", config.resource);
		object = cJSON_GetObjectItem(cmdData, "propName");
		if(NULL != object){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"propconfig propName %s %d\r\n",object->valuestring);
		}
		
		updateObj = cJSON_GetObjectItem(cmdData, "propConfig");
		if(NULL != updateObj){
			object = cJSON_GetObjectItem(updateObj, "isReportEnable");
			if(NULL != object){
				isReportEnable = object->valueint;
				if(isReportEnable)
					config.enable |= MCA_REPORT;
				else
					config.enable &= ~MCA_REPORT;
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"propconfig isReportEnable %d\r\n", isReportEnable);
			
			object = cJSON_GetObjectItem(updateObj, "updateType");
			if(NULL != object){
				config.reportIf = object->valueint;
			}
			object = cJSON_GetObjectItem(updateObj, "paraType");
			if(NULL != object){
				paraType= object->valueint/*do nothing*/;
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"propconfig paraType %d\r\n", paraType);	
			
			object = cJSON_GetObjectItem(updateObj, "value");
			if(NULL != object){
				config.reportIfPara = object->valueint;
			}
		}					
		
		if(0 == strcmp(op,"W")){
			ret = ApsCoapFindResMapIdx(config.resource,&idx);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_ERR,RESID_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			if(!(aucResMap[idx].auth & W)){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_W_ERR,RESID_W_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			
			ret = ApsDaemonActionItemParaAddCustom(CUSTOM_REPORT, &id, &config);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,PROPCONFIG_PARA_SET_ERR,PROPCONFIG_PARA_SET_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			//返回id
			cmdDataRe = cJSON_CreateObject();
			if(NULL == cmdDataRe){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropConfigProc create <cmdDataRe> object error\r\n");		
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			cJSON_AddItemToObject(cmdDataRe, "id", cJSON_CreateNumber(id));
			
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,0,NULL,cmdDataRe,mid); 		
		}else{
			ret = ApsCoapFindResMapIdx(config.resource,&idx);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_ERR,RESID_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			if(!(aucResMap[idx].auth & M)){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,RESID_W_ERR,RESID_W_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
		
			ret = ApsDaemonActionItemParaSetCustom(CUSTOM_REPORT, id, &config);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,PROPCONFIG_PARA_SET_ERR,PROPCONFIG_PARA_SET_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,0,NULL,NULL,mid);
		}
	}
	else if(0 == strcmp(op,"R")){
		/*no need rsp*/
		if(0 == isNeedRsp){
			return OPP_SUCCESS;
		}
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		object = cJSON_GetObjectItem(cmdData, "ids");
		if(NULL == object){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);			
			return OPP_FAILURE;
		}
		arraySize = cJSON_GetArraySize(object);
		if(arraySize == 0){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"update no data here\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,PROPCONFIG_IDS_R_EMPTY_ERR,PROPCONFIG_IDS_R_EMPTY_ERR_DESC,NULL,mid);
			return OPP_FAILURE; 					
		}
		for(i=0;i<arraySize;i++){
			item = cJSON_GetArrayItem(object,i);
			ret = ApsDaemonActionItemParaGetCustom(CUSTOM_REPORT,item->valueint,&config);
			if(OPP_SUCCESS != ret){
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%d get config ret %d\r\n", i,ret);
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,PROPCONFIG_IDS_R_CONFIG_ERR,PROPCONFIG_IDS_R_CONFIG_ERR_DESC,NULL,mid);
				return OPP_FAILURE; 					
			}

			ret = ApsCoapFindResMapIdx(config.resource,&idx);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,RESID_ERR,RESID_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			if(!(aucResMap[idx].auth & R)){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,RESID_R_ERR,RESID_R_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
		}
		
		ApsCoapFuncRspBatchStart(dstChl,reqId, 0, 0, NULL, PROPCONFIG_CMDID, &root, &cmdData);
		batArray=cJSON_CreateArray();
		if(NULL == batArray){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapPropConfigProc create <items> array error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			cJSON_Delete(root);
			return OPP_FAILURE;
		}
		cJSON_AddItemToObject(cmdData, "items", batArray);
		for(i=0;i<arraySize;i++){
			item = cJSON_GetArrayItem(object,i);
			ret = ApsDaemonActionItemParaGetCustom(CUSTOM_REPORT,item->valueint,&config);
			if(OPP_SUCCESS == ret){
				ApsCoapThresholdRspBatchAppend(root,cmdData,batArray,item->valueint,CUSTOM_REPORT,&config);
			}
		}
		ret = ApsCoapFuncRspBatchStop(dstChl, dstInfo, root, mid);
		if(NBIOT_SEND_BIG_ERR == ret){
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,0,PROP_SERVICE,NBIOT_SEND_BIG_ERR,NBIOT_SEND_BIG_ERR_DESC,NULL,mid);
		}
	}				
	else if(0 == strcmp(op,"D")){
		memset(&config, 0, sizeof(AD_CONFIG_T));
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		if(NULL == cmdData){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		object = cJSON_GetObjectItem(cmdData, "ids");
		if(NULL == object){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%d get config ret %d\r\n", i,ret);
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,PROPCONFIG_IDS_D_ERR,PROPCONFIG_IDS_D_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
				
		arraySize = cJSON_GetArraySize(object);
		if(arraySize == 0){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"update no data here\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,PROPCONFIG_IDS_D_EMPTY_ERR,PROPCONFIG_IDS_D_EMPTY_ERR_DESC,NULL,mid);
			return OPP_FAILURE; 					
		}
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ids array size %d\r\n",arraySize);

		for(i=0;i<arraySize;i++){
			item = cJSON_GetArrayItem(object,i);			
			ret = ApsDaemonActionItemParaGetCustom(CUSTOM_REPORT, item->valueint,&config);
			if(OPP_SUCCESS != ret)
				continue;
			ret = ApsCoapFindResMapIdx(item->valueint,&idx);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,RESID_ERR,RESID_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			if(!(aucResMap[idx].auth & D)){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,RESID_D_ERR,RESID_D_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
		}
		
		for(i=0;i<arraySize;i++){
			item = cJSON_GetArrayItem(object,i);	
			//删除
			ret = ApsDaemonActionItemParaDelCustom(CUSTOM_REPORT,item->valueint);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,PROPCONFIG_IDS_D_SET_ERR,PROPCONFIG_IDS_D_SET_ERR_DESC,NULL,mid);
				return OPP_FAILURE; 					
			}
		}
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,0,NULL,NULL,mid);
		
	}
	else
	{
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIG_CMDID,reqId,0,PROP_CFG_OP_ERR,PROP_CFG_OP_ERR_DESC,NULL,mid);
	}
	return OPP_SUCCESS;
}
int ApsCoapBitmapToIndex(const unsigned char * bitmapBuffer, int bitmapLen, int * indexBuffer, unsigned int * indexLen)
{
	int i = 0;
	int j = 0;
	
	for(i=0;i<bitmapLen;i++){
		if(bitmapBuffer[i] == 1){
			indexBuffer[j++] = i;
		}
	}
	*indexLen = j;
	return OPP_SUCCESS;
}

/*
int ApsCoapThresholdSyncProc(unsigned char dstChl, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *cmdData;
	int reqId = -1;
	unsigned int outLength = 0, outLenght1 = 0;
	char *ptr;	
	int buffer[MODULE_STATUS_ITEM_MAX];
	
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	cmdData = cJSON_CreateObject();
	if(NULL == cmdData){
		ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLDSYNC_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	
	ptr = (char *)mymalloc(MODULE_STATUS_ITEM_MAX);
	if(NULL == ptr){
		ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLDSYNC_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	ApsDaemonActionItemIndexValidity((const unsigned char**)&ptr,&outLength);
	ApsCoapBitmapToIndex(ptr,outLength,buffer, &outLenght1);

	cJSON_AddItemToObject(cmdData, "ids", cJSON_CreateIntArray(buffer, outLenght1));
	
	ApsCoapFuncRsp(dstChl,isNeedRsp,FUNC_SERVICE,THRESHOLDSYNC_CMDID,reqId,0,0,NULL,cmdData,mid);
	myfree(ptr);
	return OPP_SUCCESS;
}
*/

int ApsCoapAlarmSyncProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *cmdData;
	int reqId = 0;
	unsigned int outLength = 0, outLenght1 = 0;
	//char *ptr;
	const unsigned char* ptr;
	int buffer[MODULE_STATUS_ITEM_MAX];

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ALARMSYN_CMDID,json,"reqId",mid);	
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	cmdData = cJSON_CreateObject();
	if(NULL == cmdData){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapAlarmSyncProc create <cmdData> object error\r\n");		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMSYN_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	/*
	ptr = (char *)mymalloc(MODULE_STATUS_ITEM_MAX);
	if(NULL == ptr){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMSYN_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	ApsDaemonActionItemIndexValidityCustom(CUSTOM_ALARM, (const unsigned char**)&ptr,&outLength);
	*/
	ApsDaemonActionItemIndexValidityCustom(CUSTOM_ALARM, (const unsigned char**)&ptr,&outLength);
	ApsCoapBitmapToIndex(ptr,outLength,buffer, &outLenght1);

	cJSON_AddItemToObject(cmdData, "ids", cJSON_CreateIntArray(buffer, outLenght1));
	
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARMSYN_CMDID,reqId,0,0,NULL,cmdData,mid);

	//myfree(ptr);
	return OPP_SUCCESS;
}

int ApsCoapPropConfigSyncProc(unsigned char dstChl, unsigned char dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *cmdData;
	int reqId = 0;
	unsigned int outLength = 0, outLenght1 = 0;
	//char *ptr;
	const unsigned char* ptr;	
	int buffer[MODULE_STATUS_ITEM_MAX];

	APS_COAP_NULL_POINT_CHECK(json);
	
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PROPCONFIGSYN_CMDID,json,"reqId",mid);		
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	cmdData = cJSON_CreateObject();
	if(NULL == cmdData){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPropConfigSyncProc create <cmdData> object error\r\n");		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGSYN_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}

	/*
	ptr = (char *)mymalloc(MODULE_STATUS_ITEM_MAX);
	if(NULL == ptr){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGSYN_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}

	ApsDaemonActionItemIndexValidityCustom(CUSTOM_REPORT,(const unsigned char**)&ptr,&outLength);
	*/
	ApsDaemonActionItemIndexValidityCustom(CUSTOM_REPORT,(const unsigned char**)&ptr,&outLength);
	ApsCoapBitmapToIndex(ptr,outLength,buffer, &outLenght1);
	
	cJSON_AddItemToObject(cmdData, "ids", cJSON_CreateIntArray(buffer, outLenght1));
	
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PROPCONFIGSYN_CMDID,reqId,0,0,NULL,cmdData,mid);

	//myfree(ptr);
	return OPP_SUCCESS;
}
/*
int ApsCoapReboot(void)
{
	int ret;
	ST_REBOOT stReboot;
	unsigned int len = sizeof(ST_REBOOT);
	
	ret = AppParaRead(APS_PARA_MODULE_APS_REBOOT, REBOOT_ID, (unsigned char *)&stReboot, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "ApsCoapReboot AppParaRead ret %d\r\n",ret);
		return OPP_FAILURE;
	}
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "ApsCoapReboot %d, %d, %d, %d\r\n", stReboot.dstChl, stReboot.reqId, stReboot.doReboot, stReboot.mid);
	if(stReboot.doReboot){
		ApsCoapFuncRsp(stReboot.dstChl,1,FUNC_SERVICE,REBOOT_CMDID,stReboot.reqId,0,0,NULL,NULL,stReboot.mid);
		stReboot.doReboot = 0;
		ret = AppParaWrite(APS_PARA_MODULE_APS_REBOOT, REBOOT_ID, (unsigned char *)&stReboot, sizeof(ST_REBOOT));
		if(OPP_SUCCESS != ret){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "ApsCoapReboot AppParaWrite ret %d\r\n",ret);
			return OPP_FAILURE;
		}
	}
	return OPP_SUCCESS;
}
*/

int ApsCoapRebootProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int reqId = 0;
	int i = 0;

	APS_COAP_NULL_POINT_CHECK(json);
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,REBOOT_CMDID,json,"reqId",mid);	
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	//[code review]
	/*for(i=0; i<(sizeof(g_astRebootFunc)/sizeof(ST_REBOOT_FUNC)); i++)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"i:%d,%s\n",g_astRebootFunc[i].moduleId, g_astRebootFunc[i].moduleName);
		if(g_astRebootFunc[i].reboot)
			g_astRebootFunc[i].reboot();
	}*/
	
	startReboot = 1;
	g_iRebootTick = OppTickCountGet();
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,REBOOT_CMDID,reqId,0,0,NULL,NULL,mid);
	//osDelay(10 / portTICK_RATE_MS);
	/*stReboot.dstChl = dstChl;
	stReboot.mid = mid;
	stReboot.doReboot = 1;
	stReboot.reqId = reqId;
	ret = AppParaWrite(APS_PARA_MODULE_APS_REBOOT, REBOOT_ID, (unsigned char *)&stReboot, sizeof(ST_REBOOT));
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "ApsCoapReboot AppParaWrite ret %d\r\n",ret);
		return OPP_FAILURE;
	}*/
	//esp_restart();
	return OPP_SUCCESS;
}

int ApsCoapDoReboot(void)
{
	int i = 0;

	for(i=0; i<(sizeof(g_astRebootFunc)/sizeof(ST_REBOOT_FUNC)); i++)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"i:%d,%s\n",g_astRebootFunc[i].moduleId, g_astRebootFunc[i].moduleName);
		if(g_astRebootFunc[i].reboot)
			g_astRebootFunc[i].reboot();
	}
	return 0;
}
int ApsCoapGpsLocProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int ret = OPP_SUCCESS;
	int reqId = 0;
	ST_OPP_LAMP_LOCATION stLocationPara;
	cJSON *cmdDataRe, *comObj;
	char buffer[32] = {0};
	int support;

	APS_COAP_NULL_POINT_CHECK(json);
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,GPSLOC_CMDID,json,"reqId",mid);	
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	ret = ApsDevFeaturesGet(GPS_FEATURE,&support);
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,GPSLOC_CMDID,reqId,0,INNER_ERR,INNER_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}

	if(0 == support){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,GPSLOC_CMDID,reqId,0,GPS_NOT_SUPPORT_ERR,GPS_NOT_SUPPORT_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	
	ret = LocationRead(GPS_LOC,&stLocationPara);
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,GPSLOC_CMDID,reqId,0,GPS_GET_LOC_ERR,GPS_GET_LOC_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	
	cmdDataRe= cJSON_CreateObject();
	if(NULL == cmdDataRe){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapGpsLocProc create <cmdDataRe> object error\r\n");	
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,GPSLOC_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	comObj= cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapGpsLocProc create <comObj> object error\r\n");	
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,GPSLOC_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		cJSON_Delete(cmdDataRe);
		return OPP_FAILURE;
	}

	cJSON_AddItemToObject(cmdDataRe, "loc", comObj);	
	ftoa(buffer,stLocationPara.ldLatitude,6);
	cJSON_AddItemToObject(comObj, "lat", cJSON_CreateString(buffer));
	memset(buffer, 0, sizeof(buffer));
	ftoa(buffer,stLocationPara.ldLongitude,6);
	cJSON_AddItemToObject(comObj, "lng", cJSON_CreateString(buffer));
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,GPSLOC_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
		
	return OPP_SUCCESS;
}

int ApsCoapGpsTimeProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int ret = OPP_SUCCESS;
	int reqId = 0;	
	int zone;
	ST_OPPLE_TIME stTimePara;
	cJSON *cmdDataRe;
	char buf[64] = {0};
	int support;

	APS_COAP_NULL_POINT_CHECK(json);
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,GPSTIME_CMDID,json,"reqId",mid);		
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;

	ret = ApsDevFeaturesGet(GPS_FEATURE,&support);
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,GPSTIME_CMDID,reqId,0,INNER_ERR,INNER_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}

	if(0 == support){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,GPSTIME_CMDID,reqId,0,GPS_NOT_SUPPORT_ERR,GPS_NOT_SUPPORT_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	
	ret = TimeRead(GPS_CLK,&zone,&stTimePara);
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,GPSTIME_CMDID,reqId,0,GPS_GET_TIME_ERR,GPS_GET_TIME_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	cmdDataRe= cJSON_CreateObject();
	if(NULL == cmdDataRe){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapGpsTimeProc create <cmdDataRe> object error\r\n");	
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,GPSTIME_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	sprintf(buf, "%02hhu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu+%02hhu", (stTimePara.uwYear-2000), stTimePara.ucMon, stTimePara.ucDay, stTimePara.ucHour,stTimePara.ucMin,stTimePara.ucSec,zone);
	cJSON_AddItemToObject(cmdDataRe, "gpsTime", cJSON_CreateString(buf));
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,GPSTIME_CMDID,reqId,0,0,NULL,cmdDataRe,mid);

	return OPP_SUCCESS;
}

int ApsCoapTimeProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON * object, *cmdData, *cmdDataRe;
	int reqId = 0;
	char op[10]={0};
	U8 year,month,day,hour,min,second;
	S8 zone;
	ST_TIME time;
	char buf[64] = {0};
	int ret;
	
	APS_COAP_NULL_POINT_CHECK(json);

	object = cJSON_GetObjectItem(json, "reqId");
	if(NULL == object){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);		
		return OPP_FAILURE;
	}
	reqId = object->valueint;
	object = cJSON_GetObjectItem(json, "op");
	if(NULL == object){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);				
		return OPP_FAILURE;
	}
	
	STRNCPY(op, object->valuestring, sizeof(op));
	if(0 == strcmp(op,"W")){
		cmdData = cJSON_GetObjectItem(json,"cmdData");
		if(NULL == cmdData){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		object = cJSON_GetObjectItem(cmdData,"time");
		if(NULL == object){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		if(cJSON_String != object->type){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,0,0,JSON_TYPE_STRING_ERR,JSON_TYPE_STRING_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		if('+' == object->valuestring[strlen(object->valuestring)-3])
			ret = sscanf(object->valuestring, "%hhu-%hhu-%hhu %hhu:%hhu:%hhu+%hhu",
				&year,&month,&day,&hour,&min,&second,&zone);
		else{
			ret = sscanf(object->valuestring, "%hhu-%hhu-%hhu %hhu:%hhu:%hhu-%hhu",
				&year,&month,&day,&hour,&min,&second,&zone);
			zone = 0-zone;
		}
		if(ret < 7){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,0,0,TIME_OP_W_ERR,TIME_OP_W_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		if(month > 12 || day > 31 || hour > 23 || min > 60 || second > 60 || zone > 12 || zone < -12){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,0,0,TIME_VAL_ERR,TIME_VAL_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		ret = TimeSet(year,month,day,hour,min,second,zone);
		if(OPP_SUCCESS != ret){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,0,0,TIME_SET_ERR,TIME_SET_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,reqId,0,0,NULL,NULL,mid);
	}else if(0 == strcmp(op,"R")){
		Opple_Get_RTC(&time);
		if(time.Zone >= 0)
			sprintf(buf, "%02hhu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu+%02hhu", (time.Year-2000), time.Month, time.Day, time.Hour,time.Min,time.Sencond,time.Zone);
		else
			sprintf(buf, "%02hhu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu-%02hhu", (time.Year-2000), time.Month, time.Day, time.Hour,time.Min,time.Sencond,abs(time.Zone));
		cmdDataRe = cJSON_CreateObject();
		if(NULL == cmdDataRe){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapTimeProc create <cmdDataRe> object error\r\n"); 	
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,0,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);			
			return OPP_FAILURE;
		}
		cJSON_AddItemToObject(cmdDataRe, "time", cJSON_CreateString(buf));
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
	}else{
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,TIME_CMDID,reqId,0,TIME_OP_ERROR,TIME_OP_ERROR_DESC,NULL,mid);
	}
	
	return OPP_SUCCESS;
}

int ApsCoapNbCmdProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int reqId = 0;
	int ret;
	char *cmd;
	cJSON *cmdDataRe;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,NBCMD_CMDID,json,"reqId",mid);
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,NBCMD_CMDID,json,"cmd",mid);	
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	cmd = cJSON_GetObjectItem(json, "cmd")->valuestring;
	
	
	sArgv[0] = (char *)malloc(1064);
	if(NULL == sArgv[0]){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"malloc error\r\n");
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,NBCMD_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	memset(sArgv[0],0,1064);
	STRNCPY(sArgv[0],cmd,1064);
	sArgc = 1;
	ret = sendEvent(NBCMD_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(NBCMD_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"recvEvent NBCMD_EVENT ret fail%d\r\n", ret);		
		ARGC_FREE(rArgc,rArgv);
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,NBCMD_CMDID,0,0,NBCMD_EXEC_ERR,NBCMD_EXEC_ERR_DESC,NULL,mid);			
		
		return OPP_FAILURE;
	}
	for(int i=0;i<rArgc;i++)
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "%s", rArgv[i]);

	
	cmdDataRe = cJSON_CreateObject();
	if(NULL == cmdDataRe){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapNbCmdProc create <cmdDataRe> object error\r\n");		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,NBCMD_CMDID,0,0,NBCMD_EXEC_ERR,NBCMD_EXEC_ERR_DESC,NULL,mid);			
		ARGC_FREE(rArgc,rArgv);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(cmdDataRe, "result", cJSON_CreateString(rArgv[0]));
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,NBCMD_CMDID,0,0,0,NULL,cmdDataRe,mid);
	ARGC_FREE(rArgc,rArgv);
	
	return 0;
}

int ApsCoapHeartbeatAckCmdProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int reqId = 0;
	//int ret;
	ST_RHB_ENTRY *pstRhb;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,HEARTBEATACK_CMDID,json,"reqId",mid);		
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	MUTEX_LOCK(m_ulCoapRHB,MUTEX_WAIT_ALWAYS);
	LIST_FOREACH(pstRhb, &list_rhb, elements){
		if(pstRhb->reqId == reqId)
			LIST_REMOVE(pstRhb,elements);
	}
	MUTEX_UNLOCK(m_ulCoapRHB);
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,HEARTBEATACK_CMDID,reqId,0,0,NULL,NULL,mid);
	retryHeartbeat = 0;
	recvHeartAck = 1;
	
	return 0;
}

int ApsCoapRestoreFactoryProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int reqId = 0;
	int ret;
	//ST_RUNTIME_CONFIG stRuntime;
	ST_ELEC_CONFIG stElecConfig;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,RESTOR_CMDID,json,"reqId",mid);
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	//restore dimmer
	ret = OppLampCtrlRestoreFactory();
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_DIMMER_ERR,RESTORE_DIMMER_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	//restore runtime
	/*stRuntime.hisTime = 0;
	ret = OppLampCtrlSetRuntimeParaToFlash(&stRuntime);
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_TIME_ERR,RESTORE_TIME_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}*/
	//restore runtime and consumption
	stElecConfig.hisTime = 0;
	stElecConfig.hisConsumption = 0;
	ret = ElecWriteFlash(&stElecConfig);
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_ELEC_ERR,RESTORE_ELEC_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	ElecHisConsumptionSet(0);
	OppLampCtrlSetRtime(0,0);
	OppLampCtrlSetHtime(0,0);	
	//restore nbiot
	ret = NeulBc28RestoreFactory();
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_NBIOT_ERR,RESTORE_NBIOT_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	//restore iot
	ret = OppCoapIOTRestoreFactory();
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_IOT_ERR,RESTORE_IOT_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	//restore log config
	ret = ApsDaemonLogConfigSetDefault();
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_LOG_ERR,RESTORE_LOG_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	//restore alarm config
	ret = ApsDaemonActionItemParaSetDefaultCustom(CUSTOM_ALARM);
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_ALARM_ERR,RESTORE_ALARM_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}	
	//restore report config
	ret = ApsDaemonActionItemParaSetDefaultCustom(CUSTOM_REPORT);
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_UPDATE_ERR,RESTORE_UPDATE_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	ret = ApsCoapRestoreFactory();
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_COAP_ERR,RESTORE_COAP_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	ret = ApsWifiRestore();
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_WIFI_ERR,RESTORE_WIFI_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	ret = LocRestore();
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_LOC_ERR,RESTORE_LOC_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	/*ret = ApsExepRestore();
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_EXEP_ERR,RESTORE_EXEP_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}*/
	ret = TimeRestore();
	if(OPP_SUCCESS != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,RESTORE_TIME_ERR,RESTORE_TIME_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,RESTOR_CMDID,reqId,0,0,NULL,NULL,mid);
	g_iWifiConTick = OppTickCountGet();
	g_iAttachTick = OppTickCountGet();
	startAttach = 1;
	//esp_restart();
	return OPP_SUCCESS;
}

int ApsCoapWorkPlanProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *cmdData, *cmdDataRe, *item;
	cJSON *plans, *validDate, *jobs, *weeks, *object;
	cJSON * root, *planArray;	
	int reqId = 0;
	char op[10]={0};
	int type;
	//unsigned short port;
	int arraySize = -1;
	int i = 0;
	int ret = OPP_SUCCESS;
	U8 id, cnt = 0, valid;
	int level = 0;
	ST_WORK_SCHEME stPlanScheme,stPlanSchemeZ;
	//char buf[64] = {0};
	ST_CONFIG_PARA stConfigPara;

	APS_COAP_NULL_POINT_CHECK(json);
	memset(&stPlanScheme,0,sizeof(ST_WORK_SCHEME));
	memset(&stPlanSchemeZ,0,sizeof(ST_WORK_SCHEME));

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,json,"reqId",mid);	
	APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(json, "reqId"),cJSON_Number,mid);
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	if(reqId <= 0){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n json id is %d\r\n",reqId);
		ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,JSON_ID_ZERO_ERR,JSON_ID_ZERO_ERR_DESC,NULL,mid);
		cJSON_Delete(json);
		return OPP_FAILURE;
	}
	
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,json,"op",mid);	
	APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(json, "op"),cJSON_String,mid);
	STRNCPY(op, cJSON_GetObjectItem(json, "op")->valuestring, sizeof(op));
	if(0 == strcmp(op,"W")){
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		if(NULL == cmdData){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,0,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(json, "cmdData"),cJSON_Object,mid);
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cmdData,"id",mid);	
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cmdData,"type",mid);			
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cmdData,"level",mid);			
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cmdData,"valid",mid);
		APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(cmdData, "id"),cJSON_Number,mid);
		APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(cmdData, "type"),cJSON_Number,mid);
		APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(cmdData, "level"),cJSON_Number,mid);
		APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(cmdData, "valid"),cJSON_Number,mid);

		id = cJSON_GetObjectItem(cmdData, "id")->valueint;
		type = cJSON_GetObjectItem(cmdData, "type")->valueint;
		level = cJSON_GetObjectItem(cmdData, "level")->valueint;
		valid = cJSON_GetObjectItem(cmdData, "valid")->valueint;
		
		if(id > PLAN_MAX || id <= 0){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,PLAN_ID_OF_ERR,PLAN_ID_OF_ERR_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;
		}
		
		if(AST_MODE != type && WEEK_MODE != type && YMD_MODE != type){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,PLAN_TYPE_ERR,PLAN_TYPE_ERR_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;
		}
		
		if(level < 0 || level > 255){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,PLAN_LEVEL_ERR,PLAN_LEVEL_ERR_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;
		}

		if(0 != valid && 1 != valid){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,PLAN_VALID_ERR,PLAN_VALID_ERR_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;
		}
		
		//天文时间
		if(AST_MODE == type){
			stPlanScheme.hdr.mode = AST_MODE;
			stPlanScheme.hdr.index = id;
			stPlanScheme.hdr.priority = level;
			stPlanScheme.hdr.valid= valid;
			/**plans**/
			plans = cJSON_GetObjectItem(cmdData, "plans");
			if(NULL == plans){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(cmdData, "plans"),cJSON_Object,mid);
			/*date*/
			object = cJSON_GetObjectItem(plans, "sDate");
			if(NULL == plans){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(plans, "sDate"),cJSON_String,mid);
			sscanf(cJSON_GetObjectItem(plans, "sDate")->valuestring, "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu", 
				&stPlanScheme.u.stYmdPlan.sDate.Year, 
				&stPlanScheme.u.stYmdPlan.sDate.Month,
				&stPlanScheme.u.stYmdPlan.sDate.Day,
				&stPlanScheme.u.stYmdPlan.sDate.Hour,
				&stPlanScheme.u.stYmdPlan.sDate.Min,
				&stPlanScheme.u.stYmdPlan.sDate.Sencond);
				
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"sDate: %04d %02d %02d %02d %02d %02d\r\n",
				stPlanScheme.u.stYmdPlan.sDate.Year, 
				stPlanScheme.u.stYmdPlan.sDate.Month,
				stPlanScheme.u.stYmdPlan.sDate.Day,
				stPlanScheme.u.stYmdPlan.sDate.Hour,
				stPlanScheme.u.stYmdPlan.sDate.Min,
				stPlanScheme.u.stYmdPlan.sDate.Sencond);
			
			object = cJSON_GetObjectItem(plans, "eDate");
			if(NULL == plans){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(plans, "eDate"),cJSON_String,mid);
			sscanf(cJSON_GetObjectItem(plans, "eDate")->valuestring, "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu", 
				&stPlanScheme.u.stYmdPlan.eDate.Year, 
				&stPlanScheme.u.stYmdPlan.eDate.Month,
				&stPlanScheme.u.stYmdPlan.eDate.Day,
				&stPlanScheme.u.stYmdPlan.eDate.Hour,
				&stPlanScheme.u.stYmdPlan.eDate.Min,
				&stPlanScheme.u.stYmdPlan.eDate.Sencond);
				
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"eDate: %04d %02d %02d %02d %02d %02d\r\n",
				stPlanScheme.u.stYmdPlan.eDate.Year, 
				stPlanScheme.u.stYmdPlan.eDate.Month,
				stPlanScheme.u.stYmdPlan.eDate.Day,
				stPlanScheme.u.stYmdPlan.eDate.Hour,
				stPlanScheme.u.stYmdPlan.eDate.Min,
				stPlanScheme.u.stYmdPlan.eDate.Sencond);
			/*sDate and eDate valide judgement*/
			ret = CompareDate(cJSON_GetObjectItem(plans, "sDate")->valuestring, 
				cJSON_GetObjectItem(plans, "eDate")->valuestring);
			if(1 == ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,PLAN_DATE_ERR,PLAN_DATE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			
			/**jobs**/
			jobs = cJSON_GetObjectItem(plans, "jobs");
			if(NULL == jobs){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(plans, "jobs"),cJSON_Array,mid);
			arraySize = cJSON_GetArraySize(jobs);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"arraySize %d\r\n", arraySize);
			if(arraySize > JOB_MAX){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JOB_ID_OF_ERR,JOB_ID_OF_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			
			if(0 == arraySize){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JOB_EMPTY_ERR,JOB_EMPTY_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}

			stPlanScheme.jobNum = arraySize;
			memset(stPlanScheme.jobs, 0, sizeof(stPlanScheme.jobs));
			for(i=0; i < arraySize; i++)
			{
				item=cJSON_GetArrayItem(jobs,i);
				stPlanScheme.jobs[i].used = 1;
				object = cJSON_GetObjectItem(item, "min");
				if(NULL == object){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(item, "min"),cJSON_String,mid);
				stPlanScheme.jobs[i].intervTime = atoi(cJSON_GetObjectItem(item, "min")->valuestring);
				object = cJSON_GetObjectItem(item, "bri");
				if(NULL == object){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				if(cJSON_String != cJSON_GetObjectItem(item, "bri")->type){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_TYPE_STRING_ERR,JSON_TYPE_STRING_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				stPlanScheme.jobs[i].dimLevel = atoi(cJSON_GetObjectItem(item, "bri")->valuestring);
				if(stPlanScheme.jobs[i].dimLevel < BRI_MIN || stPlanScheme.jobs[i].dimLevel > BRI_MAX){
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ast id:%d plan min %d dimLevel %d\r\n",
						i,
						stPlanScheme.jobs[i].intervTime, 
						stPlanScheme.jobs[i].dimLevel);
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,BRI_VAL_ERR,BRI_VAL_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ast id:%d plan min %d dimLevel %d\r\n",
					i,
					stPlanScheme.jobs[i].intervTime, 
					stPlanScheme.jobs[i].dimLevel);
			}
		}
		//week
		else if(WEEK_MODE == type){
			stPlanScheme.hdr.mode = WEEK_MODE;
			stPlanScheme.hdr.index = id;
			stPlanScheme.hdr.priority = level;
			stPlanScheme.hdr.valid= valid;
			/**plans**/
			plans = cJSON_GetObjectItem(cmdData, "plans");
			if(NULL == plans){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(cmdData, "plans"),cJSON_Object,mid);
			/**valide date**/
			validDate = cJSON_GetObjectItem(plans, "validDate");
			if(NULL == validDate){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(plans, "validDate"),cJSON_Object,mid);
			stPlanScheme.u.stWeekPlan.dateValide = 1;
			
			object = cJSON_GetObjectItem(validDate, "sDate");
			if(NULL == object){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(validDate, "sDate"),cJSON_String,mid);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"sDate: %s", cJSON_GetObjectItem(validDate, "sDate")->valuestring);
			/*sscanf(cJSON_GetObjectItem(validDate, "sDate")->valuestring, "%04d-%02d-%02d %02d:%02d:%02d", 
				&stPlanScheme.u.stWeekPlan.sDate.Year, 
				&stPlanScheme.u.stWeekPlan.sDate.Month,
				&stPlanScheme.u.stWeekPlan.sDate.Day,
				&stPlanScheme.u.stWeekPlan.sDate.Hour,
				&stPlanScheme.u.stWeekPlan.sDate.Min,
				&stPlanScheme.u.stWeekPlan.sDate.Sencond);*/

			sscanf(cJSON_GetObjectItem(validDate, "sDate")->valuestring, "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu", 
				&stPlanScheme.u.stWeekPlan.sDate.Year, 
				&stPlanScheme.u.stWeekPlan.sDate.Month,
				&stPlanScheme.u.stWeekPlan.sDate.Day,
				&stPlanScheme.u.stWeekPlan.sDate.Hour,
				&stPlanScheme.u.stWeekPlan.sDate.Min,
				&stPlanScheme.u.stWeekPlan.sDate.Sencond);
			
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"sDate: %04d %02d %02d %02d %02d %02d\r\n",
				stPlanScheme.u.stWeekPlan.sDate.Year, 
				stPlanScheme.u.stWeekPlan.sDate.Month,
				stPlanScheme.u.stWeekPlan.sDate.Day,
				stPlanScheme.u.stWeekPlan.sDate.Hour,
				stPlanScheme.u.stWeekPlan.sDate.Min,
				stPlanScheme.u.stWeekPlan.sDate.Sencond);
			
			object = cJSON_GetObjectItem(validDate, "eDate");
			if(NULL == object){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(validDate, "eDate"),cJSON_String,mid);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"eDate: %s", cJSON_GetObjectItem(validDate, "eDate")->valuestring);
			/*sscanf(cJSON_GetObjectItem(validDate, "eDate")->valuestring, "%04d-%02d-%02d %02d:%02d:%02d", 
				&stPlanScheme.u.stWeekPlan.eDate.Year, 
				&stPlanScheme.u.stWeekPlan.eDate.Month,
				&stPlanScheme.u.stWeekPlan.eDate.Day,
				&stPlanScheme.u.stWeekPlan.eDate.Hour,
				&stPlanScheme.u.stWeekPlan.eDate.Min,
				&stPlanScheme.u.stWeekPlan.eDate.Sencond);*/
			sscanf(cJSON_GetObjectItem(validDate, "eDate")->valuestring, "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu", 
				&stPlanScheme.u.stWeekPlan.eDate.Year, 
				&stPlanScheme.u.stWeekPlan.eDate.Month,
				&stPlanScheme.u.stWeekPlan.eDate.Day,
				&stPlanScheme.u.stWeekPlan.eDate.Hour,
				&stPlanScheme.u.stWeekPlan.eDate.Min,
				&stPlanScheme.u.stWeekPlan.eDate.Sencond);
				
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"eDate: %04d %02d %02d %02d %02d %02d\r\n",
				stPlanScheme.u.stWeekPlan.eDate.Year, 
				stPlanScheme.u.stWeekPlan.eDate.Month,
				stPlanScheme.u.stWeekPlan.eDate.Day,
				stPlanScheme.u.stWeekPlan.eDate.Hour,
				stPlanScheme.u.stWeekPlan.eDate.Min,
				stPlanScheme.u.stWeekPlan.eDate.Sencond);
			ret = CompareDate(cJSON_GetObjectItem(validDate, "sDate")->valuestring, 
				cJSON_GetObjectItem(validDate, "eDate")->valuestring);
			if(1 == ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,PLAN_DATE_ERR,PLAN_DATE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			/**week**/
			weeks = cJSON_GetObjectItem(plans, "week");
			if((NULL == weeks) && (strlen(weeks->valuestring) != 7)){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(plans, "week"),cJSON_String,mid);
			if(7 != strlen(weeks->valuestring)){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,PLAN_WEEK_ERR,PLAN_WEEK_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			for(i=0; i<strlen(weeks->valuestring); i++){
				if('1' == weeks->valuestring[i]){
					stPlanScheme.u.stWeekPlan.bitmapWeek |= 1<<i;
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"week bitmap: %d\r\n", stPlanScheme.u.stWeekPlan.bitmapWeek);								
				}else{
					stPlanScheme.u.stWeekPlan.bitmapWeek &= ~(1<<i);
				}
			}
			/**jobs**/
			jobs = cJSON_GetObjectItem(plans, "jobs");
			if(NULL == jobs){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(plans, "jobs"),cJSON_Array,mid);
			arraySize = cJSON_GetArraySize(jobs);
			/*if(arraySize > JOB_MAX-1)*/if(arraySize > JOB_MAX){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JOB_ID_OF_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				cJSON_Delete(json);
				return OPP_FAILURE;
			}

			if(0 == arraySize){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JOB_EMPTY_ERR,JOB_EMPTY_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			stPlanScheme.jobNum = arraySize;								
			memset(stPlanScheme.jobs, 0, sizeof(stPlanScheme.jobs));								
			for(i=0; i < arraySize; i++)
			{
				item=cJSON_GetArrayItem(jobs,i);
				object = cJSON_GetObjectItem(item, "sTime");
				if(NULL == object){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(item, "sTime"),cJSON_String,mid);
				object = cJSON_GetObjectItem(item, "sTime");
				if(NULL == object){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				
				sscanf(cJSON_GetObjectItem(item, "sTime")->valuestring,
					"%d:%d", (int *)&stPlanScheme.jobs[i].startHour, (int *)&stPlanScheme.jobs[i].startMin);

				object = cJSON_GetObjectItem(item, "eTime");
				if(NULL == object){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(item, "eTime"),cJSON_String,mid);
				sscanf(cJSON_GetObjectItem(item, "eTime")->valuestring,
					"%d:%d", (int *)&stPlanScheme.jobs[i].endHour, (int *)&stPlanScheme.jobs[i].endMin);

				object = cJSON_GetObjectItem(item, "bri");
				if(NULL == object){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				if(cJSON_String != cJSON_GetObjectItem(item, "bri")->type){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_TYPE_STRING_ERR,JSON_TYPE_STRING_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				stPlanScheme.jobs[i].dimLevel = atoi(cJSON_GetObjectItem(item, "bri")->valuestring);
				if(stPlanScheme.jobs[i].dimLevel < BRI_MIN || stPlanScheme.jobs[i].dimLevel > BRI_MAX){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,BRI_VAL_ERR,BRI_VAL_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				stPlanScheme.jobs[i].used = 1;
			}
			
		}
		else if(YMD_MODE == type){
			stPlanScheme.hdr.mode = YMD_MODE;
			stPlanScheme.hdr.index = id;
			stPlanScheme.hdr.priority = level;
			stPlanScheme.hdr.valid= valid;
			/**plans**/
			plans = cJSON_GetObjectItem(cmdData, "plans");
			if(NULL == plans){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(cmdData, "plans"),cJSON_Object,mid);
			/*date*/
			object = cJSON_GetObjectItem(plans, "sDate");
			if(NULL == plans){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(plans, "sDate"),cJSON_String,mid);
			/*sscanf(cJSON_GetObjectItem(plans, "sDate")->valuestring, "%04d-%02d-%02d %02d:%02d:%02d", 
				&stPlanScheme.u.stYmdPlan.sDate.Year, 
				&stPlanScheme.u.stYmdPlan.sDate.Month,
				&stPlanScheme.u.stYmdPlan.sDate.Day,
				&stPlanScheme.u.stYmdPlan.sDate.Hour,
				&stPlanScheme.u.stYmdPlan.sDate.Min,
				&stPlanScheme.u.stYmdPlan.sDate.Sencond);*/
			sscanf(cJSON_GetObjectItem(plans, "sDate")->valuestring, "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu", 
				&stPlanScheme.u.stYmdPlan.sDate.Year, 
				&stPlanScheme.u.stYmdPlan.sDate.Month,
				&stPlanScheme.u.stYmdPlan.sDate.Day,
				&stPlanScheme.u.stYmdPlan.sDate.Hour,
				&stPlanScheme.u.stYmdPlan.sDate.Min,
				&stPlanScheme.u.stYmdPlan.sDate.Sencond);
				
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"sDate: %04d %02d %02d %02d %02d %02d\r\n",
				stPlanScheme.u.stYmdPlan.sDate.Year, 
				stPlanScheme.u.stYmdPlan.sDate.Month,
				stPlanScheme.u.stYmdPlan.sDate.Day,
				stPlanScheme.u.stYmdPlan.sDate.Hour,
				stPlanScheme.u.stYmdPlan.sDate.Min,
				stPlanScheme.u.stYmdPlan.sDate.Sencond);
			
			object = cJSON_GetObjectItem(plans, "eDate");
			if(NULL == plans){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(plans, "eDate"),cJSON_String,mid);
			/*sscanf(cJSON_GetObjectItem(plans, "eDate")->valuestring, "%04d-%02d-%02d %02d:%02d:%02d", 
				&stPlanScheme.u.stYmdPlan.eDate.Year, 
				&stPlanScheme.u.stYmdPlan.eDate.Month,
				&stPlanScheme.u.stYmdPlan.eDate.Day,
				&stPlanScheme.u.stYmdPlan.eDate.Hour,
				&stPlanScheme.u.stYmdPlan.eDate.Min,
				&stPlanScheme.u.stYmdPlan.eDate.Sencond);*/
			sscanf(cJSON_GetObjectItem(plans, "eDate")->valuestring, "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu", 
				&stPlanScheme.u.stYmdPlan.eDate.Year, 
				&stPlanScheme.u.stYmdPlan.eDate.Month,
				&stPlanScheme.u.stYmdPlan.eDate.Day,
				&stPlanScheme.u.stYmdPlan.eDate.Hour,
				&stPlanScheme.u.stYmdPlan.eDate.Min,
				&stPlanScheme.u.stYmdPlan.eDate.Sencond);
				
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"eDate: %04d %02d %02d %02d %02d %02d\r\n",
				stPlanScheme.u.stYmdPlan.eDate.Year, 
				stPlanScheme.u.stYmdPlan.eDate.Month,
				stPlanScheme.u.stYmdPlan.eDate.Day,
				stPlanScheme.u.stYmdPlan.eDate.Hour,
				stPlanScheme.u.stYmdPlan.eDate.Min,
				stPlanScheme.u.stYmdPlan.eDate.Sencond);
			/*sDate and eDate valide judgement*/
			ret = CompareDate(cJSON_GetObjectItem(plans, "sDate")->valuestring, 
				cJSON_GetObjectItem(plans, "eDate")->valuestring);
			if(1 == ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,PLAN_DATE_ERR,PLAN_DATE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			/*jobs*/
			jobs = cJSON_GetObjectItem(plans, "jobs");
			if(NULL == jobs){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(plans, "jobs"),cJSON_Array,mid);
			arraySize = cJSON_GetArraySize(jobs);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"~~~~~~~~arraySize[%d]~~~~~~~\r\n",arraySize);						
			if(arraySize > JOB_MAX){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JOB_ID_OF_ERR,JOB_ID_OF_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			if(arraySize == 0){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JOB_EMPTY_ERR,JOB_EMPTY_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE;
			}
			
			stPlanScheme.jobNum = arraySize;
			for(i=0; i < arraySize; i++)
			{
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"~~~~~~~~jobs[%d]~~~~~~~\r\n",i);
				item=cJSON_GetArrayItem(jobs,i);
				object = cJSON_GetObjectItem(item, "sTime");
				if(NULL == object){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(item, "sTime"),cJSON_String,mid);
				sscanf(cJSON_GetObjectItem(item, "sTime")->valuestring,
					"%d:%d", (int *)&stPlanScheme.jobs[i].startHour, (int *)&stPlanScheme.jobs[i].startMin);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"sTime %02d:%02d\r\n", stPlanScheme.jobs[i].startHour, stPlanScheme.jobs[i].startMin);

				object = cJSON_GetObjectItem(item, "eTime");
				if(NULL == object){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(item, "eTime"),cJSON_String,mid);
				sscanf(cJSON_GetObjectItem(item, "eTime")->valuestring,
					"%d:%d", (int *)&stPlanScheme.jobs[i].endHour, (int *)&stPlanScheme.jobs[i].endMin);

				object = cJSON_GetObjectItem(item, "bri");
				if(NULL == object){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}
				APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(item, "bri"),cJSON_String,mid);
				stPlanScheme.jobs[i].dimLevel = atoi(cJSON_GetObjectItem(item, "bri")->valuestring);
				if(stPlanScheme.jobs[i].dimLevel < BRI_MIN || stPlanScheme.jobs[i].dimLevel > BRI_MAX){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,BRI_VAL_ERR,BRI_VAL_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);
					return OPP_FAILURE;
				}

				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"eTime %02d:%02d\r\n", stPlanScheme.jobs[i].endHour, stPlanScheme.jobs[i].endMin);
				stPlanScheme.jobs[i].used = 1;
			}
		}
		
		if(AST_MODE == type){
			ret = ApsCoapAstJobsTimeIsValide(&stPlanScheme);
		}else{
			ret = ApsCoapJobsTimeIsValide(&stPlanScheme);
		}
		if(OPP_SUCCESS != ret){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapJobsTimeIsValide ret %d\r\n", ret);
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JOB_TIME_ERR,JOB_TIME_ERR_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;
		}
		
		/*清空flash*/
		memset(&stPlanSchemeZ, 0, sizeof(stPlanSchemeZ));
		OppLampCtrlAddWorkSchemeById((id-1),&stPlanSchemeZ);
		/*写flash*/
		stPlanScheme.used = 1;
		OppLampCtrlWorkSchDump(&stPlanScheme);					
		OppLampCtrlAddWorkSchemeById((id-1),&stPlanScheme);
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,0,NULL,NULL,mid);
		/*重置调光计划*/
		OppLampCtrlDimmerPolicyReset();
	}
	else if(0 == strcmp(op,"R")){
		/* no need rsp*/
		if(0 == isNeedRsp){
			return OPP_FAILURE;
		}
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		if(NULL == cmdData){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;
		}
		APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(json, "cmdData"),cJSON_Object,mid);
		object = cJSON_GetObjectItem(cmdData, "ids");
		if(NULL == object){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;
		}
		APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(cmdData, "ids"),cJSON_Array,mid);
		arraySize = cJSON_GetArraySize(object);
		if(0 == arraySize){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"no data here\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,IDS_R_EMPTY_ERR,IDS_R_EMPTY_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;						
		}

		if(arraySize > 1){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"no data here\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,IDS_MORE_ERR,IDS_MORE_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;						
		}
		
		for(cnt = 0, i=0; i < arraySize; i++){
			item=cJSON_GetArrayItem(object,i);
			OppLampCtrlGetConfigPara(&stConfigPara);
			if(item->valueint > stConfigPara.plansNum){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,IDS_R_BIG_ERR,IDS_R_BIG_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE; 					
			}
			if(item->valueint <= 0){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,IDS_R_SMALL_ERR,IDS_R_SMALL_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE; 					
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"workSchem query R ids %d\r\n", item->valueint);
			ret = OppLampCtrlGetWorkSchemeElementByIdxOut((item->valueint-1), &stPlanScheme);
			OppLampCtrlWorkSchDump(&stPlanScheme);
			if((OPP_SUCCESS == ret) && (1 == stPlanScheme.used)){
				cnt++;
			}
		}
		/*no data*/
		if(0 == cnt){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"no data here\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JOB_EMPTY_ERR,JOB_EMPTY_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;						
		}
		/*只要有一组没数据就返回错误*/
		if(arraySize > 0){
			for(i=0; i < arraySize; i++){
				item=cJSON_GetArrayItem(object,i);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"workSchem R ids %d\r\n", item->valueint);
				ret = OppLampCtrlGetWorkSchemeElementByIdxOut((item->valueint-1), &stPlanScheme);
				//wangtao todo here!!!
				if((OPP_SUCCESS != ret) || (1 != stPlanScheme.used)){
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "workSchem R id %d no data here\r\n", item->valueint);
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,PLANID_R_NOT_EXIST_ERR,PLANID_R_NOT_EXIST_ERR_DESC,NULL,mid);
					//cJSON_Delete(json);								
					return OPP_FAILURE;
				}
			}
		}
		#if 1
		if(arraySize > 0){
			ApsCoapFuncRspBatchStart(dstChl,reqId,0,0,NULL,WORKPLAN_CMDID,&root,&cmdDataRe);
			planArray=cJSON_CreateArray();
			if(NULL == planArray){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapWorkPlanProc create <planArray> array error\r\n");
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
				cJSON_Delete(root);
				return OPP_FAILURE;
			}
			cJSON_AddItemToObject(cmdDataRe, "planArray", planArray);
			for(i=0; i < arraySize; i++){
				item=cJSON_GetArrayItem(object,i);
				//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"workSchem R ids %d\r\n", item->valueint);
				ret = OppLampCtrlGetWorkSchemeElementByIdxOut((item->valueint-1), &stPlanScheme);
				//wangtao todo here!!!
				OppLampCtrlWorkSchDump(&stPlanScheme);
				ApsCoapWorkPlanRspBatchAppend(root,cmdDataRe,planArray,&stPlanScheme);
			}
			ret = ApsCoapFuncRspBatchStop(dstChl, dstInfo, root, mid);
			if(NBIOT_SEND_BIG_ERR == ret){
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,0,PROP_SERVICE,NBIOT_SEND_BIG_ERR,NBIOT_SEND_BIG_ERR_DESC,NULL,mid);
			}
		}
		#else
		if(arraySize > 0){
			ApsCoapFuncRspBatchStart(dstChl,reqId,0,0,NULL,WORKPLAN_CMDID,&root,&cmdDataRe);
			for(i=0; i < arraySize; i++){
				item=cJSON_GetArrayItem(object,i);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"workSchem R ids %d\r\n", item->valueint);

				ret = OppLampCtrlGetWorkSchemeElementByIdxOut((item->valueint-1), &stPlanScheme);
				//wangtao todo here!!!
				OppLampCtrlWorkSchDump(&stPlanScheme);
				ApsCoapWorkPlanRspBatchAppend(root,cmdDataRe,&stPlanScheme);
			}
			ret = ApsCoapFuncRspBatchStop(dstChl, root, mid);
			if(NBIOT_SEND_BIG_ERR == ret){
				ApsCoapRsp(dstChl,dstInfo,isNeedRsp,reqId,0,PROP_SERVICE,NBIOT_SEND_BIG_ERR,NBIOT_SEND_BIG_ERR_DESC,NULL,mid);
			}
		}
		#endif
		
	}						
	else if(0 == strcmp(op,"D")){
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		if(NULL == cmdData){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;
		}
		APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(json, "cmdData"),cJSON_Object,mid);
		object = cJSON_GetObjectItem(cmdData, "ids");
		if(NULL == object){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;
		}
		APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,cJSON_GetObjectItem(cmdData, "ids"),cJSON_Array,mid);		
		arraySize = cJSON_GetArraySize(object);
		if(0 == arraySize){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"no data here\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,IDS_D_EMPTY_ERR,IDS_D_EMPTY_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;						
		}
		OppLampCtrlGetConfigPara(&stConfigPara);
		if(arraySize > stConfigPara.plansNum){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,IDS_D_NUM_ERR,IDS_D_NUM_DESC,NULL,mid);
			//cJSON_Delete(json);
			return OPP_FAILURE;						
		}
		
		for(i=0; i < arraySize; i++){
			item=cJSON_GetArrayItem(object,i);
			if(item->valueint > stConfigPara.plansNum){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,IDS_D_BIG_ERR,IDS_D_BIG_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE; 					
			}
			if(item->valueint <= 0){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,IDS_D_SMALL_ERR,IDS_D_SMALL_ERR_DESC,NULL,mid);
				//cJSON_Delete(json);
				return OPP_FAILURE; 					
			}
			//ids[cnt++] = item->valueint;
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"workSchem D ids %d\r\n", item->valueint);
			//OppLampCtrlDelWorkScheme((item->valueint-1));
			memset(&stPlanScheme,0,sizeof(ST_WORK_SCHEME));
			OppLampCtrlAddWorkSchemeById(item->valueint-1,&stPlanScheme);
		}
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,SUCCESS,NULL,NULL,mid);
		/*调光测试重置*/
		OppLampCtrlDimmerPolicyReset();
	}else{
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLAN_CMDID,reqId,0,PLAN_OP_ERR,PLAN_OP_ERR_DESC,NULL,mid);
		return OPP_FAILURE; 					
	}

	return OPP_SUCCESS;
}

int ApsCoapWorkPlanSynProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int reqId = 0;
	//int ret;
	cJSON *cmdData;
	int planId[PLAN_MAX];
	int num = 0;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLANSYN_CMDID,json,"reqId",mid);	
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	cmdData = cJSON_CreateObject();
	if(NULL == cmdData){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapWorkPlanSynProc create <cmdData> object error\r\n");
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLANSYN_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	
	OppLampCtrlGetValidePlanId(planId,&num);
	/*if(num == 0){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLANSYN_CMDID,reqId,0,WORKPLANSYN_NO_PLAN_ERR,WORKPLANSYN_NO_PLAN_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}*/
	cJSON_AddItemToObject(cmdData, "ids", cJSON_CreateIntArray(planId, num));
	
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,WORKPLANSYN_CMDID,reqId,0,0,NULL,cmdData,mid);

	return OPP_SUCCESS;
}

int ApsCoapOtaProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *cmdData;
	int reqId = 0;
	char buf[64] = {0};	
	unsigned short port;
	//int type;
	int ret = OPP_SUCCESS;
	unsigned char zeroInfo[MSG_INFO_SIZE] ={0};
	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,OTAUDP_CMDID,json,"reqId",mid);
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,OTAUDP_CMDID,json,"cmdData",mid);
	cmdData = cJSON_GetObjectItem(json, "cmdData");
	//type = cJSON_GetObjectItem(cmdData, "type")->valueint;
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,OTAUDP_CMDID,cmdData,"port",mid);
	port = cJSON_GetObjectItem(cmdData, "port")->valueint;
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,OTAUDP_CMDID,cmdData,"url",mid);
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,OTAUDP_CMDID,cmdData,"version",mid);
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,OTAUDP_CMDID,cmdData,"port",mid);
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"url:%s", cJSON_GetObjectItem(cmdData, "url")->valuestring);
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"version:%s", cJSON_GetObjectItem(cmdData, "version")->valuestring);
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"port:%d", port);
	//port = cJSON_GetObjectItem(cmdData, "port")->valueint;
	g_stOtaProcess.type = OTA_RSP;
	g_stOtaProcess.reqId = reqId;
	g_stOtaProcess.process = 0;
	g_stOtaProcess.state = 0;
	g_stOtaProcess.mid = mid;
	g_stOtaProcess.dstChl = dstChl;
	g_stOtaProcess.error = 0;
	if(dstInfo && memcmp(dstInfo,zeroInfo,sizeof(zeroInfo)))
		memcpy(g_stOtaProcess.dstInfo,dstInfo,sizeof(g_stOtaProcess.dstInfo));
	STRNCPY(g_stOtaProcess.version, cJSON_GetObjectItem(cmdData, "version")->valuestring, OTA_VER_LEN);
	ApsCoapOtaProcessRsp(&g_stOtaProcess);
	STRNCPY(g_aucOtaSvrIp,cJSON_GetObjectItem(cmdData, "url")->valuestring, IP_LEN);
	itoa(port,buf,10);
	STRNCPY(g_aucOtaSvrPort,buf,PORT_LEN);
	STRNCPY(g_aucOtaFile,cJSON_GetObjectItem(cmdData, "version")->valuestring, FILE_LEN);
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"OtaStart\r\n");
	OtaStart();

	return ret;
}

int ApsCoapLSVolProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *cmdData;
	int reqId = 0;
	uint32_t voltage;
	uint8_t ret;
	
	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,LSVOL_CMDID,json,"reqId",mid);	
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	cmdData = cJSON_CreateObject();
	if(NULL == cmdData){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapLSVolProc create <cmdData> object error\r\n");
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,LSVOL_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	ret = BspLightSensorVoltageGet(&voltage);
	if(0 != ret){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,LSVOL_CMDID,reqId,0,LIGHT_SENSOR_VOL_GET_ERR,LIGHT_SENSOR_VOL_GET_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(cmdData, "voltage", cJSON_CreateNumber(voltage));
	
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,LSVOL_CMDID,reqId,0,0,NULL,cmdData,mid);

	return OPP_SUCCESS;
}

int ApsCoapLSParaProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int reqId = 0;
	cJSON *cmdData, *cmdDataRe;
	uint16_t voltage, para;
	char op[10] = {0};
	uint8_t ret;
		
	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,LSVOL_CMDID,json,"reqId",mid);	
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,WORKPLAN_CMDID,json,"op",mid);	
	STRNCPY(op, cJSON_GetObjectItem(json, "op")->valuestring, sizeof(op));
	if(0 == strcmp(op,"W")){
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,OTAUDP_CMDID,json,"cmdData",mid);
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,OTAUDP_CMDID,cmdData,"lsPara",mid);
		voltage = cJSON_GetObjectItem(cmdData, "lsPara")->valueint;
		
		ret = LightSensorParamSet(voltage);
		if(0 != ret){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,LSPARA_CMDID,reqId,0,LIGHT_SENSOR_PARA_WRITE_ERR,LIGHT_SENSOR_PARA_WRITE_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,LSPARA_CMDID,reqId,0,0,NULL,NULL,mid);
	}else if(0 == strcmp(op,"R")){
		cmdDataRe = cJSON_CreateObject();
		if(NULL == cmdDataRe){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapLSParaProc create <cmdData> object error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,LSPARA_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		ret = LightSensorParamGet(&para);
		if(0 != ret){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,LSPARA_CMDID,reqId,0,LIGHT_SENSOR_PARA_READ_ERR,LIGHT_SENSOR_PARA_READ_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		cJSON_AddItemToObject(cmdDataRe, "lsPara", cJSON_CreateNumber(para));
		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,LSPARA_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
	}else if(0 == strcmp(op,"D")){
		ret = DelLightSensorParam();
		if(0 != ret){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,LSPARA_CMDID,reqId,0,LIGHT_SENSOR_PARA_DEL_ERR,LIGHT_SENSOR_PARA_DEL_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,LSPARA_CMDID,reqId,0,0,NULL,NULL,mid);
	}else{
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,LSPARA_CMDID,reqId,0,LIGHT_SENSOR_OP_ERR,LIGHT_SENSOR_OP_ERR_DESC,NULL,mid);
	}
	return OPP_SUCCESS;	
}


int ApsCoapExepProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int reqId = 0;
	cJSON *cmdData, *cmdDataRe, *comObj;
	int exepNum;
	char op[10] = {0};
	uint8_t ret;
	ST_EXEP_INFO stExepInfo;
	ST_COAP_PKT	*pstCoapPkt;
	ST_BC28_PKT *pstBc28Pkt;
	int len = 0;
		
	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,EXEP_CMDID,json,"reqId",mid);	
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,EXEP_CMDID,json,"op",mid);	
	STRNCPY(op, cJSON_GetObjectItem(json, "op")->valuestring, sizeof(op));
	if(0 == strcmp(op,"R")){
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,EXEP_CMDID,json,"cmdData",mid);
		cmdData = cJSON_GetObjectItem(json, "cmdData");
		APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,EXEP_CMDID,cmdData,"exepNum",mid);
		exepNum = cJSON_GetObjectItem(cmdData, "exepNum")->valueint;
		if(0 == exepNum){
			ret = ApsExepReadCurExep(&stExepInfo);
		}else if(1 == exepNum){
			ret = ApsExepReadPreExep(&stExepInfo);
		}else{
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,EXEP_CMDID,reqId,0,EXEP_NUM_ERR,EXEP_NUM_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		if(0 != ret){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,EXEP_CMDID,reqId,0,EXEP_GET_ERR,EXEP_GET_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		cmdDataRe = cJSON_CreateObject();
		if(NULL == cmdDataRe){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapExepProc create <cmdData> object error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,EXEP_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}

		//if(BC28_READY == NeulBc28GetDevState()){
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapExepProc create nbState <comObj> object error\r\n");
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,EXEP_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			cJSON_AddItemToObject(cmdDataRe, "fwv", cJSON_CreateNumber(OPP_LAMP_CTRL_CFG_DATA_VER));
			cJSON_AddItemToObject(cmdDataRe, "nbState", comObj);
			cJSON_AddItemToObject(comObj, "ds", cJSON_CreateNumber(NeulBc28GetDevState())); 
			cJSON_AddItemToObject(comObj, "at", cJSON_CreateNumber(NeulBc28GetAttachEplaseTime())); 
			cJSON_AddItemToObject(comObj, "hdt", cJSON_CreateNumber(ApsCoapHeartDisOffsetSecondGet()));	
			cJSON_AddItemToObject(comObj, "adt", cJSON_CreateNumber(NeulBc28DisOffsetSecondGet()));
		//}
		cJSON_AddItemToObject(cmdDataRe, "exepNum", cJSON_CreateNumber(exepNum)); 
		cJSON_AddItemToObject(cmdDataRe, "oppRst", cJSON_CreateNumber(stExepInfo.oppleRstType));
		cJSON_AddItemToObject(cmdDataRe, "rst0", cJSON_CreateNumber(stExepInfo.rst0Type));
		cJSON_AddItemToObject(cmdDataRe, "rst1", cJSON_CreateNumber(stExepInfo.rst1Type));	
		cJSON_AddItemToObject(cmdDataRe, "rstTick", cJSON_CreateNumber(stExepInfo.tick));
		
		if(COAP_RST == stExepInfo.oppleRstType){
			comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapExepReport create coapInfo <comObj> object error\r\n");
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,EXEP_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}
			
			pstCoapPkt = (ST_COAP_PKT *)stExepInfo.data;
			cJSON_AddItemToObject(cmdDataRe, "coPkt", comObj);
			cJSON_AddItemToObject(comObj, "ts", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbTxSuc));
			cJSON_AddItemToObject(comObj, "tf", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbTxFail));
			cJSON_AddItemToObject(comObj, "tReqR", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbTxReqRetry));
			cJSON_AddItemToObject(comObj, "txRspR", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbTxRspRetry));
			cJSON_AddItemToObject(comObj, "r", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbRx));
			cJSON_AddItemToObject(comObj, "rRsp", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbRxRsp));
			cJSON_AddItemToObject(comObj, "rReq", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbRxReq));
			cJSON_AddItemToObject(comObj, "rDReq", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbDevReq));
			cJSON_AddItemToObject(comObj, "rDRsp", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbDevRsp));
			cJSON_AddItemToObject(comObj, "rU", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbUnknow));
			cJSON_AddItemToObject(comObj, "orx", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbOtaRx));
			cJSON_AddItemToObject(comObj, "otx", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbOtaTx));
			cJSON_AddItemToObject(comObj, "ots", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbOtaTxSucc));
			cJSON_AddItemToObject(comObj, "otf", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbOtaTxFail));
			cJSON_AddItemToObject(comObj, "busy", cJSON_CreateNumber(pstCoapPkt->g_iCoapBusy));
			/*comObj = cJSON_CreateObject();
			if(NULL == comObj){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapExepReport create bc28Pkt <comObj> object error\r\n");
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,EXEP_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
				return OPP_FAILURE;
			}		
			len = sizeof(ST_COAP_PKT);
			pstBc28Pkt = (ST_BC28_PKT *)&stExepInfo.data[len];
			cJSON_AddItemToObject(cmdDataRe, "bc28Pkt", comObj);
			cJSON_AddItemToObject(comObj, "rxFromQue", cJSON_CreateNumber(pstBc28Pkt->iPktRxFromQue));
			cJSON_AddItemToObject(comObj, "tx", cJSON_CreateNumber(pstBc28Pkt->iPktTx));
			cJSON_AddItemToObject(comObj, "txSucc", cJSON_CreateNumber(pstBc28Pkt->iPktTxSucc));
			cJSON_AddItemToObject(comObj, "txFail", cJSON_CreateNumber(pstBc28Pkt->iPktTxFail));
			cJSON_AddItemToObject(comObj, "txExpire", cJSON_CreateNumber(pstBc28Pkt->iPktTxExpire));
			cJSON_AddItemToObject(comObj, "nnmiRx", cJSON_CreateNumber(pstBc28Pkt->iNnmiRx));
			cJSON_AddItemToObject(comObj, "pushSuc", cJSON_CreateNumber(pstBc28Pkt->iPktPushSuc));
			cJSON_AddItemToObject(comObj, "pushFail", cJSON_CreateNumber(pstBc28Pkt->iPktPushFail));
			cJSON_AddItemToObject(comObj, "txEnable", cJSON_CreateNumber(pstBc28Pkt->iTxEnable));*/
		}
		
		ret = ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,EXEP_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
		if(NBIOT_SEND_BIG_ERR == ret){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,EXEP_CMDID,reqId,0,NBIOT_SEND_BIG_ERR,NBIOT_SEND_BIG_ERR_DESC,NULL,mid);
		}
	}else{
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,EXEP_CMDID,reqId,0,EXEP_OP_ERR,EXEP_OP_ERR_DESC,NULL,mid);
	}
	return OPP_SUCCESS;	
}


int ApsCoapAttachProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int reqId = 0;
	cJSON *cmdDataRe, *comObj;

	APS_COAP_NULL_POINT_CHECK(json);

	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,ATTACH_CMDID,json,"reqId",mid);	
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	
	
	if(BC28_READY == NeulBc28GetDevState()){
		cmdDataRe = cJSON_CreateObject();
		if(NULL == cmdDataRe){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapAttachProc create <cmdData> object error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ATTACH_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		comObj = cJSON_CreateObject();
		if(NULL == comObj){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapAttachProc create nbState <comObj> object error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ATTACH_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		cJSON_AddItemToObject(cmdDataRe, "nbState", comObj);
		cJSON_AddItemToObject(comObj, "devStatus", cJSON_CreateNumber(NeulBc28GetDevState())); 
		cJSON_AddItemToObject(comObj, "attachTime", cJSON_CreateNumber(NeulBc28GetAttachEplaseTime())); 
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ATTACH_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
	}else{
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ATTACH_CMDID,reqId,0,ATTACH_DEV_NOT_READY_ERR,ATTACH_DEV_NOT_READY_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	
	return OPP_SUCCESS;	
}

extern esp_err_t print_panic_saved(int core_id, char *outBuf);

int ApsCoapPanicProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	int reqId = 0;
	cJSON *cmdData, *cmdDataRe, *comObj;
	char *ptr0 = NULL, *ptr1 = NULL;
	char *ptr;
	int st;
	int ret0,ret1;
	char panic0[1024] = {0};
	char panic1[1024] = {0};
	
	APS_COAP_NULL_POINT_CHECK(json);
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PANIC_CMDID,json,"reqId",mid);	
	reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PANIC_CMDID,json,"cmdData",mid);
	cmdData = cJSON_GetObjectItem(json, "cmdData");
	APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,PANIC_CMDID,cmdData,"st",mid);
	st = cJSON_GetObjectItem(cmdData, "st")->valueint;

	if(0 == st){
		ApsExepGetPanic(0,&ptr0);
		ApsExepGetPanic(1,&ptr1);
		if(0 == strlen(ptr0) && 0 == strlen(ptr1)){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PANIC_CMDID,reqId,0,PANIC_EMPTY_ERR,PANIC_EMPTY_ERR_DESC,NULL,mid);
			return OPP_SUCCESS;
		}
		
		cmdDataRe = cJSON_CreateObject();
		if(NULL == cmdDataRe){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPanicProc create <cmdData> object error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PANIC_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		ptr = strstr(ptr0,"Backtrace:");
		if(ptr){
			cJSON_AddItemToObject(cmdDataRe, "panic0", cJSON_CreateString(ptr));
		}
		ptr = strstr(ptr1,"Backtrace:");
		if(ptr){
			cJSON_AddItemToObject(cmdDataRe, "panic1", cJSON_CreateString(ptr));
		}
	}else{
		ret0 = print_panic_saved(0,panic0);
		ret1 = print_panic_saved(1,panic1);
		if(OPP_SUCCESS != ret0 && OPP_SUCCESS != ret1){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PANIC_CMDID,reqId,0,PANIC_EMPTY_ERR,PANIC_EMPTY_ERR_DESC,NULL,mid);
			return OPP_SUCCESS;
		}
		cmdDataRe = cJSON_CreateObject();
		if(NULL == cmdDataRe){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPanicProc create <cmdData> object error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PANIC_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		ptr = strstr(panic0,"Backtrace:");
		if(ptr){
			cJSON_AddItemToObject(cmdDataRe, "panic0", cJSON_CreateString(ptr));
		}
		ptr = strstr(panic1,"Backtrace:");
		if(ptr){
			cJSON_AddItemToObject(cmdDataRe, "panic1", cJSON_CreateString(ptr));
		}
	}
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,PANIC_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
	
	return OPP_SUCCESS;	
}

void ApsCoapFuncProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, char * text, U32 mid)
{
	char *out;
	cJSON *json;
	int reqId = 0;
	int ret = OPP_SUCCESS;
	
	json=cJSON_Parse(text);
	if (!json) {
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"Error before: [%s]\n",cJSON_GetErrorPtr());
		ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,0,PROP_SERVICE,0,JSON_FORMAT_ERR,JSON_FORMAT_ERR_DESC,NULL,mid);
		return;
	}else
	{
		//debug
		out=cJSON_Print(json);
		if(NULL == out){
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,0,FUNC_SERVICE,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			cJSON_Delete(json);
			return;
		}
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%s\n",out);
		myfree(out);

		APS_COAPS_JSON_FORMAT_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,json,"reqId",mid);
		APS_COAPS_JSON_TYPE_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,cJSON_GetObjectItem(json, "reqId"),cJSON_Number,mid);
		reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
		if(reqId <= 0){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"\r\n json id is %d",reqId);
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,FUNC_SERVICE,0,JSON_ID_ZERO_ERR,JSON_ID_ZERO_ERR_DESC,NULL,mid);
			cJSON_Delete(json);
			return;
		}
		
		APS_COAPS_JSON_FORMAT_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,json,"cmdId",mid);
		APS_COAPS_JSON_TYPE_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,reqId,FUNC_SERVICE,cJSON_GetObjectItem(json, "cmdId"),cJSON_String,mid);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n%s\r\n", cJSON_GetObjectItem(json, "cmdId")->valuestring);		
		if(0 != strlen(cJSON_GetObjectItem(json, "cmdId")->valuestring))
		{
			if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, OTAUDP_CMDID)){
				ApsCoapOtaProc(dstChl,dstInfo,isNeedRsp,json,mid);
			}
			/*else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, THRESHOLD_CMDID)){	
				ApsCoapThresholdProc(dstChl, isNeedRsp, json, mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, THRESHOLDSYNC_CMDID)){
				ApsCoapThresholdSyncProc(dstChl, isNeedRsp, json, mid);
			}*/			
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, TIME_CMDID)){
				ApsCoapTimeProc(dstChl,dstInfo,isNeedRsp,json,mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, RESTOR_CMDID)){
				ApsCoapRestoreFactoryProc(dstChl,dstInfo,isNeedRsp,json,mid);
			}
			/*not support alarm config now*/
			/*else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, ALARM_CMDID)){
				ApsCoapAlarmProc(dstChl, dstInfo,isNeedRsp, json, mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, ALARMSYN_CMDID)){
				ApsCoapAlarmSyncProc(dstChl, dstInfo,isNeedRsp, json, mid);
			}			
			*/
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, ALARMFASTCFG_CMDID)){
				ApsCoapAlarmFastCfgProc(dstChl,dstInfo, isNeedRsp,json,mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, ALARMFASTCFGALL_CMDID)){
				ApsCoapAlarmFastCfgAllProc(dstChl,dstInfo, isNeedRsp,json,mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, ALARMFASTREAD_CMDID)){
				ApsCoapAlarmFastReadProc(dstChl,dstInfo, isNeedRsp,json,mid);
			}
			/*not support propconfig config now*/
			/*else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, PROPCONFIG_CMDID)){
				ApsCoapPropConfigProc(dstChl, isNeedRsp, json, mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, PROPCONFIGSYN_CMDID)){
				ApsCoapPropConfigSyncProc(dstChl, isNeedRsp, json, mid);
			}*/
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, PROPCONFIGFASTCFG_CMDID)){
				ApsCoapFastPropConfig(dstChl, dstInfo, isNeedRsp, json, mid);
			}
			/*else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, PROPCONFIGFASTREAD_CMDID)){
				ApsCoapFastPropRead(dstChl, isNeedRsp, json, mid);
			}*/
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, REBOOT_CMDID)){
				ApsCoapRebootProc(dstChl, dstInfo, isNeedRsp, json, mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, GPSLOC_CMDID)){
				ApsCoapGpsLocProc(dstChl, dstInfo, isNeedRsp, json, mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, GPSTIME_CMDID)){
				ApsCoapGpsTimeProc(dstChl, dstInfo, isNeedRsp, json, mid);
			}
			/*not support workplan config now*/			
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, WORKPLAN_CMDID)){
				ApsCoapWorkPlanProc(dstChl, dstInfo, isNeedRsp, json, mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, WORKPLANSYN_CMDID)){
				ApsCoapWorkPlanSynProc(dstChl,dstInfo,isNeedRsp,json,mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, PATROL_CMDID)){
				ret = ApsCoapRIRsp(dstChl,dstInfo,isNeedRsp,reqId,0,SUCCESS,NULL,mid);
				if(NBIOT_SEND_BIG_ERR == ret){					
					ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,PROP_SERVICE,0,NBIOT_SEND_BIG_ERR,NBIOT_SEND_BIG_ERR_DESC,NULL,mid);
				}
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, NBCMD_CMDID)){
				ApsCoapNbCmdProc(dstChl,dstInfo,isNeedRsp,json,mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, HEARTBEATACK_CMDID)){
				ApsCoapHeartbeatAckCmdProc(dstChl,dstInfo,isNeedRsp,json,mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, LSVOL_CMDID)){
				ApsCoapLSVolProc(dstChl,dstInfo,isNeedRsp,json,mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, LSPARA_CMDID)){
				ApsCoapLSParaProc(dstChl,dstInfo,isNeedRsp,json,mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, EXEP_CMDID)){				
				ApsCoapExepProc(dstChl,dstInfo,isNeedRsp,json,mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, ATTACH_CMDID)){
				ApsCoapAttachProc(dstChl,dstInfo,isNeedRsp,json,mid);
			}
			else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, PANIC_CMDID)){
				ApsCoapPanicProc(dstChl,dstInfo,isNeedRsp,json,mid);
			}
			/*else if(0 == strcmp(cJSON_GetObjectItem(json, "cmdId")->valuestring, FCT_CMDID)){
				
			}*/
			else{
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,cJSON_GetObjectItem(json, "cmdId")->valuestring,reqId,0,UNKNOW_CMDID_ERR,UNKNOW_CMDID_ERR_DESC,NULL,mid);
			}
		}else{
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,FUNC_SERVICE,0,FUNC_NO_CMDID_ERR,FUNC_NO_CMDID_ERR_DESC,NULL,mid);
		}
		/*release json root node*/
		cJSON_Delete(json);
	}

}

int ApsCoapLogStatus(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *object, *cmdDataRe;
	int reqId = 0;

	APS_COAP_NULL_POINT_CHECK(json);
	
	object = cJSON_GetObjectItem(json,"reqId");
	if(NULL == object){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGSTATUS_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);		
		return OPP_FAILURE;
	}
	reqId = object->valueint;
	
	object = cJSON_GetObjectItem(json, "op");
	if(NULL == object){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGSTATUS_CMDID,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);	
		return OPP_FAILURE;
	}
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"~ApsCoapLogStatus %s~~~~\r\n", object->valuestring);
	if(0 == strcmp(object->valuestring, "R")){
		cmdDataRe = cJSON_CreateObject();
		if(NULL == cmdDataRe){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapLogStatus create <cmdDataRe> object error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGSTATUS_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);	
			return OPP_FAILURE;
		}
		
		cJSON_AddItemToObject(cmdDataRe, "curLogSaveId", cJSON_CreateNumber(ApsLogSaveIdLatest()));
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGSTATUS_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
	}else{
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGSTATUS_CMDID,reqId,0,LOGSTATUS_OP_ERR,LOGSTATUS_OP_ERR_DESC,NULL,mid);
	}
	return OPP_SUCCESS;
}

int ApsCoapLogConfig(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *object, *cmdData, *cmdDataRe, *items, *log, *logIds;
	int reqId = 0, arraySize = 0, i = 0;
	int logId = -1;
	LOG_CONFIG_T config;
	int ret; 

	APS_COAP_NULL_POINT_CHECK(json);
	
	object = cJSON_GetObjectItem(json,"reqId");
	if(NULL == object){		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);		
		return OPP_FAILURE;
	}
	reqId = object->valueint;
	
	object = cJSON_GetObjectItem(json, "op");
	if(NULL == object){		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);		
		return OPP_FAILURE;
	}
	if(0 == strcmp(object->valuestring, "W")){		
		cmdData = cJSON_GetObjectItem(json,"cmdData");
		if(NULL == cmdData){		
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); 	
			return OPP_FAILURE;
		}
		items = cJSON_GetObjectItem(cmdData, "items");
		if(NULL == items){			
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); 	
			return OPP_FAILURE;
		}
		arraySize = cJSON_GetArraySize(items);
		for(i=0; i<arraySize; i++){
			log = cJSON_GetArrayItem(items,i);
			object = cJSON_GetObjectItem(log, "logId");
			if(NULL == object){				
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); 	
				return OPP_FAILURE;
			}
			config.logid = object->valueint;
			
			object = cJSON_GetObjectItem(log, "isLogReportEnable");
			if(NULL == object){				
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); 	
				return OPP_FAILURE;
			}			
			config.isLogReport = object->valueint;
			
			object = cJSON_GetObjectItem(log, "isLogEnable");
			if(NULL == object){				
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); 	
				return OPP_FAILURE;
			}
			config.isLogSave = object->valueint;
			
			ret = ApsDaemonLogIdParaSet(&config);
			if(OPP_SUCCESS != ret){
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,reqId,0,0,NULL,NULL,mid);
				return ret;
			}
		}
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,reqId,0,0,NULL,NULL,mid);
	}
	else if(0 == strcmp(object->valuestring, "R")){
		cmdData = cJSON_GetObjectItem(json,"cmdData");
		if(NULL == cmdData){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapLogConfig create <cmdData> object error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); 	
			return OPP_FAILURE;
		}
		
		
		logIds = cJSON_GetObjectItem(cmdData, "logIds");
		arraySize = cJSON_GetArraySize(logIds);
		cmdDataRe = cJSON_CreateObject();		
		if(NULL == cmdDataRe){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapLogConfig create <cmdDataRe> object error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		items=cJSON_CreateArray();
		if(NULL == items){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_ARRAY_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapLogConfig create <planArray> array error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,FUNC_SERVICE,ALARM_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			cJSON_Delete(cmdDataRe);
			return OPP_FAILURE;
		}
		cJSON_AddItemToObject(cmdDataRe, "items", items);
		
		for(i=0; i<arraySize; i++){
			object = cJSON_GetArrayItem(logIds, i);
			logId = object->valueint;
			DEBUG_LOG(DEBUG_MODULE_COAP,DLL_INFO,"logId %d", logId);
			config.logid = logId;
			ApsDaemonLogIdParaGet(&config);
			object = cJSON_CreateObject();
			if(NULL == object){
				MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapLogConfig create <object> object error\r\n");
				ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
				cJSON_Delete(cmdDataRe);
				return OPP_FAILURE;
			}	
			cJSON_AddItemToArray(items, object);
			cJSON_AddItemToObject(object, "logId", cJSON_CreateNumber(config.logid));
			cJSON_AddItemToObject(object, "isLogReportEnable", cJSON_CreateNumber(config.isLogReport));
			cJSON_AddItemToObject(object, "isLogEnable", cJSON_CreateNumber(config.isLogSave));
		}		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGCONFIG_CMDID,reqId,0,0,NULL,cmdDataRe,mid);			
	}
	
	
	return OPP_SUCCESS;
}

int ApsCoapLogPeriod(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *object, *cmdData, *cmdDataRe;
	int reqId = 0;
	LOG_CONTROL_T config;
	int ret;

	APS_COAP_NULL_POINT_CHECK(json);
	
	object = cJSON_GetObjectItem(json,"reqId");
	if(NULL == object){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGPERIOD_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); 	
		return OPP_FAILURE;
	}
	reqId = object->valueint;
	
	object = cJSON_GetObjectItem(json, "op");
	if(NULL == object){
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGPERIOD_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); 			
		return OPP_FAILURE;
	}
	if(0 == strcmp(object->valuestring, "W")){		
		cmdData = cJSON_GetObjectItem(json,"cmdData");
		if(NULL == cmdData){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGPERIOD_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); 			
			return OPP_FAILURE;
		}
		object = cJSON_GetObjectItem(cmdData, "period");
		if(NULL == object){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGPERIOD_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);			
			return OPP_FAILURE;
		}
		if(cJSON_Number != object->type){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGPERIOD_CMDID,0,0,JSON_TYPE_NUMBER_ERR,JSON_TYPE_NUMBER_ERR_DESC,NULL,mid);			
			return OPP_FAILURE;
		}
		config.period = object->valueint;
		config.dir = LOG_DIR_NB;
		ret = ApsDaemonLogControlParaSet(&config);
		if(OPP_SUCCESS != ret){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGPERIOD_CMDID,reqId,0,LOG_PERIOD_WRITE_ERR,LOG_PERIOD_WRITE_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGPERIOD_CMDID,reqId,0,0,NULL,NULL,mid);
	}
	else if(0 == strcmp(object->valuestring, "R")){		
		cmdDataRe = cJSON_CreateObject();
		if(NULL == cmdDataRe){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapLogPeriod create <cmdDataRe> object error\r\n");
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGPERIOD_CMDID,reqId,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		ApsDaemonLogControlParaGet(&config);
		cJSON_AddItemToObject(cmdDataRe, "period", cJSON_CreateNumber(config.period));
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGPERIOD_CMDID,reqId,0,0,NULL,cmdDataRe,mid);
	}
	else{
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOGPERIOD_CMDID,reqId,0,LOGPERIOD_OP_ERR,LOGPERIOD_OP_ERR_DESC,NULL,mid);
	}

	return OPP_SUCCESS;
}

int ApsCoapNumberRang(int start, int end, char * str)
{
	if(NULL == str){
		return OPP_FAILURE;
	}

	sprintf(str, "%d-%d", start, end);

	return OPP_SUCCESS;
}

int ApsCoapLog(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, cJSON *json, U32 mid)
{
	cJSON *object, *cmdData, *logSaveIdArray;
	int reqId = 0, arraySize = 0, i = 0,j = 0;
	char * str = NULL;
	int start, end, num;
	int ret;
	ST_LOG_QUERY_REPORT qReport;
	unsigned char zeroInfo[MSG_INFO_SIZE] ={0};
	
	APS_COAP_NULL_POINT_CHECK(json);
	
	object = cJSON_GetObjectItem(json,"reqId");
	if(NULL == object){		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);		
		return OPP_FAILURE;
	}
	reqId = object->valueint;
	
	object = cJSON_GetObjectItem(json, "op");
	if(NULL == object){		
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);		
		return OPP_FAILURE;
	}

	if(0 == strcmp(object->valuestring, "R")){
		cmdData = cJSON_GetObjectItem(json,"cmdData");
		if(NULL == cmdData){		
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);		
			return OPP_FAILURE;
		}
		logSaveIdArray = cJSON_GetObjectItem(cmdData, "logSaveId");
		if(NULL == logSaveIdArray){		
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOG_CMDID,0,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid);		
			return OPP_FAILURE;
		}
		arraySize = cJSON_GetArraySize(logSaveIdArray);
		if(arraySize <= 0){
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOG_CMDID,reqId,0,LOG_READ_ERR,LOG_READ_ERR_DESC,NULL,mid);
			return OPP_FAILURE;
		}
		for(i=0; i<arraySize; i++){
			object = cJSON_GetArrayItem(logSaveIdArray,i);		
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"~ApsCoapLog str %s~~~~\r\n", object->valuestring);
			str = strstr(object->valuestring, "-");
			if(str){
				sscanf(object->valuestring,"%d-%d",&start,&end);
				if(start > end){
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOG_CMDID,reqId,0,LOG_READ_ERR,LOG_READ_ERR_DESC,NULL,mid);
					return OPP_FAILURE;
				}
				//	
				//allItems = end>=start?end-start+1:start-end+1;
				//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"~ApsCoapLog %d,%d allItems %d~~~~\r\n", start,end,allItems);
				for(j=start;j<=end;j++){
					memset(&qReport,0,sizeof(qReport));
					qReport.chl = dstChl;
					if(dstInfo && memcmp(dstInfo,zeroInfo,sizeof(zeroInfo)))
						memcpy(qReport.dstInfo,dstInfo,sizeof(qReport.dstInfo));
					qReport.logSaveId = j;
					qReport.reqId = reqId;
					ret = pushQueue(&queue_qlog,(unsigned char*)&qReport,0);
					if(OPP_SUCCESS != ret){
						DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"~ApsCoapLog pushQueue ret %d~~~~\r\n", ret);
					}
				}
			}
			else{
				num = atoi(object->valuestring);
				memset(&qReport,0,sizeof(qReport));
				qReport.chl = dstChl;
				if(dstInfo && memcmp(dstInfo,zeroInfo,sizeof(zeroInfo)))
					memcpy(qReport.dstInfo,dstInfo,sizeof(qReport.dstInfo));
				qReport.logSaveId = num;
				qReport.reqId = reqId;
				ret = pushQueue(&queue_qlog,(unsigned char*)&qReport,0);
				if(OPP_SUCCESS != ret){
					DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"~ApsCoapLog pushQueue ret %d~~~~\r\n", ret);
				}
			}	
		}
	}else{
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOG_CMDID,reqId,0,LOG_OP_ERR,LOG_OP_ERR_DESC,NULL,mid);
		return OPP_FAILURE;
	}
	ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,LOG_SERVICE,LOG_CMDID,reqId,0,0,NULL,NULL,mid); 
	return OPP_SUCCESS;
}
int ApsCoapLogProc(U8 dstChl, unsigned char *dstInfo, U8 isNeedRsp, char * text, U32 mid)
{
	char *out;
	cJSON *json, *object;
	int reqId = 0;
	
	json=cJSON_Parse(text);
	if (!json) {
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"Error before: [%s]\n",cJSON_GetErrorPtr());
		ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,0,LOG_SERVICE,0,JSON_FORMAT_ERR,JSON_FORMAT_ERR_DESC,NULL,mid);
		return OPP_FAILURE;		
	}else
	{
		//debug
		out=cJSON_Print(json);
		if(NULL == out){
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,0,LOG_SERVICE,0,MEMORY_ERR,MEMORY_ERR_DESC,NULL,mid);
			cJSON_Delete(json);
			return OPP_FAILURE;
		}
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"%s\n",out);
		myfree(out);
		
		APS_COAPS_JSON_FORMAT_ERROR_WITH_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,0,LOG_SERVICE,json,"reqId",mid);
		APS_COAPS_JSON_TYPE_ERROR_WITH_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,0,LOG_SERVICE,cJSON_GetObjectItem(json, "reqId"),cJSON_Number,mid);
		reqId = cJSON_GetObjectItem(json, "reqId")->valueint;
		if(reqId <= 0){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"\r\n json id is 0");
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,LOG_SERVICE,0,JSON_ID_ZERO_ERR,JSON_ID_ZERO_ERR_DESC,NULL,mid);
			cJSON_Delete(json);
			return OPP_FAILURE;
		}
		
		APS_COAPS_JSON_FORMAT_ERROR_WITH_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,0,LOG_SERVICE,json,"cmdId",mid);
		APS_COAPS_JSON_TYPE_ERROR_WITH_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,0,LOG_SERVICE,cJSON_GetObjectItem(json,"cmdId"),cJSON_String,mid);
		object = cJSON_GetObjectItem(json,"cmdId");
		if(object != NULL){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n%s\r\n", cJSON_GetObjectItem(json, "cmdId")->valuestring);		
			if(0 == strcmp(LOGSTATUS_CMDID, object->valuestring)){
				ApsCoapLogStatus(dstChl, dstInfo, isNeedRsp,json,mid);
			}
			else if(0 == strcmp(LOG_CMDID, object->valuestring)){
				ApsCoapLog(dstChl, dstInfo, isNeedRsp, json, mid);
			}
			/*not support log config now*/
			/*else if(0 == strcmp(LOGCONFIG_CMDID, object->valuestring)){
				ApsCoapLogConfig(dstChl,isNeedRsp,json,mid);
			}*/
			else if(0 == strcmp(LOGPERIOD_CMDID, object->valuestring)){
				ApsCoapLogPeriod(dstChl,dstInfo,isNeedRsp,json,mid);
			}else{
				ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,0,PROP_SERVICE,0,UNKNOW_CMDID_ERR,UNKNOW_CMDID_ERR_DESC,NULL,mid);
			}
		}
		/*release json root node*/
		cJSON_Delete(json);
	}

	return OPP_SUCCESS;
}

void ApsCoapMsgDispatch1(unsigned char srcChl, unsigned char * srcInfo, unsigned char isRsp, unsigned char isNeedRsp, ST_DISP_PARA *para, unsigned char *pucVal, int nLen, U32 mid)
{
	ST_COAP_ENTRY *pstCoapEntry = NULL;	
	unsigned char dstChl;
	unsigned char *dstInfo;
	
	if(NULL == pucVal)
		return;

	dstChl = srcChl;
	dstInfo = srcInfo;
	//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n----ApsCoapMsgDispatch1 dstCh1 %d-----\r\n", dstChl);	
	//req
	//if(0x02 == isRsp)
	if(0x00 == isRsp)
	{
		//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n-----------req--------------------\r\n");	
		if(PROP_SERVICE == para->u.req.serviceId){
			ApsCoapPropProc(dstChl, dstInfo, isNeedRsp, (char *)pucVal, mid);
		}
		else if(FUNC_SERVICE == para->u.req.serviceId){
			ApsCoapFuncProc(dstChl, dstInfo, isNeedRsp, (char *)pucVal, mid);
		}
		else if(LOG_SERVICE == para->u.req.serviceId){
			ApsCoapLogProc(dstChl, dstInfo, isNeedRsp, (char *)pucVal, mid);
		}
	}
	//rsp
	//else if(0x03 == isRsp)
	else if(0x01 == isRsp)
	{
		//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"-----------rsp--------------------\r\n");
		if(/*(srcChl == CHL_NB) &&*/ (ApsCoapGetLogReportMid() == mid)){
			ApsCoapSetLogReportStatus(LOG_SEND_INIT,0);
		}
		if(g_isSupportRetry){
			MUTEX_LOCK(m_ulCoapMutex,MUTEX_WAIT_ALWAYS);
			LIST_FOREACH(pstCoapEntry, &list_coap, elements)
			{
				//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"g_usLastLogMid %d mid: %x\r\n", g_usLastLogMid, mid);
				
				if((REPORT_T == pstCoapEntry->type) && (mid == pstCoapEntry->mid))
				{
					//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"remove entry mid: %x\r\n", mid);
					free(pstCoapEntry->data);
					LIST_REMOVE(pstCoapEntry, elements);
					free(pstCoapEntry);				
					//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"remove entry mid: %x suc\r\n", mid);
					break;
				}
			}
			MUTEX_UNLOCK(m_ulCoapMutex);
		}
	}
}

unsigned int ApsCoapRandomPeriod()
{
	return rand()%1800000;
}

unsigned int ApsCoapStartHeartbeat()
{
	MUTEX_LOCK(g_stHeartPara.mutex, 100);
	g_stHeartPara.udwHeartTick = OppTickCountGet();
	g_stHeartPara.udwRandHeartPeriod = ApsCoapRandomPeriod();
	MUTEX_UNLOCK(g_stHeartPara.mutex);
	return 0;
}

int ApsCoapOceanconHeart(U8 dstChl)
{
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
    cJSON * root;
	cJSON * cmdData, *comObj;
	U16 outLength = 0;
	int reqId = 0;
	U8 lampSwitch = 0;
	U16 usBri = 0;
	long double lat, lng;
	ST_OPP_LAMP_CURR_ELECTRIC_INFO stElecInfo;
	U32 rTime,hTime;
	char buffer[64] = {0};
	ST_NEUL_DEV *pstNeulDev = NULL;
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ApsCoapOceanconHeart\r\n");
	root =  cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapOceanconHeart create <root> object error\r\n");
		return OPP_FAILURE;
	}	
	
	cJSON_AddItemToObject(root, "reqId", cJSON_CreateNumber(reqId = OppCoapReqIdGen()));
    cJSON_AddItemToObject(root, "cmdId", cJSON_CreateString(HEARTBEAT_CMDID));
	cmdData =  cJSON_CreateObject();
	if(NULL == cmdData){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapOceanconHeart create <cmdData> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(root, "cmdData", cmdData);
	OppLampCtrlGetSwitch(0, &lampSwitch);
    cJSON_AddItemToObject(cmdData, "switch", cJSON_CreateNumber(lampSwitch));
	OppLampCtrlGetBri(0,&usBri);
    cJSON_AddItemToObject(cmdData, "bri", cJSON_CreateNumber(usBri));

	//loc	
	/*LocGetLat(&lat);
	LocGetLng(&lng);
	//ftoa(buffer,lat,6);
	snprintf(buffer,64,"%Lf",lat);
	comObj = cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapOceanconHeart create loc <comObj> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(cmdData, "loc", comObj);
	cJSON_AddItemToObject(comObj, "lat", cJSON_CreateString(buffer));
	//ftoa(buffer,lng,6);	
	snprintf(buffer,64,"%Lf",lng);
	cJSON_AddItemToObject(comObj, "lng", cJSON_CreateString(buffer));
	*/
	//elec
	ElecGetElectricInfo(&stElecInfo);
	comObj =  cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapOceanconHeart create elec <comObj> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}	
	cJSON_AddItemToObject(cmdData, "elec", comObj);
	cJSON_AddItemToObject(comObj, "current", cJSON_CreateNumber(stElecInfo.current));							
	cJSON_AddItemToObject(comObj, "voltage", cJSON_CreateNumber(stElecInfo.voltage));							
	cJSON_AddItemToObject(comObj, "factor", cJSON_CreateNumber(stElecInfo.factor));							
	cJSON_AddItemToObject(comObj, "power", cJSON_CreateNumber(stElecInfo.power));							
	//ecInfo
	comObj =  cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapOceanconHeart create ecInfo <comObj> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}		
	cJSON_AddItemToObject(cmdData, "ecInfo", comObj);
	cJSON_AddItemToObject(comObj, "EC", cJSON_CreateNumber(stElecInfo.consumption));							
	cJSON_AddItemToObject(comObj, "HEC", cJSON_CreateNumber(stElecInfo.hisConsumption));	
	//runtime
	OppLampCtrlGetRtime(0,&rTime);
	OppLampCtrlGetHtime(0,&hTime);
	comObj =  cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapOceanconHeart create runtime <comObj> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}		
	cJSON_AddItemToObject(cmdData, "runtime",comObj);
	cJSON_AddItemToObject(comObj, "hTime", cJSON_CreateNumber(hTime)); 						
	cJSON_AddItemToObject(comObj, "rTime", cJSON_CreateNumber(rTime));
	//nb signal
	ret = sendEvent(UESTATE_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(UESTATE_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 == ret){
		pstNeulDev = (ST_NEUL_DEV *)rArgv[0];
		comObj =  cJSON_CreateObject();
		if(NULL == comObj){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapOceanconHeart create nbSignalCore <comObj> object error\r\n");
			cJSON_Delete(root);
			ARGC_FREE(rArgc,rArgv);
			return OPP_FAILURE;
		}			
		cJSON_AddItemToObject(cmdData, "nbSignalCore", comObj);
		cJSON_AddItemToObject(comObj, "rsrp", cJSON_CreateNumber(pstNeulDev->rsrp));							
		cJSON_AddItemToObject(comObj, "snr", cJSON_CreateNumber(pstNeulDev->snr));
		cJSON_AddItemToObject(comObj, "cellid", cJSON_CreateNumber(pstNeulDev->cellId));	
	}
	ARGC_FREE(rArgc,rArgv);
	
	JsonCompres(root, (char *)&coapmsgTx, &outLength);
	OppCoapSend(dstChl, NULL, CMD_MSG, REPORT_T, FUNC_SERVICE,0,coapmsgTx, outLength, OppCoapMidGet());	
	OppCoapMidIncrease();
	return OPP_SUCCESS;
}
/*if dstChl==wifi, dstInfo must be specified else no need to specify*/
int ApsCoapOceanconHeartOnline(U8 dstChl, unsigned char *dstInfo)
{
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
    cJSON * root;
	U16 outLength = 0;
	int reqId;
	ST_RHB_ENTRY *pstRhb;
	U8 zeroInfo[0] = {0};
	
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ApsCoapOceanconHeartOnline\r\n");
	root =  cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapOceanconHeartOnline create <root> object error\r\n");
		return OPP_FAILURE;
	}	
	cJSON_AddItemToObject(root, "reqId", cJSON_CreateNumber(reqId = OppCoapReqIdGen()));
    cJSON_AddItemToObject(root, "cmdId", cJSON_CreateString(HEARTBEAT_CMDID));
	JsonCompres(root, (char *)&coapmsgTx, &outLength);
	OppCoapSend(dstChl, dstInfo, CMD_MSG, REPORT_T, FUNC_SERVICE,0,coapmsgTx, outLength, m_usMid);	
	pstRhb = (ST_RHB_ENTRY *)malloc(sizeof(ST_RHB_ENTRY));
	if(NULL == pstRhb){
		return OPP_FAILURE;
	}
	pstRhb->chl = dstChl;
	if(dstInfo && memcmp(dstInfo,zeroInfo,sizeof(zeroInfo)))
		memcpy(pstRhb->info,dstInfo,sizeof(pstRhb->info));
	pstRhb->reqId = reqId;
	pstRhb->times = DEFAULT_RHB;
	pstRhb->mid = m_usMid;
	pstRhb->tick = OppTickCountGet();
	MUTEX_LOCK(m_ulCoapRHB,100);
	LIST_INSERT_HEAD(&list_rhb,pstRhb,elements);
	MUTEX_UNLOCK(m_ulCoapRHB);
	OppCoapMidIncrease();

	return OPP_SUCCESS;
}


int ApsCoapOceanconRetryHeartOnline(U8 dstChl, unsigned char *dstInfo, int reqId, unsigned short mid)
{
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
    cJSON * root;
	U16 outLength = 0;

	root =  cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapOceanconRetryHeartOnline create <root> object error\r\n");
		return OPP_FAILURE;
	}	
	
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ApsCoapOceanconRetryHeartOnline\r\n");
	cJSON_AddItemToObject(root, "reqId", cJSON_CreateNumber(reqId));
    cJSON_AddItemToObject(root, "cmdId", cJSON_CreateString(HEARTBEAT_CMDID));
	JsonCompres(root, (char *)&coapmsgTx, &outLength);
	OppCoapSend(dstChl, dstInfo, CMD_MSG, REPORT_T, FUNC_SERVICE,0,coapmsgTx, outLength, mid);	
	
	return OPP_SUCCESS;
}

/*
{
"reqId": 20,
"cmdId": "WorkPlanReport"
"cmdData": {
	"planId":1,
	"type":0,
	"jobId":1
	"switch":1,
	"bri":1000
	}
}
*/
/*if dstChl==WIFI, need dstInfo, else no need dstInfo*/
int ApsCoapWorkPlanReport(U8 dstChl, unsigned char *dstInfo, ST_WORKPLAN_REPORT *stWorkPlanR)
{
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
	cJSON * root;
	cJSON * cmdDataRe;
	U16 outLength = 0;

	if(NULL == stWorkPlanR){
		return OPP_FAILURE;
	}

	root =  cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapWorkPlanReport create <root> object error\r\n");
		return OPP_FAILURE;
	}	
	
	cJSON_AddItemToObject(root, "cmdId", cJSON_CreateString(WORKPLANREPORT_CMDID));
	cmdDataRe =  cJSON_CreateObject();
	if(NULL == cmdDataRe){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapWorkPlanReport create <cmdDataRe> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}		
	cJSON_AddItemToObject(root, "cmdData", cmdDataRe);
	cJSON_AddItemToObject(cmdDataRe, "planId", cJSON_CreateNumber(stWorkPlanR->planId));
	cJSON_AddItemToObject(cmdDataRe, "type", cJSON_CreateNumber(stWorkPlanR->type));
	cJSON_AddItemToObject(cmdDataRe, "jobId", cJSON_CreateNumber(stWorkPlanR->jobId));
	cJSON_AddItemToObject(cmdDataRe, "switch", cJSON_CreateNumber(stWorkPlanR->sw));
	cJSON_AddItemToObject(cmdDataRe, "bri", cJSON_CreateNumber(stWorkPlanR->bri));
	
	JsonCompres(root, (char *)&coapmsgTx, &outLength);
	OppCoapSend(dstChl, dstInfo, CMD_MSG, REPORT_T, FUNC_SERVICE,0,coapmsgTx, outLength, OppCoapMidGet());	
	OppCoapMidIncrease();

	return OPP_SUCCESS;
}

int ApsCoapGetTime(U8 dstChl, unsigned char * dstInfo)
{
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
    cJSON * root;
	U16 outLength = 0;

	root =  cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"OppCoapPackRuntimeState create <root> object error\r\n");
		return OPP_FAILURE;
	}	
		
    cJSON_AddItemToObject(root, "cmdId", cJSON_CreateString(GETTIME_CMDID));
	JsonCompres(root, (char *)&coapmsgTx, &outLength);
	OppCoapSend(dstChl, dstInfo,CMD_MSG, REPORT_T, FUNC_SERVICE,0,coapmsgTx, outLength, OppCoapMidGet());	
	OppCoapMidIncrease();

	return OPP_SUCCESS;
}

void ApsCoapOceanconProcess(EN_MSG_CHL chl,const MSG_ITEM_T *item)
{
	ST_COMDIR_RSQ_HDR *pstRsp;
	ST_COMDIR_REQ_HDR *pstReq;	
	ST_LOC_HDR *pstLoc;
	ST_DISP_PARA stDispPara;
	U32 mid = 0; /*coap mid 2bytes, wifi 4bytes*/
	int nLen = 0,jLen = 0;
	unsigned char isRsp = 0, isNeedRsp = 1;
	static char buffer[512] = { 0 };
	static unsigned char srcInfo[MSG_INFO_SIZE] = {0};
	
	U8 srcChl = chl;
	U8 * data = item->data;
	U16 len = item->len;
	// ip,port

	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"Enter ApsCoapOceanconProcess\r\n", len);
	RoadLamp_Dump((char *)data, len);
	//MUTEX_LOCK(m_ulCoapMutex, MUTEX_WAIT_ALWAYS);	
	memset(buffer, 0, sizeof(buffer));
	//OppCoapMsgDispatch((unsigned char *)data, len);

	if(len > JSON_R_MAX_LEN){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\nreq packet is big %d\r\n", len);
		ApsCoapRsp(srcChl,item->info,CMD_MSG,isNeedRsp,0,data[4],0,NBIOT_RECV_BIG_ERR,NBIOT_RECV_BIG_ERR_DESC,NULL,mid);
		return;
	}
	
	if(CHL_NB == srcChl){
		MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
		g_stCoapPkt.g_iCoapNbRx++;
		MUTEX_UNLOCK(g_stCoapPkt.mutex);
		
		if(data[0] == 0XFF && data[1] == 0XFE){	
			printf("\r\n-----------ota msg--------------------\r\n");	
			for(int i = 0;i < len;i++)
				printf("%02X", data[i]);
			printf("\r\n");
			MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
			g_stCoapPkt.g_iCoapNbOtaRx++;
			MUTEX_UNLOCK(g_stCoapPkt.mutex);
			NbOtaMsgProcess(data,len);
			return;
		}

		if(CLOUD_REQ_MSG == data[2]){
			MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
			g_stCoapPkt.g_iCoapNbRxReq++;
			MUTEX_UNLOCK(g_stCoapPkt.mutex);
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n-----------req--------------------\r\n");	
			pstReq = (ST_COMDIR_REQ_HDR *)data;		
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n-----------req pstReq->length %d--------------------\r\n", OPP_HTONS(pstReq->length));	
			nLen += sizeof(ST_COMDIR_REQ_HDR);
			if(OPP_HTONS(pstReq->length)){
				memcpy(buffer, (char *)&data[nLen], OPP_HTONS(pstReq->length));
				nLen += OPP_HTONS(pstReq->length);
			}
			mid = data[nLen]<<8|data[nLen+1];
			isRsp = 0;
			jLen = OPP_HTONS(pstReq->length);
			memcpy(&stDispPara.u.req, pstReq, sizeof(ST_COMDIR_REQ_HDR));
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n---------------mid: 0x%x\r\n", mid);
		}
		else if(CLOUD_RSP_MSG == data[2]){
			MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
			g_stCoapPkt.g_iCoapNbRxRsp++;
			MUTEX_UNLOCK(g_stCoapPkt.mutex);
			
			pstRsp = (ST_COMDIR_RSQ_HDR *)data;
			nLen += sizeof(ST_COMDIR_RSQ_HDR);
			//strncpy(text, &pucVal[len], pstRsp->length);
			nLen += OPP_HTONS(pstRsp->length);
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n-----------req pstRsp->length %d--------------------\r\n", OPP_HTONS(pstRsp->length));	
			
			if(OPP_HTONS(pstRsp->length)){
				memcpy(buffer, (char *)&data[nLen], OPP_HTONS(pstRsp->length));
				nLen += OPP_HTONS(pstRsp->length);
			}
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n-----------nLen %d--------------------\r\n", nLen);	
			mid = data[nLen]<<8|data[nLen+1];
			isRsp = 1;
			jLen = OPP_HTONS(pstRsp->length);
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n-----------mid %d--------------------\r\n", mid);	
			memcpy(&stDispPara.u.rsp, pstRsp, sizeof(ST_COMDIR_RSQ_HDR));
		}
		else if(DEVICE_REQ_MSG == data[2]){			
			MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
			g_stCoapPkt.g_iCoapNbDevReq++;			
			MUTEX_UNLOCK(g_stCoapPkt.mutex);
		}
		else if(DEVICE_REQ_MSG == data[2]){
			MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
			g_stCoapPkt.g_iCoapNbDevRsp++;			
			MUTEX_UNLOCK(g_stCoapPkt.mutex);
		}
		else{
			MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
			g_stCoapPkt.g_iCoapNbUnknow++;			
			MUTEX_UNLOCK(g_stCoapPkt.mutex);
			for(int i = 0;i < len;i++)
				printf("%02X", data[i]);
			printf("\r\n");
		}
	}
	else{
		/*for debug wangtao*/
		if(1){
			pstLoc = (ST_LOC_HDR *)data;
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n pstLoc->hdr %04x---------\r\n", pstLoc->hdr);
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n pstLoc->dataLen %d---------\r\n", pstLoc->dataLen);
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n OPP_HTONS(pstLoc->dataLen) %d---------\r\n", OPP_NTOHS(pstLoc->dataLen));
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n pstLoc->conType %d---------\r\n", pstLoc->conType);
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n pstLoc->isRsp %d---------\r\n", pstLoc->isRsp);
			//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n pstLoc->seqNum %d---------\r\n", OPP_NTOHL(pstLoc->seqNum));
			if(pstLoc->hdr == 0xbebebebe && (data[len-1] == 0xed && data[len-2] == 0xed)){
				/** svr->dev request**/
				isRsp = 0;
				isNeedRsp = pstLoc->isRsp;
				nLen += sizeof(ST_LOC_HDR);
				mid = OPP_NTOHL(pstLoc->seqNum);
				//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n OPP_HTONS(pstLoc->dataLen) %d---------\r\n", OPP_HTONS(pstLoc->dataLen));				
				if(OPP_NTOHS(pstLoc->dataLen) > 0){
					//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n nLen %d---------\r\n", nLen);
					memcpy(buffer, (char *)&data[nLen], OPP_NTOHS(pstLoc->dataLen)-6);/*seqNum:4+isRsp:1+contenType:1*/
					jLen = OPP_NTOHS(pstLoc->dataLen);
				}
				if(0x01 == pstLoc->conType)
					stDispPara.u.req.serviceId = PROP_SERVICE;
				else if(0x02 == pstLoc->conType)
					stDispPara.u.req.serviceId = FUNC_SERVICE;
				else if(0x03 == pstLoc->conType)
					stDispPara.u.req.serviceId = LOG_SERVICE;
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"\r\n content %s---------\r\n", buffer);
			//RoadLamp_Dump(buffer,OPP_NTOHS(pstLoc->dataLen));
		}else{
			memcpy(buffer, (char *)data, len);
			isRsp = 0;
			jLen = len;
			stDispPara.u.req.serviceId = PROP_SERVICE;
			mid = 0;
		}
		//printf("coap recv ip:%08x, port:%d\r\n", *(unsigned int *)item->info, *(unsigned short *)&item->info[4]);
		memcpy(srcInfo,item->info, sizeof(srcInfo));
	}
	
	ApsCoapMsgDispatch1(srcChl, srcInfo, isRsp, isNeedRsp,&stDispPara, (unsigned char *)buffer, jLen, mid);
}

int ApsCoapNbRxPktGet()
{
	int rxNum;
	
	MUTEX_LOCK(g_stCoapPkt.mutex,MUTEX_WAIT_ALWAYS);
	rxNum = g_stCoapPkt.g_iCoapNbRx;
	MUTEX_UNLOCK(g_stCoapPkt.mutex);

	return rxNum;
}

int ApsCoapRxTimeout()
{
	AD_CONFIG_T config;
	static U32 tick = 0;
	static U8 init = 0;
	static U32 timeout,timeout1;
	static U32 lastRxNum;
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	RESET_REASON reason_cpu0;
	
	if(!init){
		ApsDaemonActionItemParaGetCustom(CUSTOM_REPORT,HEARTBEAT_PERIOD_IDX,&config);
		timeout = config.reportIfPara * 1000 * 2 + COAP_RX_DELTA + COAP_RX_DELTA1; //ms
		if(timeout > COAP_RX_HIGH_LIMIT_TO){
			timeout = COAP_RX_HIGH_LIMIT_TO;
		}else if(timeout < COAP_RX_LOW_LIMIT_TO){
			timeout = COAP_RX_LOW_LIMIT_TO;
		}
		//2*timeout+delta or disoffset+delta max
		//if(timeout < NeulBc28DisOffsetSecondGet())
		//	timeout = NeulBc28DisOffsetSecondGet() + COAP_RX_DELTA;
		reason_cpu0 = rtc_get_reset_reason(0);		
		if(isResetReasonPowerOn(reason_cpu0)){
			timeout1 = NeulBc28DisOffsetSecondGet()*1000 + COAP_RX_DELTA1;   /*attach delta and online response time*/
			if(timeout1 > timeout)
				timeout = timeout1;
		}
		init = 1;
	}

	if(0 == tick){
		tick = OppTickCountGet();
		lastRxNum = ApsCoapNbRxPktGet();
	}else if(OppTickCountGet() - tick > timeout){
		if(ApsCoapNbRxPktGet() == lastRxNum){
			printf("*********system reboot by coap rx timeout **********************\r\n");
			ret = sendEvent(CSEARFCN_EVENT,RISE_STATE,sArgc,sArgv);
			ret = recvEvent(CSEARFCN_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
			if(0 != ret){
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"recvEvent CSEARFCN_EVENT ret fail%d\r\n", ret);
			}
			ARGC_FREE(rArgc,rArgv);
			sysModeDecreaseSet();
			ApsCoapWriteExepInfo(COAP_RST);
			ApsCoapDoReboot();
			OS_REBOOT();
		}
		tick = 0;
	}

	return OPP_SUCCESS;
}

int ApsCoapOnlineTimeout()
{
	ST_RHB_ENTRY *pstRhb = NULL;

	MUTEX_LOCK(m_ulCoapRHB,100);
	LIST_FOREACH(pstRhb, &list_rhb, elements){
		if(OppTickCountGet() - pstRhb->tick >= ONLINE_TO){
			ApsCoapOceanconRetryHeartOnline(pstRhb->chl, pstRhb->info,pstRhb->reqId, pstRhb->mid);
				if(pstRhb->times != RETRY_ALWAYS){
				if(pstRhb->times > 0)
					pstRhb->times--;
				if(pstRhb->times == 0)
					LIST_REMOVE(pstRhb,elements);
			}
			pstRhb->tick = OppTickCountGet();
			break;
		}
	}
	MUTEX_UNLOCK(m_ulCoapRHB);
	return OPP_SUCCESS;
}

int ApsCoapRandHeartTimeout()
{
	MUTEX_LOCK(g_stHeartPara.mutex,100);
	if(g_stHeartPara.udwHeartTick > 0){
		if(OppTickCountGet() - g_stHeartPara.udwHeartTick >= g_stHeartPara.udwRandHeartPeriod){
			g_stHeartPara.udwHeartTick = OppTickCountGet();
			g_stHeartPara.udwRandHeartPeriod = ApsCoapRandomPeriod();
		}
	}
	MUTEX_UNLOCK(g_stHeartPara.mutex);
	return OPP_SUCCESS;
}

int ApsCoapDataTimeout()
{
	ST_COAP_ENTRY *pstCoapEntry = NULL;
	MSG_ITEM_T item;
	int ret;
	U8 zeroInfo[MSG_INFO_SIZE] = {0};
	
	//超时重发
	if(g_isSupportRetry){
		MUTEX_LOCK(m_ulCoapMutex, MUTEX_WAIT_ALWAYS);	
		LIST_FOREACH(pstCoapEntry,&list_coap,elements)
		{			
			/*response消息立马发送*/
			if(RSP_T == pstCoapEntry->type)
			{
				MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
				g_stCoapPkt.g_iCoapNbTxRetry++;
				g_stCoapPkt.g_iCoapNbTxRspRetry++;
				MUTEX_UNLOCK(g_stCoapPkt.mutex);

				item.type = MSG_TYPE_HUAWEI;
				item.len = pstCoapEntry->length;
				item.data = pstCoapEntry->data;
				if(memcmp(pstCoapEntry->dstInfo,zeroInfo,sizeof(pstCoapEntry->dstInfo)))
					memcpy(item.info,pstCoapEntry->dstInfo,sizeof(item.info));
				if(ALARM_MSG == pstCoapEntry->msgType)
					ret = AppMessageSend(pstCoapEntry->chl,MSG_QUEUE_TYPE_TX,&item,PRIORITY_HIGH);
				else
					ret = AppMessageSend(pstCoapEntry->chl,MSG_QUEUE_TYPE_TX,&item,PRIORITY_LOW);
				if(OPP_SUCCESS == ret)
				{	
					MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
					g_stCoapPkt.g_iCoapNbTxSuc++;
					MUTEX_UNLOCK(g_stCoapPkt.mutex);
					
					myfree(pstCoapEntry->data);
					LIST_REMOVE(pstCoapEntry,elements);
					myfree(pstCoapEntry);
					break;
				}else{
					MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
					g_stCoapPkt.g_iCoapNbTxFail++;
					MUTEX_UNLOCK(g_stCoapPkt.mutex);
				}
				pstCoapEntry->times -= 1;				
				pstCoapEntry->tick = OppTickCountGet();
			}
			else{
				/*report消息延迟发送*/
				if((OppTickCountGet() - pstCoapEntry->tick) >= COAP_RETRY_TO)
				{
					MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
					g_stCoapPkt.g_iCoapNbTxRetry++;
					g_stCoapPkt.g_iCoapNbTxReqRetry++;
					MUTEX_UNLOCK(g_stCoapPkt.mutex);

					if(pstCoapEntry->times > 0)
					{
						item.type = MSG_TYPE_HUAWEI;
						item.len = pstCoapEntry->length;
						item.data = pstCoapEntry->data;
						if(memcmp(pstCoapEntry->dstInfo,zeroInfo,sizeof(pstCoapEntry->dstInfo)))
							memcpy(item.info,pstCoapEntry->dstInfo,sizeof(item.info));
						if(ALARM_MSG == pstCoapEntry->msgType)
							ret = AppMessageSend(pstCoapEntry->chl,MSG_QUEUE_TYPE_TX,&item,PRIORITY_HIGH);
						else
							ret = AppMessageSend(pstCoapEntry->chl,MSG_QUEUE_TYPE_TX,&item,PRIORITY_LOW);
						if(OPP_SUCCESS == ret){
							MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
							g_stCoapPkt.g_iCoapNbTxSuc++;
							MUTEX_UNLOCK(g_stCoapPkt.mutex);
						}else{
							MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
							g_stCoapPkt.g_iCoapNbTxFail++;
							MUTEX_UNLOCK(g_stCoapPkt.mutex);
						}
						//if(pstCoapEntry->mid != g_usLastLogMid){
							pstCoapEntry->times -= 1;
						//}
						pstCoapEntry->tick = OppTickCountGet();
					}
				}
			}
			if(pstCoapEntry->times <= 0)
			{
				if(/*(pstCoapEntry->chl == CHL_NB) &&*/ (pstCoapEntry->mid == ApsCoapGetLogReportMid())){
					ApsCoapSetLogReportStatus(LOG_SEND_INIT,OppCoapMidGet());
				}
				myfree(pstCoapEntry->data);
				LIST_REMOVE(pstCoapEntry,elements);
				myfree(pstCoapEntry);
			}
		}
		MUTEX_UNLOCK(m_ulCoapMutex);
	}
	return OPP_SUCCESS;
}

int ApsCoapQueryLogReport()
{
	ST_LOG_QUERY_REPORT qReport;
	ST_LOG_PARA stLogPara;
	int ret = 0;
	unsigned char zeroInfo[MSG_INFO_SIZE] = {0};
	
	ret = pullQueue(&queue_qlog,(unsigned char*)&qReport,sizeof(ST_LOG_QUERY_REPORT));
	if(OPP_SUCCESS == ret){
		//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"query report logSaveId %d queue length %d\n", qReport.chl, lengthQueue(&queue_qlog));
		stLogPara.chl = qReport.chl;
		if(memcmp(qReport.dstInfo,zeroInfo,sizeof(zeroInfo)))
			memcpy(stLogPara.dstInfo,qReport.dstInfo,sizeof(qReport.dstInfo));
		stLogPara.reqId = qReport.reqId;
		stLogPara.type = QUERY_REPORT;
		stLogPara.logSaveId = qReport.logSaveId;
		stLogPara.leftItems = lengthQueue(&queue_qlog);
		ret = ApsDaemonLogQuery(qReport.logSaveId, &stLogPara.log);
		/*if(OPP_SUCCESS == ret){
			ApsCoapLogReport(&stLogPara);
		}else{
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"query report ApsDaemonLogQuery ret %d\n", ret);
		}*/
		if(OPP_SUCCESS != ret){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"query report ApsDaemonLogQuery ret %d\n", ret);
		}
		stLogPara.err = ret;
		ApsCoapLogReport(&stLogPara);
	}
	return OPP_SUCCESS;
}

int ApsCoapStatusReportTimeout()
{
	/*code review reboot notify U64*/
	if(g_iReTick > 0){
		if((OppTickCountGet() - g_iReTick) >= 1000){
			ApsCoapReport(CHL_NB,NULL);
			g_iReTick = 0;
		}
	}
	return OPP_SUCCESS;
}

int ApsCoapRebootDelay()
{
	/*code review reboot notify*/
	if(startReboot){
		if((OppTickCountGet() - g_iRebootTick) >= 10000){
			startReboot = 0;
			sysModeDecreaseSet();
			ApsCoapWriteExepInfo(CMD_RST);
			ApsCoapDoReboot();
			OS_REBOOT();
		}
	}
	return OPP_SUCCESS;
}

int ApsCoapNbiotRebootDelay()
{
	if(startAttach){
		if((OppTickCountGet() - g_iAttachTick) >= 10000){
			startAttach = 0;
			//NeulBc28SetDevState(BC28_INIT);
			NbiotHardResetInner();
		}
	}
	return OPP_SUCCESS;
}

int ApsCoapRxProtectGet(void)
{
	int enable;

	MUTEX_LOCK(g_stRxProtect.mutex,60000);
	if(RX_PROTECT_DISABLE_H == g_stRxProtect.enableH && RX_PROTECT_DISABLE_L == g_stRxProtect.enableL){
		enable = RX_PROTECT_DISABLE;
	}else{
		enable = RX_PROTECT_ENABLE;
	}
	MUTEX_UNLOCK(g_stRxProtect.mutex);
	return enable;
}

int ApsCoapRxProtectSet(int enable)
{
	MUTEX_LOCK(g_stRxProtect.mutex,MUTEX_WAIT_ALWAYS);
	if(RX_PROTECT_DISABLE == enable){
		g_stRxProtect.enableH = RX_PROTECT_DISABLE_H;
		g_stRxProtect.enableL = RX_PROTECT_DISABLE_L;
	}else{
		g_stRxProtect.enableH = RX_PROTECT_ENABLE_H;
		g_stRxProtect.enableL = RX_PROTECT_ENABLE_L;
	}
	MUTEX_UNLOCK(g_stRxProtect.mutex);
	return OPP_SUCCESS;
}

int ApsCoapHeartDisParaGet(ST_DIS_HEART_SAVE_PARA *pstDisHeartPara)
{
	MUTEX_LOCK(g_stHeartDisPara.mutex,MUTEX_WAIT_ALWAYS);
	pstDisHeartPara->udwDiscreteWin = g_stHeartDisPara.udwDiscreteWin;
	pstDisHeartPara->udwDiscreteInterval = g_stHeartDisPara.udwDiscreteInterval;
	MUTEX_UNLOCK(g_stHeartDisPara.mutex);
	return 0;
}

int ApsCoapHeartDisParaSet(ST_DIS_HEART_SAVE_PARA *pstDisHeartPara)
{
	MUTEX_LOCK(g_stHeartDisPara.mutex,MUTEX_WAIT_ALWAYS);
	g_stHeartDisPara.udwDiscreteWin = pstDisHeartPara->udwDiscreteWin;
	g_stHeartDisPara.udwDiscreteInterval = pstDisHeartPara->udwDiscreteInterval;
	MUTEX_UNLOCK(g_stHeartDisPara.mutex);
	return 0;
}

int ApsCoapHeartDisOffsetSecondGet()
{
	int ret;
	unsigned char mac[6] = {0};
	U32 disOffset;
	ST_DIS_HEART_SAVE_PARA stDisHeartPara;
	
	esp_wifi_get_mac(WIFI_IF_STA, mac);
	ApsCoapHeartDisParaGet(&stDisHeartPara);
	ret = OppGetDisOffsetSecond(stDisHeartPara.udwDiscreteWin,stDisHeartPara.udwDiscreteInterval,mac[4],mac[5],&disOffset);
	if(OPP_SUCCESS != ret){
		disOffset = DIS_WINDOW_MAX_SECOND;
	}
	if(disOffset > DIS_WINDOW_MAX_SECOND){
		disOffset = DIS_WINDOW_MAX_SECOND;
	}

	return disOffset;
}

U32 ApsCoapHeartDisTick(void)
{
	static U8 first = 1;
	static U32 tick = 0;
	static U32 offset = 0;
	RESET_REASON reason_cpu0;

	reason_cpu0 = rtc_get_reset_reason(0);

	if(isResetReasonPowerOn(reason_cpu0)){
		if(first){
			offset = ApsCoapHeartDisOffsetSecondGet()*1000;
			first = 0;
		}
		if(OppTickCountGet() > offset)
			return (OppTickCountGet()-offset);
		else
			return 0;
	}else{
		return OppTickCountGet();
	}
}

int ApsCoapHeartDisParaGetFromFlash(ST_DIS_HEART_SAVE_PARA *pstDisHeartPara)
{
	int ret;
	unsigned int len = sizeof(ST_DIS_HEART_SAVE_PARA);
	
	ret = AppParaRead(APS_PARA_MODULE_APS_COAP, DIS_HEART_ID, pstDisHeartPara, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "NeulBc28GetDiscreteParaFromFlash fail ret %d\r\n",ret);
		return OPP_FAILURE;
	}
	return OPP_SUCCESS;
}

int ApsCoapHeartDisParaSetToFlash(ST_DIS_HEART_SAVE_PARA *pstDisHeartPara)
{
	int ret;

	ret = AppParaWrite(APS_PARA_MODULE_APS_COAP, DIS_HEART_ID, pstDisHeartPara, sizeof(ST_DISCRETE_SAVE_PARA));
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "NeulBc28SetDiscreteParaToFlash fail ret %d\r\n",ret);
		return ret;
	}
	return OPP_SUCCESS;
}

int ApsCoapRestoreFactory(void)
{
	int ret;
	ret = ApsCoapHeartDisParaSetToFlash(&g_stHeartDisParaDefault);
	if(OPP_SUCCESS != ret){
		return OPP_FAILURE;
	}
	ApsCoapHeartDisParaSet(&g_stHeartDisParaDefault);
	return OPP_SUCCESS;
}

int ApsCoapWifiConDelay(void)
{
	wifi_config_t wifi_config;
	ST_WIFI_CONFIG stWifiConfig;
	
	if(g_iWifiConTick > 0){
		if((OppTickCountGet() - g_iWifiConTick) >= 10000){	
			memset(&wifi_config, 0, sizeof(wifi_config_t));
			ApsWifiConfigRead(&stWifiConfig);
			strcpy((char *)wifi_config.sta.ssid, (char *)stWifiConfig.ssid);
			strcpy((char *)wifi_config.sta.password, (char *)stWifiConfig.password);
			ApsWifiStopStart(&wifi_config);
			g_iWifiConTick = 0;
		}
	}
	return 0;
}
void ApsCoapLoop(void)
{
	ApsCoapDataTimeout();
	ApsCoapQueryLogReport();
	ApsCoapStatusReportTimeout();
	ApsCoapRebootDelay();
	ApsCoapNbiotRebootDelay();
	if(RX_PROTECT_DISABLE != ApsCoapRxProtectGet())
		ApsCoapRxTimeout();
	ApsCoapOnlineTimeout();
	ApsCoapExep();
	ApsCoapPanicReport(CHL_NB,NULL);
	ApsCoapWifiConDelay();
	//ApsCoapRandHeartTimeout();
}
int OppCoapPackRuntimeState(cJSON *prop, void * data)
{
	cJSON * comObj;
	ST_RUNTIME_PROP *ptr;

	ptr = (ST_RUNTIME_PROP *)data;

	comObj =  cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"OppCoapPackRuntimeState create <comObj> object error\r\n");
		return OPP_FAILURE;
	}	
	
	cJSON_AddItemToObject(prop, "runtime",comObj);
	cJSON_AddItemToObject(comObj, "hTime", cJSON_CreateNumber(ptr->hTime));							
	cJSON_AddItemToObject(comObj, "rTime", cJSON_CreateNumber(ptr->rTime));							

	return OPP_SUCCESS;
}


int OppCoapPackSwitchState(cJSON *prop, void * data)
{
	U32 *ptr = (U32 *)data;

	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"OppCoapPackSwitchState switch %d\r\n", (U8)*ptr);
	
	cJSON_AddItemToObject(prop, "switch", cJSON_CreateNumber((U8)*ptr));

	return OPP_SUCCESS;
}

int OppCoapPackBriState(cJSON *prop, void * data)
{
	U32 *ptr = (U32 *)data;

	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"OppCoapPackBriState switch %d\r\n", (U16)*ptr);
	
	cJSON_AddItemToObject(prop, "bri", cJSON_CreateNumber((U16)*ptr));

	return OPP_SUCCESS;
}


int OppCoapPackLoctionState(cJSON *prop, void * data)
{
	cJSON * comObj = NULL;
	char buffer[20] = {0};
	ST_OPP_LAMP_LOCATION * ptr = data;

	comObj =  cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"OppCoapPackLoctionState create <comObj> object error\r\n");
		return OPP_FAILURE;
	}	
	
	cJSON_AddItemToObject(prop, "loc", comObj);
	ftoa(buffer,ptr->ldLatitude,6);
	cJSON_AddItemToObject(comObj, "lat", cJSON_CreateString(buffer));
	ftoa(buffer,ptr->ldLongitude,6);
	cJSON_AddItemToObject(comObj, "lng", cJSON_CreateString(buffer));
	
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"OppCoapPackLoctionState lat %Lf lng %Lf\r\n", ptr->ldLatitude, ptr->ldLongitude);

	return OPP_SUCCESS;
}
int OppCoapPackElecState(cJSON *prop, void * data)
{
	cJSON * comObj = NULL;

	ST_ELEC_PROP * ptr = data;

	comObj =  cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"OppCoapPackElecState create <comObj> object error\r\n");
		return OPP_FAILURE;
	}	
	
	cJSON_AddItemToObject(prop, "elec", comObj);
	cJSON_AddItemToObject(comObj, "current", cJSON_CreateNumber(ptr->current));
	cJSON_AddItemToObject(comObj, "voltage", cJSON_CreateNumber(ptr->voltage));
	cJSON_AddItemToObject(comObj, "factor", cJSON_CreateNumber(ptr->factor));
	cJSON_AddItemToObject(comObj, "power", cJSON_CreateNumber(ptr->power));
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"OppCoapPackElecState current %d voltage %d factor %d\r\n", ptr->current, ptr->voltage, ptr->factor);

	return OPP_SUCCESS;
}

int OppCoapPackNbConState(cJSON *prop, void * data)
{
	cJSON * object = NULL;
	ST_NBCON_PROP *ptr;
	//NeulBc28QueryNetstats(&regStatus,&conStatus);
	
	ptr = (ST_NBCON_PROP *)data;

	object =  cJSON_CreateObject();
	if(NULL == object){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"OppCoapPackNbConState create <object> object error\r\n");
		return OPP_FAILURE;
	}	
	
	cJSON_AddItemToObject(prop, "nbCon", object);
	cJSON_AddItemToObject(object, "reg", cJSON_CreateNumber(ptr->regStatus));
	cJSON_AddItemToObject(object, "con", cJSON_CreateNumber(ptr->conStatus));
	return OPP_SUCCESS;
}


int OppCoapPackNbSignalState(cJSON *prop, void * data)
{
	cJSON * object = NULL;
	char buffer[20] = {0};
	ST_NBSIGNAL_PROP *ptr = NULL;
	ptr = (ST_NBSIGNAL_PROP *)data;
	
	object =  cJSON_CreateObject();
	if(NULL == object){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"OppCoapPackNbSignalState create <object> object error\r\n");
		return OPP_FAILURE;
	}	
	
	//NeulBc28QueryNetstats(&regStatus,&conStatus);	
	//nbSignal
	cJSON_AddItemToObject(prop, "nbSignal", object);
	itoa(ptr->rsrp, buffer, 10);
	cJSON_AddItemToObject(object, "rsrp", cJSON_CreateString(buffer));
	itoa(ptr->rsrq, buffer, 10);						
	cJSON_AddItemToObject(object, "rsrq", cJSON_CreateString(buffer));
	cJSON_AddItemToObject(object, "ecl", cJSON_CreateNumber(ptr->signalEcl));
	cJSON_AddItemToObject(object, "cellid", cJSON_CreateNumber(ptr->cellId));
	cJSON_AddItemToObject(object, "pci", cJSON_CreateNumber(ptr->signalPCI));
	itoa(ptr->snr, buffer, 10);						
	cJSON_AddItemToObject(object, "snr", cJSON_CreateString(buffer));
	cJSON_AddItemToObject(object, "earfcn", cJSON_CreateNumber(ptr->earfcn));
	cJSON_AddItemToObject(object, "txPower", cJSON_CreateNumber(ptr->txPower));

	return OPP_SUCCESS;
}

int OppCoapPackNbBaseState(cJSON *prop, void * data)
{
	cJSON * object = NULL;
	ST_NBBASE_PROP *ptr = NULL;

	/*
	object = cJSON_CreateObject();
	if(NULL == object){
		return OPP_FAILURE;
	}
*/
	ptr = (ST_NBBASE_PROP *)data;
	//nbBase
	object =  cJSON_CreateObject();
	if(NULL == object){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"OppCoapPackNbBaseState create <object> object error\r\n");
		return OPP_FAILURE;
	}	
	cJSON_AddItemToObject(prop, "nbBase", object);
	cJSON_AddItemToObject(object, "imei", cJSON_CreateString((char *)ptr->imei));
	cJSON_AddItemToObject(object, "imsi", cJSON_CreateString((char *)ptr->imsi));
	cJSON_AddItemToObject(object, "iccid", cJSON_CreateString((char *)ptr->nccid));
	cJSON_AddItemToObject(object, "nbip", cJSON_CreateString((char *)ptr->addr));
	return OPP_SUCCESS;
}

// 0:success,1:full,2:fail
int ApsCoapStateReport(ST_REPORT_PARA *pstReportPara)
{
	ST_REPORT_ENTRY *pstReport;
	U8 regStatus, conStatus;
	int i = 0;
	ST_OPP_LAMP_CURR_ELECTRIC_INFO stElecInfo;
	//ST_OPP_LAMP_LOCATION stLocation;
	long double lng;
	long double lat;
	U8 enable;
	U16 earfcn;

	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ApsCoapStateReport chl %d resId %d value %d\r\n", pstReportPara->chl, pstReportPara->resId, pstReportPara->value);

	for(i=0;i<sizeof(aucCmdIdMap)/sizeof(ST_CMDID_MAP);i++){
		if(aucCmdIdMap[i].id == pstReportPara->resId){
			if(aucCmdIdMap[i].func)
				aucCmdIdMap[i].func(pstReportPara->chl);
			return OPP_SUCCESS;
		}
	}
	
	for(i=0;i<sizeof(aucPropMap)/sizeof(ST_PROP_MAP);i++){
		if(aucPropMap[i].id == pstReportPara->resId){
			break;
		}
	}
	if(i == sizeof(aucPropMap)/sizeof(ST_PROP_MAP)){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"resId %d error\r\n", pstReportPara->resId);
		return 2;
	}
	//优先查组合属性，再查查子属性	
	MUTEX_LOCK(m_stCoapReportMutex,MUTEX_WAIT_ALWAYS);	
	LIST_FOREACH(pstReport,&list_report,elements){
		if(0 == strcmp(pstReport->propName,aucPropMap[i].comPropName)){
			if(0 == strcmp(aucPropMap[i].propName, "rTime"))
				((ST_RUNTIME_PROP *)(pstReport->data))->rTime = pstReportPara->value;
			else if(0 == strcmp(aucPropMap[i].propName, "hTime"))
				((ST_RUNTIME_PROP *)(pstReport->data))->hTime = pstReportPara->value;
			else if(0 == strcmp(aucPropMap[i].propName, "rsrp")){
				((ST_NBSIGNAL_PROP *)(pstReport->data))->rsrp = pstReportPara->value;
			}
			else if(0 == strcmp(aucPropMap[i].propName, "reg")){
				((ST_NBCON_PROP *)(pstReport->data))->regStatus = pstReportPara->value;
			}
			///< [code review][error] wangtao 6/29>
			else if(0 == strcmp(aucPropMap[i].propName, "current")){
				((ST_ELEC_PROP *)(pstReport->data))->current = pstReportPara->value;
			}
			else if(0 == strcmp(aucPropMap[i].propName, "voltage")){
				((ST_ELEC_PROP *)(pstReport->data))->voltage = pstReportPara->value;
			}
			else{
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"resId %d error\r\n", pstReportPara->resId);
			}
			MUTEX_UNLOCK(m_stCoapReportMutex);
			return OPP_SUCCESS;
		}else if(0 == strcmp(pstReport->propName,aucPropMap[i].propName)){
			*((U32 *)pstReport->data) = pstReportPara->value;
			MUTEX_UNLOCK(m_stCoapReportMutex);
			return OPP_SUCCESS;
		}
	}	
	MUTEX_UNLOCK(m_stCoapReportMutex);	
	//if not exist create
	pstReport = (ST_REPORT_ENTRY *)mymalloc(sizeof(ST_REPORT_ENTRY));
	if(NULL == pstReport){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_NULL_POINTER);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"malloc ST_REPORT_ENTRY error\r\n");
		return 2;
	}
	if(0 == strcmp(aucPropMap[i].comPropName, "runtime")){
		pstReport->data = (ST_RUNTIME_PROP *)mymalloc(sizeof(ST_RUNTIME_PROP));
		if(NULL == pstReport->data){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_NULL_POINTER);
			myfree(pstReport);
			return 2;
		}
		((ST_RUNTIME_PROP *)(pstReport->data))->rTime = g_stThisLampInfo.stCurrStatus.uwRunTime;
		((ST_RUNTIME_PROP *)(pstReport->data))->hTime = g_stThisLampInfo.stCurrStatus.uwHisTime;
		if(0 == strcmp(aucPropMap[i].propName, "rTime"))
			((ST_RUNTIME_PROP *)(pstReport->data))->rTime = pstReportPara->value;
		if(0 == strcmp(aucPropMap[i].propName, "hTime"))
			((ST_RUNTIME_PROP *)(pstReport->data))->hTime = pstReportPara->value;
	}
	else if(0 == strcmp(aucPropMap[i].comPropName, "nbSignal")){
		ST_NEUL_DEV *pstNeulDev = NULL;
		int ret;
		char *sArgv[MAX_ARGC];
		char *rArgv[MAX_ARGC];
		int sArgc = 0, rArgc = 0;
		pstReport->data = (ST_NBSIGNAL_PROP *)mymalloc(sizeof(ST_NBSIGNAL_PROP));
		if(NULL == pstReport->data){
			myfree(pstReport);
			return 2;
		}
		
		ret = sendEvent(UESTATE_EVENT,RISE_STATE,sArgc,sArgv);
		ret = recvEvent(UESTATE_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
		if(0 != ret){
			ARGC_FREE(rArgc,rArgv);
			return 2;
		}
		pstNeulDev = rArgv[0];
		((ST_NBSIGNAL_PROP *)(pstReport->data))->rsrp = pstNeulDev->rsrp;
		((ST_NBSIGNAL_PROP *)(pstReport->data))->rsrq = pstNeulDev->rsrq;
		((ST_NBSIGNAL_PROP *)(pstReport->data))->signalEcl = pstNeulDev->signalEcl;
		((ST_NBSIGNAL_PROP *)(pstReport->data))->cellId = pstNeulDev->cellId;
		((ST_NBSIGNAL_PROP *)(pstReport->data))->signalPCI = pstNeulDev->signalPci;
		((ST_NBSIGNAL_PROP *)(pstReport->data))->snr = pstNeulDev->snr;	
		NeulBc28GetEarfcnFromRam(&enable,&earfcn);
		((ST_NBSIGNAL_PROP *)(pstReport->data))->earfcn = earfcn;
		((ST_NBSIGNAL_PROP *)(pstReport->data))->txPower = pstNeulDev->txPower;
		if(0 == strcmp(aucPropMap[i].propName, "rsrp")){
			((ST_NBSIGNAL_PROP *)(pstReport->data))->rsrp = pstReportPara->value;
		}		
		ARGC_FREE(rArgc,rArgv);
	}
	else if(0 == strcmp(aucPropMap[i].comPropName, "nbCon")){
		pstReport->data = (ST_NBCON_PROP *)mymalloc(sizeof(ST_NBCON_PROP));
		if(NULL == pstReport->data){
			myfree(pstReport);			
			return 2;
		}
		NeulBc28QueryNetstats(&regStatus, &conStatus);				
		((ST_NBCON_PROP *)(pstReport->data))->regStatus = regStatus;
		((ST_NBCON_PROP *)(pstReport->data))->conStatus = conStatus;
		if(0 == strcmp(aucPropMap[i].propName, "reg")){
			((ST_NBCON_PROP *)(pstReport->data))->regStatus = pstReportPara->value;
		}
	}else if(0 == strcmp(aucPropMap[i].comPropName, "elec")){
		pstReport->data = (ST_ELEC_PROP *)mymalloc(sizeof(ST_ELEC_PROP));
		if(NULL == pstReport->data){
			myfree(pstReport);			
			return 2;
		}
		ElecGetElectricInfo(&stElecInfo);
		((ST_ELEC_PROP *)(pstReport->data))->current = stElecInfo.current;
		((ST_ELEC_PROP *)(pstReport->data))->voltage = stElecInfo.voltage;
		((ST_ELEC_PROP *)(pstReport->data))->factor = stElecInfo.factor;
		((ST_ELEC_PROP *)(pstReport->data))->power = stElecInfo.power;		
		if(0 == strcmp(aucPropMap[i].propName, "current")){
			((ST_ELEC_PROP *)(pstReport->data))->current = pstReportPara->value;
		}
		if(0 == strcmp(aucPropMap[i].propName, "voltage")){
			((ST_ELEC_PROP *)(pstReport->data))->voltage = pstReportPara->value;
		}
	}else if(0 == strcmp(aucPropMap[i].comPropName, "loc")){
		pstReport->data = (ST_OPP_LAMP_LOCATION *)mymalloc(sizeof(ST_OPP_LAMP_LOCATION));
		if(NULL == pstReport->data){
			myfree(pstReport);			
			return 2;
		}		
		///< [code review][error] wangtao 6/29>
		//LocationRead(GPS_LOC,&stLocation);
		LocGetLat(&lat);
		LocGetLng(&lng);
		((ST_OPP_LAMP_LOCATION *)(pstReport->data))->ldLatitude = lat;
		((ST_OPP_LAMP_LOCATION *)(pstReport->data))->ldLongitude = lng;
	}else{
		pstReport->data = (U32 *)mymalloc(sizeof(U32));						
		if(NULL == pstReport->data){
			myfree(pstReport);			
			return 2;
		}
		*((U32 *)pstReport->data) = pstReportPara->value;
	}

	pstReport->chl = pstReportPara->chl;
	if(0 != strlen(aucPropMap[i].comPropName))
		STRNCPY(pstReport->propName, aucPropMap[i].comPropName, sizeof(pstReport->propName));
	else
		STRNCPY(pstReport->propName, aucPropMap[i].propName, sizeof(pstReport->propName));

	MUTEX_LOCK(m_stCoapReportMutex,MUTEX_WAIT_ALWAYS);	
	LIST_INSERT_HEAD(&list_report,pstReport,elements);
	MUTEX_UNLOCK(m_stCoapReportMutex);
	/*code review have report and delay 1s*/
	if(0 == g_iReTick)
		g_iReTick = OppTickCountGet();
	
	return OPP_SUCCESS;

}

int ApsCoapReport(U8 dstChl, unsigned char *dstInfo)
{
	int ret;
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
	U16 outLength = 0;
	int i = 0;
	ST_REPORT_ENTRY *pstReport;
	cJSON * root, *propRe;

	
	MUTEX_LOCK(m_stCoapReportMutex,MUTEX_WAIT_ALWAYS);
	if(LIST_EMPTY(&list_report)){
		//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapReportStop list_report empty\r\n");
		MUTEX_UNLOCK(m_stCoapReportMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(m_stCoapReportMutex);
	
	root = cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapReport creat <root> error\r\n");
		return 2;
	
	}
	cJSON_AddItemToObject(root, "op", cJSON_CreateString("RE"));
	propRe = cJSON_CreateObject();
	if(NULL == propRe){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);		
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"OppCoapStateReport creat <propRe> error\r\n");
		cJSON_Delete(root);
		return 2;
	}
	cJSON_AddItemToObject(root, "prop", propRe);		

	
	MUTEX_LOCK(m_stCoapReportMutex,MUTEX_WAIT_ALWAYS);
	LIST_FOREACH(pstReport,&list_report,elements)
	{
		//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapReportStop %s\r\n", pstReport->propName);
		for(i=0;i<PROP_MAX;i++){
			if(0 == strcmp(pstReport->propName, aucProperty[i].name)){
				//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapReportStop i:%d %s\r\n", i, pstReport->propName);
				if(aucProperty[i].func){
					aucProperty[i].func(propRe, pstReport->data);
				}				
			}
		}
		
		myfree(pstReport->data);
		LIST_REMOVE(pstReport,elements);
		myfree(pstReport);
	}
	MUTEX_UNLOCK(m_stCoapReportMutex);
	
	ret = JsonCompres(root,(char *)coapmsgTx,&outLength);
	if(OPP_SUCCESS != ret){
		return ret;
	}	
	
	ret = OppCoapSend(dstChl, dstInfo, REPORT_MSG, REPORT_T, PROP_SERVICE, 0, coapmsgTx, outLength, OppCoapMidGet());
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "ApsCoapReportStop OppCoapSend ret %d\r\n",ret);		
		return ret;
	}
	OppCoapMidIncrease();
	return OPP_SUCCESS;
}

int ApsCoapGetLogReportStatus(U16 *status, U16 * mid)
{
	MUTEX_LOCK(m_stLogStatus.mutex, MUTEX_WAIT_ALWAYS);
	*status = m_stLogStatus.status;
	*mid = m_stLogStatus.mid;
	MUTEX_UNLOCK(m_stLogStatus.mutex);

	return OPP_SUCCESS;
}

int ApsCoapGetLogReportMid(void)
{
	U16 mid;
	
	MUTEX_LOCK(m_stLogStatus.mutex, MUTEX_WAIT_ALWAYS);
	mid = m_stLogStatus.mid;
	MUTEX_UNLOCK(m_stLogStatus.mutex);

	return mid;
}

int ApsCoapSetLogReportStatus(U16 status, U16 mid)
{
	MUTEX_LOCK(m_stLogStatus.mutex, MUTEX_WAIT_ALWAYS);
	m_stLogStatus.status = status;
	m_stLogStatus.mid = mid;
	MUTEX_UNLOCK(m_stLogStatus.mutex);
	
	return OPP_SUCCESS;
}

int ApsCoapGetRetrySpec(void)
{
	return g_isSupportRetry;
}

int ApsCoapSetRetrySpec(U8 flag)
{
	g_isSupportRetry = flag;
	return OPP_SUCCESS;
}

ST_FASTPROP_PARA * ApsCoapFastConfigPtrGet(void)
{
	return g_stFastConfigPara;
}

int ApsCoapFastConfigSizeGet(void)
{
	return sizeof(g_stFastConfigPara)/sizeof(ST_FASTPROP_PARA);
}

ST_FASTALARM_PARA * ApsCoapFastAlarmPtrGet(void)
{
	return g_stFastAlarmPara;
}

int ApsCoapFastAlarmSizeGet(void)
{
	return sizeof(g_stFastAlarmPara)/sizeof(ST_FASTALARM_PARA);
}

int ApsCoapWriteExepInfo(U8 rstType)
{
	extern ST_BC28_PKT	g_stBc28Pkt;

	int ret;
	ST_EXEP_INFO stExepInfo;
	int len = 0;
	
	stExepInfo.oppleRstType = rstType;
	stExepInfo.rst0Type = SW_CPU_RESET;
	stExepInfo.rst1Type = SW_CPU_RESET;
	stExepInfo.tick= OppTickCountGet();
	memset(stExepInfo.data,0,sizeof(stExepInfo.data));
	if(COAP_RST == rstType){
		MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
		memcpy(stExepInfo.data, (char *)&g_stCoapPkt, sizeof(ST_COAP_PKT));
		MUTEX_UNLOCK(g_stCoapPkt.mutex);
		len += sizeof(ST_COAP_PKT);
		MUTEX_LOCK(g_stBc28Pkt.mutex, MUTEX_WAIT_ALWAYS);
		memcpy(stExepInfo.data+len, (char *)&g_stBc28Pkt, sizeof(ST_BC28_PKT));
		MUTEX_UNLOCK(g_stBc28Pkt.mutex);
		len += sizeof(ST_BC28_PKT);
	}
	
	ret = ApsExepWriteExep(&stExepInfo);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsExepWriteExep fail ret %d\r\n", ret);
		return OPP_FAILURE;
	}

	ret = ApsExepWriteNormalReboot(NORMAL_REBOOT);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsExepWriteNormalReboot fail ret %d\r\n", ret);
		return OPP_FAILURE;
	}
	return OPP_SUCCESS;
}

int ApsCoapExepReport(U8 dstChl, unsigned char * dstInfo, int exepNum, ST_EXEP_INFO * pstExepInfo)
{
	int ret;
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
	cJSON * root;
	cJSON * cmdData, *comObj;
	U16 outLength = 0;
	int reqId = 0;
	ST_COAP_PKT *pstCoapPkt;
	ST_BC28_PKT *pstBc28Pkt;
	int len = 0;
	
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ApsCoapExepReport\r\n");
	root =	cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapExepReport create <root> object error\r\n");
		return OPP_FAILURE;
	}	
	
	cJSON_AddItemToObject(root, "reqId", cJSON_CreateNumber(reqId = OppCoapReqIdGen()));
	cJSON_AddItemToObject(root, "cmdId", cJSON_CreateString(EXEP_CMDID));
	cmdData =  cJSON_CreateObject();
	if(NULL == cmdData){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapExepReport create <cmdData> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(root, "cmdData", cmdData);
	comObj = cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapExepReport create nbState <comObj> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(cmdData, "fwv", cJSON_CreateNumber(OPP_LAMP_CTRL_CFG_DATA_VER));
	cJSON_AddItemToObject(cmdData, "nbState", comObj);
	cJSON_AddItemToObject(comObj, "ds", cJSON_CreateNumber(NeulBc28GetDevState())); 
	cJSON_AddItemToObject(comObj, "at", cJSON_CreateNumber(NeulBc28GetAttachEplaseTime()));	
	cJSON_AddItemToObject(comObj, "hdt", cJSON_CreateNumber(ApsCoapHeartDisOffsetSecondGet()));	
	cJSON_AddItemToObject(comObj, "adt", cJSON_CreateNumber(NeulBc28DisOffsetSecondGet()));
	cJSON_AddItemToObject(cmdData, "exepNum", cJSON_CreateNumber(exepNum));	
	cJSON_AddItemToObject(cmdData, "oppRst", cJSON_CreateNumber(pstExepInfo->oppleRstType));
	cJSON_AddItemToObject(cmdData, "rst0", cJSON_CreateNumber(pstExepInfo->rst0Type));
	cJSON_AddItemToObject(cmdData, "rst1", cJSON_CreateNumber(pstExepInfo->rst1Type));	
	cJSON_AddItemToObject(cmdData, "rstTick", cJSON_CreateNumber(pstExepInfo->tick));
	
	if(COAP_RST == pstExepInfo->oppleRstType){
		comObj = cJSON_CreateObject();
		if(NULL == comObj){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapExepReport create coapInfo <comObj> object error\r\n");
			cJSON_Delete(root);
			return OPP_FAILURE;
		}
		pstCoapPkt = (ST_COAP_PKT *)pstExepInfo->data;
		cJSON_AddItemToObject(cmdData, "coPkt", comObj);
		cJSON_AddItemToObject(comObj, "ts", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbTxSuc));
		cJSON_AddItemToObject(comObj, "tf", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbTxFail));
		cJSON_AddItemToObject(comObj, "tReqR", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbTxReqRetry));
		cJSON_AddItemToObject(comObj, "tRspR", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbTxRspRetry));
		cJSON_AddItemToObject(comObj, "r", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbRx));
		cJSON_AddItemToObject(comObj, "rRsp", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbRxRsp));
		cJSON_AddItemToObject(comObj, "rReq", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbRxReq));
		cJSON_AddItemToObject(comObj, "rDReq", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbDevReq));
		cJSON_AddItemToObject(comObj, "rDRsp", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbDevRsp));
		cJSON_AddItemToObject(comObj, "rU", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbUnknow));
		cJSON_AddItemToObject(comObj, "orx", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbOtaRx));
		cJSON_AddItemToObject(comObj, "otx", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbOtaTx));
		cJSON_AddItemToObject(comObj, "ots", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbOtaTxSucc));
		cJSON_AddItemToObject(comObj, "otf", cJSON_CreateNumber(pstCoapPkt->g_iCoapNbOtaTxFail));
		cJSON_AddItemToObject(comObj, "busy", cJSON_CreateNumber(pstCoapPkt->g_iCoapBusy));
		/*comObj = cJSON_CreateObject();
		if(NULL == comObj){
			MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapExepReport create bc28Pkt <comObj> object error\r\n");
			cJSON_Delete(root);
			return OPP_FAILURE;
		}		
		len = sizeof(ST_COAP_PKT);
		pstBc28Pkt = (ST_BC28_PKT *)&pstExepInfo->data[len];
		cJSON_AddItemToObject(cmdData, "bc28Pkt", comObj);
		cJSON_AddItemToObject(comObj, "rxFromQue", cJSON_CreateNumber(pstBc28Pkt->iPktRxFromQue));
		cJSON_AddItemToObject(comObj, "tx", cJSON_CreateNumber(pstBc28Pkt->iPktTx));
		cJSON_AddItemToObject(comObj, "txSucc", cJSON_CreateNumber(pstBc28Pkt->iPktTxSucc));
		cJSON_AddItemToObject(comObj, "txFail", cJSON_CreateNumber(pstBc28Pkt->iPktTxFail));
		cJSON_AddItemToObject(comObj, "txExpire", cJSON_CreateNumber(pstBc28Pkt->iPktTxExpire));
		cJSON_AddItemToObject(comObj, "nnmiRx", cJSON_CreateNumber(pstBc28Pkt->iNnmiRx));
		cJSON_AddItemToObject(comObj, "pushSuc", cJSON_CreateNumber(pstBc28Pkt->iPktPushSuc));
		cJSON_AddItemToObject(comObj, "pushFail", cJSON_CreateNumber(pstBc28Pkt->iPktPushFail));
		cJSON_AddItemToObject(comObj, "txEnable", cJSON_CreateNumber(pstBc28Pkt->iTxEnable));*/
	}
	JsonCompres(root, (char *)&coapmsgTx, &outLength);
	OppCoapSend(dstChl, dstInfo,CMD_MSG, REPORT_T, FUNC_SERVICE,0,coapmsgTx, outLength, OppCoapMidGet());	
	OppCoapMidIncrease();
	return OPP_SUCCESS;
}

int ApsCoapPanicReport(U8 dstChl,unsigned char *dstInfo)
{
	int ret;
	U8 coapmsgTx[JSON_S_MAX_LEN] = {0};
	cJSON * root;
	cJSON * cmdData, *comObj;
	U16 outLength = 0;
	int reqId = 0;
	char *ptr0 = NULL, *ptr1 = NULL;
	char *ptr = NULL;
	static U8 report = 0;
	
	if(BC28_READY != NeulBc28GetDevState())
		return OPP_SUCCESS;
	if(report)
		return OPP_SUCCESS;
	
	ApsExepGetPanic(0,&ptr0);
	ApsExepGetPanic(1,&ptr1);
	if(0 == strlen(ptr0) && 0 == strlen(ptr1))
		return OPP_SUCCESS;

	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"ApsCoapPanicReport\r\n");
	root =	cJSON_CreateObject();
	if(NULL == root){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPanicReport create <root> object error\r\n");
		return OPP_FAILURE;
	}		
	cJSON_AddItemToObject(root, "reqId", cJSON_CreateNumber(reqId = OppCoapReqIdGen()));
	cJSON_AddItemToObject(root, "cmdId", cJSON_CreateString(PANIC_CMDID));
	cmdData =  cJSON_CreateObject();
	if(NULL == cmdData){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPanicReport create <cmdData> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}
	cJSON_AddItemToObject(root, "cmdData", cmdData);
	comObj = cJSON_CreateObject();
	if(NULL == comObj){
		MakeErrLog(DEBUG_MODULE_COAP,OPP_COAP_CREATE_OBJ_ERR);
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"ApsCoapPanicReport create nbState <comObj> object error\r\n");
		cJSON_Delete(root);
		return OPP_FAILURE;
	}
	ptr = strstr(ptr0,"Backtrace:");
	if(ptr)
		cJSON_AddItemToObject(cmdData, "panic0", cJSON_CreateString(ptr));
	ptr = strstr(ptr1,"Backtrace:");
	if(ptr)
		cJSON_AddItemToObject(cmdData, "panic1", cJSON_CreateString(ptr));
	JsonCompres(root, (char *)&coapmsgTx, &outLength);
	OppCoapSend(dstChl, dstInfo, CMD_MSG, REPORT_T, FUNC_SERVICE,0,coapmsgTx, outLength, OppCoapMidGet());	
	OppCoapMidIncrease();
	report = 1;
	return OPP_SUCCESS;
}
int ApsCoapExep(void)
{
	int ret;
	ST_EXEP_INFO stExepInfo;
	static int phase = 0;
	ST_IOT_CONFIG stConfigPara;
	int exepNum = 0;
	
	if(BC28_READY != NeulBc28GetDevState())
		return OPP_SUCCESS;
	/*
	OppCoapIOTGetConfig(&stConfigPara);
	if(0 == strcmp(stConfigPara.ocIp,COM_PLT)){
		if(!recvHeartAck)
			return OPP_SUCCESS;
	}*/
	
	if(0 == phase){
		ret = ApsExepReadCurExep(&stExepInfo);
		phase = 1;
		exepNum = 0;
	}else if(1 == phase){
		ret = ApsExepReadPreExep(&stExepInfo);
		phase = 2;
		exepNum = 1;
	}else{
		return OPP_SUCCESS;
	}
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapExep read exepNum %d exep error ret %d\r\n", exepNum, ret);
		return OPP_FAILURE;
	}
	ret = ApsCoapExepReport(CHL_NB,NULL,exepNum,&stExepInfo);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "ApsCoapExep call ApsCoapExepReport exepNum %d fail ret %d\r\n", exepNum, ret);
		return OPP_FAILURE;
	}
	return OPP_SUCCESS;
}

