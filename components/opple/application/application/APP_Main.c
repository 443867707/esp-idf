#include <string.h>
#include <stdio.h>
#include <Includes.h>
#include "DRV_Bc28.h"
#include "DRV_Nb.h"
#include "DRV_Watchdog.h"
#include "DRV_Led.h"
#include "DRV_Main.h"
#include "DEV_Utility.h"
#include "DEV_Main.h"
#include "SVS_Coap.h"
#include "SVS_Config.h"
#include "SVS_Udp.h"
#include "SVS_Opple.h"
#include "SVS_Time.h"
#include "SVS_MsgMmt.h"
#include "SVS_Para.h"
#include "SVS_Elec.h"
#include "SVS_Main.h"
#include "APP_LampCtrl.h"
#include "APP_Main.h"
#include "APP_Communication.h"
#include "OPP_Debug.h"
#include "Opp_Module.h"
#include "Lib_Main.h"
#include "DebugMain.h"

#include "esp_task_wdt.h"
#include "opple_main.h"

#define APPMAIN_OPT_CHECK(o,s) \
if((o) != 0){\
	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_ERROR,(s));\
}


typedef struct
{
	U32 time;
	U32 min;
	U32 max;
	U32 count;
	U32 avg;
}RTIME_T;



void NbiotWatchdogTask(void)
{
	UINT32 tick_start = 0, tick_cur = 0;
	
	WathdogInit();
	tick_start = (UINT32)OppTickCountGet();
	while(1)
	{
		tick_cur = (UINT32)OppTickCountGet();
		if (tick_cur - tick_start >= 10000)
		{
			DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "dog wang wang\r\n");
			Watchdog_trigger();
			tick_start = (UINT32)OppTickCountGet();
		}
	}
}

U8 g_ucDevInit = 0;

int NbiotDevStateChg(void)
{
	U8 devSt;
	APP_LOG_T log;
	
	devSt = NeulBc28GetDevState();
	if(BC28_INIT == devSt){
		log.id = NBNET_LOGID;
		log.level = LOG_LEVEL_INFO;
		log.module = MC_MODULE_LAMP;
		log.len = 1;
		log.log[0] = 0;
		ApsDaemonLogReport(&log);
	}else if(BC28_GIP== devSt){

	}else if(BC28_READY== devSt){
		//ApsCoapReboot();
		if(!g_ucDevInit){			
			ApsCoapOceanconHeartOnline(CHL_NB,NULL);
			log.id = NBNET_LOGID;
			log.level = LOG_LEVEL_INFO;
			log.module = MC_MODULE_LAMP;
			log.len = 1;
			log.log[0] = 1;
			ApsDaemonLogReport(&log);
			//OppCoapConfig();
			DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "connect init clk src%d\r\n", TimeGetClockSrc());
			if(NBIOT_CLK == TimeGetClockSrc() || ALL_CLK == TimeGetClockSrc())
				TimeFromNbiotClock();
			//NeulBc28QueryUestats(&g_stThisLampInfo.stNeulSignal);
			//strcpy((char *)g_stThisLampInfo.stObjInfo.aucObjectSN, (char *)g_stThisLampInfo.stNeulSignal.imei);
			//error cause g_ucNbiotDevState change
			//printf("!!!g_ucNbiotDevState %d\r\n",g_ucNbiotDevState);
			g_ucDevInit = 1;
			//printf("!!!g_ucNbiotDevState %d\r\n",g_ucNbiotDevState);
		}
	}else if(BC28_ERR== devSt){

	}

	return OPP_SUCCESS;
}
int NbiotStateChg(void)
{
	U8 regStatus = 0, conStatus = 0;
	U8 regNotify = 0;
	APP_LOG_T log;
	int ret;
	
	regNotify = NeulBc28RegisterNotify();
	if(1 == regNotify)
		OppCoapConfig();

/*
	if(BC28_READY != NeulBc28GetDevState())
		return 0;
*/

/*
	if(BC28_READY == NeulBc28GetDevState())
		return 0;
*/		

	NeulBc28QueryNetstats(&regStatus, &conStatus);
	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "NbiotStateChg %d %d\r\n", regStatus, conStatus);
	/*×¢²á×´Ì¬*/
	if((NOT_REG == regStatus) || (TRY_REG == regStatus) || (UNKNOW_REG == regStatus) || (DENY_REG == regStatus))
	{
		log.id = NBNET_LOGID;
		log.level = LOG_LEVEL_WARN;
		log.module = MC_MODULE_LAMP;
		log.len = 1;
		log.log[0] = 2;
		ApsDaemonLogReport(&log);
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "\r\n------------some error reg %d--------------\r\n", regStatus);
		//NeulBc28SetDevState(BC28_ERR);		
		/*ret = NbiotHardReset(5000);
		if(0 == ret){
			DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "nbiot hard reset fail by regstatus %d\r\n",regStatus);
		}else{
			DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "nbiot hard reset ok by regstatus %d\r\n",regStatus);
		}*/
	}	
	/*idle*/
	if(0 == conStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "NbiotStateChg nbiot idle\r\n");
		/*sleep*/
		//ApsCoapOceanconHeart();
	}
	/*connect*/
	else if(1 == conStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "NbiotStateChg nbiot connect\r\n");
		/*if(!init)
		{
			printf("connect init clk src%d\r\n", TimeGetClockSrc());
			if(NBIOT_CLK == TimeGetClockSrc())
				TimeFromNbiotClock();
			
			NeulBc28QueryUestats(&g_stThisLampInfo.stNeulSignal);
			strcpy(g_stThisLampInfo.stObjInfo.aucObjectSN, g_stThisLampInfo.stNeulSignal.imei);
			OppCoapConfig();
			//error cause g_ucNbiotDevState change
			printf("!!!g_ucNbiotDevState %d\r\n",g_ucNbiotDevState);
			init = 1;
			printf("!!!g_ucNbiotDevState %d\r\n",g_ucNbiotDevState);
		}*/
	}
	/*psm*/
	else if(2 == conStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "NbiotStateChg nbiot psm send heart\r\n");
		//ApsCoapOceanconHeart(CHL_NB);
	}
	return 0;
}
void MainRebootLog(void)
{
	APP_LOG_T log;

	log.id = REBOOT_LOGID;
	log.level = LOG_LEVEL_WARN;
	log.module = MC_MODULE_MAIN;
	log.len = 0;
	ApsDaemonLogReport(&log);

}
void MainPrintInfoDelay(int timeout)
{
	static U32 tick = 0;

	if(0 == tick){
		tick = OppTickCountGet();
	}else if(OppTickCountGet() - tick > timeout){
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_ERROR, "MainThread run..........\r\n");
		tick = 0;
	}
}
void MainInit()
{
	ST_CONFIG_PARA stConfigPara;

	Print_ProductClass_Sku_Ver();
  	DEBUG_MAIN_INIT();
	// DEBUG_LOG_INIT();
	SVS_MAIN_INIT_FIRST();
	APP_MAIN_INIT_FIRST();
	DRV_MAIN_INIT();
	DEV_MAIN_INIT();
  	SVS_MAIN_INIT_SECOND();
	LIB_MAIN_INIT();
  	APP_MAIN_INIT_SECOND();
	//APP_MAIN_INIT();
	MainRebootLog();	
	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "init phase 1.............\r\n");
	MsgMmtSenderReg(CHL_NB, (p_fun_sender)NeulBc28DataSend, (p_fun_isbusy)NeulBc28IsBusy);
	MsgMmtSenderReg(CHL_WIFI, (p_fun_sender)UdpSend, (p_fun_isbusy)UdpIsBusy);
	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "init phase 2.............\r\n");
	RegOPPTimerGetRtc((pfGetRtc)TimeRead);
	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "init phase 3.............\r\n");
	//NeulBc28StateChgReg(NbiotStateChg);
	//NeulBc28DevChgReg(NbiotDevStateChg);
	//NeulBc28TimeReg(TimeSetCallback);
	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "init phase 4.............\r\n");
	OppLampCtrlGetConfigParaFromFlash();
	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "OppLampCtrlGetConfigParaFromFlash.............\r\n");
	OppLampCtrlGetConfigPara(&stConfigPara);
	//OppLampCtrlSetDimType(stConfigPara.dimType);
	LampOuttypeSet(stConfigPara.dimType);
	//OppLampCtrlSetDimLevel(BOOT_SRC,500);
	OppLampCtrlOnOff(BOOT_SRC, 1);
	
	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "init phase 5.............\r\n");		
	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "init phase 6.............\r\n");

	OppLampCtrlReadWorkSchemeFromFlash();

}

void sysModeReset(void)
{
	static int sysModeResetFlag=0;
	OS_TICK_T tick;
	U16 mode,cnt;
	int res;

	if(sysModeResetFlag == 1) return;
	
	tick = OS_GET_TICK();
	if(tick >= SYS_MODE_NORMAL_RUN_TIME)
	{	
		if(sysModeSet(SYS_MODE_NORMAL,0)!=0)
		{
			DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_ERROR, "sysModeReset fail,try again!\r\n");
			if(sysModeSet(SYS_MODE_NORMAL,0)!=0)
			{
				DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_ERROR, "sysModeReset fail again!\r\n");
			}
			else
			{
				DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "Try aggin setting sysModeReset success!\r\n");
			}
		}
		else
		{
			DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO, "sysModeReset success!\r\n");
		}
		sysModeResetFlag = 1;
	}
}

extern U32 tickDiff(U32 a, U32 b);
void MainThread(void *pvParameter)
{
	/*
	static RTIME_T t[12+1];
	static U32 moment[12];
	char* t_name[12]={
	    "ApsLampLoop()",
	    "OppApsLogLoop()",
	    "ApsCoapLoop()",
	    "MsgMmtLoop()",
	    "LocationLoop()",
	    "TimeLoop()",
	    "LogDbLoop()",
	    "NeulBc28Loop()",
		"LedLoop()",
		"OppLampCtrlLoop()",
		"AppComLoop()",
	    "AppDaemonLoop()",
	};
	int i=0;
	U32 tmp;
	*/
	//APPMAIN_OPT_CHECK(esp_task_wdt_add(NULL),"App main task wdt add fail\r\n");
	
    //MainInit();
    while(1)
    {	
		SVS_MAIN_LOOP();
		DEV_MAIN_LOOP();
		DRV_MAIN_LOOP();
		APP_MAIN_LOOP();
		MainPrintInfoDelay(60000);
		vTaskDelay(10 / portTICK_PERIOD_MS);
		taskWdtReset();
		sysModeReset();
		//APPMAIN_OPT_CHECK(esp_task_wdt_reset(),"App main task wdt reset fail\r\n");
		/*
		i=0;
		moment[i++] = OppTickCountGet();
		ApsLampLoop();
		moment[i++] = OppTickCountGet();
	    OppApsLogLoop();
	    moment[i++] = OppTickCountGet();
	    ApsCoapLoop();
	    moment[i++] = OppTickCountGet();
	    MsgMmtLoop();
	    moment[i++] = OppTickCountGet();
	    LocationLoop();
	    moment[i++] = OppTickCountGet();
	    TimeLoop();
	    moment[i++] = OppTickCountGet();
	    LogDbLoop();
	    moment[i++] = OppTickCountGet();
	    NeulBc28Loop(); 
	    moment[i++] = OppTickCountGet();
		LedLoop(); 
		moment[i++] = OppTickCountGet();
		OppLampCtrlLoop(); 
		moment[i++] = OppTickCountGet();
		AppComLoop();
		moment[i++] = OppTickCountGet();
	    AppDaemonLoop();
	    moment[i++] = OppTickCountGet();

		for(int j=0;j<12;j++)
	    {
	    	t[j].time = moment[j+1]-moment[j];
	    	if(t[j].time > t[j].max) t[j].max = t[j].time;
	    }
		tmp = moment[12]-moment[0]-t[4].time-t[7].time;
		if(tmp > 0 ){ // > 0
			DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO,"=====================================\r\n");
			
		    for(int j=0;j<12;j++)
		    {
		    	DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO,"%s------ %d ms,max=%d\r\n",t_name[j],t[j].time,t[j].max);
		    }
		    tmp = moment[12]-moment[0];
		    DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_INFO,"total:%dms,other(except loc,bc28)=%dms\r\n",tmp,tmp-t[4].time-t[7].time);
		}
		*/
		
    }
}

