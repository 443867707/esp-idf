#include "cli-interpreter.h"
#include "SVS_Log.h"
#include "SVS_Exep.h"
#include "SVS_Coap.h"
#include "SVS_Config.h"
#include "SVS_MsgMmt.h"
#include "APP_LampCtrl.h"
#include "LIB_List.h"
#include "string.h"
#include <sys/socket.h>

extern LIST_HEAD(listc,_list_entry) list_coap;
extern T_MUTEX m_ulCoapMutex;
extern LIST_HEAD(listr,_list_report_entry) list_report;
extern T_MUTEX m_stCoapReportMutex;
extern LIST_HEAD(listh,_list_retry_hb_entry) list_rhb;
extern T_MUTEX m_ulCoapRHB;

extern ST_COAP_PKT g_stCoapPkt;

void CommandPropReport(void);
void CommandPropAlarm(void);
void CommandLogReportStatus(void);
void CommandSetLogReportStatus(void);
void CommandGetRuntime(void);
void CommandSetRuntime(void);
void CommandGetRuntime(void);
void CommandGetHtimeFlash(void);
void CommandSetHtimeFlash(void);
void CommandGetIotConfig(void);
void CommandSetIotConfig(void);
void CommandGetIotConfigFromFlash(void);
void CommandSetIotConfigToFlash(void);
void CommandShowRetryList(void);
void CommandShowReportList(void);
void CommandSetRetrySpec(void);
void CommandGetRetrySpec(void);
void CommandHeartBeat(void);
void CommandCoapPkt(void);
void CommandCoapBusySet(void);
void CommandCoapBusyGet(void);
void CommandTestTx(void);
void CommandShowRhbList(void);
void CommandGetSvrTimeFromNb(void);
void CommandGetSvrTimeFromWifi(void);
void CommandHeartPeriodGet(void);
void CommandHeartPeriodSet(void);
void CommandRxProtectSet(void);
void CommandRxProtectGet(void);
void CommandExepGet(void);
void CommandExepSet(void);
void CommandExepReport(void);
void CommandHeartDisParaGet(void);
void CommandHeartDisParaSet(void);
void CommandHeartDisOffsetGet(void);
void CommandHeartDisTickGet(void);
void CommandGetLighttime(void);
void CommandSetLighttime(void);


const char* const report[] = {
	"propId","valude"
};

const char* const alarmDesc[] = {
"alarmId","type","valude"
};
const char* const runtime[] = {
"rTime","hTime"
};
const char* const lighttime[] = {
"lTime","hlTime"
};
const char* const htime[] = {
"htime"
};
const char* const retry[] = {
"0:not support, 1:support"
};
const char* const busy[] = {
"0:not busy, 1:busy"
};
const char* const getTime[] = {
"0:nb channel, 1:wifi channel"
};

const char* const logStatusDesc[] = {
"LOG_SEND_ERR=0,LOG_SEND_SUCC=1,LOG_SEND_ING=2,LOG_SEND_INIT=3",
"mid"
};
const char* const exepDesc[] = {
"cur:0,pre:1"
};
const char* const rstDesc[] = {
"COAP_RST:0,OTA_RST:1,CMD_RST:2,POWER_RST:3,UNKNOW_RST:4"
};
const char* const heartDisDesc[] = {
"window",
"interval"
};
const char* const testPktDesc[] = {
"pkt number",
"pkt interval time"
};
const char* const wifiTimeDesc[] = {
"udp svr ip",
"udp svr port"
};

CommandEntry CommandTableGetTime[]={
	CommandEntryActionWithDetails("nbiot", CommandGetSvrTimeFromNb, "", "get time from nbiot server...", NULL),	
	CommandEntryActionWithDetails("wifi", CommandGetSvrTimeFromWifi, "su", "get time from wifi server...", wifiTimeDesc), 
	CommandEntryTerminator()
};

CommandEntry CommandTableProp[] =
{
	CommandEntryActionWithDetails("report", CommandPropReport, "uu", "g_pstReProp report...", report),
	CommandEntryActionWithDetails("alarm", CommandPropAlarm, "uu", "g_pstReProp alarm...", alarmDesc),
	CommandEntryActionWithDetails("gLog", CommandLogReportStatus, "", "get log report status...", NULL),
	CommandEntryActionWithDetails("sLog", CommandSetLogReportStatus, "uu", "set log report status...", logStatusDesc),
	CommandEntryActionWithDetails("gRuntime", CommandGetRuntime, "", "get runtime...", NULL),
	CommandEntryActionWithDetails("sRuntime", CommandSetRuntime, "uu", "set runtime...", runtime),
	CommandEntryActionWithDetails("gfHtime", CommandGetHtimeFlash, "", "get htime from flash...", NULL),
	CommandEntryActionWithDetails("sfHtime", CommandSetHtimeFlash, "u", "set htime to flash...", htime),
	CommandEntryActionWithDetails("gLighttime", CommandGetLighttime, "", "get light time...", NULL),
	CommandEntryActionWithDetails("sLighttime", CommandSetLighttime, "uu", "set light time...", lighttime),
	CommandEntryActionWithDetails("sIot", CommandSetIotConfig, "u", "set iot config...", NULL),
	CommandEntryActionWithDetails("gIot", CommandGetIotConfig, "", "get iot config...", NULL),
	CommandEntryActionWithDetails("sfIot", CommandSetIotConfigToFlash, "u", "set iot config to flash...", NULL),
	CommandEntryActionWithDetails("gfIot", CommandGetIotConfigFromFlash, "", "get iot config from flash...", NULL),
	CommandEntryActionWithDetails("retryListShow", CommandShowRetryList, "", "show coap retry list...", NULL),
	CommandEntryActionWithDetails("gSupportRetry", CommandGetRetrySpec, "", "get coap report retry spec...", NULL),
	CommandEntryActionWithDetails("sSupportRetry", CommandSetRetrySpec, "u", "set coap report retry spec...", retry),
	CommandEntryActionWithDetails("testTx", CommandTestTx, "uu", "tx test times...", testPktDesc),
	CommandEntryActionWithDetails("heart", CommandHeartBeat, "", "send heart beat...", NULL),
	CommandEntryActionWithDetails("pkt", CommandCoapPkt, "", "copa packet...", NULL),
	CommandEntryActionWithDetails("busySet", CommandCoapBusySet, "u", "copa busy set...", NULL),
	CommandEntryActionWithDetails("busyGet", CommandCoapBusyGet, "", "copa busy get...", busy),
	CommandEntryActionWithDetails("rhbShow", CommandShowRhbList, "", "heatbeat list show...", NULL),
	//CommandEntryActionWithDetails("getTime", CommandGetSvrTime, "", "get time from nbiot server...", NULL),
	CommandEntryActionWithDetails("getHeartPeriod", CommandHeartPeriodGet, "", "get heart period...", NULL),
	CommandEntryActionWithDetails("setHeartPeriod", CommandHeartPeriodSet, "u", "set heart period...", NULL),
	CommandEntryActionWithDetails("getRxProtect", CommandRxProtectGet, "", "get rx protect...", NULL),
	CommandEntryActionWithDetails("setRxProtect", CommandRxProtectSet, "u", "set rx protect...", NULL),
	CommandEntryActionWithDetails("exepGet", CommandExepGet, "u", "get exep...", exepDesc),
	CommandEntryActionWithDetails("exepSet", CommandExepSet, "u", "set exep...", rstDesc),
	CommandEntryActionWithDetails("exepReport", CommandExepReport, "u", "exep report...", exepDesc),
	CommandEntryActionWithDetails("heartDisParaGet", CommandHeartDisParaGet, "", "heart dis para get...", NULL),
	CommandEntryActionWithDetails("heartDisParaSet", CommandHeartDisParaSet, "uu", "heart dis para set...", heartDisDesc),
	CommandEntryActionWithDetails("heartDisOffsetGet", CommandHeartDisOffsetGet, "", "heart dis offset get", NULL),
	CommandEntryActionWithDetails("heartDisTickGet", CommandHeartDisTickGet, "", "heart dis tick get", NULL),
	CommandEntrySubMenu("getTime", CommandTableGetTime, "get time from server"),
	
	CommandEntryTerminator()
};

void CommandPropReport(void)
{
	ST_REPORT_PARA para;
	
	CLI_PRINTF("propId %d  value %d\r\n", CliIptArgList[0][0],CliIptArgList[1][0]);
	para.chl = CHL_NB;
	para.resId = CliIptArgList[0][0];
	para.value = CliIptArgList[1][0];
	ApsCoapStateReport(&para);
	//ApsCoapReportStop(CHL_NB);
}

void CommandPropAlarm(void)
{
	ST_ALARM_PARA para;

	CLI_PRINTF("alarm %d  type %d value %d\r\n", CliIptArgList[0][0],CliIptArgList[1][0], CliIptArgList[2][0]);
	para.dstChl = CHL_NB;
	para.alarmId = CliIptArgList[0][0];
	para.status = CliIptArgList[1][0];
	para.value = CliIptArgList[2][0];
	memset(para.dstInfo,0,sizeof(para.dstInfo));
	ApsCoapAlarmReport(&para);

}

void CommandLogReportStatus(void)
{
	U16 status,mid;

	ApsCoapGetLogReportStatus(&status,&mid);
	CLI_PRINTF("logReport Status %d mid %d\r\n",status,mid);
}

void CommandSetLogReportStatus(void)
{
	ApsCoapSetLogReportStatus(CliIptArgList[0][0],CliIptArgList[1][0]);
}

void CommandGetRuntime(void)
{
	U32 htime, rtime;

	OppLampCtrlGetHtime(0, &htime);
	OppLampCtrlGetRtime(0, &rtime);
	CLI_PRINTF("rtime %d htime %d\r\n", rtime, htime);
}
void CommandSetRuntime(void)
{
	if(CliIptArgList[0][0]>MAX_RUNTIME){
		CLI_PRINTF("htime should not greate than %d\r\n", MAX_RUNTIME);
		return;
	}
	if(CliIptArgList[1][0]>MAX_RUNTIME){
		CLI_PRINTF("rtime should not greate than %d\r\n", MAX_RUNTIME);
		return;
	}
	OppLampCtrlSetHtime(0, CliIptArgList[0][0]);
	OppLampCtrlSetRtime(0, CliIptArgList[1][0]);
}
void CommandGetLighttime(void)
{
	U32 hltime, ltime;

	OppLampCtrlGetHLtime(0, &hltime);
	OppLampCtrlGetLtime(0, &ltime);
	CLI_PRINTF("ltime %d hltime %d\r\n", ltime, hltime);
}
void CommandSetLighttime(void)
{
	if(CliIptArgList[0][0]>MAX_LIGHT_TIME){
		CLI_PRINTF("hltime should not greate than %d\r\n", MAX_LIGHT_TIME);
		return;
	}
	if(CliIptArgList[1][0]>MAX_LIGHT_TIME){
		CLI_PRINTF("ltime should not greate than %d\r\n", MAX_LIGHT_TIME);
		return;
	}
	OppLampCtrlSetHLtime(0, CliIptArgList[0][0]);
	OppLampCtrlSetLtime(0, CliIptArgList[1][0]);
}


void CommandGetHtimeFlash(void)
{
	//ST_CONFIG_PARA stConfigPara;
	ST_RUNTIME_CONFIG stConfigPara;
	int ret;
	
	ret = OppLampCtrlGetRuntimeParaFromFlash(&stConfigPara);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("OppLampCtrlGetConfigPara ret %d \r\n", ret);
		return;
	}
	CLI_PRINTF("htime %d \r\n", stConfigPara.hisTime);
}

void CommandSetHtimeFlash(void)
{
	//ST_CONFIG_PARA stConfigPara;
	ST_ELEC_CONFIG stElecInfo;
	int ret;

	CLI_PRINTF("CliIptArgList[0][0] %d \r\n", CliIptArgList[0][0]);

	/*ret = OppLampCtrlGetRuntimeParaFromFlash(&stConfigPara);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("OppLampCtrlGetConfigPara ret %d \r\n", ret);
		return;
	}*/
	ret = ElecReadFlash(&stElecInfo);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("ElecReadFlash fail ret %d \r\n", ret);
		return;
	}
	stElecInfo.hisTime = CliIptArgList[0][0];
	ret = ElecWriteFlash(&stElecInfo);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("ElecWriteFlash ret %d \r\n", ret);
		return;
	}
	
	/*ret = OppLampCtrlSetRuntimeParaToFlash(&stConfigPara);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("OppLampCtrlSetConfigPara ret %d \r\n", ret);
		return;
	}*/
}

void CommandGetIotConfig(void)
{
	ST_IOT_CONFIG stConfigPara;
	int ret;
	
	ret = OppCoapIOTGetConfig(&stConfigPara);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("OppCoapIOTGetConfig ret %d\r\n", ret);
	}
	CLI_PRINTF("ip:%s, port:%d", stConfigPara.ocIp, stConfigPara.ocPort);
}

void CommandSetIotConfig(void)
{
//	ST_IOT_CONFIG stConfigPara;

	OppCoapIOTSetConfig(&g_stIotConfigDefault);
}

void CommandGetIotConfigFromFlash(void)
{
	ST_IOT_CONFIG stConfigPara;

	OppCoapIOTGetConfigFromFlash(&stConfigPara);
	CLI_PRINTF("ip:%s, port:%d", stConfigPara.ocIp, stConfigPara.ocPort);
}

void CommandSetIotConfigToFlash(void)
{
	OppCoapIOTSetConfigToFlash(&g_stIotConfigDefault);
	OppCoapIOTSetConfig(&g_stIotConfigDefault);
}

void CommandShowRetryList(void)
{
	ST_COAP_ENTRY *pstCoapEntry = NULL;	
	U16 status,mid;

	CLI_PRINTF("\r\ncurrent tick %d\r\n", OppTickCountGet());

	MUTEX_LOCK(m_ulCoapMutex, MUTEX_WAIT_ALWAYS);
	CLI_PRINTF("\r\n***************************\r\n");
	LIST_FOREACH(pstCoapEntry, &list_coap, elements)
	{
		CLI_PRINTF("type:%d,chl:%d,mid:%d,tick:%d,length:%d,time:%d\r\n", 
			pstCoapEntry->type, pstCoapEntry->chl, pstCoapEntry->mid, pstCoapEntry->tick, pstCoapEntry->length, pstCoapEntry->times);

	}
	CLI_PRINTF("\r\n***************************\r\n");
	ApsCoapGetLogReportStatus(&status,&mid);
	CLI_PRINTF("logReport Status %d mid %d\r\n",status,mid);
	CLI_PRINTF("\r\n***************************\r\n");
	
	MUTEX_UNLOCK(m_ulCoapMutex);
}

void CommandShowReportList(void)
{
	ST_REPORT_ENTRY *pstReport;

	MUTEX_LOCK(m_stCoapReportMutex, MUTEX_WAIT_ALWAYS);
	LIST_FOREACH(pstReport,&list_report,elements){
		CLI_PRINTF("\r\n***************************\r\n");
		CLI_PRINTF("ch:%d, propName:%s\r\n",pstReport->chl,pstReport->propName);
		CLI_PRINTF("\r\n***************************\r\n");		
	}
	MUTEX_UNLOCK(m_stCoapReportMutex);
}

void CommandSetRetrySpec(void)
{
	ApsCoapSetRetrySpec(CliIptArgList[0][0]);

}

void CommandGetRetrySpec(void)
{
	CLI_PRINTF("support report retry: %d\r\n", ApsCoapGetRetrySpec());
}

void CommandHeartBeat(void)
{
	ApsCoapOceanconHeart(0/*CHL_NB*/);
}

void CommandTestTx(void)
{
	int i = 0;
	int num = CliIptArgList[0][0];

	for(i=0;i<num;i++){
		ApsCoapOceanconHeart(0/*CHL_NB*/);
		vTaskDelay(CliIptArgList[1][0] / portTICK_PERIOD_MS);
	}
}

void CommandCoapPkt(void)
{
	MUTEX_LOCK(g_stCoapPkt.mutex,MUTEX_WAIT_ALWAYS);
	CLI_PRINTF("nb tx succ num: %d\r\n", g_stCoapPkt.g_iCoapNbTxSuc);
	CLI_PRINTF("nb tx fail num: %d\r\n", g_stCoapPkt.g_iCoapNbTxFail);
	CLI_PRINTF("nb tx retry num: %d\r\n", g_stCoapPkt.g_iCoapNbTxRetry);
	CLI_PRINTF("nb tx retry req num: %d\r\n", g_stCoapPkt.g_iCoapNbTxReqRetry);
	CLI_PRINTF("nb tx retry rsp num: %d\r\n", g_stCoapPkt.g_iCoapNbTxRspRetry);
	CLI_PRINTF("nb rx pkt num: %d\r\n", g_stCoapPkt.g_iCoapNbRx);
	CLI_PRINTF("nb rx clound rsp num: %d\r\n", g_stCoapPkt.g_iCoapNbRxRsp);
	CLI_PRINTF("nb rx clound req num: %d\r\n", g_stCoapPkt.g_iCoapNbRxReq);
	CLI_PRINTF("nb rx dev req num: %d\r\n", g_stCoapPkt.g_iCoapNbDevReq);
	CLI_PRINTF("nb rx dev rsp num: %d\r\n", g_stCoapPkt.g_iCoapNbDevRsp);
	CLI_PRINTF("nb rx pkt unknow num: %d\r\n", g_stCoapPkt.g_iCoapNbUnknow);
	CLI_PRINTF("nb ota tx pkt num: %d\r\n", g_stCoapPkt.g_iCoapNbOtaTx);
	CLI_PRINTF("nb ota tx pkt succ num: %d\r\n", g_stCoapPkt.g_iCoapNbOtaTxSucc);
	CLI_PRINTF("nb ota tx pkt fail num: %d\r\n", g_stCoapPkt.g_iCoapNbOtaTxFail);
	MUTEX_UNLOCK(g_stCoapPkt.mutex);
}

void CommandCoapBusySet(void)
{
	MUTEX_LOCK(g_stCoapPkt.mutex,MUTEX_WAIT_ALWAYS);
	g_stCoapPkt.g_iCoapBusy = CliIptArgList[0][0];
	MUTEX_UNLOCK(g_stCoapPkt.mutex);
}

void CommandCoapBusyGet(void)
{
	MUTEX_LOCK(g_stCoapPkt.mutex,MUTEX_WAIT_ALWAYS);
	CLI_PRINTF("coap busy: %d\r\n", g_stCoapPkt.g_iCoapBusy);
	MUTEX_UNLOCK(g_stCoapPkt.mutex);
}
/*heart beat retry*/
void CommandShowRhbList(void)
{
	ST_RHB_ENTRY *pstRhbEntry = NULL;	
	U16 status,mid;

	CLI_PRINTF("\r\ncurrent tick %d\r\n", OppTickCountGet());

	MUTEX_LOCK(m_ulCoapRHB, 100);
	CLI_PRINTF("\r\n***************************\r\n");
	LIST_FOREACH(pstRhbEntry, &list_rhb, elements)
	{
		CLI_PRINTF("chl:%d,reqId:%d,mid:%d,tick:%d,times:%d\r\n", 
			pstRhbEntry->chl, pstRhbEntry->reqId, pstRhbEntry->mid, pstRhbEntry->tick, pstRhbEntry->times);

	}
	CLI_PRINTF("\r\n***************************\r\n");
	
	MUTEX_UNLOCK(m_ulCoapRHB);
}

void CommandGetSvrTimeFromNb(void)
{
	ApsCoapGetTime(CHL_NB,NULL);
}

void CommandGetSvrTimeFromWifi(void)
{
	unsigned int addr;
	unsigned char dstInfo[MSG_INFO_SIZE]={0};

	inet_aton(&CliIptArgList[0][0], &addr);
	*(unsigned int *)&dstInfo[0] = ntohl(addr);
	*(unsigned short *)&dstInfo[4] = CliIptArgList[1][0];
	ApsCoapGetTime(CHL_WIFI,dstInfo);
}

void CommandHeartPeriodGet(void)
{
	AD_CONFIG_T config;

	ApsDaemonActionItemParaGetCustom(CUSTOM_REPORT,HEARTBEAT_PERIOD_IDX,&config);
	CLI_PRINTF("heart period %d\r\n", config.reportIfPara);

}

void CommandHeartPeriodSet(void)
{
	int ret;
	AD_CONFIG_T config;
	
	config.resource = RSC_HEART;
	config.enable |= MCA_REPORT;
	config.targetid = 0;
	config.reportIf = RI_TIMING;
	config.reportIfPara = CliIptArgList[0][0];
	ret = ApsDaemonActionItemParaSetCustom(CUSTOM_REPORT, HEARTBEAT_PERIOD_IDX, &config);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("CommandHeartPeriodSet fail %d\r\n", ret);
	}
}

void CommandRxProtectSet(void)
{
	ApsCoapRxProtectSet(CliIptArgList[0][0]);
}

void CommandRxProtectGet(void)
{
	CLI_PRINTF("rx protect %s\r\n",ApsCoapRxProtectGet()==0?"disable":"enable");
}

char *oppRstType[] = {
	"COAP_RST",
	"OTA_RST",
	"CMD_RST",
	"POWER_RST",
	"UNKNOW_RST"
};

char *cpuRstType[] = {
    "NO_MEAN",
    "POWERON_RESET",
    "NO_MEAN",
    "SW_RESET",
    "OWDT_RESET",
    "DEEPSLEEP_RESET",
    "SDIO_RESET",
    "TG0WDT_SYS_RESET",
    "TG1WDT_SYS_RESET",
    "RTCWDT_SYS_RESET",
    "INTRUSION_RESET",
    "TGWDT_CPU_RESET",
    "SW_CPU_RESET",
    "RTCWDT_CPU_RESET",
    "EXT_CPU_RESET",
    "RTCWDT_BROWN_OUT_RESET",
    "RTCWDT_RTC_RESET",
};
void CommandExepGet(void)
{
	int ret;
	int len = 0;
	ST_EXEP_INFO stExepInfo;
	ST_COAP_PKT *pstCoapPkt;
	ST_BC28_PKT *pstBc28Pkt;

	if(0 == CliIptArgList[0][0]){
		ret = ApsExepReadCurExep(&stExepInfo);
	}else if(1 == CliIptArgList[0][0]){
		ret = ApsExepReadPreExep(&stExepInfo);
	}else{
		CLI_PRINTF("input para %d is error\r\n", CliIptArgList[0][0]);
		return;
	}
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("ApsExepReadCurExep or ApsExepReadPreExep fail ret %d\r\n", ret);
		return;
	}
	CLI_PRINTF("rstType:%d cpu0:%d cpu1:%d rebootTick:%u\r\n", stExepInfo.oppleRstType,stExepInfo.rst0Type, stExepInfo.rst1Type, stExepInfo.tick);
	CLI_PRINTF("rstType:%s cpu0:%s cpu1:%s rebootTick:%u\r\n", oppRstType[stExepInfo.oppleRstType],cpuRstType[stExepInfo.rst0Type], cpuRstType[stExepInfo.rst1Type], stExepInfo.tick);
	if(COAP_RST == stExepInfo.oppleRstType){
		pstCoapPkt = (ST_COAP_PKT *)stExepInfo.data;
		CLI_PRINTF("nb tx succ num: %d\r\n", pstCoapPkt->g_iCoapNbTxSuc);
		CLI_PRINTF("nb tx fail num: %d\r\n", pstCoapPkt->g_iCoapNbTxFail);
		CLI_PRINTF("nb tx retry num: %d\r\n", pstCoapPkt->g_iCoapNbTxRetry);
		CLI_PRINTF("nb tx retry req num: %d\r\n", pstCoapPkt->g_iCoapNbTxReqRetry);
		CLI_PRINTF("nb tx retry rsp num: %d\r\n", pstCoapPkt->g_iCoapNbTxRspRetry);
		CLI_PRINTF("nb rx pkt num: %d\r\n", pstCoapPkt->g_iCoapNbRx);
		CLI_PRINTF("nb rx clound rsp num: %d\r\n", pstCoapPkt->g_iCoapNbRxRsp);
		CLI_PRINTF("nb rx clound req num: %d\r\n", pstCoapPkt->g_iCoapNbRxReq);
		CLI_PRINTF("nb rx dev req num: %d\r\n", pstCoapPkt->g_iCoapNbDevReq);
		CLI_PRINTF("nb rx dev rsp num: %d\r\n", pstCoapPkt->g_iCoapNbDevRsp);
		CLI_PRINTF("nb rx pkt unknow num: %d\r\n", pstCoapPkt->g_iCoapNbUnknow);
		CLI_PRINTF("nb ota rx pkt num: %d\r\n", pstCoapPkt->g_iCoapNbOtaRx);
		CLI_PRINTF("nb ota tx pkt num: %d\r\n", pstCoapPkt->g_iCoapNbOtaTx);
		CLI_PRINTF("nb ota tx pkt succ num: %d\r\n", pstCoapPkt->g_iCoapNbOtaTxSucc);
		CLI_PRINTF("nb ota tx pkt fail num: %d\r\n", pstCoapPkt->g_iCoapNbOtaTxFail);
		CLI_PRINTF("coap is busy: %d\r\n", pstCoapPkt->g_iCoapBusy);
		len = sizeof(ST_COAP_PKT);
		pstBc28Pkt = (ST_BC28_PKT *)&stExepInfo.data[len];
		CLI_PRINTF("rx pkt from msgqueue num:%d\r\n", pstBc28Pkt->iPktRxFromQue);
		CLI_PRINTF("tx pkt num:%d\r\n", pstBc28Pkt->iPktTx);
		CLI_PRINTF("tx pkt faild num:%d\r\n", pstBc28Pkt->iPktTxFail);
		CLI_PRINTF("tx pkt expire num:%d\r\n", pstBc28Pkt->iPktTxExpire);
		CLI_PRINTF("tx pkt succ num:%d\r\n", pstBc28Pkt->iPktTxSucc);
		CLI_PRINTF("rx NNMI pkt num:%d\r\n", pstBc28Pkt->iNnmiRx);
		CLI_PRINTF("pkt push suc num:%d\r\n", pstBc28Pkt->iPktPushSuc);
		CLI_PRINTF("pkt push fail num:%d\r\n", pstBc28Pkt->iPktPushFail);
	}
}

void CommandExepSet(void)
{
	ApsCoapWriteExepInfo(CliIptArgList[0][0]);
}

void CommandExepReport(void)
{
	int ret;
	ST_EXEP_INFO stExepInfo;
	
	if(0 == CliIptArgList[0][0]){
		ret = ApsExepReadCurExep(&stExepInfo);
	}else if(1 == CliIptArgList[0][0]){
		ret = ApsExepReadPreExep(&stExepInfo);
	}else{
		CLI_PRINTF("input para %d is error\r\n", CliIptArgList[0][0]);
		return;
	}
	ApsCoapExepReport(CHL_NB,NULL,CliIptArgList[0][0],&stExepInfo);
}

void CommandHeartDisParaGet(void)
{
	ST_DIS_HEART_SAVE_PARA stDisHeartPara;

	ApsCoapHeartDisParaGet(&stDisHeartPara);
	CLI_PRINTF("win:%d interval:%d\r\n", stDisHeartPara.udwDiscreteWin,stDisHeartPara.udwDiscreteInterval);
}

void CommandHeartDisParaSet(void)
{
	int ret;
	ST_DIS_HEART_SAVE_PARA stDisHeartPara;

	stDisHeartPara.udwDiscreteWin = CliIptArgList[0][0];
	stDisHeartPara.udwDiscreteInterval = CliIptArgList[1][0];
	ret = ApsCoapHeartDisParaSetToFlash(&stDisHeartPara);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("vApsCoapHeartDisParaSetToFlash fail ret %d\r\n", ret);
		return;
	}
	ApsCoapHeartDisParaSet(&stDisHeartPara);
}

void CommandHeartDisOffsetGet(void)
{
	CLI_PRINTF("hearDisOffset:%d\r\n",ApsCoapHeartDisOffsetSecondGet());
}

void CommandHeartDisTickGet(void)
{
	CLI_PRINTF("curTick:%d hearDisTick:%d\r\n",OppTickCountGet(),ApsCoapHeartDisTick());
}
