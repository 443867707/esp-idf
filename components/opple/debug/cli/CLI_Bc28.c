#include "cli-interpreter.h"
#include "DRV_Bc28.h"
#include "APP_LampCtrl.h"
#include "SVS_Para.h"
#include "SVS_Config.h"
#include "string.h"
#include "DRV_Nb.h"

extern void BC28_STRNCPY(char * dst, char * src, int len);

void CommandUeStatusGet(void);
void CommandConStateGet(void);
void CommandDevStateGet(void);
void CommandIotStateGet(void);
void CommandParaGet(void);
void CommandParaSet(void);
void CommandNbInfoGet(void);
void CommandNbCsqGet(void);
void CommandNbDevStateSet(void);
void CommandNbRcvBuffer(void);
void CommandNbSendBuffer(void);
void CommandNbNnmiGet(void);
void CommandNbDataSend(void);
void CommandNbDataRead(void);
void CommandNbLogLevelGet(void);
void CommandNbLogLevelSet(void);
void CommandNbCommand(void);
void CommandIotRegSet(void);
void CommandSetHaveData(void);
void CommandHardReset(void);
void CommandNbPktGet(void);
void CommandNbTxEnableSet(void);
void CommandNbTxEnableGet(void);
void CommandNbImeiGet(void);
void CommandNbIccidGet(void);
void CommandNbIccidImmGet(void);
void CommandNbImsidGet(void);
void CommandNbVerGet(void);
void CommandNbPidGet(void);
void CommandRegSet(void);
void CommandRegGet(void);
void CommandIotSet(void);
void CommandNbDiag(void);
void CommandResetInner(void);
void CommandNbCsearfcnSet(void);
void CommandEarfcnSet(void);
void CommandEarfcnGet(void);
void CommandNconfigGet(void);
void CommandApnSet(void);
void CommandApnGet(void);
void CommandCgattSet(void);
void CommandCfunSet(void);
void CommandIotAddrSet(void);
void CommandIotAddrGet(void);
void CommandBc28AbnormlSet(void);
void CommandBc28AbnormlGet(void);
void CommandHardResetStateGet(void);
void CommandDfotaStateGet(void);
void CommandBc28DisWinSet(void);
void CommandBc28DisWinGet(void);
void CommandBc28DisOffsetGet(void);
void CommandBc28DisEnableSet(void);
void CommandBc28DisEnableGet(void);
void CommandBc28DisTickGet(void);
void CommandBc28BandSet(void);
void CommandBc28BandGet(void);
void CommandBc28ParaSet(void);
void CommandBc28ParaGet(void);


const char* const nbArguments_devState[] = {
  "0.BC28_INIT,1.BC28_CGA,2.BC28_GIP,3.BC28_READY,4.BC28_ERR,5.BC28_TEST,6.BC28_UPDATE,7.BC28_RESET,8.BC28_WAITCGA"
};

const char* const nbArguments_send[] = {
  "send data len",
  "send data",
};
const char* const nbArguments_log[] = {
  "core:PROTOCOL,APPLICATION,SECURITY",
  "level:VERBOSE,NORMAL,WARNING,ERROR,NONE",
};
const char* const nbArguments_cmd[] = {
  "nbiot command"
};
const char* const nbArguments_reg[] = {
  "0.reg 1.dereg"
};
const char* const nbArguments_iotstate[] = {
	"-4 iot not work" \
	"-3 iot init"
	"-2 iot wait REGISTERNOTIFY" \
	"-1 start iot register" \
  "0 Register complete" \
"1 Deregister complete" \
"2 Update register complete" \
"3 Object observe complete" \
"4 Bootstrap complete" \
"5 5/0/3 resource observe complete" \
"6 Notify the device to receive update package URL" \
"7 Notify the device to receive update message" \
"9 Cancel object 19/0/0 observe"
};
const char* const nbArguments_earfcn[] = {
  "0.earfcn disable 1.earfcn enable",
  "earfcn<0-65535>, 0 auto select"
};
const char* const nbArguments_bc28[] = {
  "0.earfcn disable 1.earfcn enable",
  "earfcn<0-65535>, 0 auto select"
  "band:28,5,20,8,3,1"
};

#define FIRMWARE_NONE               	0
#define FIRMWARE_DOWNLOADING        	1
#define FIRMWARE_DOWNLOADED         	2
#define FIRMWARE_UPDATING           	3
#define FIRMWARE_FOTA_UPGRADE_REBOOT    4
#define FIRMWARE_UPDATE_SUCCESS     	5
#define FIRMWARE_UPDATE_OVER        	6
#define FIRMWARE_UPDATE_FAILED      	7

const char* const dfotaDesc[] = {
  "FIRMWARE_NONE",
  "FIRMWARE_DOWNLOADING",
  "FIRMWARE_DOWNLOADED",
  "FIRMWARE_UPDATING",
  "FIRMWARE_FOTA_UPGRADE_REBOOT",
  "FIRMWARE_UPDATE_SUCCESS",
  "FIRMWARE_UPDATE_OVER",
  "FIRMWARE_UPDATE_FAILED"
};

const char* const iotAddrArg_desc[] = {
	"COM_PLT:117.60.157.137,TEST_PLT:180.101.147.115"
};
extern ST_BC28_PKT   g_stBc28Pkt;


CommandEntry CommandTableBc28[] =
{
	CommandEntryActionWithDetails("ueStatus", CommandUeStatusGet, "", "get UE status...", NULL),
	CommandEntryActionWithDetails("conState", CommandConStateGet, "", "get con state...", NULL),
	CommandEntryActionWithDetails("gDevState", CommandDevStateGet, "", "get dev state...", NULL),
	CommandEntryActionWithDetails("sDevState",	CommandNbDevStateSet,	 "u", "set dev state...", nbArguments_devState),
	CommandEntryActionWithDetails("iotState", CommandIotStateGet, "", "get iot state...", nbArguments_iotstate),
	CommandEntryActionWithDetails("iotReg", CommandIotRegSet, "u", "set iot reg/dereg...", nbArguments_reg),
	CommandEntryActionWithDetails("rPara", 	  CommandParaGet,     "", "read para...", NULL),
	CommandEntryActionWithDetails("sPara",	  CommandParaSet,     "", "write para...", NULL),
	CommandEntryActionWithDetails("info",	  CommandNbInfoGet,	  "", "get nb info...", NULL),
	CommandEntryActionWithDetails("csq",	  CommandNbCsqGet,   "", "get nb rsrp...", NULL),
	CommandEntryActionWithDetails("nqmgr",	  CommandNbRcvBuffer,	 "", "get nqmgr...", NULL),
	CommandEntryActionWithDetails("nqmgs",	  CommandNbSendBuffer,  "", "get nb nqmgs...", NULL),
	CommandEntryActionWithDetails("nnmi",	  CommandNbNnmiGet,	 "", "get nb nnmi...", NULL),
	CommandEntryActionWithDetails("send",	  CommandNbDataSend,  "us", "send data...", nbArguments_send),
	CommandEntryActionWithDetails("read",	  CommandNbDataRead,  "", "read data...", NULL),
	CommandEntryActionWithDetails("logLevelSet",	  CommandNbLogLevelSet,  "ss", "log levele set...", nbArguments_log),
	CommandEntryActionWithDetails("logLevelGet",	  CommandNbLogLevelGet,  "", "log level get...", NULL),
	CommandEntryActionWithDetails("cmd",	  CommandNbCommand,  "s", "bc28 cmd...", nbArguments_cmd),
	CommandEntryActionWithDetails("setHaveData",	  CommandSetHaveData,  "u", "set data status...", NULL),
	CommandEntryActionWithDetails("hardReset",	  CommandHardReset,  "", "nbiot hard reset...", NULL),
	CommandEntryActionWithDetails("pkt",	  CommandNbPktGet,  "", "bc28 ptk...", NULL),
	CommandEntryActionWithDetails("txSet",	  CommandNbTxEnableSet,	"u", "tx enable set...", NULL),
	CommandEntryActionWithDetails("txGet",	  CommandNbTxEnableGet,	"", "tx enable get...", NULL),
	CommandEntryActionWithDetails("imeiGet",	  CommandNbImeiGet, "", "nbiot module imei get...", NULL),
	CommandEntryActionWithDetails("iccidGet",	  CommandNbIccidGet, "", "nbiot module iccid get...", NULL),
	CommandEntryActionWithDetails("imsiGet",	  CommandNbImsidGet, "", "nbiot module imsi get...", NULL),
	CommandEntryActionWithDetails("verGet",	  CommandNbVerGet, "", "nbiot module imsi get...", NULL),
	CommandEntryActionWithDetails("pidGet",   CommandNbPidGet, "", "nbiot module product id get...", NULL),
	CommandEntryActionWithDetails("regSet",	  CommandRegSet,	  "u", "set reg state...", NULL),
	CommandEntryActionWithDetails("regGet",   CommandRegGet,	  "", "get reg state...", NULL),
	CommandEntryActionWithDetails("iotSet", CommandIotSet, "u", "set iot state...", nbArguments_iotstate),
	CommandEntryActionWithDetails("diag", CommandNbDiag, "", "nbiot module diagnose...", NULL),
	CommandEntryActionWithDetails("innerReset", CommandResetInner, "", "nbiot inner hard reset...", NULL),
	CommandEntryActionWithDetails("csearfcn", CommandNbCsearfcnSet, "", "nbiot clear store earfcn...", NULL),
	CommandEntryActionWithDetails("earfcnSet", CommandEarfcnSet, "uu", "nbiot set earfcn...", nbArguments_earfcn),
	CommandEntryActionWithDetails("earfcnGet", CommandEarfcnGet, "", "nbiot get earfcn...", NULL),
	CommandEntryActionWithDetails("configGet", CommandNconfigGet, "", "nbiot get config...", NULL),
	CommandEntryActionWithDetails("apnSet", CommandApnSet, "s", "nbiot apn set...", NULL),
	CommandEntryActionWithDetails("apnGet", CommandApnGet, "", "nbiot apn get...", NULL),
	CommandEntryActionWithDetails("cgattSet", CommandCgattSet, "u", "nbiot cgatt set...", NULL),
	CommandEntryActionWithDetails("cfunSet", CommandCfunSet, "u", "nbiot cfun set...", NULL),
	CommandEntryActionWithDetails("iotAddrSet", CommandIotAddrSet, "s", "nbiot iot address set...", iotAddrArg_desc),
	CommandEntryActionWithDetails("iotAddrGet", CommandIotAddrGet, "", "nbiot iot address get...", NULL),
	CommandEntryActionWithDetails("abnormalSet", CommandBc28AbnormlSet, "u", "nbiot abnormal set...", NULL),
	CommandEntryActionWithDetails("abnormalGet", CommandBc28AbnormlGet, "", "nbiot abnormal get...", NULL),
	CommandEntryActionWithDetails("hardRstRsultGet", CommandHardResetStateGet, "", "nbiot hard reset state get...", NULL),
	CommandEntryActionWithDetails("dfotaStateGet", CommandDfotaStateGet, "", "nbiot dfota state get...", NULL),
	CommandEntryActionWithDetails("disParaSet", CommandBc28DisWinSet, "uu", "nbiot attach discrete para set...", NULL),
	CommandEntryActionWithDetails("disParaGet", CommandBc28DisWinGet, "", "nbiot attach discrete para get...", NULL),
	CommandEntryActionWithDetails("disOffsetGet", CommandBc28DisOffsetGet, "", "nbiot attach discrete num get...", NULL),
	CommandEntryActionWithDetails("disEnableGet", CommandBc28DisEnableGet, "", "nbiot attach discrete enable set...", NULL),
	CommandEntryActionWithDetails("disEnableSet", CommandBc28DisEnableSet, "u", "nbiot attach discrete enable get...", NULL),
	CommandEntryActionWithDetails("disTickGet", CommandBc28DisTickGet, "", "nbiot discrete tick get...", NULL),
	CommandEntryActionWithDetails("iccidImmGet",	  CommandNbIccidImmGet, "", "nbiot module iccid get immediately ...", NULL),
	CommandEntryActionWithDetails("bandGet", CommandBc28BandGet, "", "nbiot band get...", NULL),
	CommandEntryActionWithDetails("bandSet",	  CommandBc28BandSet, "u", "nbiot band set ...", NULL),
	CommandEntryActionWithDetails("bc28ParaGet", CommandBc28ParaGet, "", "bc28 para get...", nbArguments_bc28),
	CommandEntryActionWithDetails("bc28ParaSet",	  CommandBc28ParaSet, "uuu", "bc28 para set ...", nbArguments_bc28),
	CommandEntryTerminator()
};

void CommandUeStatusGet(void)
{
	ST_NEUL_DEV *pstNeulDev = NULL;
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(UESTATE_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(UESTATE_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent UESTATE_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}

	pstNeulDev = (ST_NEUL_DEV *)rArgv[0];
	CLI_PRINTF("IMEI:%s\r\n", pstNeulDev->imei);
	CLI_PRINTF("IMSI:%s\r\n", pstNeulDev->imsi);
	CLI_PRINTF("nccid:%s\r\n", pstNeulDev->nccid);
	CLI_PRINTF("pdpAddr0:%s\r\n", pstNeulDev->pdpAddr0);
	CLI_PRINTF("pdpAddr1:%s\r\n", pstNeulDev->pdpAddr1);
	CLI_PRINTF("rsrp:%d\r\n", pstNeulDev->rsrp);
	CLI_PRINTF("totalPower:%d\r\n", pstNeulDev->totalPower);
	CLI_PRINTF("txPower:%d\r\n", pstNeulDev->txPower);
	CLI_PRINTF("txTime:%d\r\n", pstNeulDev->txTime);
	CLI_PRINTF("rxTime:%d\r\n", pstNeulDev->rxTime);
	CLI_PRINTF("cellId:%d\r\n", pstNeulDev->cellId);
	CLI_PRINTF("signalEcl:%d\r\n", pstNeulDev->signalEcl);
	CLI_PRINTF("snr:%d\r\n", pstNeulDev->snr);
	CLI_PRINTF("earfcn:%d\r\n", pstNeulDev->earfcn);
	CLI_PRINTF("signalPci:%d\r\n", pstNeulDev->signalPci);
	CLI_PRINTF("rsrq:%d\r\n", pstNeulDev->rsrq);
	CLI_PRINTF("opmode:%d\r\n", pstNeulDev->opmode);

	ARGC_FREE(rArgc,rArgv);
}

void CommandConStateGet(void)
{
	U8 regStatus, conStatus;

	NeulBc28QueryNetstats(&regStatus, &conStatus);

	CLI_PRINTF("regStatus %d  conStatus %d \r\n", regStatus, conStatus);
	

}

char *devStateDesc[]={
	"BC28_INIT",
	"BC28_CGA",
	"BC28_GIP",
	"BC28_READY",
	"BC28_ERR",
	"BC28_TEST",
	"BC28_UPDATE",
	"BC28_RESET",
	"BC28_WAITCGA",
	"BC28_UNKNOW"
};
void CommandDevStateGet(void)
{
	CLI_PRINTF("dev state is %s\r\n", devStateDesc[NeulBc28GetDevState()]);
}

void CommandIotStateGet(void)
{
	signed char iotStatus;
	NeulBc28QueryIOTstats(&iotStatus);

	CLI_PRINTF("iot state is %d\r\n", iotStatus);
}

void CommandIotRegSet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;

	sArgv[0] = (char *)malloc(20);
	if(NULL == sArgv[0]){
		CLI_PRINTF("malloc error\r\n");
		return;
	}
	memset(sArgv[0],0,20);
	itoa(CliIptArgList[0][0],sArgv[0],10);
	sArgc = 1;
	
	ret = sendEvent(IOTREG_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(IOTREG_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("IOTREG_EVENT recv fail ret %d\r\n", ret);
	}
}

void CommandParaGet(void)
{
	ST_NB_CONFIG stConfigPara;

	/*
	ret = AppParaRead(APS_PARA_MODULE_LAMP, LAMPCONFIG_ID, (unsigned char *)&stConfigPara, &len);	
	if(0 != ret){
		CLI_PRINTF("CommandParaGet read %d %d fail %d\r\n", APS_PARA_MODULE_LAMP, LAMPCONFIG_ID, ret);
	}
	*/
	NeulBc28GetConfig(&stConfigPara);
	CLI_PRINTF("bc28 config band:%d\r\n" \
		"scram:%d\r\n" \
		"apn:%s\r\n" \
		"earfcn:%d\r\n",
		stConfigPara.band,
		stConfigPara.scram,
		stConfigPara.apn,
		stConfigPara.earfcn);
}

void CommandParaSet(void)
{
	int ret;
	/*
	ret = AppParaWrite(APS_PARA_MODULE_LAMP, LAMPCONFIG_ID, (unsigned char *)&g_stConfigParaDefault, sizeof(ST_CONFIG_PARA));
	if(OPP_SUCCESS != ret)
		CLI_PRINTF("CommandParaSet default config err %d\r\n", ret);

	memcpy(&g_stConfigPara, &g_stConfigParaDefault, sizeof(ST_CONFIG_PARA));
	*/
	ret = NeulBc28SetConfigToFlash(&g_stNbConfigDefault);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("NeulBc28SetConfigToFlash write fail %d\r\n", ret);
	}

	NeulBc28SetConfig(&g_stNbConfigDefault);
}

char *regDesc[] = {
	"Not registered, UE is not currently searching an operator to register to",
	"Registered, home network",
	"Not registered, but UE is currently trying to attach or searching an operator to register to",
	"Registration denied",
	"Unknown (e.g. out of E-UTRAN coverage)",
	"Registered, roaming",
	"invalid register"
};
char *conDesc[] = {
	"idle",
	"connect",
	"psm"
};
char *devDesc[] = {
"BC28_INIT",
"BC28_CGA",
"BC28_GIP",
"BC28_READY",
"BC28_ERR",
"BC28_TEST",
"BC28_UPDATE",
"BC28_RESET",
"BC28_WAITCGA",
"BC28_UNKNOW"
};
char *iotDesc[] = {
"Register complete",
"Deregister complete",
"Update register complete",
"Object observe complete",
"Bootstrap complete",
"5/0/3 resource observe complete",
"Notify the device to receive update package URL",
"Notify the device to receive update message",
"None",
"Cancel object 19/0/0 observe"
};
char *iotMinusDesc[] = {
	"IOT_START_REG",
	"IOT_WAIT_MSG",
	"IOT_INIT",
	"IOT_NOWORK",
};
/*
return      :  -1, input para error
		     -2, query imei error
		     -3, query imsi error
		     -4, query iccid error
		     -5, dev not ready
		     -6, query NUESTATS error
*/
void CommandNbInfoGet(void)
{
	ST_NEUL_DEV *pstNeulDev = NULL;
	signed char iotStatus;
	U8 regStatus, conStatus;
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	U8 ucCurSt, ucLastSt;
	
	ret = sendEvent(UESTATE_EVENT,RISE_STATE,sArgc,sArgv);
	/*[code review]SEM_WAIT_ALWAYS->some time*/
	ret = recvEvent(UESTATE_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	pstNeulDev = (ST_NEUL_DEV *)rArgv[0];
	//not attach noiot or query uestatus error
	if(ret == 0){
		CLI_PRINTF("IMEI:%s\r\n", pstNeulDev->imei);
		CLI_PRINTF("IMSI:%s\r\n", pstNeulDev->imsi);
		CLI_PRINTF("iccid:%s\r\n", pstNeulDev->nccid);	
		CLI_PRINTF("pdpAddr0:%s\r\n", pstNeulDev->pdpAddr0);
		CLI_PRINTF("pdpAddr1:%s\r\n", pstNeulDev->pdpAddr1);
		CLI_PRINTF("rsrp:%d\r\n", pstNeulDev->rsrp);
		CLI_PRINTF("totalPower:%d\r\n", pstNeulDev->totalPower);
		CLI_PRINTF("txPower:%d\r\n", pstNeulDev->txPower);
		CLI_PRINTF("txTime:%d\r\n", pstNeulDev->txTime);
		CLI_PRINTF("rxTime:%d\r\n", pstNeulDev->rxTime);
		CLI_PRINTF("cellId:%d\r\n", pstNeulDev->cellId);
		CLI_PRINTF("signalEcl:%d\r\n", pstNeulDev->signalEcl);
		CLI_PRINTF("snr:%d\r\n", pstNeulDev->snr);
		CLI_PRINTF("earfcn:%d\r\n", pstNeulDev->earfcn);
		CLI_PRINTF("signalPci:%d\r\n", pstNeulDev->signalPci);
		CLI_PRINTF("rsrq:%d\r\n", pstNeulDev->rsrq);
		CLI_PRINTF("opmode:%d\r\n", pstNeulDev->opmode);
	}else if(ret == -2){
		CLI_PRINTF("read imei error\r\n");
	}else if(ret == -3){
		CLI_PRINTF("IMEI:%s\r\n", pstNeulDev->imei);
		CLI_PRINTF("read imsi error\r\n");
	}else if(ret == -4){
		CLI_PRINTF("IMEI:%s\r\n", pstNeulDev->imei);
		CLI_PRINTF("IMSI:%s\r\n", pstNeulDev->imsi);
		CLI_PRINTF("read iccid error\r\n");
	}else if(ret < -4){
		CLI_PRINTF("IMEI:%s\r\n", pstNeulDev->imei);
		CLI_PRINTF("IMSI:%s\r\n", pstNeulDev->imsi);
		CLI_PRINTF("iccid:%s\r\n", pstNeulDev->nccid);
	}
	ARGC_FREE(rArgc,rArgv);
	
	NeulBc28QueryNetstats(&regStatus, &conStatus);
	CLI_PRINTF("regStatus %s \r\n\r\n", regDesc[regStatus]);
	CLI_PRINTF("conStatus %s \r\n", conDesc[conStatus]);

	NeulBc28QueryIOTstats(&iotStatus);
	if(iotStatus < 0 ||iotStatus > 9)
		CLI_PRINTF("iot state is %s\r\n", iotMinusDesc[abs(iotStatus)-1]);
	else
		CLI_PRINTF("iot state is %s\r\n", iotDesc[(U8)iotStatus]);

	CLI_PRINTF("nbiot dev state is %s\r\n", devDesc[NeulBc28GetDevState()]);
	CLI_PRINTF("nbiot tx status %d sendlen %d\r\n", GetDataStatus(),GetDataLength());
	NeulBc28GetFwState(&ucCurSt,&ucLastSt);
	if(ucCurSt <= FIRMWARE_UPDATE_FAILED || ucLastSt <= FIRMWARE_UPDATE_FAILED){
		CLI_PRINTF("DFOTA last:%s cur:%s\r\n", dfotaDesc[ucLastSt], dfotaDesc[ucCurSt]);	
	}
	
	return;
}

void CommandNbCsqGet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(CSQ_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(CSQ_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent CSQ_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}

	CLI_PRINTF("rsrp:%s,ber:%s\r\n", rArgv[0], rArgv[1]);
	
	ARGC_FREE(rArgc,rArgv);
	return;
}

void CommandNbDevStateSet(void)
{
	int devState;
	
	devState = CliIptArgList[0][0];
	NeulBc28SetDevState(devState);
}

void CommandNbRcvBuffer(void)
{
	char buffer[128] = {'\0'};
	NeulBc28GetUnrmsgCount(buffer, 128);
	CLI_PRINTF("%s", buffer);
}

void CommandNbSendBuffer(void)
{
	char buffer[128] = {'\0'};
	NeulBc28GetUnsmsgCount(buffer,128);
	CLI_PRINTF("%s", buffer);
}
void CommandNbNnmiGet(void)
{
	char buffer[128] = {'\0'};
	NeulBc28GetNnmi(buffer, 128);
	CLI_PRINTF("%s", buffer);

}

void CommandNbDataSend(void)
{
	NeulBc28SendCoapPaylaodByNmgs((const char*)&CliIptArgList[1][0],CliIptArgList[0][0]);
}

void CommandNbDataRead(void)
{
	char buffer[128] = {'\0'};
	NeulBc28ReadCoapMsgByNmgr(buffer, sizeof(buffer));
	CLI_PRINTF("%s", buffer);
}

void CommandNbLogLevelGet(void)
{
	//char buffer[128] = {'\0'};
	//NeulBc28LogLevelGet(buffer);
	//CLI_PRINTF("%s", buffer);
}

void CommandNbLogLevelSet(void)
{
	//NeulBc28LogLevelSet(&CliIptArgList[0][0],&CliIptArgList[1][0]);

}

void CommandNbCommand(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	sArgv[0] = (char *)malloc(1024);
	if(NULL == sArgv[0]){
		CLI_PRINTF("malloc error\r\n");
		return;
	}
	memset(sArgv[0],0,1024);
	strncpy(sArgv[0],(char *)&CliIptArgList[0][0],strlen((char *)&CliIptArgList[0][0]));
	sArgc = 1;
	ret = sendEvent(NBCMD_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(NBCMD_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent SETCDP_EVENT ret fail%d\r\n", ret);		
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	for(int i=0;i<rArgc;i++)
		CLI_PRINTF("%s", rArgv[i]);
	ARGC_FREE(rArgc,rArgv);
}

void CommandSetHaveData(void)
{
	SetDataStatus(CliIptArgList[1][0]);
}

void CommandHardReset(void)
{
	int ret;
	ret = NbiotHardReset(5000);
	if(0 == ret){
		CLI_PRINTF("reset fail\r\n");
	}else{
		CLI_PRINTF("reset ok\r\n");
	}
}
void CommandNbPktGet(void)
{
	MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
	CLI_PRINTF("rx pkt from msgqueue num:%d\r\n", g_stBc28Pkt.iPktRxFromQue);
	CLI_PRINTF("tx pkt num:%d\r\n", g_stBc28Pkt.iPktTx);
	CLI_PRINTF("tx pkt faild num:%d\r\n", g_stBc28Pkt.iPktTxFail);
	CLI_PRINTF("tx pkt expire num:%d\r\n", g_stBc28Pkt.iPktTxExpire);
	CLI_PRINTF("tx pkt succ num:%d\r\n", g_stBc28Pkt.iPktTxSucc);
	CLI_PRINTF("rx NNMI pkt num:%d\r\n", g_stBc28Pkt.iNnmiRx);
	CLI_PRINTF("pkt push suc num:%d\r\n", g_stBc28Pkt.iPktPushSuc);
	CLI_PRINTF("pkt push fail num:%d\r\n", g_stBc28Pkt.iPktPushFail);
	MUTEX_UNLOCK(g_stBc28Pkt.mutex);
}

void CommandNbTxEnableSet(void)
{
	MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
	g_stBc28Pkt.iTxEnable = CliIptArgList[0][0];
	MUTEX_UNLOCK(g_stBc28Pkt.mutex);	
}

void CommandNbTxEnableGet(void)
{
	MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
	CLI_PRINTF("tx enable:%d\r\n", g_stBc28Pkt.iTxEnable);
	MUTEX_UNLOCK(g_stBc28Pkt.mutex);	
}

void CommandNbImeiGet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(IMEI_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(IMEI_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent IMEI_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}

	CLI_PRINTF("imei:%s\r\n", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void CommandNbIccidGet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(ICCID_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(ICCID_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent ICCID_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}

	CLI_PRINTF("iccid:%s\r\n", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void CommandNbIccidImmGet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(ICCIDIMM_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(ICCIDIMM_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent ICCIDIMM_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}

	CLI_PRINTF("iccid:%s\r\n", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void CommandNbImsidGet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(IMSI_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(IMSI_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent IMSI_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}

	CLI_PRINTF("imsi:%s\r\n", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void CommandNbVerGet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(VER_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(VER_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent VER_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}

	CLI_PRINTF("ver:%s\r\n", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);

}

void CommandNbPidGet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(PID_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(PID_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent PID_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}

	CLI_PRINTF("Display Product Identification Information:\r\n%s\r\n", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);

}

void CommandNbCsearfcnSet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(CSEARFCN_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(CSEARFCN_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent CSEARFCN_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	ARGC_FREE(rArgc,rArgv);
	NeulBc28SetDevState(BC28_INIT);
}

void CommandRegSet(void)
{
	NeulBc28SetRegState(CliIptArgList[0][0]);
}
void CommandRegGet(void)
{
	CLI_PRINTF("reg:%d\r\n",NeulBc28GetRegState());
}
void CommandIotSet(void)
{
	NeulBc28SetIotState(CliIptArgList[0][0]);
	OppCoapIOTStateChg();
}

void CommandNbDiag(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	int rssi, ber;

	//product information
	ret = sendEvent(PID_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(PID_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("please check nbiot module false welding!!!\r\n");
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	ARGC_FREE(rArgc,rArgv);

	//bc28 imei
	ret = sendEvent(IMEI_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(IMEI_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("please check nbiot module itself, maybe nbiot module is broken\r\n");
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	ARGC_FREE(rArgc,rArgv);
	//sim card imsi
	ret = sendEvent(IMSI_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(IMSI_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("please check SIM card false welding\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	ARGC_FREE(rArgc,rArgv);
	//rssi
	ret = sendEvent(CSQ_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(CSQ_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent CSQ_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	rssi = atoi(rArgv[0]);
	ber = atoi(rArgv[1]);
	if(rssi == 99){
		CLI_PRINTF("no nbiot signal\r\n");		
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	ARGC_FREE(rArgc,rArgv);

	CLI_PRINTF("nbiot diag is ok, if it cannot attach to nbiot network please check antenna or other hardware problem\r\n");
}

void CommandResetInner(void)
{
	NbiotHardResetInner();
}

void CommandHardResetStateGet(void)
{
	CLI_PRINTF("reboot status:%s,result:%s\r\n", NeulBc28HardRestStateGet()==0?"REBOOTING":"REBOOTED",
		NeulBc28HardRestResultGet()==0?"SUCCESS":"FAIL");
}

void CommandDfotaStateGet(void)
{	
	U8 ucCurSt,ucLastSt;

	NeulBc28GetFwState(&ucCurSt,&ucLastSt);
	if(ucCurSt > FIRMWARE_UPDATE_FAILED || ucLastSt > FIRMWARE_UPDATE_FAILED){
		CLI_PRINTF("NeulBc28GetFwState last:%d cur:%d fail\r\n", ucLastSt, ucCurSt);
		return;
	}
	CLI_PRINTF("DFOTA last:%s cur:%s\r\n", dfotaDesc[ucLastSt], dfotaDesc[ucCurSt]);	
}

void CommandEarfcnSet(void)
{	
	NeulBc28SetEarfcnToRam(CliIptArgList[0][0],CliIptArgList[1][0]);
}

void CommandEarfcnGet(void)
{
	U8 enable;
	U16 earfcn;

	NeulBc28GetEarfcnFromRam(&enable,&earfcn);

	CLI_PRINTF("enable:%d earfcn:%d\r\n", enable, earfcn);
}

void CommandNconfigGet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(NCONFIG_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(NCONFIG_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 == ret){
		CLI_PRINTF("recvEvent NCONFIG_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}

	CLI_PRINTF("%s", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void CommandApnSet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;

	sArgv[0] = (char *)malloc(APN_NAME_LEN);
	if(NULL == sArgv[0]){
		CLI_PRINTF("malloc error\r\n");
		return;
	}
	memset(sArgv[0],0,APN_NAME_LEN);
	strncpy(sArgv[0],(char *)&CliIptArgList[0][0],APN_NAME_LEN);
	sArgc = 1;
	ret = sendEvent(CGDCONTSET_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(CGDCONTSET_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent CGDCONTSET_EVENT ret fail %d\r\n", ret);
	}

	ARGC_FREE(rArgc,rArgv);
}

void CommandApnGet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;

	ret = sendEvent(CGDCONTGET_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(CGDCONTGET_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent CGDCONTSET_EVENT ret fail %d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	CLI_PRINTF("%s", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void CommandCgattSet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;

	sArgv[0] = (char *)malloc(20);
	if(NULL == sArgv[0]){
		CLI_PRINTF("malloc error\r\n");
		return;
	}
	memset(sArgv[0],0,20);
	itoa(CliIptArgList[0][0],sArgv[0],10);
	sArgc = 1;
	
	ret = sendEvent(CGATTSET_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(CGATTSET_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("CGATTSET_EVENT recv fail ret %d\r\n", ret);
	}
	ARGC_FREE(rArgc,rArgv);
}


void CommandCfunSet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;

	sArgv[0] = (char *)malloc(20);
	if(NULL == sArgv[0]){
		CLI_PRINTF("malloc error\r\n");
		return;
	}
	memset(sArgv[0],0,20);
	itoa(CliIptArgList[0][0],sArgv[0],10);
	sArgc = 1;
	
	ret = sendEvent(CFUNSET_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(CFUNSET_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("CFUNSET_EVENT recv fail ret %d\r\n", ret);
	}
	ARGC_FREE(rArgc,rArgv);
}

void CommandIotAddrSet(void)
{
	int ret;
	ST_IOT_CONFIG stIotConfig;
	
	if(!is_ipv4_address((char *)&CliIptArgList[0][0])){
		CLI_PRINTF("CommandIotAddrSet input ipv4 address format error\r\n");
		return;
	}
	OppCoapIOTGetConfig(&stIotConfig);

	BC28_STRNCPY(stIotConfig.ocIp, (char *)&CliIptArgList[0][0], sizeof(stIotConfig.ocIp));
	ret = OppCoapIOTSetConfigToFlash(&stIotConfig);
	if(0 != ret){
		CLI_PRINTF("OppCoapIOTSetConfigToFlash fail ret %d\r\n", ret);
		return;
	}
	OppCoapIOTSetConfig(&stIotConfig);
}

void CommandIotAddrGet(void)
{
	ST_IOT_CONFIG stIotConfig;

	OppCoapIOTGetConfig(&stIotConfig);
	CLI_PRINTF("addr:%s,port:%d\r\n", stIotConfig.ocIp, stIotConfig.ocPort);	
}

void CommandBc28AbnormlSet(void)
{
	NeulBc28SetSupportAbnormal(CliIptArgList[0][0]);

}

void CommandBc28AbnormlGet(void)
{
	CLI_PRINTF("support abnormal:%d\r\n",NeulBc28GetSupportAbnormal());
}

void CommandBc28DisWinSet(void)
{
	int ret;
	ST_DISCRETE_SAVE_PARA stDisSavePara;

	stDisSavePara.udwDiscreteWin = CliIptArgList[0][0];
	stDisSavePara.udwDiscreteInterval= CliIptArgList[1][0];
	ret = NeulBc28SetDiscreteParaToFlash(&stDisSavePara);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("NeulBc28SetDiscreteParaToFlash fai ret %d\r\n",ret);
	}
	NeulBc28SetDiscretePara(&stDisSavePara);
}

void CommandBc28DisWinGet(void)
{
	int ret;
	ST_DISCRETE_SAVE_PARA stDisSavePara;
	
	NeulBc28GetDiscretePara(&stDisSavePara);
	CLI_PRINTF("discrete window:%d intrval:%d\r\n",stDisSavePara.udwDiscreteWin,stDisSavePara.udwDiscreteInterval);
}

void CommandBc28DisOffsetGet(void)
{
	CLI_PRINTF("discrete offset:%d\r\n",NeulBc28DisOffsetSecondGet());
}

void CommandBc28DisEnableSet(void)
{
	NeulBc28SetDisEnable(CliIptArgList[0][0]);
}

void CommandBc28DisEnableGet(void)
{
	CLI_PRINTF("discrete enable:%d\r\n",NeulBc28GetDisEnable());
}

void CommandBc28DisTickGet(void)
{
	CLI_PRINTF("curTick:%d discrete tick:%d\r\n", OppTickCountGet(),NeulBc28AttachDisTick());
}

void CommandBc28BandSet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;

	if(CliIptArgList[0][0] != 5 
		&& CliIptArgList[0][0] != 8 
		&& CliIptArgList[0][0] != 3 
		&& CliIptArgList[0][0] != 28
		&& CliIptArgList[0][0] != 20
		&& CliIptArgList[0][0] != 1){
		CLI_PRINTF("band should be [28,5,20,8,3,1]\r\n");
		return;
	}

	sArgv[0] = (char *)malloc(20);
	if(NULL == sArgv[0]){
		CLI_PRINTF("malloc error\r\n");
		return;
	}
	memset(sArgv[0],0,20);
	itoa(CliIptArgList[0][0],sArgv[0],10);
	sArgc = 1;
	
	ret = sendEvent(BANDSET_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(BANDSET_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("BANDSET_EVENT recv fail ret %d please check cfun=0\r\n", ret);
	}
	ARGC_FREE(rArgc,rArgv);
}

void CommandBc28BandGet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;

	ret = sendEvent(BANDGET_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(BANDGET_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("BANDGET_EVENT recv fail ret %d\r\n", ret);
		return;
	}
	CLI_PRINTF("band:%s\r\n", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void CommandBc28ParaSet(void)
{	
	if(CliIptArgList[2][0] != 5 
		&& CliIptArgList[2][0] != 8 
		&& CliIptArgList[2][0] != 3 
		&& CliIptArgList[2][0] != 28
		&& CliIptArgList[2][0] != 20
		&& CliIptArgList[2][0] != 1){
		CLI_PRINTF("band should be [28,5,20,8,3,1]\r\n");
		return;
	}

	NeulBc28SetParaToRam(CliIptArgList[0][0],CliIptArgList[1][0],CliIptArgList[2][0]);
}

void CommandBc28ParaGet(void)
{
	U8 enable;
	U16 earfcn,band;

	NeulBc28GetParaFromRam(&enable,&earfcn,&band);

	CLI_PRINTF("enable:%d earfcn:%d band:%d\r\n", enable, earfcn, band);
}

