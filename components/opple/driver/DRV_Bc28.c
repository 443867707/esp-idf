/*!
    \file  neul_bc28.c
    \brief NEUL BC28 IoT model driver 
*/

/*
    Copyright (C) 2016 GigaDevice

    2016-10-19, V1.0.0, demo for GD32F4xx
*/
#include <Includes.h>
#include <string.h>
#include <stdlib.h>
#include "DRV_Bc28.h"
#include "DRV_Led.h"
#include "DRV_Nb.h"
#include "DEV_Utility.h"
#include "SVS_Opple.h"
#include "OPP_Debug.h"
#include "SVS_MsgMmt.h"
#include "SVS_Para.h"
#include "SVS_Log.h"
#include "SVS_Config.h"
#include "APP_LampCtrl.h"
#include <string.h>
#include "opple_main.h"
#include "esp_wifi.h"

U8 g_ucLastConState = 0, g_ucConState = 0;
U8 g_ucLastPsmState = 0, g_ucPsmState = 0;
U8 g_ucLastRegState = INVALID_REG, g_ucRegState = INVALID_REG;
signed char g_cLastIOTState = IOT_NOWORK, g_cIOTState = IOT_NOWORK;
U8 m_ucCoapIsBusy = 0;
U8 m_ucUdpIsBusy = 0;
U8 m_ucRegNotify = 0;

static char neul_bc95_buf[NEUL_MAX_BUF_SIZE];
static char neul_bc95_wbuf[NEUL_MAX_BUF_SIZE];
static char neul_bc95_tmpbuf[NEUL_MAX_BUF_SIZE];

static U8 m_ucProtoType = UNKNOW_PRO;
static U8 g_ucLastNbiotDevState = BC28_INIT, g_ucNbiotDevState = BC28_INIT;

static T_MUTEX  g_stNbConfigMutex;
static T_MUTEX  g_stDevStateMutex;
static T_MUTEX  g_stLastDevStateMutex;
static T_MUTEX  g_stConStateMutex;
static T_MUTEX  g_stLastConStateMutex;
static T_MUTEX  g_stRegStateMutex;
static T_MUTEX  g_stLastRegStateMutex;
static T_MUTEX  g_stPsmStateMutex;
static T_MUTEX  g_stLastPsmStateMutex;
static T_MUTEX  g_stIotStateMutex;
static T_MUTEX  g_stLastIotStateMutex;
static T_MUTEX  g_stRegNtyMutex;
static T_MUTEX  g_stRxMutex;


static int g_iAtRebooted = 0;    /*0, rebooting 1,rebooted*/
ST_HARD_RESET g_stHardReset = {
	.iHardRebooted = REBOOTING,
	.iResult = REBOOT_OK,
};

ST_BC28_PKT   g_stBc28Pkt = {
	.iPktRxFromQue = 0,
	.iPktTx = 0,
	.iPktTxSucc = 0,	
	.iPktTxFail = 0,
	.iPktTxExpire = 0,
	.iNnmiRx = 0,
	.iPktPushSuc = 0,
	.iPktPushFail = 0,
};
static ST_FW_ST g_stFwSt = {
	.ucFwState = FIRMWARE_NONE,
	.ucFwLastState = FIRMWARE_NONE,
};

static ST_SUPPORT_ABNORMAL_PARA g_stSupportAbnormal = {
	.udwAbnormalH = ABNORMAL_PROTECT_ENABLE_H,
	.udwAbnormalL = ABNORMAL_PROTECT_ENABLE_L
};
static ST_BC28_RAM_PARA g_stBc28RamPara = {
	.enableH = EARFCN_DISABLE_H,
	.enableL = EARFCN_DISABLE_L,
	.earfcn = 0,
	.band = 0,
};
static ST_ATTACH_TIME g_stAttachTime = {
	.startTick = 0,
	.endTick = 0,
};
static ST_DISCRETE_SAVE_PARA g_stDisParaDefault = {
	.udwDiscreteWin = DIS_DEFAULT_WINDOWS,
	.udwDiscreteInterval = DIS_DEFAULT_INTERVAL,
};
static ST_DISCRETE_PARA g_stDisPara;

static ST_DISCRETE_ENABLE g_stDisEnable = {
	.enableH = DIS_ENABLE_H,
	.enableL = DIS_ENABLE_L,
};

int uart_data_read(char *buf, int maxrlen, int mode, int timeout);
int uart_data_write(char *buf, int writelen, int mode);
static int NeulBc28StrToHex(const char *bufin, int len, char *bufout);
static int NeulBc28QueryNccid(char *buf);
static int NeulBc28QueryImsi(char *buf);
static int NeulBc28QueryImei(char *buf);
static int NeulBc28QueryPid(char *buf);
void BC28_STRNCPY(char * dst, char * src, int len);
static int NeulBc28SetFwState(U8 ucCurSt);
static int NeulBc28Abnormal(void);
static int NeulBc28NconfigGet(char *buf, int len);
static int NeulBc28Cgatt(int onOff);
static int NeulBc28ApnConfigSet(char * apn);
static int NeulBc28ApnConfigGet(char * buf, int len);
static int NeulBc28OnOff(int onoff);
static int NeulBc28SetBand(int band);
static int NeulBc28GetBand(char *buf);
char *strrpl(char *in, char *out, int outlen, const char *src, char *dst);

static neul_dev_operation_t neul_ops = {
    NbiotDataRead/*uart_data_read*/,
    uart_data_write,
    NULL,
    NULL,
    NULL,
    NULL,
};
static remote_info udp_socket[NEUL_MAX_SOCKETS] = {
	{0, -1, {0}}
};
static neul_dev_info_t neul_dev = {
	{'\0'},
	{'\0'},
	{'\0'},
	{'\0'},
	{'\0'},
    neul_bc95_buf,
    neul_bc95_wbuf,
    0,
    udp_socket,
    &neul_ops,
    {0}
};
ST_NB_CONFIG g_stNbConfigDefault={
	.band = 5,
	.scram = 1,
	.apn = "psm0.eDRX0.ctnb",
	.earfcn = 0,
};

ST_NB_CONFIG g_stNbConfig;
ST_TX_DATA g_stTxData = {
	.haveData = 0,
	.tick = 0,
	.data = {0},
	.len = 0,
};


ST_EVENT g_astEvent[MAX_EVENT]={
	{NULL,NULL,SETCDP_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,SETREGMOD_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,IOTREG_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,TIME_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,UESTATE_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,NBCMD_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,CSQ_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,VER_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,RMSG_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,SMSG_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,IMEI_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,ICCID_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,IMSI_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,PID_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,CSEARFCN_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,NCONFIG_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,CGDCONTSET_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,CGDCONTGET_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,CGATTSET_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,CFUNSET_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,ICCIDIMM_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,BANDSET_EVENT,0,0,{NULL},0,0,{NULL}},
	{NULL,NULL,BANDGET_EVENT,0,0,{NULL},0,0,{NULL}},
};

/* ============================================================
func name   :  initEvent
discription :  init event
               
param       :  
return      :  0
Revision    : 
=============================================================== */
static int initEvent()
{
	int i = 0;
	for(i=0;i<MAX_EVENT;i++){
		MUTEX_CREATE(g_astEvent[i].mutex);
		g_astEvent[i].sem = xSemaphoreCreateBinary();
		if(NULL == g_astEvent[i].sem){
			//printf("initEvent OS_SEMAPHORE_CREATE %d error\r\n", i);
		}
	}

	return 0;
}


/* ============================================================
func name   :  sendEvent
discription :  send event to event process
               
param       :  event: event type, state: event state, argc: para number, argv: parameter 
return      :  0:success
Revision    : 
=============================================================== */
int sendEvent(int event, int state, int argc, char *argv[])
{
	int i = 0;

	//printf("sendEvent event %d state %d argc %d\r\n", event, state, argc);
	MUTEX_LOCK(g_astEvent[event].mutex,MUTEX_WAIT_ALWAYS);
	for(i=0;i<argc;i++){
		//printf("sendEvent copy para %d\r\n", i);
		g_astEvent[event].reqArgv[i] = argv[i];
	}		
	g_astEvent[event].state = state;
	MUTEX_UNLOCK(g_astEvent[event].mutex);

	return 0;
}

/* ============================================================
func name   :  recvEvent
discription :  receive special event
               
param       :  input para: event: event type, timeout: wait time
		      output para: argc: para number, argv: parameter, 
return      :  0:success
Revision    : 
=============================================================== */
int recvEvent(int event, int *argc, char *argv[], int timeout)
{
	int i;
	int ret = 0;
	
	while(1){
		BaseType_t r = OS_SEMAPHORE_TAKE(g_astEvent[event].sem,timeout);
		if (r == pdFALSE){
			//printf("take event %d  timeout\r\n", event);
			return -1;
		}
		MUTEX_LOCK(g_astEvent[event].mutex,MUTEX_WAIT_ALWAYS);
		if(g_astEvent[event].state == OVER_STATE){
			//printf("recvEvent event %d OVER_STATE ret %d argc %d\r\n", event,g_astEvent[event].retCode,g_astEvent[event].rspArgc);
			*argc = g_astEvent[event].rspArgc;
			for(i=0;i<g_astEvent[event].rspArgc;i++){
				argv[i] = g_astEvent[event].rspArgv[i];
			}
			MUTEX_UNLOCK(g_astEvent[event].mutex);
			return g_astEvent[event].retCode;
		}
		MUTEX_UNLOCK(g_astEvent[event].mutex);
	}
	//[code review] delete to test
	//MUTEX_UNLOCK(g_astEvent[event].mutex);
	return ret;
}

/* ============================================================
func name   :  EventProc
discription :  process event
               
param       :   
return      :  0:success
Revision    : 
=============================================================== */
static int EventProc()
{
	int i = 0;
	int ret;
	//int argc = 0;

	for(i=0; i<MAX_EVENT; i++){
		MUTEX_LOCK(g_astEvent[i].mutex,MUTEX_WAIT_ALWAYS);
		if(RISE_STATE == g_astEvent[i].state){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "EventProc %d RISE_STATE\r\n", i);
			if(SETCDP_EVENT == g_astEvent[i].event){
				ret = NeulBc28SetCdpServer(g_astEvent[i].reqArgv[0]);
				g_astEvent[i].retCode = ret;
				g_astEvent[i].rspArgc = 0;
			}else if(SETREGMOD_EVENT == g_astEvent[i].event){
				ret = NeulBc28HuaweIotSetRegmod(atoi(g_astEvent[i].reqArgv[0]));
				g_astEvent[i].retCode = ret;
				g_astEvent[i].rspArgc = 0;
			}else if(IOTREG_EVENT == g_astEvent[i].event){
				ret = NeulBc28HuaweiIotRegistration(atoi(g_astEvent[i].reqArgv[0]));
				g_astEvent[i].retCode = ret;
				g_astEvent[i].rspArgc = 0;
			}else if(TIME_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(36);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,36);
				ret = NeulBc28QueryClock(g_astEvent[i].rspArgv[0]);
				g_astEvent[i].retCode = ret;	
				g_astEvent[i].rspArgc = 1;
			}else if(UESTATE_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(sizeof(ST_NEUL_DEV));
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,sizeof(ST_NEUL_DEV));
				ret = NeulBc28QueryUestats((ST_NEUL_DEV *)g_astEvent[i].rspArgv[0]);
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;
			}
			else if(NBCMD_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(NEUL_MAX_BUF_SIZE);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,NEUL_MAX_BUF_SIZE);
				ret = NeulBc28Command(g_astEvent[i].reqArgv[0],g_astEvent[i].rspArgv[0],NEUL_MAX_BUF_SIZE);
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;
			}
			else if(CSQ_EVENT == g_astEvent[i].event){
				int rsrp,ber;
				char buf[128] ={0};
				g_astEvent[i].rspArgv[0] = (char *)malloc(128);
				if(NULL == g_astEvent[i].rspArgv[0]){
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				g_astEvent[i].rspArgv[1] = (char *)malloc(128);
				if(NULL == g_astEvent[i].rspArgv[1]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				ret = NeulBc28QueryCsq(&rsrp,&ber);
				itoa(rsrp,buf,10);
				BC28_STRNCPY(g_astEvent[i].rspArgv[0], buf, 128);
				itoa(ber,buf,10);
				BC28_STRNCPY(g_astEvent[i].rspArgv[1], buf, 128);
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 2;
			}
			else if(VER_EVENT == g_astEvent[i].event){
				char aucNbVer[256] = {'\0'};
				
				g_astEvent[i].rspArgv[0] = (char *)malloc(256);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,256);
				if(strlen(neul_dev.revision)){
					BC28_STRNCPY(aucNbVer, neul_dev.revision+2, strlen(neul_dev.revision)-6);
					strrpl(aucNbVer,g_astEvent[i].rspArgv[0],256,"\r\n"," ");
					ret = 0;
				}else{
					ret = NeulBc28GetVersion(g_astEvent[i].rspArgv[0],256);
				}
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;
			}
			else if(RMSG_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(128);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,128);
				ret = NeulBc28GetUnrmsgCount(g_astEvent[i].rspArgv[0], 128);
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;				

			}
			else if(SMSG_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(128);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,128);
				ret = NeulBc28GetUnsmsgCount(g_astEvent[i].rspArgv[0],128);
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;				
			}else if(IMEI_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(128);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,128);
				if(strlen((char *)neul_dev.imei)){
					BC28_STRNCPY(g_astEvent[i].rspArgv[0],(char *)neul_dev.imei,128);
					ret = 0;
				}else{
					ret = NeulBc28QueryImei(g_astEvent[i].rspArgv[0]);
				}
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;				
				
			}else if(ICCID_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(128);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,128);
				if(strlen((char *)neul_dev.nccid)){
					BC28_STRNCPY(g_astEvent[i].rspArgv[0],(char *)neul_dev.nccid,128);
					ret = 0;
				}else{			
					ret = NeulBc28QueryNccid(g_astEvent[i].rspArgv[0]);
				}
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;				
			}else if(ICCIDIMM_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(128);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,128);
				ret = NeulBc28QueryNccid(g_astEvent[i].rspArgv[0]);
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;				
			}else if(IMSI_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(128);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,128);
				if(strlen((char *)neul_dev.imsi)){
					BC28_STRNCPY(g_astEvent[i].rspArgv[0],(char *)neul_dev.imsi,128);
					ret = 0;
				}else{			
					ret = NeulBc28QueryImsi(g_astEvent[i].rspArgv[0]);
				}				
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;
			}else if(PID_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(128);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,128);
				ret = NeulBc28QueryPid(g_astEvent[i].rspArgv[0]);
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;
			}
			else if(CSEARFCN_EVENT == g_astEvent[i].event){
				ret = NeulBc28Abnormal();
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 0;
			}
			else if(NCONFIG_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(1024);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,1024);
				ret = NeulBc28NconfigGet(g_astEvent[i].rspArgv[0], 1024);
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;
			}
			else if(CGDCONTSET_EVENT == g_astEvent[i].event){
				ret = NeulBc28ApnConfigSet(g_astEvent[i].reqArgv[0]);
				g_astEvent[i].retCode = ret;
				g_astEvent[i].rspArgc = 0;
			}
			else if(CGDCONTGET_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(1024);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,1024);
				ret = NeulBc28ApnConfigGet(g_astEvent[i].rspArgv[0],1024);
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;
			}
			else if(CGATTSET_EVENT == g_astEvent[i].event){
				ret = NeulBc28Cgatt(atoi(g_astEvent[i].reqArgv[0]));
				g_astEvent[i].retCode = ret;
				g_astEvent[i].rspArgc = 0;
			}
			else if(CFUNSET_EVENT == g_astEvent[i].event){
				ret = NeulBc28OnOff(atoi(g_astEvent[i].reqArgv[0]));
				g_astEvent[i].retCode = ret;
				g_astEvent[i].rspArgc = 0;
			}
			else if(BANDSET_EVENT == g_astEvent[i].event){
				ret = NeulBc28SetBand(atoi(g_astEvent[i].reqArgv[0]));
				g_astEvent[i].retCode = ret;
				g_astEvent[i].rspArgc = 0;
			}
			else if(BANDGET_EVENT == g_astEvent[i].event){
				g_astEvent[i].rspArgv[0] = (char *)malloc(128);
				if(NULL == g_astEvent[i].rspArgv[0]){
					ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
					MUTEX_UNLOCK(g_astEvent[i].mutex);
					continue;
				}
				memset(g_astEvent[i].rspArgv[0],0,128);
				ret = NeulBc28GetBand(g_astEvent[i].rspArgv[0]);
				g_astEvent[i].retCode = ret;				
				g_astEvent[i].rspArgc = 1;
			}
			else{
				g_astEvent[i].retCode = -1;				
				g_astEvent[i].rspArgc = 0;
				printf("unknow event %d\r\n",i);
			}
			ARGC_FREE(g_astEvent[i].reqArgc,g_astEvent[i].reqArgv);
			g_astEvent[i].state = OVER_STATE;
			MUTEX_UNLOCK(g_astEvent[i].mutex);
			OS_SEMAPHORE_GIVE(g_astEvent[i].sem);
			//printf("EventProc OVER_STATE\r\n");
			continue;
		}		
		MUTEX_UNLOCK(g_astEvent[i].mutex);
	}
	return 0;
}

/* ============================================================
func name   :  GetDataStatus
discription :  get data buffer status
               
param       :   
return      :  buffer status: 0, override write 1,  not allowed override 
Revision    : 
=============================================================== */
int GetDataStatus()
{
	MUTEX_LOCK(g_stTxData.mutex,MUTEX_WAIT_ALWAYS);
	int status = g_stTxData.haveData;
	MUTEX_UNLOCK(g_stTxData.mutex);
	return status;
}

/* ============================================================
func name   :  GetDataLength
discription :  get data buffer length
               
param       :   
return      :  data buffer length
Revision    : 
=============================================================== */
int GetDataLength()
{
	MUTEX_LOCK(g_stTxData.mutex,MUTEX_WAIT_ALWAYS);
	int length = g_stTxData.len;
	MUTEX_UNLOCK(g_stTxData.mutex);
	return length;
}
/* ============================================================
func name   :  SetDataStatus
discription :  get data buffer length
               
param       :   input: haveData, 0:no data, 1:have data
return      :  data buffer length
Revision    : 
=============================================================== */
int SetDataStatus(int haveData)
{
	MUTEX_LOCK(g_stTxData.mutex,MUTEX_WAIT_ALWAYS);
	g_stTxData.haveData = haveData;
	MUTEX_UNLOCK(g_stTxData.mutex);
	return 0;

}

/* ============================================================
func name   :  TxDataProc
discription :  send data in data buffer
               
param       :   
return      :  0, success
Revision    : 
=============================================================== */
static int TxDataProc()
{
	int ret;

	MUTEX_LOCK(g_stTxData.mutex,MUTEX_WAIT_ALWAYS);
	if(g_stTxData.haveData){
		if(0 == g_stTxData.tick){
			g_stTxData.tick = OppTickCountGet();
		}else if(OppTickCountGet() - g_stTxData.tick > TXDATA_DEFAULT_TO){
			g_stTxData.haveData = 0;
			g_stTxData.tick = 0;
			MUTEX_UNLOCK(g_stTxData.mutex);

			MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
			g_stBc28Pkt.iPktTxExpire++;
			MUTEX_UNLOCK(g_stBc28Pkt.mutex);
			return 0;
		}
		
		ret = NeulBc28SendCoapPaylaod(g_stTxData.data,g_stTxData.len);
		//send succ or send packet bigger than mtu
		if(0 == ret || -1 == ret){
			g_stTxData.haveData = 0;
			g_stTxData.tick = 0;
		}
		//g_stTxData.haveData = 0;
	}
	MUTEX_UNLOCK(g_stTxData.mutex);

	return 0;
}

static void RxDataProc()
{
	char outbuf[NEUL_MAX_BUF_SIZE] = {0};
	//NbiotDataRead(outbuf, NEUL_MAX_BUF_SIZE, 0, 0);
	NbiotDataRead(outbuf, NEUL_MAX_BUF_SIZE, 0, 10);
}


/*split URC msg*/
static int UrcMsgProc(char *inBuffer,int bufLen)
{
	char *str = NULL;
	int temp = 0;
	U8 con, reg, psm, iot, lastIot;
	MSG_ITEM_T item;
	static char rx_tmpbuf[NEUL_MAX_BUF_SIZE] = {0};
	static char outBuffer[NEUL_MAX_BUF_SIZE] = {0};	
	static int readlen = 0;
	int inLen = bufLen;
	int ret = 0,ret1 = 0;
	
	while(1){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,  "len:%d,%s",inLen,inBuffer);
		if(!strstr(inBuffer,"\r\n+CSCON:") 
			&& !strstr(inBuffer,"\r\n+CEREG:") 
			&& !strstr(inBuffer,"\r\n+NPSMR:")
			&& !strstr(inBuffer,"\r\nREGISTERNOTIFY")
			&& !strstr(inBuffer,"\r\n+QLWEVTIND=")
			&& !strstr(inBuffer,"\r\n+QLWEVTIND:")
			&& !strstr(inBuffer,"\r\nREBOOT_CAUSE_APPLICATION_WATCHDOG")
			&& !strstr(inBuffer,"\r\nREBOOT_CAUSE_SECURITY_PMU_POWER_ON_RESET")
			&& !strstr(inBuffer,"\r\nREBOOT_CAUSE_APPLICATION_AT")
			&& !strstr(inBuffer,"\r\nREBOOT_CAUSE_SECURITY_RESET_PIN")
			&& !strstr(inBuffer,"\r\nREBOOT_CAUSE_SECURITY_FOTA_UPGRADE")
			&& !strstr(inBuffer,"\r\nFIRMWARE DOWNLOADING")
			&& !strstr(inBuffer,"\r\nFIRMWARE DOWNLOADED")
			&& !strstr(inBuffer,"\r\nFIRMWARE UPDATING")
			&& !strstr(inBuffer,"\r\nFIRMWARE UPDATE SUCCESS")
			&& !strstr(inBuffer,"\r\nFIRMWARE UPDATE FAILED")
			&& !strstr(inBuffer,"\r\nFIRMWARE UPDATE OVER")
			&& !strstr(inBuffer,"\r\n+NNMI:")){
			break;
		}
		str = strstr(inBuffer, "\r\n+CSCON:");
		if(str)
		{
			ret = 0;
			ret1 = 0;
			if(strchr(str, ',')){
				ret1 = sscanf(str, "\r\n+CSCON:%d,%hhu%*[\r\n]", &temp, &con);
				if(2 != ret1){
					ret = sscanf(str, "\r\n+CSCON:%hhu%*[\r\n]", &con);
				}
			}
			else
				ret = sscanf(str, "\r\n+CSCON:%hhu%*[\r\n]", &con);
			if(1 == ret || 2 == ret1){
				DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "\r\n+++++++g_ucConState %d+++++++\r\n", con);	
				NeulBc28SetConState(con);
			}

			URC_FILTER(inBuffer,inLen,str,"\r\n+CSCON:","\r\n");
		}
		str = strstr(inBuffer, "\r\n+CEREG:");
		if(str) 
		{
			ret = 0;
			ret1 = 0;
			if(strchr(str, ',')){
				ret1 = sscanf(str, "\r\n+CEREG:%d,%hhu%*[\r\n]", &temp, &reg);
				if(2 != ret1){
					ret = sscanf(str, "\r\n+CEREG:%hhu%*[\r\n]", &reg);
				}
			}
			else
				ret = sscanf(str, "\r\n+CEREG:%hhu%*[\r\n]", &reg);			
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "\r\n+++++++g_ucRegState %d+++++++\r\n", reg);
			if(1 == ret || 2 == ret1){
				if(reg < INVALID_REG)
					NeulBc28SetRegState(reg);
				else
					DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "\r\n +CEREG error %s,%d+++++++\r\n", str,reg);
			}
			URC_FILTER(inBuffer,inLen,str,"\r\n+CEREG:","\r\n");
		}
		str = strstr(inBuffer, "\r\n+NPSMR:");
		if(str){
			if(strchr(str, ','))
				ret = sscanf(str, "+NPSMR:%d,%hhu%*[\r\n]", &temp, &psm);
			else
				ret = sscanf(str, "\r\n+NPSMR:%hhu%*[\r\n]", &psm);
			if(0 != ret){
				DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "\r\n+++++++g_ucPsmState %d+++++++\r\n", psm);
				NeulBc28SetPsmState(psm);
			}
			URC_FILTER(inBuffer,inLen,str,"\r\n+NPSMR:","\r\n");
		}
		/*after reboot and attached to network, the module will send REGISTERNOTIFY
		message to the device, then the device triggers registration by command AT+QLWSREGIND*/
		str = strstr(inBuffer, "\r\nREGISTERNOTIFY");
		if(str){
			NeulBc28SetRegisterNotify(1);
			URC_FILTER(inBuffer,inLen,str,"\r\nREGISTERNOTIFY","\r\n");
		}
		str = strstr(inBuffer, "\r\n+QLWEVTIND=");
		if(str)
		{
			ret = sscanf(str, "\r\n+QLWEVTIND=%hhd%*[\r\n]", &iot);
			if(0 != ret){
				DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "IoT status %d\r\n", iot);
				NeulBc28SetLastIotState(NeulBc28GetIotState());
				NeulBc28SetIotState(iot);
				//iot status
				if(NeulBc28GetLastIotState() != iot)
				{
					if(neul_dev.ops->iot_chg)
						neul_dev.ops->iot_chg();
				}
			}
			URC_FILTER(inBuffer,inLen,str,"\r\n+QLWEVTIND=","\r\n");
		}
		
		/*
		AT+CGMR
		SECURITY_A,V150R100C10B180
		PROTOCOL_A,V150R100C10B180
		APPLICATION_A,V150R100C10B180
		SECURITY_B,V150R100C10B180
		*/
		str = strstr(inBuffer, "\r\n+QLWEVTIND:");
		if(str)
		{
			ret = sscanf(str, "\r\n+QLWEVTIND:%hhd%*[\r\n]", &iot);
			if(0 != ret){
				DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "IoT status %d\r\n", iot);
				NeulBc28SetIotState(iot);
				lastIot = NeulBc28GetLastIotState();
				//iot status
				if(lastIot != iot)
				{
					if(neul_dev.ops->iot_chg)
						neul_dev.ops->iot_chg();
					NeulBc28SetLastIotState(iot);
				}
			}
			URC_FILTER(inBuffer,inLen,str,"\r\n+QLWEVTIND:","\r\n");
		}
		//over 6 times register iot platform fail, cause watchdog
		str = strstr(inBuffer, "\r\nREBOOT_CAUSE_APPLICATION_WATCHDOG");
		if(str){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "REBOOT_CAUSE_APPLICATION_WATCHDOG\r\n");
			//NeulBc28SetDevState(BC28_INIT);
			NbiotHardResetInner();
			//URC_FILTER(inBuffer,bufLen,str,"\r\nREBOOT_CAUSE_APPLICATION_WATCHDOG","\r\nNeul\r\nOK\r\n");
			URC_FILTER(inBuffer,inLen,str,"\r\nREBOOT_CAUSE_APPLICATION_WATCHDOG","\r\nOK\r\n");
		}
		str = strstr(inBuffer, "\r\nREBOOT_CAUSE_SECURITY_PMU_POWER_ON_RESET");
		if(str){
			printf("find nREBOOT_CAUSE_SECURITY_PMU_POWER_ON_RESET\r\n");
			//g_iHardRebooted = 1;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "REBOOT_CAUSE_SECURITY_PMU_POWER_ON_RESET\r\n");
			//URC_FILTER(inBuffer,bufLen,str,"\r\nREBOOT_CAUSE_SECURITY_PMU_POWER_ON_RESET","\r\nNeul\r\nOK\r\n");
			URC_FILTER(inBuffer,inLen,str,"\r\nREBOOT_CAUSE_SECURITY_PMU_POWER_ON_RESET","\r\nOK\r\n");
		}
		str = strstr(inBuffer, "\r\nREBOOT_CAUSE_APPLICATION_AT");
		if(str){
			g_iAtRebooted = 1;    //rebooted
			//RoadLamp_Dump(inBuffer,inLen);
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "REBOOT_CAUSE_APPLICATION_AT\r\n");
			//URC_FILTER(inBuffer,bufLen,str,"\r\nREBOOT_CAUSE_APPLICATION_AT","\r\nNeul\r\nOK\r\n");
			//URC_FILTER(inBuffer,bufLen,str,"\r\nREBOOT_CAUSE_APPLICATION_AT","Neul\r\nOK\r\n");
			URC_FILTER(inBuffer,inLen,str,"\r\nREBOOT_CAUSE_APPLICATION_AT","\r\nOK\r\n");
			//printf("%s", inBuffer);
		}
		str = strstr(inBuffer, "\r\nREBOOT_CAUSE_SECURITY_RESET_PIN");
		if(str){
			//printf("\r\nREBOOT_CAUSE_SECURITY_RESET_PIN");
			MUTEX_LOCK(g_stHardReset.mutex,MUTEX_WAIT_ALWAYS);
			g_stHardReset.iHardRebooted = REBOOTED;    //rebooted
			g_stHardReset.iResult = REBOOT_OK;
			MUTEX_UNLOCK(g_stHardReset.mutex);
			NeulBc28SetDevState(BC28_INIT);
			URC_FILTER(inBuffer,inLen,str,"\r\nREBOOT_CAUSE_SECURITY_RESET_PIN","\r\nOK\r\n");
		}
		str = strstr(inBuffer, "\r\nREBOOT_CAUSE_SECURITY_FOTA_UPGRADE");
		if(str){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "REBOOT_CAUSE_SECURITY_FOTA_UPGRADE");
			NeulBc28SetFwState(FIRMWARE_FOTA_UPGRADE_REBOOT);
			URC_FILTER(inBuffer,inLen,str,"\r\nREBOOT_CAUSE_SECURITY_FOTA_UPGRADE","\r\nOK\r\n");
		}
		
		str = strstr(inBuffer, "\r\nFIRMWARE DOWNLOADING");
		if(str){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "FIRMWARE DOWNLOADING\r\n");
			NeulBc28SetFwState(FIRMWARE_DOWNLOADING);
			NeulBc28SetDevState(BC28_UPDATE);
			URC_FILTER(inBuffer,inLen,str,"\r\nFIRMWARE DOWNLOADING","\r\n");
		}
		str = strstr(inBuffer, "\r\nFIRMWARE DOWNLOADED");
		if(str){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "FIRMWARE DOWNLOADED\r\n");
			NeulBc28SetFwState(FIRMWARE_DOWNLOADED);
			URC_FILTER(inBuffer,inLen,str,"\r\nFIRMWARE DOWNLOADED","\r\n");
		}
		str = strstr(inBuffer, "\r\nFIRMWARE UPDATING");
		if(str){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "FIRMWARE UPDATING\r\n");
			NeulBc28SetFwState(FIRMWARE_UPDATING);
			URC_FILTER(inBuffer,inLen,str,"\r\nFIRMWARE UPDATING","\r\n");
		}
		str = strstr(inBuffer, "\r\nFIRMWARE UPDATE SUCCESS");
		if(str){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "FIRMWARE UPDATE SUCCESS\r\n");
			NeulBc28SetFwState(FIRMWARE_UPDATE_SUCCESS);
			URC_FILTER(inBuffer,inLen,str,"\r\nFIRMWARE UPDATE SUCCESS","\r\n");
		}		
		//FIRMWARE UPDATE FAILED
		str = strstr(inBuffer, "\r\nFIRMWARE UPDATE FAILED");
		if(str){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "FIRMWARE UPDATE FAILED\r\n");
			//NeulBc28SetDevState(BC28_INIT);
			NeulBc28SetFwState(FIRMWARE_UPDATE_FAILED);
			NbiotHardResetInner();
			URC_FILTER(inBuffer,inLen,str,"\r\nFIRMWARE UPDATE FAILED","\r\n");
		}
		//FIRMWARE UPDATE OVER
		str = strstr(inBuffer, "\r\nFIRMWARE UPDATE OVER");
		if(str){
			NeulBc28SetFwState(FIRMWARE_UPDATE_OVER);
			NeulBc28SetFwState(FIRMWARE_NONE);
			NeulBc28SetDevState(BC28_READY);
			URC_FILTER(inBuffer,inLen,str,"\r\nFIRMWARE UPDATE OVER","\r\n");
		}
		str = strstr(inBuffer, "\r\n+NNMI:");
		if(str){
			//printf("find nnmi before filter %s\r\n", inBuffer);
			MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
			g_stBc28Pkt.iNnmiRx++;
			MUTEX_UNLOCK(g_stBc28Pkt.mutex);
			
			memset(rx_tmpbuf,0,sizeof(rx_tmpbuf));
			ret = sscanf(str, "\r\n+NNMI:%d,%s\r\n", &readlen, rx_tmpbuf);
			if(0 != ret){
				URC_FILTER(inBuffer,inLen,str,"\r\n+NNMI:","\r\n");
				//printf("after nnmi before filter %s\r\n", inBuffer);
				NeulBc28StrToHex(rx_tmpbuf, readlen*2, outBuffer);
				item.type = MSG_TYPE_HUAWEI;
				item.len = readlen;
				item.data = (unsigned char *)outBuffer;
				ret = MsgPush(CHL_NB,MSG_QUEUE_TYPE_RX,&item,0,PRIORITY_LOW);
				if(0 != ret){
					MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
					g_stBc28Pkt.iPktPushFail++;
					MUTEX_UNLOCK(g_stBc28Pkt.mutex);
					
					DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "rx buffer is full\r\n");
				}else{
					MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
					g_stBc28Pkt.iPktPushSuc++;
					MUTEX_UNLOCK(g_stBc28Pkt.mutex);
				}
			}else{
				DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "scanf +NNMI fail\r\n");
			}
		}
		
		/*str = strstr(inBuffer, "+CCLK:");
		if(str){
			printf("");
			//+CCLK:18/05/04,06:02:42+32
			sscanf(str, "+CCLK:%hhu/%hhu/%hhu,%hhu:%hhu:%hhu+%hhu\r\n",
				&year, &month, &day,
				&hour, &min, &second, &zone);
			
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"y %d, m %d, d %d, h %d, m %d, s %d, z %d\r\n", year, month, day, hour, min, second, zone);
		
			if(neul_dev.ops->time_set)
				neul_dev.ops->time_set(year, month, day, hour, min, second, zone);
		}*/
	}	

	return inLen;
}

int NbiotDataRead(char *buf, int maxrlen, int mode, unsigned int timeout)
{
	static U32 startTick,reStartTick;
	U32 time, time1;
	int rlen, rlen1 = 0;
	char tmpBuf[NEUL_MAX_BUF_SIZE] = {0};
	/*
	1.串口收数据
	2.URC消息处理
	*/
	MUTEX_LOCK(g_stRxMutex,MUTEX_WAIT_ALWAYS);
	startTick = OppTickCountGet();
	while(1){
		reStartTick = OppTickCountGet();
		time1 = (reStartTick - startTick);
		time = (timeout>time1)?(timeout - time1):0;
		if(time == 0){
			//printf("break tick %d\r\n", reStartTick);
			break;
		}	
		rlen = uart_data_read(tmpBuf, NEUL_MAX_BUF_SIZE, mode, time);
		if(rlen > 0){
			//printf("uart_data_read rlen %d\r\n", rlen);
			//printf("%s", tmpBuf);
			rlen1 = UrcMsgProc(tmpBuf, rlen);
			if(rlen1 == 0) //only urc
			{
				//printf("UrcMsgProc rlen1 = 0 tick %d\r\n", OppTickCountGet());
				continue;
			}
			else if(rlen1 > 0) //not only urc
			{
				break;
			}
		}
		
	}
	memcpy(buf,tmpBuf,rlen1);
	MUTEX_UNLOCK(g_stRxMutex);
	return rlen1;
}

int NbiotHardResetStart(void)
{
	int status;
	int res;
	
	MUTEX_LOCK(g_stHardReset.mutex,MUTEX_WAIT_ALWAYS);
	g_stHardReset.iHardRebooted = REBOOTING;
	g_stHardReset.iResult = REBOOT_UNKNOW;
	MUTEX_UNLOCK(g_stHardReset.mutex);

	res = NeulBc28SetDevState(BC28_RESET);
	if(res!=0) return res;
	res = NbReset();
	if(res!=0) return res;
	
	//vTaskDelay(timeout / portTICK_RATE_MS);
	
	return 0;
}

int NbiotHardResetResult(void)
{
	int status;
	
	MUTEX_LOCK(g_stHardReset.mutex,MUTEX_WAIT_ALWAYS);
	status = g_stHardReset.iHardRebooted;
	MUTEX_UNLOCK(g_stHardReset.mutex);
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NbiotHardReset\r\n");
	
	return status;
}

int NbiotHardReset(int timeout)
{
	int status;
	int res;
	
	MUTEX_LOCK(g_stHardReset.mutex,MUTEX_WAIT_ALWAYS);
	g_stHardReset.iHardRebooted = REBOOTING;	
	g_stHardReset.iResult = REBOOT_UNKNOW;
	MUTEX_UNLOCK(g_stHardReset.mutex);

	res = NeulBc28SetDevState(BC28_RESET);
	if(res!=0) return res;
	res = NbReset();
	if(res!=0) return res;
	
	vTaskDelay(timeout / portTICK_RATE_MS);

	MUTEX_LOCK(g_stHardReset.mutex,MUTEX_WAIT_ALWAYS);
	status = g_stHardReset.iHardRebooted;
	MUTEX_UNLOCK(g_stHardReset.mutex);
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NbiotHardReset\r\n");
	
	return status;
}

int NbiotHardResetInner(void)
{
	int ret;

	MUTEX_LOCK(g_stHardReset.mutex,MUTEX_WAIT_ALWAYS);
	g_stHardReset.iHardRebooted = REBOOTING;
	g_stHardReset.iResult = REBOOT_UNKNOW;
	MUTEX_UNLOCK(g_stHardReset.mutex);
	
	NeulBc28SetDevState(BC28_RESET);
	ret = NbReset();
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NbiotHardResetInner\r\n");
	
	return ret;
}

// 参数说明:
// in， 源字符串
// out, 存放最后结果的字符串
// outlen，out最大的大小
// src，要替换的字符串
// dst，替换成什么字符串
char *strrpl(char *in, char *out, int outlen, const char *src, char *dst)
{
    char *p = in;
    unsigned int  len = outlen - 1;

    // 这几段检查参数合法性
    if((NULL == src) || (NULL == dst) || (NULL == in) || (NULL == out))
    {
        return NULL;
    }
    if((strcmp(in, "") == 0) || (strcmp(src, "") == 0))
    {
        return NULL;
    }
    if(outlen <= 0)
    {
        return NULL;
    }

    while((*p != '\0') && (len > 0))
    {
        if(strncmp(p, src, strlen(src)) != 0)
        {
            int n = strlen(out);

            out[n] = *p;
            out[n + 1] = '\0';

            p++;
            len--;
        }
        else
        {
            strncat(out, dst, outlen);
            p += strlen(src);
            len -= strlen(dst);
        }
    }

    return out;
}

void BC28_STRNCPY(char * dst, char * src, int len)
{
	strncpy(dst, src, len);
	if(strlen(src) > len-1){
		dst[len-1] = '\0';
	}
}
 
U8 is_ipv4_address(char* host){
    char* p=host;
    U8 fieldIndex=0;
    U8 numberNum=0;
    U8 fieldValue=0;
    while(*p){
        if(*p>='0' && *p<='9'){
            numberNum++;
            fieldValue*=10;
            fieldValue+=(*p-'0');
            if(fieldValue>255) return 0;
        }
        else if(*p=='.'){
            if(numberNum==0) return 0;
            fieldIndex++;
            if(fieldIndex>=4) return 0;
            numberNum=fieldValue=0;
        }
        else return 0;
        p++;
    }
    if(fieldIndex!=3 || numberNum==0) return 0;
    return 1;
}

int NeulBc28SetConfig(ST_NB_CONFIG *pstConfigPara)
{	
	if(NULL == pstConfigPara)
		return OPP_FAILURE;
	MUTEX_LOCK(g_stNbConfigMutex,MUTEX_WAIT_ALWAYS);	
	memcpy(&g_stNbConfig, pstConfigPara, sizeof(ST_NB_CONFIG));
	MUTEX_UNLOCK(g_stNbConfigMutex);
	return OPP_SUCCESS;
}

int NeulBc28GetConfig(ST_NB_CONFIG *pstConfigPara)
{
	if(NULL == pstConfigPara)
		return OPP_FAILURE;
	MUTEX_LOCK(g_stNbConfigMutex,MUTEX_WAIT_ALWAYS);	
	memcpy(pstConfigPara, &g_stNbConfig, sizeof(ST_NB_CONFIG));
	MUTEX_UNLOCK(g_stNbConfigMutex);
	return OPP_SUCCESS;
}

int NeulBc28GetConfigFromFlash(ST_NB_CONFIG *pstConfigPara)
{
	int ret = OPP_SUCCESS;
	unsigned int len = sizeof(ST_NB_CONFIG);
	ST_NB_CONFIG stConfigPara;

	MUTEX_LOCK(g_stNbConfigMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaRead(APS_PARA_MODULE_APS_NB, NBCONFIG_ID, (unsigned char *)&stConfigPara, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_NB, DLL_INFO,"nb config is empty, use default config\r\n");
		//printf("nb config is empty, use default config\r\n");
		memcpy(pstConfigPara,&g_stNbConfigDefault,sizeof(ST_NB_CONFIG));
	}else{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "nb read config from flash success\r\n");
		//printf("nb read config from flash success\r\n");
		memcpy(pstConfigPara,&stConfigPara,sizeof(ST_NB_CONFIG));
	}
	
	MUTEX_UNLOCK(g_stNbConfigMutex);
	return OPP_SUCCESS;
}

int NeulBc28SetConfigToFlash(ST_NB_CONFIG *pstConfigPara)
{
	int ret = OPP_SUCCESS;

	MUTEX_LOCK(g_stNbConfigMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaWrite(APS_PARA_MODULE_APS_NB, NBCONFIG_ID, (unsigned char *)pstConfigPara, sizeof(ST_NB_CONFIG));
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_NB, DLL_INFO,"NeulBc28SetConfigToFlash write flash ret %d\r\n", ret);
		MUTEX_UNLOCK(g_stNbConfigMutex);
		return ret;
	}
	
	MUTEX_UNLOCK(g_stNbConfigMutex);
	return OPP_SUCCESS;
}

int NeulBc28RestoreFactory(void)
{
	int ret;

	ret = NeulBc28SetConfigToFlash(&g_stNbConfigDefault);
	if(OPP_SUCCESS != ret){
		return OPP_FAILURE;
	}

	NeulBc28SetConfig(&g_stNbConfigDefault);

	ret = NeulBc28SetDiscreteParaToFlash(&g_stDisParaDefault);
	if(OPP_SUCCESS != ret){
		return OPP_FAILURE;
	}
	NeulBc28SetDiscretePara(&g_stDisParaDefault);
	return OPP_SUCCESS;
}

int NeulBc28GetVersion(char * version,int len)
{
	char * cmd = "AT+CGMR\r";
	int ret;
	char aucNbVer[256] = {'\0'};
	
	//query version
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed        
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s,%d\r\n", __FUNCTION__, __LINE__);     
        return -1;
    }
    memset(neul_dev.rbuf, 0, 256);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 256, 0, 300);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s,%d\r\n", __FUNCTION__, __LINE__);        
        return -1;
    }
	
	BC28_STRNCPY(aucNbVer, neul_dev.rbuf+2, ret-6);
	//aucNbVer[ret-6] = '\0';
	strrpl(aucNbVer,version,len,"\r\n"," ");
	return 0;
}

/* ============================================================
func name   :   NeulBc28SetUpdParam
discription :   this func just called after the socket created
                set the remote ip address and port the socket sendto or receive fro
param       :   
             rip  @ input param, remote client/server ip address
             port @ input param, remote client/server port
             socket @ input param, local socket handle
return      :  0 mean ok, !0 means param err
Revision    : 
=============================================================== */
int NeulBc28SetUpdParam(const char *rip, const unsigned short port, const int socket)
{
	int i = 0;
	
    if (NULL == rip || 0 == port)
    {
        return -1;
    }
    if (strlen(rip) >= NEUL_IP_LEN)
    {
        return -2;
    }
	//[code review]
	for(i=0;i<NEUL_MAX_SOCKETS;i++){
		if(-1 == neul_dev.addrinfo[i].socket){
			break;
		}
	}
	if(NEUL_MAX_SOCKETS == i){
		return -3;
	}
    neul_dev.remotecount++;
    (neul_dev.addrinfo+i)->port = port;
    (neul_dev.addrinfo+i)->socket = socket;
    memcpy((neul_dev.addrinfo+i)->ip, rip, strlen(rip));
    return 0;
}

/* ============================================================
func name   :   NeulBc28SetUpdParam
discription :   this func just called after the socket created
                set the remote ip address and port the socket sendto or receive fro
param       :   
             socket @ input param, local socket handle that need to reset
return      :  0 mean ok, !0 means param err
Revision    : 
=============================================================== */
static int NeulBc28ResetUpdParam(const int socket)
{
	int i;
	
    if (socket < 0)
    {
        return -1;
    }
    neul_dev.remotecount--;
	
	for(i=0;i<NEUL_MAX_SOCKETS;i++){
		if(socket == neul_dev.addrinfo[i].socket){
			(neul_dev.addrinfo+i)->port = 0;
			(neul_dev.addrinfo+i)->socket = -1;
			memset((neul_dev.addrinfo+i)->ip, 0, NEUL_IP_LEN);
			break;
		}
	}
    return 0;
}

/* ============================================================
func name   :   NeulBc28SetCdpInfo
discription :   set cdp server ip address
param       :   
             cdp  @ input param, cdp server's ip address
return      :  0 mean ok, !0 means param err
Revision    : 
=============================================================== */
int NeulBc28SetCdpInfo(const char *cdp)
{
    if(NULL == cdp)
    {
        return -1;
    }
    if (strlen(cdp) >= NEUL_IP_LEN)
    {
        return -2;
    }
    memcpy(neul_dev.cdpip, cdp, strlen(cdp));
    return 0;
}

/* ============================================================
func name   :  NeulBc28HwDetect
discription :  detect bc28 hardware 
param       :  None 
return      :  0 bc28 is connectted to mcu ok , !0 means not ok
Revision    : 
=============================================================== */
static int NeulBc28HwDetect(void)
{
    int ret = 0;
    char *p = "AT+CGMM\r";
    char *cmd = "AT+CGMR\r";
    char *cmd0 = "AT+CGMI\r";
	
    ret = neul_dev.ops->dev_write(p, strlen(p), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28HwDetect cmd %s write fail ret %d\r\n", p, ret);
        return -1;
    }
    if (NULL == neul_dev.rbuf)
    {
		//MakeErrLog(DEBUG_MODULE_BC28,OPP_NULL_POINTER);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28HwDetect cmd neul_dev.rbuf is null\r\n", p, ret);
        return -2;
    }
    //NeulBc28Sleep(10);
    memset(neul_dev.rbuf, 0, NEUL_MANUFACT_LEN);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MANUFACT_LEN, 0, 300);
    if (ret <= 0)
    {
        //read bc28 manufacturer info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28HwDetect cmd %s read msg(%s) fail ret %d\r\n", p, neul_dev.rbuf, ret);
        return -3;
    }

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", p, neul_dev.rbuf);

#if 0
    str = strstr(neul_dev.rbuf,"Hi15RM1-HLB_V1.0");
    if (!str)
    {
        //can't find bc28 manufacturer info
        return -4;
    }

    str = strstr(neul_dev.rbuf,"BC28JA-02-STD");
    if (!str)
    {
        //can't find bc28 manufacturer info
        return -4;
    }
#endif

	//query version
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28HwDetect cmd %s write fail ret %d\r\n", cmd, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 256);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 256, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28HwDetect cmd %s read msg(%s) fail ret %d\r\n", cmd, neul_dev.rbuf, ret);
        return -1;
    }

	//printf("%s %s\n", cmd, neul_dev.rbuf);
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_DEBUG,"%s %s\n", cmd, neul_dev.rbuf);
	BC28_STRNCPY((char *)neul_dev.revision,neul_dev.rbuf,NEUL_VER_LEN);
	//query company
    ret = neul_dev.ops->dev_write(cmd0, strlen(cmd0), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);        
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28HwDetect cmd %s write fail ret %d\r\n", cmd0, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);        
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28HwDetect cmd %s read msg(%s) fail ret %d\r\n", cmd0, neul_dev.rbuf, ret);
        return -1;
    }

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd0, neul_dev.rbuf);
	
    return 0;
}


/* ============================================================
func name   :  NeulBc28SetBand
discription :  set bc28 work band 
param       :  
               band @ input param, bc28 work band BC95-CM set to 8, 
                      BC95-SL set to 5, BC95-VF set to 20
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc28SetBand(int band)
{
    char *p = "AT+NBAND=";
    int ret = 0;
    char *str = NULL;
    memset(neul_dev.wbuf, 0, 16);
    if (band == 8 || band == 5 || band == 20 || band == 3 || band == 28 || band == 1)
    {
        sprintf(neul_dev.wbuf, "%s%d%c", p, band, '\r');
    }
    else
    {
        //error band for bc28        
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_BAND_INVALIDE);        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s %d band:%d\n", __FUNCTION__, __LINE__, band);
        return -1;
    }
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed        
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %d\n", __FUNCTION__, __LINE__);        
        return -1;
    }
    memset(neul_dev.rbuf, 0, 16);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 16, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %d\n", __FUNCTION__, __LINE__);        
        return -3;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", neul_dev.wbuf, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (NULL == str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);		 
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %d\n", __FUNCTION__, __LINE__);    
        return -1;
    }
    return 0;
}


/* ============================================================
func name   :  NeulBc28SetFrequency
discription :  set bc28 work frequency 
param       :  0,信道自动选择
		      其他指定信道
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc28SetFrequency(void)
{
    char *p = "AT+NEARFCN=";
    char *cmd0 = "AT+CFUN=0\r";	
    int ret = 0;
    char *str = NULL;
	//ST_NB_CONFIG stConfigPara;
	U8 enable;
	U16 earfcn;

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"\r\n~~~~~~~~auto frequency start~~~~~~~~~\r\n");

	/*ret =NeulBc28GetConfig(&stConfigPara);
	if(OPP_SUCCESS == ret){
		channel = stConfigPara.earfcn;
	}else{
		channel = 0;
	}*/
	NeulBc28GetEarfcnFromRam(&enable, &earfcn);
	if(EARFCN_DISABLE == enable){
		//printf("NeulBc28GetEarfcnFromRam not earfcn");
		return 0;
	}
	//printf("do NeulBc28SetFrequency\r\n");
	memset(neul_dev.wbuf, 0, 64);
    sprintf(neul_dev.wbuf, "%s%d,%d%c", p, 0, earfcn, '\r');
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetFrequency write %s fail ret %d\r\n", neul_dev.wbuf, ret);    
        return -1;
    }
    memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetFrequency read %s msg(%s) fail ret %d\r\n", neul_dev.wbuf, neul_dev.rbuf, ret);    
        return -3;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", neul_dev.wbuf, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (NULL == str)
    {
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetFrequency read msg(%s) not contain OK\r\n", neul_dev.rbuf);    
        return -1;
    }	
    ret = neul_dev.ops->dev_write(cmd0, strlen(cmd0), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetFrequency write cmd %s fail ret %d\n", cmd0, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 6000);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetFrequency cmd %s read msg %s fail ret %d\n", cmd0, neul_dev.rbuf, ret);
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd0, neul_dev.rbuf);
    if (!strstr(neul_dev.rbuf,"OK"))
    {
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetFrequency cmd %s read msg %s no OK\n", cmd0, neul_dev.rbuf);
        return -1;
    }
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"\r\n~~~~~~~~auto frequency end~~~~~~~~~\r\n");
	
    return 0;
}


/* ============================================================
func name   :  NeulBc28GetBand
discription :  set bc28 work band 
param       :  
               band @ input param, bc28 work band BC95-CM set to 8, 
                      BC95-SL set to 5, BC95-VF set to 20
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc28GetBand(char *buf)
{
    char *p = "AT+NBAND?\r";
    int ret = 0;
    char *str = NULL;
	char tmp[128] = {0};
	
    ret = neul_dev.ops->dev_write(p, strlen(p), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 16);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 16, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        return -3;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", p, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (NULL == str)
    {
        return -1;
    }
	ret = sscanf(neul_dev.rbuf, "\r\n+NBAND:%s%*[\r\n]", tmp);
	if(0 == ret)
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"sscanf nband error\r\n");
		return -1;
	}
	BC28_STRNCPY(buf,tmp,128);
    return 0;
}
static int NeulBc28PSMSet(void)
{
    int ret = 0;
    char *cmd = "AT+CPSMS=0\r";
	char *str = NULL;
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "NeulBc28PSMSet\r\n");
	/* Get NCCID */
	ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
	if (ret < 0)
	{
		//write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28PSMSet write %s fail ret %d\r\n", cmd, ret);    
		return -1;
	}
	memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 300);
	if (ret <= 0)
	{
		//read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);        		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28PSMSet read %s msg(%s) fail ret %d\r\n", cmd, neul_dev.rbuf, ret);    
		return -1;
	}

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\r\n", cmd, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (NULL == str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28PSMSet read msg(%s) not contain OK\r\n",neul_dev.rbuf);    
        return -1;
    }
	return 0;
}


static int NeulBc28ErrorReport(void)
{
    int ret = 0;
    char *cmd = "AT+CMEE=1\r";
	char *str = NULL;
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "NeulBc28ErrorReport\r\n");
	/* Get NCCID */
	ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
	if (ret < 0)
	{
		//write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28ErrorReport write %s fail ret %d\r\n", cmd, ret);    
		return -1;
	}
	memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 300);
	if (ret <= 0)
	{
		//read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);        		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28ErrorReport read %s msg(%s) fail ret %d\r\n", cmd, neul_dev.rbuf, ret);    
		return -1;
	}

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\r\n", cmd, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (NULL == str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28PSMSet read msg(%s) not contain OK\r\n",neul_dev.rbuf);    
        return -1;
    }
	return 0;
}

static int NeulBc28ATESet(void)
{
    int ret = 0;
    char *cmd = "ATE0\r";

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "NeulBc28ATESet\r\n");
	/* Get NCCID */
	ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
	if (ret < 0)
	{
		//write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ATESet write fail ret %d\r\n", ret);
		return -1;
	}
	memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 300);
	if (ret <= 0)
	{
		//read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);		
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ATESet read msg(%s) fail ret %d\r\n", neul_dev.rbuf, ret);		
		return -1;
	}

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\r\n", cmd, neul_dev.rbuf);

	return 0;
}

static int NeulBc28PSMR(void)
{
    int ret = 0;
	/*AT+NPSMR Power Saving Mode Status Report 
	<n> Integer type. Enable/disable unsolicited result code.
	0 Disable unsolicited result code
	1 Enable unsolicited result code “+NPSMR:<mode>” */
    char *cmd = "AT+NPSMR=1\r";
	char *str = NULL;
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "NeulBc28PSMR\r\n");
	/* Get NCCID */
	ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
	if (ret < 0)
	{
		//write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28PSMR write %s fail ret %d\r\n", cmd, ret);    
		return -1;
	}
	memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 300);
	if (ret <= 0)
	{
		//read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28PSMR read %s msg(%s) fail ret %d\r\n", cmd, neul_dev.rbuf, ret);    
		return -1;
	}

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\r\n", cmd, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (NULL == str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28PSMR read msg(%s) not contain OK\r\n", neul_dev.rbuf);    
        return -1;
    }

	return 0;
}

static int NeulBc28Edrx(void)
{
	int ret = 0;
	char *cmd = "AT+NPTWEDRXS?\r";
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "NeulBc28Edrx\r\n");
	/* Get NCCID */
	ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
	if (ret < 0)
	{
		//write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s,%d\r\n", __FUNCTION__, __LINE__);
		return -1;
	}
	memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 300);
	if (ret <= 0)
	{
		//read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);		
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s,%d\r\n", __FUNCTION__, __LINE__);		
		return -1;
	}
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\r\n", cmd, neul_dev.rbuf);

	return 0;
}

static int NeulBc28QueryNccid(char *buf)
{
    int ret = 0;
    char *str = NULL;	
    char *cmd = "AT+NCCID?\r";
	int i;
	
    /* Get NCCID */
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"write cmd error %s\r\n", cmd);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 38);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 38, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"read cmd length < 0 error %s\r\n", cmd);        
        return -1;
    }
	str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);		
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "cmd:%s\r\n,read msg:%s\r\n", cmd, neul_dev.rbuf);
        return -1;
    }
	ret = sscanf(neul_dev.rbuf,"\r\n+NCCID:%s%*[\r\n]",buf);
	if(0 == ret){
		return -1;
	}
	if(strlen(buf) != NEUL_NCCID_LEN-1){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "nccid %s length %d is error\r\n",buf,strlen(buf));
		return -1;
	}
	for(i=0;i<strlen(buf);i++){
		if(buf[i]<'0'||buf[i]>'9'){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "nccid %s format is error\r\n",buf);
			return -1;
		}
	}
	return 0;
}

static int NeulBc28QueryImsi(char *buf)
{
    int ret = 0;
    char *str = NULL;	
    char *cmd = "AT+CIMI\r";

    /* Get NCCID */
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s,%d\r\n", __FUNCTION__, __LINE__);        
        return -1;
    }
	
	//printf("%s %s\n", cmd, neul_dev.rbuf);
	str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);		
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
	ret = sscanf(neul_dev.rbuf,"\r\n%s%*[\r\n]",buf);
	if(0 == ret){
		return -1;
	}
	return 0;
}

/*product id*/
static int NeulBc28QueryPid(char *buf)
{
    int ret = 0;
    char *str = NULL;	
    char *cmd = "ATI\r";

    /* Get NCCID */
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s write fail ret %d\r\n", cmd, ret);        
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);		        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s read fail ret %d\r\n", cmd, ret);        
        return -1;
    }
	
	//printf("%s %s\n", cmd, neul_dev.rbuf);
	str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);		        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s %s not contain OK\r\n", cmd, neul_dev.rbuf);        
        return -1;
    }
	ret = sscanf(neul_dev.rbuf,"%[^OK]",buf);
	if(0 == ret){
		return -1;
	}
	return 0;
}

static int NeulBc28QueryImei(char *buf)
{
    int ret = 0;
    char *str = NULL;	
    char *cmd = "AT+CGSN=1\r";

    /* Get NCCID */
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);		        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s,%d\r\n", __FUNCTION__, __LINE__);        
        return -1;
    }
	
	//printf("\r\n+CGSN:%s %s\n", cmd, neul_dev.rbuf);
	str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);		        
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
	str = strstr(neul_dev.rbuf,"+CGSN:");
	if (!str)
	{
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_CGSN);		        
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "not contain +CGSN\r\n");
		return -1;
	}
	BC28_STRNCPY((char *)buf, str+6, NEUL_IMEI_LEN);
	//buf[NEUL_IMEI_LEN-1] = '\0';
	return 0;
}

/* ============================================================
func name   :  NeulBc28ActiveNetwork
discription :  active bc28's network 
               this will take some time
param       :  None
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc28ActiveNetwork(void)
{
    int ret = 0;
    char *str = NULL;
    char *cmd0 = "AT+CGSN=1\r";
    char *cmd1 = "AT+CFUN=1\r"; 
    char *cmd2 = "AT+CIMI\r";
    //char *cmd3 = "AT+CGDCONT=1,\"IP\",\"HUAWEI.COM\"\r";
    //char *cmd3 = "AT+CGDCONT=1,\"IP\",\"psm0.eDRX0.ctnb\"\r";
    //char *cmd3 = "AT+CGDCONT=1,\"IP\",\"CTNB\"\r";
    //char *cmd3 = "AT+CGDCONT=1,\"IP\",\"PCCW\"\r";
    char *cmd4 = "AT+CGATT=1\r";
    char *cmd5 = "AT+CEREG=1\r";
    char *cmd6 = "AT+CSCON=1\r";
    char *cmd7 = "AT+CGDCONT=1,\"IP\",";	
	ST_NB_CONFIG stConfigPara;
	U8 enable;
	U16 band;
	U8 tmp;
	
    /* Get IMEI */
    ret = neul_dev.ops->dev_write(cmd0, strlen(cmd0), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork write %s fail ret %d\r\n", cmd0, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);		        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork read %s msg(%s) fail ret %d\r\n", cmd0, neul_dev.rbuf, ret);
        return -1;
    }

	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd0, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"+CGSN:");
    if (!str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_CGSN);    
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s msg(%s) not contain +CGSN\r\n", cmd0, neul_dev.rbuf);
        return -1;
    }
#if 0
    if (strlen(str) < 25)
    {
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s,%d\r\n", __FUNCTION__, __LINE__);    
        return -1;
    }
#endif	
	BC28_STRNCPY((char *)neul_dev.imei, str+6, NEUL_IMEI_LEN);
	//neul_dev.imei[NEUL_IMEI_LEN-1] = '\0';
//wangtao
#if 1    
    /* Config CFUN */
    ret = neul_dev.ops->dev_write(cmd1, strlen(cmd1), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork write %s fail ret %d\r\n", cmd1, ret);
        return -1;
    }
    //NeulBc28Sleep(4000);//need wait 4 sconds or more, it's a little long
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 6, 0, 6000);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);		        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork read %s msg(%s) fail ret %d\r\n", cmd1, neul_dev.rbuf, ret);
        return -1;
    }

	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd1, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);		        
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28ActiveNetwork cmd %s msg(%s) not contain OK\r\n", cmd1, neul_dev.rbuf);
        return -1;
    }    
 #endif
 
    //NeulBc28Sleep(1000);
    /* Get IMSI */
    ret = neul_dev.ops->dev_write(cmd2, strlen(cmd2), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);		        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork write %s fail ret %d\r\n", cmd2, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 64);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 64, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);		        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork read %s msg(%s) fail ret %d\r\n", cmd2, neul_dev.rbuf, ret);
        return -1;
    }

	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd2, neul_dev.rbuf);
    /*str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		printf("%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }*/
#if 0    
    if (strlen(neul_dev.rbuf) < 19)
    {
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s,%d\r\n", __FUNCTION__, __LINE__);    
        return -1;
    }
#endif	
	BC28_STRNCPY((char *)neul_dev.imsi, neul_dev.rbuf+2, NEUL_IMSI_LEN);
	//neul_dev.imsi[NEUL_IMEI_LEN-1] = '\0';
//set band error wangtao 

    /* set band to do... */
	/*电信*/
	//NeulBc28GetBand();
	NeulBc28GetBandFromRam(&enable,&band);
	if(EARFCN_ENABLE == enable){
		if(0 == band){
			ret = NeulBc28SetBand(5);
			tmp = 5;
		}else{
			ret = NeulBc28SetBand(band);
			tmp = band;
		}
    	if (ret < 0)
    	{
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetBand %d err ret %d\n", tmp, ret);
        	return -1;
    	}
	}
    //ret = NeulBc28SetBand(5);
    //b300 not need set band
    /*ret = NeulBc28GetConfig(&stConfigPara);
	if(OPP_SUCCESS != ret){
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork NeulBc28GetConfig fail ret %d\r\n", ret);
		return -1;
	}*/
    /*ret = NeulBc28SetBand(stConfigPara.band);
    	if (ret < 0)
    	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetBand err ret %d\n", ret);
        	return -1;
    	}*/
   
#if 0
	/*联通*/
    ret = NeulBc28SetBand(8);
    if (ret < 0)
    {
        return -1;
    }
#endif
	//NeulBc28GetBand();
	ret = NeulBc28Cgatt(0);
	if(ret < 0)
	{
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork NeulBc28Cgatt 0 fail ret %d\r\n", ret);
        return -1;
	}
    #if 1
	memset(neul_dev.wbuf, 0, 128);
    //sprintf(neul_dev.wbuf, "%s\"%s\"%c", cmd7, stConfigPara.apn, '\r');
    sprintf(neul_dev.wbuf, "%s\"%s\"%c", cmd7, g_stNbConfigDefault.apn, '\r');
    /* Config apn */
    //ret = neul_dev.ops->dev_write(cmd3, strlen(cmd3), 0);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork write %s fail ret %d\r\n", neul_dev.wbuf, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork read %s msg(%s) fail ret %d\r\n", neul_dev.wbuf, neul_dev.rbuf, 
        ret);
        return -2;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", neul_dev.wbuf, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork %s read msg(%s) not contain OK\r\n", neul_dev.wbuf, neul_dev.rbuf);
        return -1;
    }
    #endif
    
    /* Active Network */
    ret = neul_dev.ops->dev_write(cmd4, strlen(cmd4), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork write %s fail ret %d\r\n", cmd4, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 1000);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork read %s msg(%s) fail ret %d\r\n", cmd4, neul_dev.rbuf, ret);
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd4, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);				
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork %s read msg(%s) not OK\r\n", cmd4, neul_dev.rbuf);
        return -1;
    }
	
    /* CEREG */
    ret = neul_dev.ops->dev_write(cmd5, strlen(cmd5), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork write %s fail ret %d\r\n", cmd5, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork read %s msg(%s)fail ret %d\r\n", cmd5, neul_dev.rbuf, ret);
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd5, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);				
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "CEREG Not Attach\n");
        return -1;
    }
    /* CSCON */
    ret = neul_dev.ops->dev_write(cmd6, strlen(cmd6), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork write %s fail ret %d\r\n", cmd6, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork read %s msg(%s) fail ret %d\r\n", cmd6, neul_dev.rbuf, ret);
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd6, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);				
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "msg(%s) CSCON Not Attach\n", neul_dev.rbuf);
        return -1;
    }

    return 0;
}

static int NeulBc28Abnormal(void)
{
    char *cmd0 = "AT+CFUN=0\r";
    char *cmd1 = "AT+NCSEARFCN\r";
	int ret = 0;

	
    ret = neul_dev.ops->dev_write(cmd0, strlen(cmd0), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Abnormal write cmd %s fail ret %d\n", cmd0, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 6000);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Abnormal cmd %s read msg %s fail ret %d\n", cmd0, neul_dev.rbuf, ret);
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %s\n", cmd0, neul_dev.rbuf);
    if (!strstr(neul_dev.rbuf,"OK"))
    {
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Abnormal cmd %s read msg %s no OK\n", cmd0, neul_dev.rbuf);
        return -1;
    }

    ret = neul_dev.ops->dev_write(cmd1, strlen(cmd1), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Abnormal write cmd %s fail ret %d\n", cmd1, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Abnormal cmd %s read msg %s fail ret %d\n", cmd1, neul_dev.rbuf, ret);
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %s\n", cmd1, neul_dev.rbuf);
    if (!strstr(neul_dev.rbuf,"OK"))
    {
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Abnormal cmd %s read msg %s no OK\n", cmd1, neul_dev.rbuf);
        return -1;
    }

	return 0;
}

/* ============================================================
func name   :  NeulBc28DeactiveNetwork
discription :  active bc28's network 
               this will take some time
param       :  None
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc28DeactiveNetwork(void)
{
    int ret = 0;
    char *str = NULL;
    char *cmd0 = "AT+CGATT=0\r";
	
    /* Deactive Network */
    ret = neul_dev.ops->dev_write(cmd0, strlen(cmd0), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 1000);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd0, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);				
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "CGATT Not Attach\n");
        return -1;
    }
	return 0;
}

/* ============================================================
func name   :  NeulBc28GetNetstat
discription :  get bc28's network status 
               
param       :  None
return      :  1 connected , 0 not connect
Revision    : 
=============================================================== */
static int NeulBc28GetNetstat(void)
{
    char *cmd = "AT+CGATT?\r";
    char *str = NULL;
    int ret = 0;
    
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);				
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28GetNetstat write cmd %s error %d\r\n", cmd, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 64);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 64, 0, 500);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);				
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28GetNetstat read ret %d\r\n", ret);        
        return -1;
    }
	//DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"CGATT:1");
    if (!str)
    {
        return -1;
    }
    
    return 0;
}
/* ============================================================
func name   :  NeulBc28QueryClock
discription :  query if bc28 get clock 
               
param       :  None
return      :  0 get clock , !0 no clock
Revision    : 
=============================================================== */
int NeulBc28QueryClock(char * time)
{
    char *cmd = "AT+CCLK?\r";
    char *str = NULL;
    int ret = 0;
    
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 36, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd, neul_dev.rbuf);
	

	str = strstr(neul_dev.rbuf, "+CCLK:");
	if(!str)
	{
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_CCLK);				
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "no cclk data\r\n");
		return -1;
	}

	if(strlen(str) > 36)
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "error length %d %s\r\n", strlen(neul_dev.rbuf), neul_dev.rbuf);
		return -1;
	}
	
	BC28_STRNCPY(time, str, 36);
    
    return 0;
    
}

/* ============================================================
func name   :  NeulBc28QueryIp
discription :  query if bc28 get ip address 
               
param       :  None
return      :  0 get ip , !0 no ip
Revision    : 
=============================================================== */
static int NeulBc28QueryIp(void)
{
    char *cmd = "AT+CGPADDR\r";
    char *str = NULL;
    int ret = 0;
    
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 64);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 64, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }
    //if get +CGPADDR:0 \r\n OK , no ip get ,need reboot
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd, neul_dev.rbuf);
	
    //str = strstr(neul_dev.rbuf,"CGPADDR:1");
    //str = strstr(neul_dev.rbuf,"+CGPADDR:0,");
    str = strstr(neul_dev.rbuf,"+CGPADDR:0,");
	if(str)
	{
		sscanf(str, "+CGPADDR:0,%s%*[\r\n]", neul_dev.pdpAddr0);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "\r\n~~~~~pdpaddr0 %s\r\n", neul_dev.pdpAddr0);
	}
    str = strstr(neul_dev.rbuf,"+CGPADDR:1,");
    if (str)
    {
    	sscanf(str, "+CGPADDR:1,%s%*[\r\n]", neul_dev.pdpAddr1);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "\r\n~~~~~pdpaddr1 %s\r\n", neul_dev.pdpAddr1);
    }
    if((0 == strlen((char *)neul_dev.pdpAddr0)) && (0 == strlen((char *)neul_dev.pdpAddr1)))
    {
		return -1;
	}
	
    return 0;
}


/* ============================================================
func name   :  NeulBc28Nconfig
discription :  query if bc28 get ip address 
               
param       :  None
return      :  0 get ip , !0 no ip
Revision    : 
=============================================================== */
static int NeulBc28Nconfig(void)
{
    char *cmd = "AT+NCONFIG?\r";
//    char *str = NULL;
    int ret = 0;

	//printf("%s start\r\n", cmd);
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Nconfig write %s fail ret %d\r\n", cmd, ret);
        return -1;
    }
	
    memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 512, 0, 600);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Nconfig read %s fail ret %d\r\n", cmd, ret);
        return -1;
    }
    //if get +CGPADDR:0 \r\n OK , no ip get ,need reboot
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd, neul_dev.rbuf);

	//printf("%s end\r\n", cmd);
	    
    return 0;
    
}

/* ============================================================
func name   :  NeulBc28QueryUestats
discription :  query if bc28 get ip address 
               
param       :  None
return      :  -1, input para error
		     -2, query imei error
		     -3, query imsi error
		     -4, query iccid error
		     -5, dev not ready
		     -6, query NUESTATS error
Revision    : 
=============================================================== */
int NeulBc28QueryUestats(ST_NEUL_DEV * pstNeulDev)
{
    char *cmd = "AT+NUESTATS\r";
    char *str = NULL, *str1 = NULL;
    int ret = 0;
	char buffer[128] = {0};
	int len = 0;
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "\r\n~~~~~uestats start~~~~~~~\r\n");    
	if(NULL == pstNeulDev)
	{
		MakeErrLog(DEBUG_MODULE_BC28,OPP_NULL_POINTER);				
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "pstNeulDev is NULL\r\n");
		return -1;
	}

	len = strlen((char *)neul_dev.imei);
	if(0 == len){
		ret = NeulBc28QueryImei((char *)neul_dev.imei);
		if(ret < 0)
			return -2;
		//len = strlen((char *)neul_dev.imei);
	}
	
	BC28_STRNCPY((char *)pstNeulDev->imei, (char *)neul_dev.imei, NEUL_IMEI_LEN);
	//pstNeulDev->imei[len] = '\0';
	
	len = strlen((char *)neul_dev.imsi);
	if(0 == len) {
		ret = NeulBc28QueryImsi((char *)neul_dev.imsi);
		if(ret < 0)
			return -3;
		//len = strlen((char *)neul_dev.imsi);
	}
	BC28_STRNCPY((char *)pstNeulDev->imsi, (char *)neul_dev.imsi, NEUL_IMSI_LEN);
	//pstNeulDev->imsi[len] = '\0';
	
	len = strlen((char *)neul_dev.nccid);
	if(0 == len){
		ret = NeulBc28QueryNccid((char *)neul_dev.nccid);
		if(ret < 0)
			return -4;
		//len = strlen((char *)neul_dev.nccid);
	}
	BC28_STRNCPY((char *)pstNeulDev->nccid, (char *)neul_dev.nccid, NEUL_NCCID_LEN);
	//pstNeulDev->nccid[len] = '\0';
	
	len = strlen((char *)neul_dev.pdpAddr0);
	if(0 < len){
		BC28_STRNCPY((char *)pstNeulDev->pdpAddr0, (char *)neul_dev.pdpAddr0, NEUL_IPV6_LEN);
		//pstNeulDev->pdpAddr0[len] = '\0';
	}
	len = strlen((char *)neul_dev.pdpAddr1);
	if(0 < len) {
		BC28_STRNCPY((char *)pstNeulDev->pdpAddr1, (char *)neul_dev.pdpAddr1, NEUL_IPV6_LEN);
		//pstNeulDev->pdpAddr1[len] = '\0';
	}

	if(BC28_READY != NeulBc28GetDevState())
		return -5;
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -6;
    }
    memset(neul_dev.rbuf, 0, 256);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 256, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        return -6;
    }
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "cmd %s:%s\r\n", cmd, neul_dev.rbuf);

	if(strlen(neul_dev.rbuf) == 0)
		return -6;
	/*
	AT+NUESTATS
	 
	Signal power:-988
	Total power:-760
	TX power:-32768
	TX time:0
	RX time:772
	Cell ID:227992403
	ECL:255
	SNR:55
	EARFCN:2509
	PCI:50
	RSRQ:-118
	OPERATOR MODE:0
	
	OK
	
	*/

	str = strstr(neul_dev.rbuf, "Signal power:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "Signal power:%d",  &pstNeulDev->rsrp);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "Signal power: %d\r\n", pstNeulDev->rsrp);
		}
	}
	str = strstr(neul_dev.rbuf, "Total power:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "Total power:%d",  &pstNeulDev->totalPower);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "Total power: %d\r\n", pstNeulDev->totalPower);
		}
	}
	
	str = strstr(neul_dev.rbuf, "TX power:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "TX power:%d",  &pstNeulDev->txPower);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "TX power: %d\r\n", pstNeulDev->txPower);
		}
	}
	str = strstr(neul_dev.rbuf, "TX time:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "TX time:%u",  &pstNeulDev->txTime);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "TX time: %d\r\n", pstNeulDev->txTime);
		}
	}
	str = strstr(neul_dev.rbuf, "RX time:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "RX time:%u",  &pstNeulDev->rxTime);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "RX time: %d\r\n", pstNeulDev->rxTime);
		}
	}

	str = strstr(neul_dev.rbuf, "Cell ID:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "Cell ID:%u",  &pstNeulDev->cellId);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "Cell ID: %d\r\n", pstNeulDev->cellId);
		}
	}

	str = strstr(neul_dev.rbuf, "ECL:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "ECL:%u",  &pstNeulDev->signalEcl);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "ECL: %d\r\n", pstNeulDev->signalEcl);
		}
	}
	str = strstr(neul_dev.rbuf, "SNR:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "SNR:%d",  &pstNeulDev->snr);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "SNR: %d\r\n", pstNeulDev->snr);
		}
	}
	str = strstr(neul_dev.rbuf, "EARFCN:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "EARFCN:%u",  &pstNeulDev->earfcn);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "EARFCN: %d\r\n", pstNeulDev->earfcn);
		}
	}
	str = strstr(neul_dev.rbuf, "PCI:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "PCI:%u",  &pstNeulDev->signalPci);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "PCI: %d\r\n", pstNeulDev->signalPci);
		}
	}
	str = strstr(neul_dev.rbuf, "RSRQ:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "RSRQ:%d",  &pstNeulDev->rsrq);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "RSRQ: %d\r\n", pstNeulDev->rsrq);
		}
	}
	str = strstr(neul_dev.rbuf, "OPERATOR MODE:");
	if(NULL != str)
	{
		str1 = strstr(str, "\r\n");
		if(NULL != str1)
		{
			BC28_STRNCPY(buffer, str, (str1-str+1));
			//buffer[str1-str] = '\0';
			ret = sscanf(buffer, "OPERATOR MODE:%hhu",  &pstNeulDev->opmode);
			if(0 == ret)
				return -6;
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "OPERATOR MODE: %d\r\n", pstNeulDev->opmode);
		}
	}
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "\r\n~~~~~uestats end~~~~~~~~~\r\n");        
    return 0;
}

/*call by daemon*/
U32 NeulBc28QueryRsrp(U8 targetId, U32 * rsrp)
{
	int ret;
	ST_NEUL_DEV *pstNeulDev;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(UESTATE_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(UESTATE_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		ARGC_FREE(rArgc,rArgv);
		return 1;
	}
	
	pstNeulDev = (ST_NEUL_DEV *)rArgv[0];
	if(0 == ret){
		*rsrp = pstNeulDev->rsrp;
	}
	ARGC_FREE(rArgc,rArgv);

	return ret;
}

U32 NeulBc28QueryRegCon(U8 targetId, U32 * regCon)
{
	U8 reg;

	reg = NeulBc28GetRegState();
	if(reg == 1 || reg == 5)
		*regCon = 1;
	else
		*regCon = 0;

	return 0;
}

/*
*@conStatus: 0:idle, 1:connect, 2:psm
*@regStatus:
	0 Not registered, UE is not currently searching an operator to register to
	1 Registered, home network
	2 Not registered, but UE is currently trying to attach or searching an operator to register to
	3 Registration denied
	4 Unknown (e.g. out of E-UTRAN coverage)
	5 Registered, roaming
*/
int NeulBc28QueryNetstats(U8 * regStatus, U8 * conStatus)
{
	U8 con = NeulBc28GetConState();
	U8 reg = NeulBc28GetRegState();
	*regStatus = reg;
	if(0 == con && 1 == g_ucPsmState)
	{
		*conStatus = 2;
	}
	else
	{
		*conStatus = con;
	}
	//DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"%d\r\n", g_ucRegState);
	//DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"%d\r\n", g_ucConState);

	return 0;
}

/*
0 Register complete
1 Deregister complete
2 Update register complete
3 Object observe complete
4 Bootstrap complete
5 5/0/3 resource observe complete
6 Notify the device to receive update package URL
7 Notify the device to receive update message
9 Cancel object 19/0/0 observe

*/
int NeulBc28QueryIOTstats(signed char * iotStatus)
{
	MUTEX_LOCK(g_stIotStateMutex, MUTEX_WAIT_ALWAYS);
	*iotStatus = g_cIOTState;
	MUTEX_UNLOCK(g_stIotStateMutex);
	
	return 0;
}

int NeulBc28QueryLastIOTstats(signed char * iotStatus)
{
	MUTEX_LOCK(g_stLastIotStateMutex, MUTEX_WAIT_ALWAYS);
	*iotStatus = g_cLastIOTState;
	MUTEX_UNLOCK(g_stLastIotStateMutex);
	
	return 0;
}

int NeulBc28SetRegisterNotify(int regNty)
{	
	MUTEX_LOCK(g_stRegNtyMutex,MUTEX_WAIT_ALWAYS);
	m_ucRegNotify = regNty;
	MUTEX_UNLOCK(g_stRegNtyMutex);
	return 0;
}

int NeulBc28RegisterNotify(void)
{
	U8 regNty;
	
	MUTEX_LOCK(g_stRegNtyMutex,MUTEX_WAIT_ALWAYS);
	regNty = m_ucRegNotify;
	MUTEX_UNLOCK(g_stRegNtyMutex);
	return regNty;
}
/* ============================================================
func name   :  NeulBc28QueryCsq
discription :  query if bc28 get ip address 
               
param       :  None
return      :  0 get ip , !0 no ip
Revision    : 
=============================================================== */
int NeulBc28QueryCsq(int * rsrp, int * ber)
{
    char *cmd = "AT+CSQ\r";
    int ret = 0;

    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"NeulBc28QueryCsq write ret %d\n", ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 64);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 64, 0, 300);
    if (ret <= 0)
    {
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"NeulBc28QueryCsq read ret %d\n", ret);
        return -1;
    }
    //if get +CGPADDR:0 \r\n OK , no ip get ,need reboot
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"%s %s\n", cmd, neul_dev.rbuf);
	//+CSQ:99,99
	
	ret = sscanf(neul_dev.rbuf, "\r\n+CSQ:%d,%d\r\n\r\nOK\r\n", rsrp, ber);
	if(0 == ret){
		return -1;
	}
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"rsrp %d ber %d\r\n", *rsrp ,*ber);
	//没信号
	if(*rsrp == 99)
	{
		return -1;
	}
	
	
    return 0;
}
/* ============================================================
func name   :  NeulBc28Reboot
discription :  reboot bc95 
               
param       :  None
return      :  0 connected , !0 not connect
Revision    : 
=============================================================== */
static int NeulBc28Reboot(void)
{
    char *cmd = "AT+NRB\r";
    char *str = NULL;
    int ret = 0;

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"%s,%d\r\n", __FUNCTION__, __LINE__);
	
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc95 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);				
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s,%d\r\n", __FUNCTION__, __LINE__);
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"%s,%d\r\n", __FUNCTION__, __LINE__);

    memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 300);
    if (ret <= 0)
    {
        //read bc95 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);				
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s,%d\r\n", __FUNCTION__, __LINE__); 
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"REBOOTING");
    if (!str)
    {
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);				
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s,%d\r\n", __FUNCTION__, __LINE__);    
        return -1;
    }
    
    return 0;
}
/* ============================================================
func name   :  NeulBc28SetCdpServer
discription :  1. The write command is available after the IMEI number has been set.
			2. The changes will take effect before successful network attachment. 
               	3. with REGSWT=0 trigger REIGSTERNOTIFY after reboot
param       :  ipaddr @ input param cdp server ip address
return      :  0 set ok , -1 common error, -2 not need set ncdp
Revision    : 
=============================================================== */
int NeulBc28SetCdpServer(const char *ipaddr)
{
    char *cmd = "AT+NCDP=";
    char *cmd2 = "AT+NCDP?\r";
    char *str = NULL;
    int ret = 0;
    char buffer[64] = {0};
	int port = 0;
	
    if (NULL == ipaddr || strlen(ipaddr) >= 16)
    {
        return -1;
    }

	if(!is_ipv4_address(ipaddr)){
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetCdpServer ipaddr format error %s\r\n", ipaddr); 
        return -1;
	}
    //query the setted ip back
    ret = neul_dev.ops->dev_write(cmd2, strlen(cmd2), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s", cmd2, neul_dev.rbuf);
	//if ipaddress want to config is same with the ip address in nbiot module, do nothing
    str = strstr(neul_dev.rbuf,ipaddr);
    if (str)
    {
        return 0;
    }
	
    memset(neul_dev.wbuf, 0, 64);
    sprintf(neul_dev.wbuf, "%s%s%c", cmd, ipaddr, '\r');
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "AT+NCDP=%s %s", ipaddr, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }
    
    //query the setted ip back
    ret = neul_dev.ops->dev_write(cmd2, strlen(cmd2), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s", cmd2, neul_dev.rbuf);	
    str = strstr(neul_dev.rbuf,ipaddr);
    if (!str)
    {
        return -1;
    }
    
    return 0;
}

int NeulBc28HuaweiDataEncryption(void)
{
	char *cmd = "AT+QSECSWT=0\r";
	int ret;
	
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s", cmd, neul_dev.rbuf);
	return 0;
}

/*
0,手动
1,自动*/
int NeulBc28HuaweIotSetRegmod(int flag)
{
	char *cmd;
	char *cmd0 = "AT+QREGSWT=0\r";
	char *cmd1 = "AT+QREGSWT=1\r";
	int ret;

	if(!flag)
	{
		cmd = cmd0;
	}
	else
	{
		cmd = cmd1;
	}
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28HuaweIotSetRegmod write cmd %s fail ret %d\r\n", cmd, ret); 
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28HuaweIotSetRegmod cmd %s read fail ret %d\r\n", cmd, ret); 
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s", cmd, neul_dev.rbuf);
	if(!strstr(neul_dev.rbuf,"OK")){
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28HuaweIotSetRegmod cmd %s read msg(%s) not contain OK\r\n", cmd, neul_dev.rbuf); 
		return -1;
	}
		
	return 0;
}

/*
the command is used to set registration mode after the module reboot
*/
int NeulBc28HuaweiIotQueryReg(int * type)
{
	char *cmd = "AT+QREGSWT?\r";
	int ret;
	
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28HuaweiIotQueryReg write %s error\r\n");
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28HuaweiIotQueryReg %s read error\r\n");
        return -1;
    }

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s", cmd, neul_dev.rbuf);
	ret = sscanf(neul_dev.rbuf,"\r\n+QREGSWT:%d%*[\r\n]",type);
	if(0 == ret){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28HuaweiIotQueryReg sscanf error\r\n");
		return -1;
	}
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "NeulBc28HuaweiIotQueryReg type %d\r\n", *type);
	
	return 0;
}

/* ============================================================
func name   :  NeulBc28HuaweiIotRegistration
discription :  0 Trigger register operation
			1 Trigger deregister operation
param       :  0 Trigger register operation, 1 Trigger deregister operation
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc28HuaweiIotRegistration(const int flag)
{
    char *cmd0 = "AT+QLWSREGIND=0\r";
    char *cmd1 = "AT+QLWSREGIND=1\r";
    char *cmd2 = "AT+QLWSREGIND=2\r";
	char *cmd = NULL;
    int ret = 0;

    memset(neul_dev.wbuf, 0, 64);
	if(flag == 0)
	{
    	cmd = cmd0;
	}
	if(flag == 1)
	{
    	cmd = cmd1;
	}
	if(flag == 2)
	{
    	cmd = cmd2;
	}
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);				
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s write error %d\r\n", __FUNCTION__, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);				
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s read error %d\r\n", __FUNCTION__, ret);
        return -1;
    }

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s", cmd, neul_dev.rbuf);
    
    return 0;
}

int NeulBc28QueryNatSpeed(int * baud_rate, int * sync_mode, int * stopbits, int * parity)
{
    char *cmd = "AT+NATSPEED?\r";
    int ret = 0;
	
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28QueryNatSpeed write cmd %s fail ret %d\r\n", cmd, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28QueryNatSpeed %s read msg(%s) fail ret %d\r\n", cmd, neul_dev.rbuf, ret);
        return -1;
    }

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s", cmd, neul_dev.rbuf);
	if(!strstr(neul_dev.rbuf,"OK")){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28QueryNatSpeed no OK\r\n");
		return -1;
	}
	
	ret = sscanf(neul_dev.rbuf, "\r\n+NATSPEED:%d,%d,%d,%d%*[\r\n]", baud_rate, sync_mode, stopbits, parity);
	if(0 == ret){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28QueryNatSpeed sscanf fail ret %d\r\n", ret);
		return -1;
	}
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "band:%d sync:%d stop:%d parity:%d\r\n", *baud_rate, *sync_mode, *stopbits, *parity);
	return 0;
}

int NeulBc28SetNatSpeed(void)
{
	char *cmd = "AT+NATSPEED=9600,3,1,2,1\r";
    int ret = 0;
	
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28SetNatSpeed write fail ret %d\r\n", ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28SetNatSpeed read msg(%s) fail ret %d\r\n",neul_dev.rbuf, ret);
        return -1;
    }

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s", cmd, neul_dev.rbuf);

	return 0;

}

/* ============================================================
func name   :  NeulBc28HexToStr
discription :  convert char to hex string 
               
param       :  bufin @ input param , the data that need to convert
               len @ input param, data length
               bufout @ output param, hex string data 
return      :  0 convert ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc28HexToStr(const unsigned char *bufin, int len, char *bufout)
{
    int i = 0;
    #if 0
    int tmp = 0;
    #endif
    if (NULL == bufin || len <= 0 || NULL == bufout)
    {
        return -1;
    }
    for(i = 0; i < len; i++)
    {
        #if 0
        tmp = bufin[i]>>4;
        bufout[i*2] = tmp > 0x09?tmp+0x37:tmp+0x30;
        tmp = bufin[i]&0x0F;
        bufout[i*2+1] = tmp > 0x09?tmp+0x37:tmp+0x30;
        #else
        sprintf(bufout+i*2, "%02X", bufin[i]);
        #endif
    }
    return 0; 
}

/* ============================================================
func name   :  NeulBc28HexToStr
discription :  convert hex string to hex data 
               
param       :  bufin @ input param , the data that need to convert
               len @ input param, data length
               bufout @ output param, hex data after convert 
return      :  0 send ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc28StrToHex(const char *bufin, int len, char *bufout)
{
    int i = 0;
    unsigned char tmp2 = 0x0;
    unsigned int tmp = 0;
    if (NULL == bufin || len <= 0 || NULL == bufout)
    {
        return -1;
    }
    for(i = 0; i < len; i = i+2)
    {
        #if 1
        tmp2 =  bufin[i];
        tmp2 =  tmp2 <= '9'?tmp2-0x30:tmp2-0x37;
        tmp =  bufin[i+1];
        tmp =  tmp <= '9'?tmp-0x30:tmp-0x37;
        bufout[i/2] =(tmp2<<4)|(tmp&0x0F);
        #else
        sscanf(bufin+i, "%02x", &tmp);
        bufout[i/2] = tmp;
        #endif
    }
    return 0; 
}

/* ============================================================
func name   :  NeulBc28SendCoapPaylaod
discription :  send coap message(hex data) to cdp server 
               
param       :  buf @ input param , the data that send to cdp
               sendlen @ input param, data length
return      :  0 send ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc28SendCoapPaylaod(const char *buf, int sendlen)
{
    char *cmd = "AT+QLWULDATA=";
    int ret = 0;
	char *str;

    if (NULL == buf || sendlen > MAX_MTU)
    {    
		MakeErrLog(DEBUG_MODULE_BC28,OPP_NULL_POINTER);				
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SendCoapPaylaod error sendlen %d\r\n",sendlen);
        return -1;
    }
    memset(neul_dev.wbuf, 0, NEUL_MAX_BUF_SIZE);
    memset(neul_bc95_tmpbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = NeulBc28HexToStr((unsigned char *)buf, sendlen, neul_bc95_tmpbuf);
    sprintf(neul_dev.wbuf, "%s%d,%s%c", cmd, sendlen, neul_bc95_tmpbuf, '\r');

	MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
	g_stBc28Pkt.iPktTx++;
	MUTEX_UNLOCK(g_stBc28Pkt.mutex);
	
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc95 failed        
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);				
		MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
		g_stBc28Pkt.iPktTxFail++;
		MUTEX_UNLOCK(g_stBc28Pkt.mutex);
		
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s write ret %d sendlen %d\n", cmd, ret, sendlen);
        return -2;
    }
    memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, 64, 0, 300);	
    if (ret <= 0)
    {   
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);				
        //read bc28 read set return value info failed
		MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
		g_stBc28Pkt.iPktTxFail++;
		MUTEX_UNLOCK(g_stBc28Pkt.mutex);
        
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %s error", cmd, neul_dev.rbuf);        
        return -3;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s,%s", neul_dev.wbuf, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");	
	if (!str)
	{	
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_READ_MSG_NO_OK);				
		MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
		g_stBc28Pkt.iPktTxFail++;
		MUTEX_UNLOCK(g_stBc28Pkt.mutex);
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s error devState %d\r\n", cmd, neul_dev.rbuf, NeulBc28GetDevState());    
    	return -4;
	}
	MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
	g_stBc28Pkt.iPktTxSucc++;
	MUTEX_UNLOCK(g_stBc28Pkt.mutex);
	
    return 0;
}

/* ============================================================
func name   :  NeulBc28GetUnrmsgCount
discription :  recv coap message(hex data) from cdp server 
               
param       :  buf @ input param , the data that send to cdp
               sendlen @ input param, data length
return      :  >=0 is read data length , <0 not ok
Revision    : 
=============================================================== */
int NeulBc28GetUnrmsgCount(char *buf, int len)
{
    char *cmd = "AT+NQMGR\r\n";
    int ret = 0;
    char *str = NULL;
    int msgcnt = 0;
    
    memset(neul_dev.rbuf, 0, 128);
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 128, 0, 300);
    if (ret < 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }
	
    str = strstr(neul_dev.rbuf,"BUFFERED");
    if (!str)
    {
        return 0;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd, neul_dev.rbuf);

	ret = sscanf(neul_dev.rbuf, "\r\nBUFFERED=%d,%s", &msgcnt, neul_dev.wbuf);
	if(0 == ret)
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s sscanf ret \n", cmd, ret);
		return -1;
	}
	
	BC28_STRNCPY(buf, neul_dev.rbuf, len);
    if (msgcnt < 0 )
    {
        return 0;
    }
	
	if (msgcnt > 0 )
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %s\n", cmd, neul_dev.rbuf);
	}
    return msgcnt;
}

int NeulBc28GetUnsmsgCount(char *buf, int len)
{
    char *cmd = "AT+NQMGS\r\n";
    int ret = 0;
    
    memset(neul_dev.rbuf, 0, 128);
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 128, 0, 300);
    if (ret < 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd, neul_dev.rbuf);
	
	BC28_STRNCPY(buf, neul_dev.rbuf, len);

	return 0;

}
int NeulBc28GetNnmi(char *buf, int len)
{
    char *cmd = "AT+NNMI?\r\n";
    int ret = 0;
	
    memset(neul_dev.rbuf, 0, 128);
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 128, 0, 300);
    if (ret < 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }
	BC28_STRNCPY(buf, neul_dev.rbuf, len);
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd, neul_dev.rbuf);

	return 0;
}

/* ============================================================
func name   :  NeulBc95SendCoapPaylaod
discription :  send coap message(hex data) to cdp server 
               
param       :  buf @ input param , the data that send to cdp
               sendlen @ input param, data length
return      :  0 send ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc28SendCoapPaylaodByNmgs(const char *buf, int sendlen)
{
    char *cmd = "AT+NMGS=";
    char *cmd2 = "AT+NQMGS\r";
    int ret = 0;
    char *str = NULL;
    static int sndcnt = 0;
    int curcnt = 0;
    
    if (NULL == buf || sendlen >= 512)
    {
        return -1;
    }
		
    memset(neul_dev.wbuf, 0, 64);
    memset(neul_bc95_tmpbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = NeulBc28HexToStr((unsigned char *)buf, sendlen, neul_bc95_tmpbuf);
    sprintf(neul_dev.wbuf, "%s%d,%s%c", cmd, sendlen, neul_bc95_tmpbuf, '\r');
	printf("write buffer: %s\r\n", neul_dev.wbuf);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        printf("%s write ret %d sendlen %d\n", cmd, ret, sendlen);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
	#if 0
	while(1)
	{
	    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 200);
		#if 0
	    if (ret <= 0)
	    {
	        //read bc95 read set return value info failed
	        printf("%s %s error sendlen %d\n", cmd, neul_dev.rbuf, sendlen);
			RoadLamp_Dump(buf, sendlen);
	        return -1;
	    }
		#else
	    if (ret > 0)
	    {
	        printf("%s %s sendlen %d\n", cmd, neul_dev.rbuf, sendlen);
			break;
		}
		#endif
	}
	#else
	
	//ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 5000);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
	if (ret <= 0)
	{
		//read bc28 read set return value info failed
		printf("%s %s error sendlen %d\n", cmd, neul_dev.rbuf, sendlen);
		//RoadLamp_Dump(buf, sendlen);
		return -1;
	}
	#endif
	printf("%s %s", cmd, neul_dev.rbuf);
    str = strstr(neul_dev.rbuf,"OK");
	if (!str)
	{
    	//printf("%s error", cmd);    
    	return -1;
	}

    /* query the message if send */
    memset(neul_dev.rbuf, 0, 64);
    ret = neul_dev.ops->dev_write(cmd2, strlen(cmd2), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        printf("%s write ret %d", cmd2, ret);        
        return -1;
    }
    //ret = neul_dev.ops->dev_read(neul_dev.rbuf, 64, 0, 5000);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);	
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        printf("%s %s error", cmd2, neul_dev.rbuf);        
        return -1;
    }
		
    str = strstr(neul_dev.rbuf,"SENT="); 
    if (!str)
    {
        return -1;
    }
		
    ret = sscanf(str, "SENT=%d,%s", &curcnt, neul_dev.wbuf);
	if(0 == ret){
		return -1;
	}
    if (curcnt == sndcnt){
        return -1;
    }
    sndcnt = curcnt;
    return 0;
}
/* ============================================================
func name   :  NeulBc95ReadCoapMsg
discription :  recv coap message(hex data) from cdp server 
               
param       :  buf @ input param , the data that send to cdp
               sendlen @ input param, data length
return      :  >=0 is read data length , <0 not ok
Revision    : 
=============================================================== */
int NeulBc28ReadCoapMsgByNmgr(char *outbuf, int maxrlen)
{
    char *cmd = "AT+NMGR\r\n";
    int ret = 0;
    char *str = NULL;
    int readlen = 0;

    if (NULL == outbuf || maxrlen == 0)
    {
        return -1;
    }

    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 300);
    if (ret < 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }

	//printf("NMGR %s", neul_dev.rbuf);
	
    str = strstr(neul_dev.rbuf,"OK");
    if (!str && strlen(neul_dev.rbuf) <=10 )
    {
        return 0;
    }
    ret = sscanf(neul_dev.rbuf, "\r\n%d,%s\r\n\r\nOK\r\n",&readlen, neul_bc95_tmpbuf);
	if (0 == ret){
		return -1;
	}
	
    if (readlen > 0){
        NeulBc28StrToHex(neul_bc95_tmpbuf, readlen*2, outbuf);
        return readlen;
    }
    return 0;
}

/* ============================================================
func name   :  NeulBc95ReadCoapMsg
discription :  recv coap message(hex data) from cdp server 
               
param       :  buf @ input param , the data that send to cdp
               sendlen @ input param, data length
return      :  >=0 is read data length , <0 not ok
Revision    : 
=============================================================== */
int NeulBc28LogLevelSet(const char *core, const char *level)
{
    char *cmd = "AT+NLOGLEVEL=";
    int ret = 0;

    memset(neul_dev.wbuf, 0, 64);
    sprintf(neul_dev.wbuf, "%s%s,%s%c", cmd, core, level, '\r');
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 300);
    if (ret < 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }

    return 0;
}


/* ============================================================
func name   :  NeulBc95ReadCoapMsg
discription :  recv coap message(hex data) from cdp server 
               
param       :  buf @ input param , the data that send to cdp
               sendlen @ input param, data length
return      :  >=0 is read data length , <0 not ok
Revision    : 
=============================================================== */
int NeulBc28LogLevelGet(char * buffer, int len)
{
    char *cmd = "AT+NLOGLEVEL?";
    int ret = 0;

    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 300);
    if (ret < 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s", cmd, neul_dev.rbuf);

	BC28_STRNCPY(buffer, neul_dev.rbuf, len);

    return 0;
}

int NeulBc28Command(const char *cmd, char *buffer, int len)
{
	int ret;

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s", cmd);
	
    ret = neul_dev.ops->dev_write((char *)cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 1000);
    if (ret < 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s", cmd, neul_dev.rbuf);
	BC28_STRNCPY(buffer, neul_dev.rbuf, len);
	return 0;
}

/* ============================================================
func name   :  NeulBc28CreateUdpSocket
discription :  create udp socket 
               
param       :  localport @ input param , the port local socket used
return      :  >=0 socket handle , < 0 not ok
Revision    : 
=============================================================== */
int NeulBc28CreateUdpSocket(unsigned short localport)
{
    char *cmd = "AT+NSOCR=DGRAM,17,";
    int ret = 0;
    int socket = -1;
	
    if (0 == localport)
    {
        return -1;
    }
    memset(neul_dev.wbuf, 0, 64);
    sprintf(neul_dev.wbuf, "%s%d,1\r", cmd, localport);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "write %s error ret %d\r\n", neul_dev.wbuf, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "read %s error ret %d\r\n", neul_dev.rbuf, ret);
        return -1;
    }
	/*
	printf("~~~~%s~~~~", neul_dev.rbuf);
    	str = strstr(neul_dev.rbuf,"OK");
    	if (!str)
    	{
        return -1;
    	}
	printf("#########\r\n");	
	for(i = 0; i<strlen(neul_dev.rbuf); i++)
	{
		printf("%02x ", neul_dev.rbuf[i]);
	}
	printf("!!!!!!!!!!\r\n");
	*/
    //sscanf(neul_dev.rbuf, "%d\r%s",&socket, neul_bc95_tmpbuf);
    ret = sscanf(neul_dev.rbuf, "\nOK\r\n\r\n%d\r\n\r\nOK\r\n",&socket);
	if(0 == ret){
		return -1;
	}
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "error11 socket %d\r\n", socket);
	
    if (socket >= 0)
    {
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "socket %d\r\n", socket);
        return socket;
    }
    return -1;
}

/* ============================================================
func name   :  NeulBc28CloseUdpSocket
discription :  close udp socket 
               
param       :  socket @ input param , the socket handle that need close
return      :  0 close ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc28CloseUdpSocket(int socket)
{
    char *cmd = "AT+NSOCL=";
    int ret = 0;
    char *str = NULL;
    
    memset(neul_dev.wbuf, 0, 64);
    sprintf(neul_dev.wbuf, "%s%d\r", cmd, socket);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }

    NeulBc28ResetUpdParam(socket);
    
    return 0;
}

/* ============================================================
func name   :  NeulBc28SocketConfigRemoteInfo
discription :  set remote address info that socket will send data to 
               
param       :  socket @ input param , the socket handle that need close
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc28SocketConfigRemoteInfo(int socket, const char *remoteip, unsigned short port)
{
    int ret = 0;
    if (socket < 0 || NULL == remoteip || port == 0)
    {
        return -1;
    }
    ret = NeulBc28SetUpdParam(remoteip, port, socket);
    return ret;
}

/* ============================================================
func name   :  NeulBc28UdpSend
discription :  send data to socket 
               
param       :  socket @ input param , the data will send to this socket
               buf @ input param, the data buf
               sendlen @ input param, the send data length
return      :  0 send data ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc28UdpSend(int socket, const char *buf, int sendlen)
{
    char *cmd = "AT+NSOST=";
    int ret = 0;
    char *str = NULL;
    
    ret = sscanf(neul_dev.rbuf, "%d\r%s",&socket, neul_bc95_tmpbuf);
	if(0 == ret){
		return -1;
	}
	
    if (socket < 0 || NULL == buf || 0 == sendlen)
    {
        return -1;
    }
    
    memset(neul_dev.wbuf, 0, NEUL_MAX_BUF_SIZE);
    memset(neul_bc95_tmpbuf, 0, NEUL_MAX_BUF_SIZE);
    NeulBc28HexToStr((unsigned char *)buf, sendlen, neul_bc95_tmpbuf);
    sprintf(neul_dev.wbuf, "%s%d,%s,%d,%d,%s\r", cmd, socket,
            (neul_dev.addrinfo+socket)->ip,
            (neul_dev.addrinfo+socket)->port,
            sendlen,
            neul_bc95_tmpbuf);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        return -1;
    }
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        return -1;
    }
    return 0;
}

/* ============================================================
func name   :  NeulBc28UdpRead
discription :  read data from socket 
               
param       :  socket @ input param , the data will read from this socket
               buf @ out param, store the data read
               maxrlen @ input param, the max read data length
return      :  >0 read data length , <0 not ok
Revision    : 
=============================================================== */
int NeulBc28UdpRead(int socket,char *buf, int maxrlen, int mode)
{
    char *cmd = "AT+NSORF=";
    int ret = 0;
    char *str = NULL;
    int rlen = 0;
    int rskt = -1;
    int port = 0;
    int readleft = 0;
	char buf1[300] = {0};
	int dot1, dot2, dot3, dot4;
	int len = 0;
	int sock, len1;
	
    if (socket < 0 || NULL == buf || maxrlen <= 0)
    {
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
    
    sprintf(neul_dev.wbuf, "%s%d,%d\r", cmd, socket, maxrlen);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
    memset(neul_dev.rbuf, 0, 100);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 100, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\r\n", neul_dev.wbuf, neul_dev.rbuf);
#if 0	
	for(;;)
	{
		if(neul_dev.rbuf[i] == '\0')
			break;
		printf("0x%x ", neul_dev.rbuf[i]);
		//printf("%c ", neul_dev.rbuf[i]);
		i++;
	}
	printf("\r\n");
#endif	
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }

	str = strstr(neul_dev.rbuf, "\r\n+NSONMI:");
	/*if(!str)
	{
		return -1;
	}
	*/
	if(str)
	{
	#if 1
		len = str - neul_dev.rbuf;
		if(len == 0)
		{
			ret = sscanf(neul_dev.rbuf, "\r\n+NSONMI:%d,%d\r\n\r\n%d,%d.%d.%d.%d,%d,%d,%s,%d\r\n%s\r\n\r\n", 
									&sock,
									&len1,
									&rskt,
									&dot1,
									&dot2,
									&dot3,
									&dot4,
									&port,
									&rlen,
									neul_bc95_tmpbuf,
									&readleft,
									neul_dev.wbuf);
			if(0 == ret){
				return -1;
			}
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "+++buf1 %s\r\n", neul_bc95_tmpbuf);			
			
			if (rlen > 0)
			{
				DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "+++>sock %d port %d rlen %d %s\r\n", rskt, port, rlen, neul_bc95_tmpbuf);			
				NeulBc28StrToHex(neul_bc95_tmpbuf, rlen*2, buf);
			}
			
			return rlen;
		}
		else
#endif
		{
			BC28_STRNCPY(buf1, neul_dev.rbuf, str - neul_dev.rbuf);
			//buf1[str - neul_dev.rbuf] = '\0';
		}
	}
	else
	{
		BC28_STRNCPY(buf1, neul_dev.rbuf, strlen(neul_dev.rbuf));
		//buf1[strlen(neul_dev.rbuf)] = '\0';
	}
		
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "!!!buf1 %s\r\n", buf1);
	
    ret = sscanf(/*neul_dev.rbuf*/buf1, "\r\n%d,%d.%d.%d.%d,%d,%d,%s,%d\r\n%s\r\n\r\n", 
							&rskt,
                            &dot1,
                            &dot2,
                            &dot3,
                            &dot4,
                            &port,
                            &rlen,
                            neul_bc95_tmpbuf,
                            &readleft,
                            neul_dev.wbuf);
	if(0 == ret){
		return -1;
	}
	
    if (rlen > 0)
    {
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "==>sock %d port %d rlen %d %s\r\n", rskt, port, rlen, neul_bc95_tmpbuf);    
        NeulBc28StrToHex(neul_bc95_tmpbuf, rlen*2, buf);
    }
    
    return rlen;
}


/* ============================================================
func name   :  NeulBc28UdpNewRead
discription :  read data from socket 
               
param       :  socket @ input param , the data will read from this socket
               buf @ out param, store the data read
               maxrlen @ input param, the max read data length
return      :  >0 read data length , <0 not ok
Revision    : 
=============================================================== */
int NeulBc28UdpNewRead(int socket,char *buf, int maxrlen, int mode, int * rleft)
{
    //AT+NSORF=0,4
    char *str = NULL;    
    char *cmd = "AT+c=";
    int ret = 0;
    int rlen = 0;
    int rskt = -1;
    int port = 0;
    static int readleft = 0;
    int dot1, dot2, dot3, dot4;
    int len = 0;
    int sock;
    int num;
    char *buf2 = "\r\nOK\r\n";
	
	//printf("%s %d\r\n", __FUNCTION__, __LINE__);
    if (socket < 0 || NULL == buf || maxrlen <= 0)
    {
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
    
    sprintf(neul_dev.wbuf, "%s%d,%d\r", cmd, socket, maxrlen);
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
    memset(neul_dev.rbuf, 0, maxrlen);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, maxrlen, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %d\r\n", __FUNCTION__, __LINE__);	
        return -1;
    }
	
	//printf("\r\n***********\r\n%s\r\n%s\r\n**************\r\n", neul_dev.wbuf, neul_dev.rbuf);


    str = strstr(neul_dev.rbuf,"\r\n+CSCON:");
    if (str)
    {
        return -1;
    }

	if(strcmp(neul_dev.rbuf, buf2) == 0)
	{
		*rleft = 0;
		return 0;
	}
	str = strstr(neul_dev.rbuf, "\r\n+NSONMI:");
	/*if(!str)
	{
		return -1;
	}
	*/
	if(str)
	{
		len = str - neul_dev.rbuf;
		if(len == 0)
		{
			ret = sscanf(neul_dev.rbuf, "\r\n+NSONMI:%d,%d\r\n", 
									&sock,
									&readleft);
			if(0 == ret){
				return -1;
			}
			//printf("1111 neul_bc95_tmpbuf %s\r\n", neul_bc95_tmpbuf);			
			/*
			if (rlen > 0)
			{
				printf("+++>sock %d port %d rlen %d %s\r\n", rskt, port, rlen, neul_bc95_tmpbuf);			
				NeulBc28StrToHex(neul_bc95_tmpbuf, rlen*2, buf);
			}
			*/
			
			//printf("111 NSONMI sock %d len %d\r\n", sock, readleft);

			*rleft = readleft;			
			return 0;
		}
		else
		{
		#if 0
			BC28_STRNCPY(buf1, neul_dev.rbuf, str - neul_dev.rbuf);
			buf1[str - neul_dev.rbuf] = '\0';
		#else
			ret = sscanf(neul_dev.rbuf, "\r\nOK\r\n+NSONMI:%d,%d\r\n\r\n", 
								&sock,
								&readleft);
			if(0 == ret){
				return -1;
			}
			//printf("222 NSONMI sock %d len %d\r\n", sock, readleft);

			*rleft = readleft; 
			return 0;
		#endif
		}
	}
/*
	else
	{
		BC28_STRNCPY(buf1, neul_dev.rbuf, strlen(neul_dev.rbuf));
		buf1[strlen(neul_dev.rbuf)] = '\0';
	}
*/		
	//printf("!!!neul_dev.rbuf %s\r\n", neul_dev.rbuf);

	num = string_count(neul_dev.rbuf, ",");

	//printf("\r\nnum %d\r\n", num);

	memset(neul_bc95_tmpbuf, 0, sizeof(neul_bc95_tmpbuf));
	
	if(num == 4)
	{
	    ret = sscanf(neul_dev.rbuf, "\r\n%d,%d.%d.%d.%d,%d,%d,%s", 
								&rskt,
	                            &dot1,
	                            &dot2,
	                            &dot3,
	                            &dot4,
	                            &port,
	                            &rlen,
	                            neul_bc95_tmpbuf);
		if(0 == ret){
			return -1;
		}
		if(strlen(neul_bc95_tmpbuf) < rlen)
		{
			*rleft = readleft - strlen(neul_bc95_tmpbuf);
		}
		else
		{
			*rleft = readleft - rlen;
		}
		
		//printf("4 neul_bc95_tmpbuf rlen %d rleft %d %s\r\n", strlen(neul_bc95_tmpbuf), *rleft, neul_bc95_tmpbuf);
	}
	if(num == 5)
	{
	    ret = sscanf(neul_dev.rbuf, "\r\n%d,%d.%d.%d.%d,%d,%d,%[^,],%d\r\nOK\r\n", 
								&rskt,
	                            &dot1,
	                            &dot2,
	                            &dot3,
	                            &dot4,
	                            &port,
	                            &rlen,
	                            neul_bc95_tmpbuf,
	                            &readleft);
		if(0 == ret){
			return -1;
		}
		*rleft = readleft;
		//printf("5 neul_bc95_tmpbuf rlen %d rleft %d %s\r\n", rlen, *rleft, neul_bc95_tmpbuf);
	}
	if(num == 0)
	{
		rlen = strlen(neul_dev.rbuf);
		BC28_STRNCPY(neul_bc95_tmpbuf, neul_dev.rbuf, strlen(neul_dev.rbuf));
		//neul_bc95_tmpbuf[strlen(neul_dev.rbuf)] = '\0';

		*rleft = readleft - rlen;		
		//printf("0 neul_bc95_tmpbuf rlen %d rfetft %d %s\r\n", rlen, *rleft, neul_bc95_tmpbuf);
	}
	
    if (rlen > 0)
    {
		//printf("==>sock %d port %d rlen %d %s\r\n", rskt, port, rlen, neul_bc95_tmpbuf);    
        NeulBc28StrToHex(neul_bc95_tmpbuf, rlen*2, buf);
    }
    
    return rlen;
}
static int NeulBc28NconfigGet(char *buf, int len)
{
	int ret = 0, rlen = 0;
    char *cmd = "AT+NCONFIG?\r";
	
	ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
	if(ret < 0){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28NconfigGet write %s fail ret %d\r\n", cmd, ret);
		return -1;
	}
	
    memset(neul_dev.rbuf, 0, len);
    rlen = neul_dev.ops->dev_read(neul_dev.rbuf, len, 0, 1000);
	if(rlen <= 0){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28NconfigGet read %s msg(%s)fail ret %d\r\n", cmd, neul_dev.rbuf, ret);        
        return -1;
	}
    if (!strstr(neul_dev.rbuf,"OK"))
    {
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28NconfigGet read msg(%s) not contain OK\r\n", neul_dev.rbuf);        
        return -1;
    }
	BC28_STRNCPY(buf, neul_dev.rbuf, len);
	return rlen;
}

static int NeulBc28ApnConfigSet(char * apn)
{
	int ret = 0;
    char *cmd = "AT+CGDCONT=1,\"IP\",";	
	
	memset(neul_dev.wbuf, 0, 128);
    sprintf(neul_dev.wbuf, "%s\"%s\"%c", cmd, apn, '\r');
    /* Config apn */
    ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork write %s fail ret %d\r\n", neul_dev.wbuf, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork read %s msg(%s) fail ret %d\r\n", neul_dev.wbuf, neul_dev.rbuf, 
        ret);
        return -2;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", neul_dev.wbuf, neul_dev.rbuf);
    if (!strstr(neul_dev.rbuf,"OK"))
    {
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork %s read msg(%s) not contain OK\r\n", neul_dev.wbuf, neul_dev.rbuf);
        return -1;
    }
	return 0;
}

static int NeulBc28ApnConfigGet(char * buf, int len)
{
	int ret = 0;
	char *cmd = "AT+CGDCONT?\r"; 
	
	/* Config apn */
	ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
	if (ret < 0)
	{
		//write data to bc28 failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork write %s fail ret %d\r\n", cmd, ret);
		return -1;
	}
	memset(neul_dev.rbuf, 0, len);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, len, 0, 300);
	if (ret <= 0)
	{
		//read bc28 read set return value info failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork read %s msg(%s) fail ret %d\r\n", cmd, neul_dev.rbuf, 
		ret);
		return -2;
	}
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd, neul_dev.rbuf);
	if (!strstr(neul_dev.rbuf,"OK"))
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28ActiveNetwork %s read msg(%s) not contain OK\r\n", cmd, neul_dev.rbuf);
		return -1;
	}

	BC28_STRNCPY(buf,neul_dev.rbuf,len);
	
	return 0;
}

/* ============================================================
func name   :  NeulBc28SetAutoConnect
discription :  set bc95 module auto connect network
               
param       :  None
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc28SetAutoConnect(int flag)
{
    int ret = 0;
    char *str = NULL;
    char *p1 = "AT+NCONFIG=AUTOCONNECT,TRUE\r";
    char *p2 = "AT+NCONFIG=AUTOCONNECT,FALSE\r";
	char *p;
	
    if (flag)
    {
        //set auto connect
        ret = neul_dev.ops->dev_write(p1, strlen(p1), 0);
		p = p1;
    }
    else
    {
        //not auto connect
        ret = neul_dev.ops->dev_write(p2, strlen(p2), 0);
		p = p2;
    }
	if(ret < 0)
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28SetAutoConnect write %s fail ret %d\r\n", p, ret);
		return -1;
	}
	
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28SetAutoConnect read %s msg(%s)fail ret %d\r\n", p, neul_dev.rbuf, ret);        
        return -1;
    }

    if (flag)
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", p1, neul_dev.rbuf);
    }
	else
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO,"%s %s\n", p2, neul_dev.rbuf);
	}
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"NeulBc28SetAutoConnect read msg(%s) not contain OK\r\n", neul_dev.rbuf);        
        return -1;
    }
    return 0;
}

/* ============================================================
func name   :  NeulBc28SetScramble
discription :  set bc28 module scramble
               
param       :  None
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc28SetScramble(int flag)
{
    int ret = 0;
    char *str = NULL;
    char *p1 = "AT+NCONFIG=CR_0354_0338_SCRAMBLING,TRUE\r";
    char *p2 = "AT+NCONFIG=CR_0354_0338_SCRAMBLING,FALSE\r";
    char *p3 = "AT+NCONFIG=CR_0859_SI_AVOID,TRUE\r";
    char *p4 = "AT+NCONFIG=CR_0859_SI_AVOID,FALSE\r";

	//CR_0354_0338_SCRAMBLING
    if (flag)
    {
        //set CR_0354_0338_SCRAMBLING trte
        ret = neul_dev.ops->dev_write(p1, strlen(p1), 0);
		
    }
    else
    {
        //CR_0354_0338_SCRAMBLING false
        ret = neul_dev.ops->dev_write(p2, strlen(p2), 0);
    }
	if(ret < 0){
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetScramble write CR_0354_0338_SCRAMBLING fail ret %d\r\n", ret);    
		return -1;
	}
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetScramble read CR_0354_0338_SCRAMBLING fail msg(%s) ret %d\r\n", neul_dev.rbuf, ret);    
        return -1;
    }

    if (flag)
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", p1, neul_dev.rbuf);
    }
	else
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", p2, neul_dev.rbuf);
	}
	
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetScramble read CR_0354_0338_SCRAMBLING msg(%s) not contain OK\r\n", neul_dev.rbuf);    
        return -1;
    }
	
	//SI_AVOID
    if (flag)
    {
        //set SI_AVOID true
        ret = neul_dev.ops->dev_write(p3, strlen(p3), 0);
		
    }
    else
    {
        //set SI_AVOID false
        ret = neul_dev.ops->dev_write(p4, strlen(p4), 0);
    }
	if(ret < 0)
	{
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetScramble write CR_0859_SI_AVOID fail ret %d\r\n", ret);    
		return -1;
	}
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetScramble read CR_0859_SI_AVOID fail ret %d\r\n", ret);    
        return -1;
    }

    if (flag)
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", p3, neul_dev.rbuf);
    }
	else
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", p4, neul_dev.rbuf);
	}
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetScramble read CR_0859_SI_AVOID msg not contain OK\r\n");    
        return -1;
    }
	
    return 0;
}

/* ============================================================
func name   :  NeulBc28SetReselection
discription :  set bc28 module scramble
               
param       :  None
return      :  0 set ok , !0 not ok
Revision    : 
=============================================================== */
static int NeulBc28SetReselection(int flag)
{
    int ret = 0;
    char *str = NULL;
    char *p1 = "AT+NCONFIG=COMBINE_ATTACH,TRUE\r";
    char *p2 = "AT+NCONFIG=COMBINE_ATTACH,FALSE\r";
    char *p3 = "AT+NCONFIG=CELL_RESELECTION,TRUE\r";
    char *p4 = "AT+NCONFIG=CELL_RESELECTION,FALSE\r";
	char *p = NULL;
	
	//CR_0354_0338_SCRAMBLING
    if (flag)
    {
        //set COMBINE_ATTACH true
        ret = neul_dev.ops->dev_write(p1, strlen(p1), 0);
		p = p1;
    }
    else
    {
        //set COMBINE_ATTACH false
        ret = neul_dev.ops->dev_write(p2, strlen(p2), 0);
		p = p2;
    }
	if(ret < 0){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetReselection write %s error\r\n", p);
		return -1;
	}
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetReselection read %s msg(%s) error\r\n", p, neul_dev.rbuf);
        return -1;
    }

    if (flag)
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", p1, neul_dev.rbuf);
    }
	else
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", p2, neul_dev.rbuf);
	}
	
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetReselection read msg not contain OK\r\n");
        return -1;
    }
	
	//SI_AVOID
    if (flag)
    {
        //set CELL_RESELECTION true
        ret = neul_dev.ops->dev_write(p3, strlen(p3), 0);
		p = p3;
    }
    else
    {
        //set CELL_RESELECTION false
        ret = neul_dev.ops->dev_write(p4, strlen(p4), 0);
		p = p4;
    }
	if(ret < 0){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetReselection write %s error\r\n", p);
		return -1;
	}
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetReselection read %s msg(%s) error\r\n", p, neul_dev.rbuf);
        return -1;
    }

    if (flag)
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", p3, neul_dev.rbuf);
    }
	else
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", p4, neul_dev.rbuf);
	}
    str = strstr(neul_dev.rbuf,"OK");
    if (!str)
    {
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28SetReselection read %s msg(%s) not contain OK\r\n", p, neul_dev.rbuf);
        return -1;
    }
	
    return 0;
}

/*
0 The automatic FOTA upgrade mode is used.
1 The controlled FOTA upgrade mode is used
2 MCU notify module to start downloading the version file.
3 MCU notify module to cancel version file download.
4 MCU notify module to start update.
5 MCU notify module to cancel update .
*/
int NeulBc28DfotaSet(int updateMode)
{
	char *cmd = "AT+QLWFOTAIND=";
	int ret;

    memset(neul_dev.wbuf, 0, NEUL_MAX_BUF_SIZE);
	sprintf(neul_dev.wbuf, "%s%d%c", cmd, updateMode, '\r');
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"write %s,%s\r\n", neul_dev.wbuf);		
	ret = neul_dev.ops->dev_write(neul_dev.wbuf, strlen(neul_dev.wbuf), 0);
	if (ret < 0)
	{
		//write data to bc28 failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_WRITE_ERR);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s,%d\r\n", __FUNCTION__, __LINE__);
		return -1;
	}
	memset(neul_dev.rbuf, 0, NEUL_MAX_BUF_SIZE);
	ret = neul_dev.ops->dev_read(neul_dev.rbuf, NEUL_MAX_BUF_SIZE, 0, 300);
	if (ret <= 0)
	{
		//read bc28 read set return value info failed
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_UART_READ_TIMEOUT);
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s,%d\r\n", __FUNCTION__, __LINE__);		
		return -1;
	}
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR,"%s,%s\r\n", neul_dev.wbuf, neul_dev.rbuf);

	return 0;
}

int NeulBc28HardRestStateGet()
{
	int hardReboot;
	
	MUTEX_LOCK(g_stHardReset.mutex,MUTEX_WAIT_ALWAYS);
	hardReboot = g_stHardReset.iHardRebooted;	//rebooted
	MUTEX_UNLOCK(g_stHardReset.mutex);
	return hardReboot;
}

int NeulBc28HardRestStateSet(int reboot)
{	
	MUTEX_LOCK(g_stHardReset.mutex,MUTEX_WAIT_ALWAYS);
	g_stHardReset.iHardRebooted = reboot;	//rebooted
	MUTEX_UNLOCK(g_stHardReset.mutex);
	return 0;
}

/**/
/* ============================================================
func name	:  NeulBc28UdpNewRead
discription :  get nbiot hardreset result, this command can be called until 6s to 10min from device reboot		   
param		:  
return		:  0, reboot success
   			   1, reboot fail
    		   2, rebooting please wait
Revision	: 
=============================================================== */
int NeulBc28HardRestResultGet()
{
	int ret;
	
	if(REBOOTING == NeulBc28HardRestStateGet()){
		return 2;
	}
	
	MUTEX_LOCK(g_stHardReset.mutex,MUTEX_WAIT_ALWAYS);
	if(REBOOT_OK == g_stHardReset.iResult)
		ret = 0;
	else
		ret = 1;
	MUTEX_UNLOCK(g_stHardReset.mutex);
	return ret;
}

int NeulBc28HardRestResultSet(int result)
{
	MUTEX_LOCK(g_stHardReset.mutex,MUTEX_WAIT_ALWAYS);
	g_stHardReset.iResult = result;
	MUTEX_UNLOCK(g_stHardReset.mutex);
	return 0;
}

static int NeulBc28OnOff(int onoff)
{
    char *cmd0 = "AT+CFUN?\r";
    char *cmd1 = "AT+CFUN=0\r";
    char *cmd2 = "AT+CFUN=1\r";
	char *cmd;
	int ret = 0;
	int cfun;
	
    ret = neul_dev.ops->dev_write(cmd0, strlen(cmd0), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28OnOff write cmd %s fail ret %d\n", cmd0, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 6000);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28OnOff cmd %s read msg %s fail ret %d\n", cmd0, neul_dev.rbuf, ret);
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd0, neul_dev.rbuf);
    if (!strstr(neul_dev.rbuf,"OK"))
    {
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28OnOff cmd %s read msg %s no OK\n", cmd0, neul_dev.rbuf);
        return -1;
    }

	sscanf(neul_dev.rbuf, "\r\n+CFUN:%d%*[\r\n]", &cfun);
	printf("NeulBc28OnOff cfun:%d\r\n", cfun);

	if(cfun == onoff){
		return 0;
	}

	if(0 == onoff){
		cmd = cmd1;
	}else{
		cmd = cmd2;
	}
	
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28OnOff write cmd %s fail ret %d\n", cmd, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 6000);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28OnOff cmd %s read msg %s fail ret %d\n", cmd, neul_dev.rbuf, ret);
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd0, neul_dev.rbuf);
    if (!strstr(neul_dev.rbuf,"OK"))
    {
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28OnOff cmd %s read msg %s no OK\n", cmd, neul_dev.rbuf);
        return -1;
    }
	
	return 0;
}



static int NeulBc28Cgatt(int onOff)
{
    char *cmd0 = "AT+CGATT?\r";
    char *cmd1 = "AT+CGATT=0\r";
    char *cmd2 = "AT+CGATT=1\r";
	char *cmd;
	int ret = 0;
	int cgatt;
	
    ret = neul_dev.ops->dev_write(cmd0, strlen(cmd0), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Cgatt write cmd %s fail ret %d\n", cmd0, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Cgatt cmd %s read msg %s fail ret %d\n", cmd0, neul_dev.rbuf, ret);
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd0, neul_dev.rbuf);
    if (!strstr(neul_dev.rbuf,"OK"))
    {
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Cgatt cmd %s read msg %s no OK\n", cmd0, neul_dev.rbuf);
        return -1;
    }

	ret = sscanf(neul_dev.rbuf, "\r\n+CGATT:%d%*[\r\n]", &cgatt);
	if(0 == ret){
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Cgatt cmd %s sscanf fail ret\n", cmd0, ret);
		return -1;
	}
	//printf("NeulBc28Cgatt cgatt %d onOff:%d\r\n", cgatt, onOff);

	if(cgatt == onOff){
		return 0;
	}

	if(0 == onOff){
		cmd = cmd1;
	}else{
		cmd = cmd2;
	}
	
    ret = neul_dev.ops->dev_write(cmd, strlen(cmd), 0);
    if (ret < 0)
    {
        //write data to bc28 failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28OnOff write cmd %s fail ret %d\n", cmd, ret);
        return -1;
    }
    memset(neul_dev.rbuf, 0, 32);
    ret = neul_dev.ops->dev_read(neul_dev.rbuf, 32, 0, 300);
    if (ret <= 0)
    {
        //read bc28 read set return value info failed
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28OnOff cmd %s read msg %s fail ret %d\n", cmd, neul_dev.rbuf, ret);
        return -1;
    }
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "%s %s\n", cmd0, neul_dev.rbuf);
    if (!strstr(neul_dev.rbuf,"OK"))
    {
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28OnOff cmd %s read msg %s no OK\n", cmd, neul_dev.rbuf);
        return -1;
    }
	
	//NeulBc28Sleep(3000);
	return 0;
}


/* ============================================================
func name   :  NeulBc28HwInit
discription :  init neul bc28 device 
               
param       :  None
return      :  0 init ok , !0 not ok
Revision    : 
=============================================================== */
int NeulBc28HwInit(void)
{
    int ret = 0;
	ST_NB_CONFIG stConfigPara;
	ST_IOT_CONFIG stIotConfigPara;
	static int doReboot = 0;
	static int times = 0;
	//int needReboot = 0;
	char outbuf[NEUL_MAX_BUF_SIZE] = {0};
	int type;
	int baud_rate, sync_mode, stopbits, parity;

	//hard rebooting
	if(REBOOTING == NeulBc28HardRestStateGet()){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "Bc28 hard rebooting...\r\n");
		NbiotDataRead(outbuf,NEUL_MAX_BUF_SIZE,0,100);		
		vTaskDelay(10 / portTICK_PERIOD_MS);
		return -1;
	}
	if(!doReboot){
		//printf("BC28 reboot\r\n");
    	ret = NeulBc28Reboot();
		if(ret < 0){			
			MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_AT_REBOOT_ERR);
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Reboot ret %d\r\n", ret);
			return ret;
		}
		doReboot = 1;
		g_iAtRebooted = 0;
	}
	//waiting bc28 rebooted
	if(0 == g_iAtRebooted){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "AT rebooting...\r\n");
		NbiotDataRead(outbuf,NEUL_MAX_BUF_SIZE,0,100);
		return -1;
	}else{
		printf("soft reboot tick %d\r\n", OppTickCountGet());
		if(times++ > DEVINIT_RETRY_TIMES){
			doReboot = 0;
			times = 0;
		}
	}

	//no echo
	ret = NeulBc28ATESet();
    if (ret < 0){
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_AT_REBOOT_ERR);
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28ATESet fail ret %d\r\n", OppTickCountGet(),ret);    
        return ret;
    }
	NeulBc28Sleep(30);
	ret = NeulBc28QueryNatSpeed(&baud_rate, &sync_mode, &stopbits, &parity);
    if (ret < 0){
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_QRY_UART);
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28QueryNatSpeed fail ret %d\r\n", OppTickCountGet(), ret);    
        return ret;
    }
	if(UART_BAUDRATE_DEFAULT != baud_rate 
		|| UART_SYNC_DEFAULT != sync_mode
		|| UART_STOPBIT_DEFAULT != stopbits){
		NeulBc28Sleep(30);
		ret = NeulBc28SetNatSpeed();
	    if (ret < 0){
			MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_SET_UART);
	        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28SetNatSpeed fail ret %d\r\n", OppTickCountGet(), ret);    
	        return ret;
	    }
	}
	NeulBc28Sleep(30);
	ret = NeulBc28ErrorReport();
	if(ret < 0){
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_ACTIVE_NETWORK_ERR);
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28ErrorReport fail ret %d\r\n", OppTickCountGet(), ret);    
		return ret;
	}
	//NeulBc28QueryNatSpeed();
	//NeulBc28Sleep(2000);
    ret = NeulBc28HwDetect();
    if (ret < 0){
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_HWDET_ERR);
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28HwDetect fail ret %d\r\n", OppTickCountGet(), ret);    
        return ret;
    }
	NeulBc28Sleep(30);	
	//NeulBc28DeactiveNetwork();
    //not auto connect
    ret = NeulBc28NconfigGet(outbuf,NEUL_MAX_BUF_SIZE);
	if(ret < 0){
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28NconfigGet fail ret %d\r\n", OppTickCountGet(), ret);    
		return ret;
	}	
	NeulBc28Sleep(30);

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "NeulBc28NconfigGet buffer len %d %s",ret,outbuf);
	if(!strstr(outbuf,"+NCONFIG:AUTOCONNECT,FALSE")){
	    ret = NeulBc28SetAutoConnect(0);
		if (ret < 0){
			MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_AUTO_CON_ERR);
	        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28SetAutoConnect fail ret %d\r\n", OppTickCountGet(), ret);    
			return ret;
		}
		NeulBc28Sleep(30);		
		//needReboot = 1;
	}

	if(!strstr(outbuf,"+NCONFIG:CR_0354_0338_SCRAMBLING,TRUE")
		||!strstr(outbuf,"+NCONFIG:CR_0859_SI_AVOID,TRUE")){
		ret = NeulBc28SetScramble(1);
		if (ret < 0){
			MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_SCRAM_ERR);
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28SetScramble(1) fail ret %d\r\n", OppTickCountGet(), ret);    
			return ret;
		}			
		NeulBc28Sleep(30);	
		//needReboot = 1;
	}
	//NeulBc28Sleep(30);	
    //NeulBc28SetScramble(0);
    //版本大于657sp2
	if(!strstr(outbuf,"+NCONFIG:COMBINE_ATTACH,TRUE")
		||!strstr(outbuf,"+NCONFIG:CELL_RESELECTION,TRUE")){
	    ret = NeulBc28SetReselection(1);
		if(ret < 0){
			MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_RESELECT_ERR);
	        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28SetReselection fail ret %d\r\n", OppTickCountGet(), ret);    
			return ret;
		}
		NeulBc28Sleep(30);		
		//needReboot = 1;		
	}
	//HUAWEI IOT
	if(COAP_PRO == m_ucProtoType){
		//first need to set cdp server ip address
		OppCoapIOTGetConfig(&stIotConfigPara);
		ret = NeulBc28SetCdpServer(stIotConfigPara.ocIp);
		if(0 != ret){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28SetCdpServer fail ret %d\r\n", OppTickCountGet(), ret);	  
			return ret;
		}
		NeulBc28Sleep(30);		
		//if iot regmod=auto chang to manu and hard reboot
		ret = NeulBc28HuaweiIotQueryReg(&type);
		if(0 == ret && 1 == type){
			ret = NeulBc28HuaweIotSetRegmod(0);
			if(0 != ret){
				DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28HuaweIotSetRegmod fail ret %d\r\n", OppTickCountGet(), ret);	  
				return ret;
			}
		}
		NeulBc28SetLastIotState(NeulBc28GetIotState());
		NeulBc28SetIotState(IOT_INIT);
	}	
	//do reboot
	/*if(needReboot){
		doReboot = 0;
		return -1;
	}*/
	/*NeulBc28Sleep(30);
	ret = NeulBc28OnOff(0);
	if(ret < 0){
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28OnOff fail ret %d\r\n", OppTickCountGet(), ret);    
		return ret;
	}*/
	NeulBc28Sleep(30);
	ret = NeulBc28SetFrequency();
	if(ret < 0){
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_FREQUENCY_ERR);
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28SetFrequency fail ret %d\r\n", OppTickCountGet(), ret);    
		return ret;
	}
	
	NeulBc28Sleep(30);
	ret = NeulBc28PSMSet();
	if(ret < 0){
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_PSM_ERR);
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28PSMSet fail ret %d\r\n", OppTickCountGet(), ret);    
		return ret;
	}
	
	NeulBc28Sleep(30);	
	ret = NeulBc28PSMR();
	if(ret < 0){
		MakeErrLog(DEBUG_MODULE_BC28,OPP_BC28_PSM_REPORT_ERR);
        DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28PSMR fail ret %d\r\n", OppTickCountGet(), ret);    
		return ret;
	}
	
    return 0;
}
int NeulBc28Sleep(int ticks)
{
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_DEBUG, "NeulBc28Sleep\r\n");
	//printf("%d\r\n", OppTickCountGet());
    vTaskDelay(ticks / portTICK_PERIOD_MS);
	//printf("%d\r\n", OppTickCountGet());
	
    return 0;
}

//AT+NTSETID=1,460012345678966

//\r\nNeul \r\nOK\r\n
//typedef int (*neul_read)(char *buf, int maxrlen, int mode);
//typedef int (*neul_write)(const char *buf, int writelen, int mode);


//static int uart_data_read(char *buf, int maxrlen, int mode)
extern SemaphoreHandle_t g_NbRxSem;

int uart_data_read(char *buf, int maxrlen, int mode, int timeout)
{
	int retlen = 0;
	
    if (NULL == buf || maxrlen <= 0 || timeout < 0)
    {
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "%s %d\r\n", __FUNCTION__, __LINE__);
        return 0;
    }
	while(1){
		BaseType_t r = OS_SEMAPHORE_TAKE(g_NbRxSem,timeout);
		if (r == pdFALSE){
			//printf("uart_data_read take sem timeout\r\n");
			return -1;
		}
		retlen = NbUartReadBytes((U8 *)buf, maxrlen, 0);
		if(retlen>0){
			//printf("uart_data_read: %s\r\n", buf);
			return retlen;
		}
	}

    return retlen;
}

int NeulBc28Init(void)
{
	int ret = 0;
	ST_DISCRETE_SAVE_PARA stDisPara;

    ret = NbUartInit(LOS_DEV_UART_DUALBUF_SIZE, 0);
    if (ret != 0)
    {
    	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "init usart2 error\r\n");
        return -1;
    }
	
	MUTEX_CREATE(g_stTxData.mutex);
	MUTEX_CREATE(g_stNbConfigMutex);
	MUTEX_CREATE(g_stDevStateMutex);
	MUTEX_CREATE(g_stLastDevStateMutex);
	MUTEX_CREATE(g_stConStateMutex);
	MUTEX_CREATE(g_stLastConStateMutex);
	MUTEX_CREATE(g_stRegStateMutex);
	MUTEX_CREATE(g_stLastRegStateMutex);
	MUTEX_CREATE(g_stPsmStateMutex);
	MUTEX_CREATE(g_stLastPsmStateMutex);
	MUTEX_CREATE(g_stIotStateMutex);
	MUTEX_CREATE(g_stLastIotStateMutex);
	MUTEX_CREATE(g_stRegNtyMutex);
	MUTEX_CREATE(g_stRxMutex);
	MUTEX_CREATE(g_stHardReset.mutex);
	MUTEX_CREATE(g_stBc28Pkt.mutex);
	MUTEX_CREATE(g_stFwSt.mutex);
	MUTEX_CREATE(g_stSupportAbnormal.mutex);
	MUTEX_CREATE(g_stBc28RamPara.mutex);
	MUTEX_CREATE(g_stAttachTime.mutex);
	MUTEX_CREATE(g_stDisPara.mutex);
	MUTEX_CREATE(g_stDisEnable.mutex);
	
	initEvent();
	
	ret = NeulBc28GetDiscreteParaFromFlash(&stDisPara);
	if(OPP_SUCCESS == ret){
		NeulBc28SetDiscretePara(&stDisPara);
	}else{
		NeulBc28SetDiscretePara(&g_stDisParaDefault);
	}
	return 0;
}

int NeulBc28ProtoInit(EN_PROTOCOL_TYPE protocolType)
{
	if(protocolType >= UNKNOW_PRO)
		return -1;
	
    m_ucProtoType = protocolType;

	return 0;
}

int NeulBc28SetDevState(int state)
{
	if(state >= BC28_UNKNOW)
		return -1;

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "NeulBc28SetDevState %d\r\n", state);
	MUTEX_LOCK(g_stDevStateMutex,MUTEX_WAIT_ALWAYS);
	g_ucNbiotDevState = state;
	MUTEX_UNLOCK(g_stDevStateMutex);

	return 0;
}

int NeulBc28GetDevState(void)
{
	U8 devState;

	MUTEX_LOCK(g_stDevStateMutex,MUTEX_WAIT_ALWAYS);
	devState = g_ucNbiotDevState;
	MUTEX_UNLOCK(g_stDevStateMutex);

	return devState;
}

int NeulBc28SetLastDevState(int state)
{
	if(state >= BC28_UNKNOW)
		return -1;

	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "NeulBc28SetLastDevState %d\r\n", state);
	MUTEX_LOCK(g_stLastDevStateMutex,MUTEX_WAIT_ALWAYS);
	g_ucLastNbiotDevState = state;
	MUTEX_UNLOCK(g_stLastDevStateMutex);

	return 0;
}

int NeulBc28GetLastDevState(void)
{
	U8 devState;

	MUTEX_LOCK(g_stLastDevStateMutex,MUTEX_WAIT_ALWAYS);
	devState = g_ucLastNbiotDevState;
	MUTEX_UNLOCK(g_stLastDevStateMutex);

	return devState;
}

int NeulBc28SetConState(int con)
{
	MUTEX_LOCK(g_stConStateMutex,MUTEX_WAIT_ALWAYS);
	g_ucConState = con;
	MUTEX_UNLOCK(g_stConStateMutex);

	return 0;
}

int NeulBc28GetConState(void)
{
	U8 con;

	MUTEX_LOCK(g_stConStateMutex,MUTEX_WAIT_ALWAYS);
	con = g_ucConState;
	MUTEX_UNLOCK(g_stConStateMutex);

	return con;
}

int NeulBc28SetLastConState(int con)
{
	MUTEX_LOCK(g_stLastConStateMutex,MUTEX_WAIT_ALWAYS);
	g_ucLastConState = con;
	MUTEX_UNLOCK(g_stLastConStateMutex);

	return 0;
}

int NeulBc28GetLastConState(void)
{
	U8 con;

	MUTEX_LOCK(g_stLastConStateMutex,MUTEX_WAIT_ALWAYS);
	con = g_ucLastConState;
	MUTEX_UNLOCK(g_stLastConStateMutex);

	return con;
}

int NeulBc28SetRegState(int reg)
{
	MUTEX_LOCK(g_stRegStateMutex,MUTEX_WAIT_ALWAYS);
	g_ucRegState = reg;
	MUTEX_UNLOCK(g_stRegStateMutex);

	return 0;
}

int NeulBc28GetRegState(void)
{
	U8 reg;

	MUTEX_LOCK(g_stRegStateMutex,MUTEX_WAIT_ALWAYS);
	reg = g_ucRegState;
	MUTEX_UNLOCK(g_stRegStateMutex);

	return reg;
}

int NeulBc28SetLastRegState(int reg)
{
	MUTEX_LOCK(g_stLastRegStateMutex,MUTEX_WAIT_ALWAYS);
	g_ucLastRegState = reg;
	MUTEX_UNLOCK(g_stLastRegStateMutex);

	return 0;
}

int NeulBc28GetLastRegState(void)
{
	U8 reg;

	MUTEX_LOCK(g_stLastRegStateMutex,MUTEX_WAIT_ALWAYS);
	reg = g_ucLastRegState;
	MUTEX_UNLOCK(g_stLastRegStateMutex);

	return reg;
}

int NeulBc28SetPsmState(int psm)
{
	MUTEX_LOCK(g_stPsmStateMutex,MUTEX_WAIT_ALWAYS);
	g_ucPsmState = psm;
	MUTEX_UNLOCK(g_stPsmStateMutex);

	return 0;
}

int NeulBc28GetPsmState(void)
{
	U8 psm;

	MUTEX_LOCK(g_stPsmStateMutex,MUTEX_WAIT_ALWAYS);
	psm = g_ucPsmState;
	MUTEX_UNLOCK(g_stPsmStateMutex);

	return psm;
}

int NeulBc28SetLastPsmState(int psm)
{
	MUTEX_LOCK(g_stLastPsmStateMutex,MUTEX_WAIT_ALWAYS);
	g_ucLastPsmState = psm;
	MUTEX_UNLOCK(g_stLastPsmStateMutex);

	return 0;
}

int NeulBc28GetLastPsmState(void)
{
	U8 psm;

	MUTEX_LOCK(g_stLastPsmStateMutex,MUTEX_WAIT_ALWAYS);
	psm = g_ucLastPsmState;
	MUTEX_UNLOCK(g_stLastPsmStateMutex);

	return psm;
}

int NeulBc28SetIotState(int iot)
{
	MUTEX_LOCK(g_stIotStateMutex,MUTEX_WAIT_ALWAYS);
	g_cIOTState = iot;
	MUTEX_UNLOCK(g_stIotStateMutex);

	return 0;
}

int NeulBc28GetIotState(void)
{
	U8 iot;

	MUTEX_LOCK(g_stIotStateMutex,MUTEX_WAIT_ALWAYS);
	iot = g_cIOTState;
	MUTEX_UNLOCK(g_stIotStateMutex);

	return iot;
}

int NeulBc28SetLastIotState(int iot)
{
	MUTEX_LOCK(g_stLastIotStateMutex,MUTEX_WAIT_ALWAYS);
	g_cLastIOTState = iot;
	MUTEX_UNLOCK(g_stLastIotStateMutex);

	return 0;
}

int NeulBc28GetLastIotState(void)
{
	U8 iot;

	MUTEX_LOCK(g_stLastIotStateMutex,MUTEX_WAIT_ALWAYS);
	iot = g_cLastIOTState;
	MUTEX_UNLOCK(g_stLastIotStateMutex);

	return iot;
}

static int NeulBc28SetFwState(U8 ucCurSt)
{
	MUTEX_LOCK(g_stFwSt.mutex, MUTEX_WAIT_ALWAYS);
	g_stFwSt.ucFwLastState = g_stFwSt.ucFwState;
	g_stFwSt.ucFwState = ucCurSt;
	MUTEX_UNLOCK(g_stFwSt.mutex);
	return 0;
}

int NeulBc28GetFwState(U8 * pucCurSt, U8 * pucLastSt)
{
	MUTEX_LOCK(g_stFwSt.mutex, MUTEX_WAIT_ALWAYS);
	*pucCurSt = g_stFwSt.ucFwState;
	*pucLastSt = g_stFwSt.ucFwLastState;
	MUTEX_UNLOCK(g_stFwSt.mutex);
	return 0;
}

int NeulBc28SetSupportAbnormal(int abnormal)
{
	MUTEX_LOCK(g_stSupportAbnormal.mutex,MUTEX_WAIT_ALWAYS);
	if(0 == abnormal){
		g_stSupportAbnormal.udwAbnormalH = ABNORMAL_PROTECT_DISABLE_H;
		g_stSupportAbnormal.udwAbnormalL = ABNORMAL_PROTECT_DISABLE_L;
	}
	else{
		g_stSupportAbnormal.udwAbnormalH = ABNORMAL_PROTECT_ENABLE_H;
		g_stSupportAbnormal.udwAbnormalL = ABNORMAL_PROTECT_ENABLE_L;
	}
	MUTEX_UNLOCK(g_stSupportAbnormal.mutex);
	return 0;
}

int NeulBc28GetSupportAbnormal(void)
{
	int abnormal;
	MUTEX_LOCK(g_stSupportAbnormal.mutex,MUTEX_WAIT_ALWAYS);
	if(ABNORMAL_PROTECT_ENABLE_H == g_stSupportAbnormal.udwAbnormalH 
		&& ABNORMAL_PROTECT_ENABLE_L == g_stSupportAbnormal.udwAbnormalL)
		abnormal = ABNORMAL_PROTECT_ENABLE;
	else
		abnormal = ABNORMAL_PROTECT_DISABLE;
	MUTEX_UNLOCK(g_stSupportAbnormal.mutex);
	
	return abnormal;
}

int NeulBc28SetEarfcnToRam(U8 enable, U16 earfcn)
{
	MUTEX_LOCK(g_stBc28RamPara.mutex,MUTEX_WAIT_ALWAYS);
	if(0 == enable){
		g_stBc28RamPara.enableH = EARFCN_DISABLE_H;
		g_stBc28RamPara.enableL = EARFCN_DISABLE_L;
	}
	else{
		g_stBc28RamPara.enableH = EARFCN_ENABLE_H;
		g_stBc28RamPara.enableL = EARFCN_ENABLE_L;
	}
	g_stBc28RamPara.earfcn = earfcn;
	g_stBc28RamPara.band = 0;
	MUTEX_UNLOCK(g_stBc28RamPara.mutex);
	//NeulBc28OnOff(0);
	//NeulBc28DeactiveNetwork();
	NeulBc28SetDisEnable(DIS_DISABLE);
	NeulBc28SetDevState(BC28_INIT);
	return 0;
}

int NeulBc28GetEarfcnFromRam(U8 *enable, U16 *earfcn)
{
	MUTEX_LOCK(g_stBc28RamPara.mutex,MUTEX_WAIT_ALWAYS);
	if(g_stBc28RamPara.enableH == EARFCN_ENABLE_H && g_stBc28RamPara.enableL == EARFCN_ENABLE_L){
		*enable = EARFCN_ENABLE;
	}else{
		*enable = EARFCN_DISABLE;
	}
	*earfcn = g_stBc28RamPara.earfcn;
	MUTEX_UNLOCK(g_stBc28RamPara.mutex);
	return 0;
}

int NeulBc28GetBandFromRam(U8 *enable, U16 *band)
{
	MUTEX_LOCK(g_stBc28RamPara.mutex,MUTEX_WAIT_ALWAYS);
	if(g_stBc28RamPara.enableH == EARFCN_ENABLE_H && g_stBc28RamPara.enableL == EARFCN_ENABLE_L){
		*enable = EARFCN_ENABLE;
	}else{
		*enable = EARFCN_DISABLE;
	}
	*band = g_stBc28RamPara.band;
	MUTEX_UNLOCK(g_stBc28RamPara.mutex);
	return 0;
}


int NeulBc28SetParaToRam(U8 enable, U16 earfcn, U16 band)
{
	MUTEX_LOCK(g_stBc28RamPara.mutex,MUTEX_WAIT_ALWAYS);
	if(0 == enable){
		g_stBc28RamPara.enableH = EARFCN_DISABLE_H;
		g_stBc28RamPara.enableL = EARFCN_DISABLE_L;
	}
	else{
		g_stBc28RamPara.enableH = EARFCN_ENABLE_H;
		g_stBc28RamPara.enableL = EARFCN_ENABLE_L;
	}
	g_stBc28RamPara.earfcn = earfcn;
	g_stBc28RamPara.band = band;
	MUTEX_UNLOCK(g_stBc28RamPara.mutex);
	NeulBc28SetDisEnable(DIS_DISABLE);
	NeulBc28SetDevState(BC28_INIT);
	return 0;
}

int NeulBc28GetParaFromRam(U8 *enable, U16 *earfcn, U16 *band)
{
	MUTEX_LOCK(g_stBc28RamPara.mutex,MUTEX_WAIT_ALWAYS);
	if(g_stBc28RamPara.enableH == EARFCN_ENABLE_H && g_stBc28RamPara.enableL == EARFCN_ENABLE_L){
		*enable = EARFCN_ENABLE;
	}else{
		*enable = EARFCN_DISABLE;
	}
	*earfcn = g_stBc28RamPara.earfcn;
	*band = g_stBc28RamPara.band;
	MUTEX_UNLOCK(g_stBc28RamPara.mutex);
	return 0;

}
int NeulBc28SetAttachStartTime()
{
	MUTEX_LOCK(g_stAttachTime.mutex, MUTEX_WAIT_ALWAYS);
	g_stAttachTime.startTick = OppTickCountGet();
	MUTEX_UNLOCK(g_stAttachTime.mutex);
	return 0;
}

int NeulBc28SetAttachEndTime()
{
	MUTEX_LOCK(g_stAttachTime.mutex, MUTEX_WAIT_ALWAYS);
	g_stAttachTime.endTick = OppTickCountGet();
	MUTEX_UNLOCK(g_stAttachTime.mutex);
	return 0;
}

U32 NeulBc28GetAttachEplaseTime()
{
	U32 eplase;
	
	MUTEX_LOCK(g_stAttachTime.mutex, MUTEX_WAIT_ALWAYS);
	eplase = g_stAttachTime.endTick - g_stAttachTime.startTick;
	MUTEX_UNLOCK(g_stAttachTime.mutex);
	
	return eplase;
}

U32 NeulBc28SetDiscreteParaToFlash(ST_DISCRETE_SAVE_PARA *pstDisSavePara)
{
	int ret;

	ret = AppParaWrite(APS_PARA_MODULE_APS_NB, DISCRETEWIN_ID, pstDisSavePara, sizeof(ST_DISCRETE_SAVE_PARA));
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "NeulBc28SetDiscreteParaToFlash fail ret %d\r\n",ret);
		return ret;
	}
	return OPP_SUCCESS;
}

U32 NeulBc28GetDiscreteParaFromFlash(ST_DISCRETE_SAVE_PARA *pstDisSavePara)
{
	int ret;
	unsigned int len = sizeof(ST_DISCRETE_SAVE_PARA);
	
	ret = AppParaRead(APS_PARA_MODULE_APS_NB, DISCRETEWIN_ID, pstDisSavePara, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "NeulBc28GetDiscreteParaFromFlash fail ret %d\r\n",ret);
		return OPP_FAILURE;
	}
	return OPP_SUCCESS;
}

U32 NeulBc28SetDiscretePara(ST_DISCRETE_SAVE_PARA *pstDisSavePara)
{
	MUTEX_LOCK(g_stDisPara.mutex, MUTEX_WAIT_ALWAYS);
	if(pstDisSavePara->udwDiscreteWin > DIS_WINDOW_MAX_SECOND)
		g_stDisPara.udwDiscreteWin = DIS_WINDOW_MAX_SECOND;
	else
		g_stDisPara.udwDiscreteWin = pstDisSavePara->udwDiscreteWin;
	g_stDisPara.udwDiscreteInterval = pstDisSavePara->udwDiscreteInterval;
	MUTEX_UNLOCK(g_stDisPara.mutex);
	return 0;
}

U32 NeulBc28GetDiscretePara(ST_DISCRETE_SAVE_PARA *pstDisSavePara)
{
	
	MUTEX_LOCK(g_stDisPara.mutex, MUTEX_WAIT_ALWAYS);
	if(g_stDisPara.udwDiscreteWin > DIS_WINDOW_MAX_SECOND)
		pstDisSavePara->udwDiscreteWin = DIS_WINDOW_MAX_SECOND;
	else
		pstDisSavePara->udwDiscreteWin = g_stDisPara.udwDiscreteWin;
	pstDisSavePara->udwDiscreteInterval = g_stDisPara.udwDiscreteInterval;
	MUTEX_UNLOCK(g_stDisPara.mutex);
	return 0;
}


U32 NeulBc28SetDiscreteWindow(U32 disWin)
{
	MUTEX_LOCK(g_stDisPara.mutex, MUTEX_WAIT_ALWAYS);
	g_stDisPara.udwDiscreteWin = disWin;
	MUTEX_UNLOCK(g_stDisPara.mutex);

	return 0;
}

U32 NeulBc28GetDiscreteWindow(void)
{
	U32 disWin;
	
	MUTEX_LOCK(g_stDisPara.mutex, MUTEX_WAIT_ALWAYS);
	disWin = g_stDisPara.udwDiscreteWin;
	MUTEX_UNLOCK(g_stDisPara.mutex);
	return disWin;
}

U32 NeulBc28GetDiscreteScale(void)
{
	U32 disScale;
	
	MUTEX_LOCK(g_stDisPara.mutex, MUTEX_WAIT_ALWAYS);
	disScale = g_stDisPara.udwDiscreteInterval;
	MUTEX_UNLOCK(g_stDisPara.mutex);
	return disScale;
}

U32 NeulBc28SetDisEnable(U32 enable)
{
	MUTEX_LOCK(g_stDisEnable.mutex, MUTEX_WAIT_ALWAYS);
	if(DIS_DISABLE == enable){
		g_stDisEnable.enableH = DIS_DISABLE_H;
		g_stDisEnable.enableL = DIS_DISABLE_L;
	}else{
		g_stDisEnable.enableH = DIS_ENABLE_H;
		g_stDisEnable.enableL = DIS_ENABLE_L;
	}
	MUTEX_UNLOCK(g_stDisEnable.mutex);
	return 0;
}

U32 NeulBc28SetDisEnableWithoutMutex(U32 enable)
{
	if(DIS_DISABLE == enable){
		g_stDisEnable.enableH = DIS_DISABLE_H;
		g_stDisEnable.enableL = DIS_DISABLE_L;
	}else{
		g_stDisEnable.enableH = DIS_ENABLE_H;
		g_stDisEnable.enableL = DIS_ENABLE_L;
	}
	return 0;
}

U32 NeulBc28GetDisEnable(void)
{
	U32 enable;
	
	MUTEX_LOCK(g_stDisEnable.mutex, MUTEX_WAIT_ALWAYS);
	if(DIS_DISABLE_H == g_stDisEnable.enableH && DIS_DISABLE_L == g_stDisEnable.enableL){
		enable = DIS_DISABLE;
	}else{
		enable = DIS_ENABLE;
	}
	MUTEX_UNLOCK(g_stDisEnable.mutex);
	return enable;
}

/******************************************************************************
Function    : OppGetNBConOffset
Description : 获取网络附着的偏移时间
Note        : (none)
Input Para  : @disPeriodWnd  离散的时间窗口 单位 秒
               @disIntvlInSnd 离散点的间隔时间 单位 秒
               @mac1    mac地址右第二字节 f9 (24:0a:c4:28:f9:6c)
               @mac0    mac地址右第一字节 6c (24:0a:c4:28:f9:6c)    
 Output Para : @outDisOffset 
                                      根据输入参数算出的离散偏移时间 单位秒
Return      : OPP_SUCCESS 成功
                             OPP_FAIL   参数错误等
******************************************************************************/
U8 OppGetDisOffsetSecond(U32 disPeriodWnd, U32 disIntvl, U8 mac1,U8 mac0,U32* outDisOffset)
{
     U32 wnd  = disPeriodWnd;
     U32 intvl=  disIntvl;
     U32 wndMod = 1;
     U32 wndIntrvl = 1;
     U32 intrvlMod = 1;
     U32 offseH = 0;
     U32 offseL = 0;
     if( NULL == outDisOffset){
	 	return OPP_FAILURE;
     }
	 
     if(wnd > DIS_WINDOW_MAX_SECOND){
	 	wnd = DIS_WINDOW_MAX_SECOND;
     }
     if(0 == wnd){
	 	wnd = 1;
     }        
     if(intvl > wnd){
	 	intvl = wnd;
     }        
     if(0 == intvl ){
	 	intvl = 1;
     }
     
     wndMod = wnd/intvl;
     if(0 == wndMod){
	 	wndMod = 1;
     }
     
     intrvlMod = intvl/DIS_INTRVL_SECOND;
     if(0 == intrvlMod){
	 	intrvlMod = 1;
     }
     offseH = (mac0 % wndMod) * intvl;
     offseL = ((mac1&0x0F) % intrvlMod)*DIS_INTRVL_SECOND;
     * outDisOffset = offseH + offseL;
     if((* outDisOffset) > wnd){
	 	(* outDisOffset) = wnd;
     }
     if((* outDisOffset) > DIS_WINDOW_MAX_SECOND){
	 	(* outDisOffset) = DIS_WINDOW_MAX_SECOND;
     }
     return OPP_SUCCESS;
}

/*单位s*/
U32 NeulBc28DisOffsetSecondGet()
{
	int ret;
	unsigned char mac[6] = {0};
	U32 disOffset;
	ST_DISCRETE_SAVE_PARA stDisSavePara;
	
	esp_wifi_get_mac(WIFI_IF_STA, mac);
	NeulBc28GetDiscretePara(&stDisSavePara);
	ret = OppGetDisOffsetSecond(stDisSavePara.udwDiscreteWin,stDisSavePara.udwDiscreteInterval,mac[4],mac[5],&disOffset);
	if(OPP_SUCCESS != ret){
		disOffset = DIS_WINDOW_MAX_SECOND;
	}
	if(disOffset > DIS_WINDOW_MAX_SECOND){
		disOffset = DIS_WINDOW_MAX_SECOND;
	}

	return disOffset;
}

U32 NeulBc28AttachDisTick(void)
{
	static U8 first = 1;
	static U32 tick = 0;
	static U32 offset = 0;

	if(first){
		offset = NeulBc28DisOffsetSecondGet()*1000;
		first = 0;
	}
		
	if(OppTickCountGet() > offset)
		return (OppTickCountGet()-offset);
	else
		return 0;
}

int NeulBc28StateChgReg(neul_state_chg func)
{
	neul_dev.ops->state_chg = func;

	return 0;
}

int NeulBc28DevChgReg(neul_state_chg func)
{
	neul_dev.ops->dev_chg = func;

	return 0;
}

int NeulBc28IotChgReg(neul_state_chg func)
{
	neul_dev.ops->iot_chg = func;

	return OPP_SUCCESS;
}

int NeulBc28TimeReg(neul_time_set func)
{
	neul_dev.ops->time_set = func;

	return 0;
}

/* ============================================================
func name   :  NeulBc28DevStateTimeout
discription :  dev state timeout proc
               
param       :   
return      :  0, success
Revision    : 
=============================================================== */
int NeulBc28DevStateTimeout()
{
	static U32 tick_start;
	//U8 devState;
	//static U8 times = 0;
	static int init = 0;

	if(!init){
		tick_start = OppTickCountGet();
		init = 1;
	}

	if(NeulBc28GetLastDevState() != NeulBc28GetDevState()){
		//DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "state chhhhhhhhhh\r\n");
		//lastDevSt = g_ucNbiotDevState;
		tick_start = 0;
	}
	else{
		if(NeulBc28GetDevState() != BC28_READY
			&& NeulBc28GetDevState() != BC28_TEST
			&& NeulBc28GetDevState() != BC28_UPDATE
			&& NeulBc28GetDevState() != BC28_WAITCGA){
			if(tick_start == 0){
				DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "devstate %d start count\r\n", NeulBc28GetDevState());
				tick_start = OppTickCountGet();
			}else{
				if((OppTickCountGet() - tick_start) >= DEVSTATE_DEFAULT_TO){
					DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "devstate %d timeout\r\n", NeulBc28GetDevState());
					NbiotHardResetInner();
					tick_start = 0;
				}
			}
		}
	}
	return 0;
}

/* ============================================================
func name   :  NeulBc28RegTimeout
discription :  reg 异常重启
               
param       :   
return      :  0, success
Revision    : 
=============================================================== */
int NeulBc28RegTimeout()
{
	static U32 tick_start = 0;
	
	/*reg state process*/
	if(NeulBc28GetLastRegState() != NeulBc28GetRegState()){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "NeulBc28RegTimeout last %d cur %d\r\n",
			NeulBc28GetLastRegState(), NeulBc28GetRegState());
		if(REG == NeulBc28GetRegState()
			|| ROAMING_REG == NeulBc28GetRegState()){
			tick_start = 0;
		}else{
			tick_start = OppTickCountGet();
		}
	}
	if(tick_start > 0){
		if(OppTickCountGet() - tick_start > REGSTATE_DEFAULT_TO/*120000*/){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "NeulBc28RegTimeout\r\n");
			//NeulBc28SetDevState(BC28_INIT);
			NbiotHardResetInner();
			tick_start = 0;
		}
	}
	return 0;
}

int NeulBc28FirmwareTimeout(void)
{
	static U32 tick_start = 0;
	static U8 ucCurSt, ucLastSt;

	NeulBc28GetFwState(&ucCurSt,&ucLastSt);
	if(ucLastSt != ucCurSt){
		if(FIRMWARE_NONE == ucCurSt
			|| FIRMWARE_UPDATE_OVER == ucCurSt
			|| FIRMWARE_UPDATE_FAILED == ucCurSt){
			tick_start = 0;
		}else{
			tick_start = OppTickCountGet();
		}
	}
	
	if(tick_start > 0){
		if(OppTickCountGet() - tick_start > FIRMWARE_DEFAULT_TO){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28FirmwareTimeout\r\n");
			NbiotHardResetInner();;
			tick_start = 0;
		}
	}
		
	return 0;
}

int NeulBc28HardRstTimeout(void)
{
	static U32 tick_start = 0;

	//rebooting
	if(REBOOTING == NeulBc28HardRestStateGet()){
		if(tick_start == 0){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28HardRstTimeout start count\r\n");
			tick_start = OppTickCountGet();
		}
	}else{
		tick_start = 0;
	}

	if(tick_start > 0){
		if(OppTickCountGet() - tick_start > HARD_RESET_TO){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28HardRstTimeout\r\n");
			NeulBc28HardRestStateSet(REBOOTED);
			NeulBc28HardRestResultSet(REBOOT_EXPIRE);
			tick_start = 0;
		}
	}
	
	return 0;
}
/* ============================================================
func name   :  NeulBc28Loop
discription :  bc28 main loop
               
param       :   
return      :  0, success
Revision    : 
=============================================================== */
int NeulBc28Loop(void)
{
	int ret = 0;
	static U32 disTick = 0;

	if(BC28_INIT != NeulBc28GetDevState()){
		if((NeulBc28GetLastConState() != NeulBc28GetConState()) 
			|| (NeulBc28GetLastRegState() != NeulBc28GetRegState()) 
			|| (NeulBc28GetLastPsmState() != NeulBc28GetPsmState())
			|| (1 == NeulBc28RegisterNotify()))
		{
			if(neul_dev.ops->state_chg)
				neul_dev.ops->state_chg();
			NeulBc28SetLastConState(NeulBc28GetConState());
			NeulBc28SetLastRegState(NeulBc28GetRegState());
			NeulBc28SetLastPsmState(NeulBc28GetPsmState());
			NeulBc28SetRegisterNotify(0);
		}
	}

	if(NeulBc28GetLastDevState() != NeulBc28GetDevState()){
		if(neul_dev.ops->dev_chg)
			neul_dev.ops->dev_chg();
		NeulBc28SetLastDevState(NeulBc28GetDevState());
	}
	if(BC28_INIT == NeulBc28GetDevState())
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "bc28 init..........\r\n");		
		NeulBc28Sleep(100);		
		EventProc();
		NeulBc28SetRegState(INVALID_REG);
		NeulBc28SetLastRegState(INVALID_REG);
		NeulBc28SetIotState(IOT_NOWORK);
		NeulBc28SetLastIotState(IOT_NOWORK);
		NeulBc28SetFwState(FIRMWARE_NONE);
		NeulBc28SetAttachStartTime();
		ret = NeulBc28HwInit();
		if(0 == ret){
			NeulBc28SetDevState(BC28_WAITCGA);
			disTick = 0;
		}else{
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "bc28 reinit..........\r\n");
		}
	}
	else if(BC28_WAITCGA == NeulBc28GetDevState())
	{
		NeulBc28Sleep(100);
		EventProc();		
		RxDataProc();
		if(DIS_DISABLE != NeulBc28GetDisEnable()){
			if(0 == disTick){
				printf("discrete:%d start\r\n",OppTickCountGet());
				disTick = OppTickCountGet();
			}else if(OppTickCountGet()-disTick > NeulBc28DisOffsetSecondGet()*1000){
				printf("discrete:%d over to active network\r\n",OppTickCountGet());
				ret = NeulBc28ActiveNetwork();
				if(0 == ret){			
					NeulBc28SetDevState(BC28_CGA);
					disTick = 0;
				}else{
					DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "bc28 reactive network..........\r\n");
				}
			}
		}else{
			printf("active network immediately\r\n");
			ret = NeulBc28ActiveNetwork();
			if(0 == ret){			
				NeulBc28SetDevState(BC28_CGA);
			}else{
				DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "bc28 reactive network..........\r\n");
			}
		}
	}
	else if(BC28_CGA == NeulBc28GetDevState())
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "bc28 cgatt..........\r\n");
		NeulBc28Sleep(100);		
		EventProc();
		ret = NeulBc28GetNetstat();
		if(0 == ret){
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "bc28 swtich to gip");
			NeulBc28SetAttachEndTime();
			NeulBc28SetDevState(BC28_GIP);
			NeulBc28SetLastIotState(NeulBc28GetIotState());
			NeulBc28SetIotState(IOT_WAIT_MSG);
		}
	}
	else if(BC28_GIP == NeulBc28GetDevState())
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "bc28 get ip..........\r\n");
		NeulBc28Sleep(100);		
		EventProc();
		ret = NeulBc28QueryIp();
		if(0 == ret)
		{
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "~~~~~~~~~~~nb attach success~~~~~~~~~~~~~~~~\n");
			ret = NeulBc28QueryNccid((char *)neul_dev.nccid);
			if(0 != ret)
				NeulBc28QueryNccid((char *)neul_dev.nccid);
			NeulBc28Edrx();
			NeulBc28SetDevState(BC28_READY);		
		}
	}
	else if(BC28_READY == NeulBc28GetDevState())
	{
		if(COAP_PRO == m_ucProtoType)
		{
			TxDataProc();
			EventProc();
			RxDataProc();
		}
		else
		{
			TxDataProc();
			EventProc();
			RxDataProc();
		}
	}
	else if(BC28_ERR == NeulBc28GetDevState())
	{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "bc28 error..........\r\n");	
		//NeulBc28SetDevState(BC28_INIT);
		NbiotHardResetInner();
	}
	else if(BC28_TEST == NeulBc28GetDevState()){
		//do nothing here
		EventProc();
		RxDataProc();		
	}
	else if(BC28_UPDATE == NeulBc28GetDevState() || BC28_RESET == NeulBc28GetDevState()){
		EventProc();
		RxDataProc();
	}else{
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "bc28 unknow state %d..........\r\n",NeulBc28GetDevState());	
		//NeulBc28SetDevState(BC28_INIT);
		NbiotHardResetInner();
	}
	return 0;
}

//ms
void NeulBc28PrintInfoDelay(int timeout)
{
	static U32 tick = 0;

	if(0 == tick){
		tick = OppTickCountGet();
	}else if(OppTickCountGet() - tick > timeout){
		DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "NeulBc28Thread run..........\r\n");
		tick = 0;
	}
}
extern int NbiotStateChg(void);
extern int NbiotDevStateChg(void);
/* ============================================================
func name   :  NeulBc28Thread
discription :  bc28 main thread
               
param       :   
return      :  
Revision    : 
=============================================================== */
void NeulBc28Thread(void *pvParameter)
{
	ST_NB_CONFIG stConfigPara;

	//register callback function
	NeulBc28StateChgReg(NbiotStateChg);
	NeulBc28DevChgReg(NbiotDevStateChg);
	//read nbiot config para
	NeulBc28GetConfigFromFlash(&stConfigPara);
	NeulBc28SetConfig(&stConfigPara);	
	//hard reset
	NbiotHardResetInner();
	printf("-0tick:%d\r\n", OppTickCountGet());
	while(1){
		NeulBc28Loop();
		if(NeulBc28GetSupportAbnormal()){
			NeulBc28DevStateTimeout();
			NeulBc28RegTimeout();
			NeulBc28FirmwareTimeout();
			NeulBc28HardRstTimeout();
		}
		NeulBc28PrintInfoDelay(60000);
		taskWdtReset();
		//vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

/* ============================================================
func name   :  NeulBc28DataSend
discription :  bc28 data copy to data buffer
               
param       :   type:opple,huawei
			buf: send data 
			sendlen:send data length
return      :  0,success
Revision    : 
=============================================================== */
int NeulBc28DataSend(const MSG_ITEM_T *item)
{
	const char *buf = (char*)item->data;
	int sendlen = item->len;
	U8 type = item->type;
	
	DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "type %d sendlen %d\r\n", type, sendlen);
	if(MSG_TYPE_OPPLE == type)
	{
		m_ucCoapIsBusy = 1;
		NeulBc28UdpSend(neul_dev.addrinfo->socket, buf, sendlen);
		m_ucCoapIsBusy = 0;
	}
	else
	{
		MUTEX_LOCK(g_stTxData.mutex,MUTEX_WAIT_ALWAYS);
		/*if(g_stTxData.haveData){
			MUTEX_UNLOCK(g_stTxData.mutex);
			return 1;
		}*/
		//do not have data
		if(sendlen > MAX_MTU){			
			DEBUG_LOG(DEBUG_MODULE_BC28, DLL_INFO, "sendlen %d > MTU %d\r\n", sendlen, MAX_MTU);
			MUTEX_UNLOCK(g_stTxData.mutex);
			return -1;
		}
		g_stTxData.haveData = 1;
		g_stTxData.len = sendlen;
		memcpy(g_stTxData.data,buf,sendlen);		
		MUTEX_UNLOCK(g_stTxData.mutex);

		MUTEX_LOCK(g_stBc28Pkt.mutex,MUTEX_WAIT_ALWAYS);
		g_stBc28Pkt.iPktRxFromQue++;
		MUTEX_UNLOCK(g_stBc28Pkt.mutex);
	}
	return 0;
}

/* ============================================================
func name   :  NeulBc28IsBusy
discription :  to judge bc28 is busy
               
param       :   
return      :  0,not busy  1,busy
Revision    : 
=============================================================== */
U8 NeulBc28IsBusy(void)
{
	int dev, iot;

	//return 0;
	
	dev = NeulBc28GetDevState();
	if(BC28_READY != dev){
		return 1;
	}

	//iot platform create object not success
	iot = NeulBc28GetIotState();
	if((IOT_1900_OBSERVE != iot) && (IOT_UPREG != iot)){
		return 1;
	}

	MUTEX_LOCK(g_stTxData.mutex,MUTEX_WAIT_ALWAYS);
	if(g_stTxData.haveData){
		MUTEX_UNLOCK(g_stTxData.mutex);
		return 1;
	}
	MUTEX_UNLOCK(g_stTxData.mutex);

	return 0;
}

//static int uart_data_write(const char *buf, int writelen, int mode)
int uart_data_write(char *buf, int writelen, int mode)
{
	int ret;
	
    if (NULL == buf || writelen <= 0)
    {
        return 0;
    }
    ret = NbUartWriteBytes(buf, writelen);
    return ret;
}

int uart_data_flush(void)
{
    memset(neul_bc95_buf, 0, NEUL_MAX_BUF_SIZE);
    return 0;
}
