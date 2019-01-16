#include <Includes.h>
#include <DRV_Gps.h>
#include <DRV_SpiMeter.h>
#include <DRV_DimCtrl.h>
#include <DRV_Bc28.h>
#include <OPP_Debug.h>
#include <Opp_Module.h>
#include <APP_Daemon.h>
#include <APP_LampCtrl.h>
#include <SVS_Time.h>
#include <APP_Main.h>
#include <DEV_Utility.h>
#include <SVS_Coap.h>
#include <SVS_Lamp.h>
#include <SVS_Para.h>
#include <SVS_Location.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "esp_wifi.h"
#include <SVS_Elec.h>

extern xSemaphoreHandle g_stLampCfgMutex;
T_MUTEX g_stRuntimeMutex;
T_MUTEX g_stLampCtrlMutex;

ST_OPP_LAMP_INFO g_stThisLampInfo;
ST_WORK_SCHEME g_stPlanScheme[PLAN_MAX];
ST_WORK_SCHEME g_stPlanSchemeDefault[PLAN_MAX]={
		{
			.used = 1,
			.hdr={
				.index = 0,
				.priority = 1,
				.valid = 1,
				.mode = AST_MODE,
			},
			.jobNum = 3,
			.jobs[0]={
				.used = 1,
				.intervTime = 180,
				.dimLevel = 1000,
			},
			.jobs[1]={
				.used = 1,
				.intervTime = 240,
				.dimLevel = 700,
			},
			.jobs[2]={
				.used = 1,
				.intervTime = 120,
				.dimLevel = 1000,
			},
		}
};

ST_CONFIG_PARA g_stConfigParaDefault={
	.period = 1,
	.dimType = 0,
	.plansNum = PLAN_MAX,
};

ST_CONFIG_PARA g_stConfigPara;


U16 g_usAstTimerId = 0;
U16 g_usHeartTimerId = 0;
U32 g_ulAstTimeLoss = 0;
TimerHandle_t g_stDelayTimer = NULL;
int m_iLightOnOff = 1;
char g_aActTime[ACTTIME_LEN]="00-00-00 00:00:00";

void Print_ProductClass_Sku_Ver(void)
{
    U32 Temp_ProductClass = PRODUCTCLASS;
    U32 Temp_ProductSku = PRODUCTSKU;
    U32 Temp_Ver = OPP_LAMP_CTRL_CFG_DATA_VER;
    
    printf("\r\n  ***** class%08x sku%08x ver:%d %s %s****** \r\n", Temp_ProductClass,Temp_ProductSku, Temp_Ver, __DATE__, __TIME__);
}

void OppLampCtrlParaDump(ST_CONFIG_PARA *pstConfigPara)
{
	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"module_lamp config para dimType:%d\r\n",
		pstConfigPara->dimType);
}
void OppLampCtrlVar()
{
	unsigned char mac[6];

	//ST_AST stAstTime;

	//g_stThisLampInfo.stLocation.ldLatitude = DEFAULT_LATITUDE;
	//g_stThisLampInfo.stLocation.ldLongitude = DEFAULT_LONGITUDE;

	//Opple_Calc_Ast_Time(2000, 1, 1, g_stThisLampInfo.stLocation.ldLatitude, g_stThisLampInfo.stLocation.ldLatitude, &stAstTime);
	memset(&g_stThisLampInfo, 0, sizeof(ST_OPP_LAMP_INFO));
	strcpy((char *)g_stThisLampInfo.stObjInfo.productName, PRODUCTNAME);					 /*产品名称*/
	strcpy((char *)g_stThisLampInfo.stObjInfo.protId, PROTID);						 /*HUAWEI 协议号*/
	strcpy((char *)g_stThisLampInfo.stObjInfo.model, MODULE);
	strcpy((char *)g_stThisLampInfo.stObjInfo.manu, MANU);
	g_stThisLampInfo.stObjInfo.uwProductSku = PRODUCTSKU;
	g_stThisLampInfo.stObjInfo.uwProductClass = PRODUCTCLASS;
	g_stThisLampInfo.stObjInfo.ulVersion = OPP_LAMP_CTRL_CFG_DATA_VER;
	g_stThisLampInfo.stObjInfo.enDimType = DIM_010_OUT;
	g_stThisLampInfo.stObjInfo.ucHasRelay = 1;	
	esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf((char *)g_stThisLampInfo.stObjInfo.aucObjectSN,16,"%02X%02X%02X%02X%02X%02X",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	//g_stThisLampInfo.stCurrStatus.ucSwitch = 1;
	//g_stThisLampInfo.stCurrStatus.usBri = 1000;
	//ElecSetElectricInfo();
}
void OppLampCtrlInit()
{
	MUTEX_CREATE(g_stRuntimeMutex);
	MUTEX_CREATE(g_stLampCtrlMutex);
	//MUTEX_CREATE(g_stLampCfgMutex);
	//DimCtrlInit();
	//MeterInit();
	//GpsUartInit();
	//ApsLampInit();
	OppLampCtrlVar();
	//OppLampCtrlSetDimLevel(1000);
	//OppLampCtrlOnOff(1);
	return;
}


ST_WORK_SCHEME * OppLampCtrlGetEmptyWorkSchemeElement(void)
{
	int i = 0;
	
	for(i = 0; i < PLAN_MAX; i++)
	{

		if(g_stPlanScheme[i].used == 0)
		{
			//*index = i;
			return &g_stPlanScheme[i];
		}
	}
	return NULL;
}

U32 OppLampCtrlAddWorkScheme(ST_WORK_SCHEME *pstPlanScheme)
{
	int i = 0;

	if(NULL == pstPlanScheme)
		return OPP_FAILURE;
	
	for(i = 0; i < PLAN_MAX; i++)
	{
		if(g_stPlanScheme[i].used == 0)
		{
			memcpy(&g_stPlanScheme[i], pstPlanScheme, sizeof(ST_WORK_SCHEME));
			return OPP_SUCCESS;
		}
	}

	if(i >= PLAN_MAX)
		return OPP_FAILURE;
	
	return OPP_SUCCESS;
}

U32 OppLampCtrlAddWorkSchemeById(U8 id, ST_WORK_SCHEME *pstPlanScheme)
{
	//int i = 0;
	ST_WORK_SCHEME plan[PLAN_MAX];
	//int len = sizeof(ST_WORK_SCHEME)*PLAN_MAX;
	int ret = OPP_SUCCESS;
	
	if((NULL == pstPlanScheme) || (id > PLAN_MAX-1))
		return OPP_FAILURE;

	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlAddWorkSchemeById id %d write\r\n", id);
	//memcpy(&g_stPlanScheme[id%PLAN_MAX], pstPlanScheme, sizeof(ST_WORK_SCHEME));
	//AppParaRead(APS_PARA_MODULE_APS_LAMP, 1, (U8 *)plan, &len);
	memcpy(plan, &g_stPlanScheme, sizeof(ST_WORK_SCHEME)*PLAN_MAX);
	memcpy(&plan[id], pstPlanScheme, sizeof(ST_WORK_SCHEME));
	ret = AppParaWrite(APS_PARA_MODULE_LAMP, WORKSCHEME_ID, (U8 *)plan, sizeof(ST_WORK_SCHEME)*PLAN_MAX);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlAddWorkSchemeById ret %d\r\n", ret);
		return OPP_FAILURE;
	}
	memcpy(&g_stPlanScheme[id], pstPlanScheme, sizeof(ST_WORK_SCHEME));
	OppLampCtrlWorkSchDump(&plan[id]);
	
	return OPP_SUCCESS;
}

U32 OppLampCtrlDelWorkScheme(U8 idx)
{
	ST_WORK_SCHEME plan[PLAN_MAX];
	//int len = sizeof(ST_WORK_SCHEME)*PLAN_MAX;

	if((idx > PLAN_MAX-1))
		return OPP_FAILURE;

	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlDelWorkScheme id %d write\r\n", idx);
	//AppParaRead(APS_PARA_MODULE_APS_LAMP, 1, (U8 *)plan, &len);
	memcpy(plan, &g_stPlanScheme, sizeof(ST_WORK_SCHEME)*PLAN_MAX);
	memset(&plan[idx], 0, sizeof(ST_WORK_SCHEME));
	AppParaWrite(APS_PARA_MODULE_LAMP, WORKSCHEME_ID, (U8 *)plan, sizeof(ST_WORK_SCHEME)*PLAN_MAX);
	memset(&g_stPlanScheme[idx], 0, sizeof(ST_WORK_SCHEME));

	return OPP_SUCCESS;
}

ST_WORK_SCHEME * OppLampCtrlGetWorkSchemeElementByIdx(U8 index)
{
/*
	if(index >= PLAN_MAX)
		index = PLAN_MAX;
*/
	return &g_stPlanScheme[index%PLAN_MAX];
}

int OppLampCtrlGetWorkSchemeElementByIdxOut(U8 index, ST_WORK_SCHEME * pstPlanScheme)
{
	//int len = sizeof(ST_WORK_SCHEME)*PLAN_MAX;
	
	if((NULL == pstPlanScheme) || (index > PLAN_MAX-1)){
		return OPP_FAILURE;
	}
	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlGetWorkSchemeElementByIdxOut id %d\r\n", index);
	//AppParaRead(APS_PARA_MODULE_APS_LAMP, 1, (U8 *)g_stPlanScheme, &len);
	memcpy(pstPlanScheme, &g_stPlanScheme[index], sizeof(ST_WORK_SCHEME));
	OppLampCtrlWorkSchDump(pstPlanScheme);
	
	return OPP_SUCCESS;
}

void OppLampCtrlWorkSchDump(ST_WORK_SCHEME * pstPlanScheme)
{
	int i = 0;

	if(NULL == pstPlanScheme){
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlWorkSchDump pstPlanScheme is NULL\r\n");
		return;
	}
	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"\r\n~~~~~~~~~~OppLampCtrlWorkSchDump plan %d~~~~~~~~~~~~~\r\n", pstPlanScheme->hdr.index);
	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"used %d\r\n", pstPlanScheme->used);
	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"index %d priority %d valide %d mode %d\r\n", pstPlanScheme->hdr.index, pstPlanScheme->hdr.priority, pstPlanScheme->hdr.valid, pstPlanScheme->hdr.mode);
	if(WEEK_MODE == pstPlanScheme->hdr.mode){
		if(pstPlanScheme->u.stWeekPlan.dateValide){
			DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"sDate: y %04d m %02d d %02d h %02d m %02d s %02d\r\n", 
				pstPlanScheme->u.stWeekPlan.sDate.Year,
				pstPlanScheme->u.stWeekPlan.sDate.Month,
				pstPlanScheme->u.stWeekPlan.sDate.Day,
				pstPlanScheme->u.stWeekPlan.sDate.Hour,
				pstPlanScheme->u.stWeekPlan.sDate.Sencond,
				pstPlanScheme->u.stWeekPlan.sDate.Min);
			DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"eDate: y %04d m %02d d %02d h %02d m %02d s %02d\r\n", 
				pstPlanScheme->u.stWeekPlan.eDate.Year,
				pstPlanScheme->u.stWeekPlan.eDate.Month,
				pstPlanScheme->u.stWeekPlan.eDate.Day,
				pstPlanScheme->u.stWeekPlan.eDate.Hour,
				pstPlanScheme->u.stWeekPlan.eDate.Sencond,
				pstPlanScheme->u.stWeekPlan.eDate.Min);
			DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"weekbitmap: %d\r\n", pstPlanScheme->u.stWeekPlan.bitmapWeek);	
		}
	}
	else if(YMD_MODE == pstPlanScheme->hdr.mode){
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"sDate: y %04d m %02d d %02d h %02d m %02d s %02d\r\n", 
			pstPlanScheme->u.stYmdPlan.sDate.Year,
			pstPlanScheme->u.stYmdPlan.sDate.Month,
			pstPlanScheme->u.stYmdPlan.sDate.Day,
			pstPlanScheme->u.stYmdPlan.sDate.Hour,
			pstPlanScheme->u.stYmdPlan.sDate.Sencond,
			pstPlanScheme->u.stYmdPlan.sDate.Min);
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"eDate: y %04d m %02d d %02d h %02d m %02d s %02d\r\n", 
			pstPlanScheme->u.stYmdPlan.eDate.Year,
			pstPlanScheme->u.stYmdPlan.eDate.Month,
			pstPlanScheme->u.stYmdPlan.eDate.Day,
			pstPlanScheme->u.stYmdPlan.eDate.Hour,
			pstPlanScheme->u.stYmdPlan.eDate.Sencond,
			pstPlanScheme->u.stYmdPlan.eDate.Min);
	}
	
	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"jobNum %d\r\n", pstPlanScheme->jobNum);

	for(i=0;i<JOB_MAX;i++){
		if(1 != pstPlanScheme->jobs[i].used)
			continue;
		
		if(AST_MODE == pstPlanScheme->hdr.mode){
			DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"used %d interv %d dim %d\r\n", pstPlanScheme->jobs[i].used, pstPlanScheme->jobs[i].intervTime,
				pstPlanScheme->jobs[i].dimLevel);
		}else{
			DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"used %d start %02d:%02d, %02d:%02d dim %d\r\n", pstPlanScheme->jobs[i].used,
				pstPlanScheme->jobs[i].startHour,pstPlanScheme->jobs[i].startMin,
				pstPlanScheme->jobs[i].endHour,pstPlanScheme->jobs[i].endMin,
				pstPlanScheme->jobs[i].dimLevel);
		}
	
	}
	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"\r\n~~~~~~~~~~~~~~~~~~~~~~~\r\n");
}

U32 OppLampCtrlSetConfigPara(ST_CONFIG_PARA *pstConfigPara)
{
	int ret = OPP_SUCCESS;
	
	if(NULL == pstConfigPara)
		return OPP_FAILURE;
	MUTEX_LOCK(g_stLampCfgMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaWrite(APS_PARA_MODULE_LAMP, LAMPCONFIG_ID, (U8 *)pstConfigPara, sizeof(ST_CONFIG_PARA));
	if(OPP_SUCCESS != ret){
		MUTEX_UNLOCK(g_stLampCfgMutex);
		return ret;
	}
	
	memcpy(&g_stConfigPara, pstConfigPara, sizeof(ST_CONFIG_PARA));
	MUTEX_UNLOCK(g_stLampCfgMutex);
	return OPP_SUCCESS;
}

U32 OppLampCtrlGetConfigParaFromFlash()
{
	int ret = OPP_SUCCESS;
	unsigned int len = sizeof(ST_CONFIG_PARA);
	ST_CONFIG_PARA stConfigPara;

	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlGetConfigParaFromFlash\r\n");
	MUTEX_LOCK(g_stLampCfgMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaRead(APS_PARA_MODULE_LAMP, LAMPCONFIG_ID, (unsigned char *)&stConfigPara, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"lampctrl config is empty, use default config\r\n");
		memcpy(&g_stConfigPara,&g_stConfigParaDefault,sizeof(ST_CONFIG_PARA));
	}else{
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "lampctrl read config from flash success\r\n");
		memcpy(&g_stConfigPara,&stConfigPara,sizeof(ST_CONFIG_PARA));
	}
	
	OppLampCtrlParaDump(&stConfigPara);

	MUTEX_UNLOCK(g_stLampCfgMutex);
	return OPP_SUCCESS;
}
U32 OppLampCtrlGetConfigPara(ST_CONFIG_PARA *pstConfigPara)
{
	//int ret = OPP_SUCCESS;
	
	if(NULL == pstConfigPara)
		return OPP_FAILURE;
	MUTEX_LOCK(g_stLampCfgMutex,MUTEX_WAIT_ALWAYS);	
	memcpy(pstConfigPara, &g_stConfigPara, sizeof(ST_CONFIG_PARA));
	MUTEX_UNLOCK(g_stLampCfgMutex);
	return OPP_SUCCESS;
}

U32 OppLampCtrlGetRuntimeParaFromFlash(ST_RUNTIME_CONFIG * pstRuntime)
{
	int ret = OPP_SUCCESS;
	unsigned int len = sizeof(ST_RUNTIME_CONFIG);
	ST_RUNTIME_CONFIG stRuntime;

	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlGetRuntimeParaFromFlash\r\n");
	MUTEX_LOCK(g_stRuntimeMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaRead(APS_PARA_MODULE_LAMP, RUNTIME_ID, (unsigned char *)&stRuntime, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlGetRuntimeParaFromFlash fail ret %d\r\n", ret);
	}
	memcpy(pstRuntime, &stRuntime, sizeof(ST_RUNTIME_CONFIG));
	MUTEX_UNLOCK(g_stRuntimeMutex);
	return OPP_SUCCESS;
}

U32 OppLampCtrlSetRuntimeParaToFlash(ST_RUNTIME_CONFIG * pstRuntime)
{
	int ret = OPP_SUCCESS;


	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlSetRuntimeParaToFlash\r\n");
	MUTEX_LOCK(g_stRuntimeMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaWrite(APS_PARA_MODULE_LAMP, RUNTIME_ID, (unsigned char *)pstRuntime, sizeof(ST_RUNTIME_CONFIG));
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlSetRuntimeParaToFlash ret %d\r\n", ret);
	}
	MUTEX_UNLOCK(g_stRuntimeMutex);
	return OPP_SUCCESS;

}

void OppLampCtrlReadWorkSchemeFromFlash(void)
{
	ST_WORK_SCHEME plan[PLAN_MAX];
	int i = 0;
	unsigned int len = sizeof(ST_WORK_SCHEME)*PLAN_MAX;
	int ret = OPP_SUCCESS;
	
	ret = AppParaRead(APS_PARA_MODULE_LAMP, WORKSCHEME_ID, (unsigned char *)plan, &len);
	if(OPP_SUCCESS == ret)
	{
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"have work scheme\r\n");
		memcpy(g_stPlanScheme, plan, PLAN_MAX * sizeof(ST_WORK_SCHEME));
		for(i=0;i<PLAN_MAX;i++){
			OppLampCtrlWorkSchDump(&g_stPlanScheme[i]);
		}
	}else{
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "there is no work scheme in flash\r\n");
	}
}
U32 OppLampCtrlRestoreFactory(void)
{
	ST_WORK_SCHEME plan[PLAN_MAX];
	//int len = sizeof(ST_WORK_SCHEME)*PLAN_MAX;
	int ret;

	ret = OppLampCtrlSetConfigPara(&g_stConfigParaDefault);
	if(OPP_SUCCESS != ret){
		return ret;
	}
	
	memset(plan, 0, sizeof(ST_WORK_SCHEME)*PLAN_MAX);
	ret = AppParaWrite(APS_PARA_MODULE_LAMP, WORKSCHEME_ID, (U8 *)plan, sizeof(ST_WORK_SCHEME)*PLAN_MAX);
	if(OPP_SUCCESS != ret){
		return ret;
	}
	memset(g_stPlanScheme, 0, sizeof(ST_WORK_SCHEME)*PLAN_MAX);
	OppLampCtrlDimmerPolicyReset();

	return OPP_SUCCESS;
}
void swap(ST_JOB *p1, ST_JOB *p2) {
    ST_JOB temp;

	memcpy(&temp, p1, sizeof(ST_JOB));
	memcpy(p1, p2, sizeof(ST_JOB));
	memcpy(p2, &temp, sizeof(ST_JOB));	
}

void bubble_sort(ST_JOB array[], const int num) {
    int j, k;
    for (j = 0; j < num - 1; j++) {
        for (k = 0; k < num - 1 - j; k++) {
            if (array[k].intervTime > array[k + 1].intervTime) {
                swap(&array[k], &array[k + 1]);
            }
        }
    }
}

/*
*"2015-08-06 09:32:60"
*str1<str2, n=0
*str1>str2, n=1
*str1=str2, n=2
*/
int CompareDate(const char * str1,const char * str2)
{
	char pStr1[10], pStr2[10];
	if (strlen(str1)< 19 || strlen(str2)< 19 )
	{
		return -1;
	}

	memset(pStr1,0,10);
	memset(pStr2,0,10);
	memcpy(pStr1,str1,4);
	memcpy(pStr2,str2,4);
	if (atoi(pStr1) == atoi(pStr2))
	{
		memset(pStr1,0,10);
		memset(pStr2,0,10);
		memcpy(pStr1,str1+5,2);
		memcpy(pStr2,str2+5,2);	
		if (atoi(pStr1) == atoi(pStr2))
		{	
			memset(pStr1,0,10);
			memset(pStr2,0,10);
			memcpy(pStr1,str1+5+3,2);
			memcpy(pStr2,str2+5+3,2);
			if (atoi(pStr1) == atoi(pStr2))
			{
				memset(pStr1,0,10);
				memset(pStr2,0,10);
				memcpy(pStr1,str1+5+3+3,2);
				memcpy(pStr2,str2+5+3+3,2);
				if (atoi(pStr1) == atoi(pStr2))
				{
					memset(pStr1,0,10);
					memset(pStr2,0,10);
					memcpy(pStr1,str1+5+3+3+3,2);
					memcpy(pStr2,str2+5+3+3+3,2);
					if (atoi(pStr1) == atoi(pStr2))
					{
						memset(pStr1,0,10);
						memset(pStr2,0,10);
						memcpy(pStr1,str1+5+3+3+3+3,2);
						memcpy(pStr2,str2+5+3+3+3+3,2);
						if (atoi(pStr1) == atoi(pStr2))
						{
							return 2;
						}
						return atoi(pStr1) > atoi(pStr2);
					}
					else 
					return atoi(pStr1) > atoi(pStr2);
				}
				else
					return atoi(pStr1) > atoi(pStr2);
			}
			else
				return atoi(pStr1) > atoi(pStr2);

		}else
		return atoi(pStr1) > atoi(pStr2);	
	}
	else
		return atoi(pStr1) > atoi(pStr2);	
	return -1;
}

/*
*"09:32"
*str1<str2, n=0
*str1>str2, n=1
*str1=str2, n=2
*/
int CompareHM(const char * str1,const char * str2)
{
	char pStr1[5], pStr2[5];
	if (strlen(str1)< 5 || strlen(str2)< 5 )
	{
		return -1;
	}
	memset(pStr1,0,5);
	memset(pStr2,0,5);
	memcpy(pStr1,str1,2);
	memcpy(pStr2,str2,2);
	if (atoi(pStr1) == atoi(pStr2))
	{
		memset(pStr1,0,5);
		memset(pStr2,0,5);
		
		memcpy(pStr1,str1+3,2);
		memcpy(pStr2,str2+3,2);
		if (atoi(pStr1) == atoi(pStr2))
		{
			return 2;
		}
		else
			return atoi(pStr1) > atoi(pStr2);
	}
	else
		return atoi(pStr1) > atoi(pStr2);

	return -1;
}

#if 0
void OppLampCtrlDimmerPolicy(void)
{
	ST_TIME time;
	static U32 onLamptime;
	U32 temp;
	static int i = 0;
	int week = 0;
	static ST_AST stAstTime;
	ST_WORK_SCHEME *pstPriPlanScheme = NULL;
	static U8 jobIndex = 0;
	char sDate[20], eDate[20], cDate[20];
	char sTime[5], eTime[5], cTime[5];
	int n1, n2;
	
	//1.获取时间
	Opple_Get_RTC(&time);
	//printf("%d-%d-%d-%d-%d-%d\r\n", time.Year, time.Month, time.Day, time.Hour, time.Min, time.Sencond);
	sscanf(cDate, "%04d-%02d-%02d %02d:%02d:%02d", time.Year, time.Month, time.Day, time.Hour, time.Sencond, time.Sencond);
	//2.匹配时间
	for(i = 0; i < PLAN_MAX; i++)
	{
		if(g_stPlanScheme[i].used && g_stPlanScheme[i].hdr.valid)
		{
			if(YMD_MODE == g_stPlanScheme[i].hdr.mode)
			{
				sscanf(sDate, "%04d-%02d-%02d %02d:%02d:%02d", 
					g_stPlanScheme[i].u.stYmdPlan.sDate.Year,
					g_stPlanScheme[i].u.stYmdPlan.sDate.Month,
					g_stPlanScheme[i].u.stYmdPlan.sDate.Day,
					g_stPlanScheme[i].u.stYmdPlan.sDate.Hour,
					g_stPlanScheme[i].u.stYmdPlan.sDate.Min,
					g_stPlanScheme[i].u.stYmdPlan.sDate.Sencond);
				
				sscanf(eDate, "%04d-%02d-%02d %02d:%02d:%02d", 
					g_stPlanScheme[i].u.stYmdPlan.eDate.Year,
					g_stPlanScheme[i].u.stYmdPlan.eDate.Month,
					g_stPlanScheme[i].u.stYmdPlan.eDate.Day,
					g_stPlanScheme[i].u.stYmdPlan.eDate.Hour,
					g_stPlanScheme[i].u.stYmdPlan.eDate.Min,
					g_stPlanScheme[i].u.stYmdPlan.eDate.Sencond);
				n1 = CompareDate(cDate,sDate);
				n2 = CompareDate(cDate,eDate);
				/*cDate < sDate || cDate > eDate*/
				if(n1==0 || n2==1)
				{
					OppLampCtrlOnOff(0);
					continue;
				}
				else
				{
					//first time
					if(NULL == pstPriPlanScheme)
					{
						pstPriPlanScheme = &g_stPlanScheme[i];
					}
					//high priority
					else if(pstPriPlanScheme->hdr.priority > g_stPlanScheme[i].hdr.priority)
					{
						pstPriPlanScheme = &g_stPlanScheme[i];
					}
				}
			}
			else if(WEEK_MODE == g_stPlanScheme[i].hdr.mode)
			{
				if(g_stPlanScheme[i].u.stWeekPlan.dateValide)
				{
					sscanf(sDate, "%04d-%02d-%02d %02d:%02d:%02d", 
						g_stPlanScheme[i].u.stWeekPlan.sDate.Year,
						g_stPlanScheme[i].u.stWeekPlan.sDate.Month,
						g_stPlanScheme[i].u.stWeekPlan.sDate.Day,
						g_stPlanScheme[i].u.stWeekPlan.sDate.Hour,
						g_stPlanScheme[i].u.stWeekPlan.sDate.Min,
						g_stPlanScheme[i].u.stWeekPlan.sDate.Sencond);
					
					sscanf(eDate, "%04d-%02d-%02d %02d:%02d:%02d", 
						g_stPlanScheme[i].u.stWeekPlan.eDate.Year,
						g_stPlanScheme[i].u.stWeekPlan.eDate.Month,
						g_stPlanScheme[i].u.stWeekPlan.eDate.Day,
						g_stPlanScheme[i].u.stWeekPlan.eDate.Hour,
						g_stPlanScheme[i].u.stWeekPlan.eDate.Min,
						g_stPlanScheme[i].u.stWeekPlan.eDate.Sencond);
						n1 = CompareDate(cDate,sDate);
						n2 = CompareDate(cDate,eDate);
						/*cDate < sDate || cDate > eDate*/
						if(n1==0 || n2==1)
						{
							OppLampCtrlOnOff(0);
							continue;
						}
				}
				week = TimeCacWeek(time.Year, time.Month, time.Day);
				if(1<<week & g_stPlanScheme[i].u.stWeekPlan.bitmapWeek)
				{
					//first time
					if(NULL == pstPriPlanScheme)
					{
						pstPriPlanScheme = &g_stPlanScheme[i];
					}
					//high priority
					else if(pstPriPlanScheme->hdr.priority > g_stPlanScheme[i].hdr.priority)
					{
						pstPriPlanScheme = &g_stPlanScheme[i];
					}
				}
			}
			else if(AST_MODE == g_stPlanScheme[i].hdr.mode)
			{
				Opple_Calc_Ast_Time(time.Year, time.Month, time.Day, g_stThisLampInfo.stLocation.ldLatitude, g_stThisLampInfo.stLocation.ldLongitude, &stAstTime);

				//high priority
				if(NULL == pstPriPlanScheme)
				{
					pstPriPlanScheme = &g_stPlanScheme[i];
				}
				else if(pstPriPlanScheme->hdr.priority > g_stPlanScheme[i].hdr.priority)
				{
					pstPriPlanScheme = &g_stPlanScheme[i];
				}
			}
			
		}
	}
	
	//5.执行调光策略
	if(NULL == pstPriPlanScheme){
		//printf("\r\n~~~~no dim work~~~~~\r\n");
		return;
	}

	//printf("\r\n~~~~do dim work~~~~~\r\n");
	if(jobIndex < pstPriPlanScheme->jobNum)
	{
		if(AST_MODE != pstPriPlanScheme->hdr.mode)
		{
			sscanf(cDate, "%02d:%02d", time.Hour, time.Min);
			sscanf(sDate, "%02d:%02d", pstPriPlanScheme->jobs[jobIndex].startHour, pstPriPlanScheme->jobs[jobIndex].startMin);
			sscanf(eDate, "%02d:%02d", pstPriPlanScheme->jobs[jobIndex].endHour, pstPriPlanScheme->jobs[jobIndex].endMin);
			n1 = CompareHM(cDate,sDate);
			n2 = CompareHM(cDate,eDate);			
			/*cDate < sDate || cDate > sDate*/
			if(n1==0 || n2==1)
			{
				//off lamp
				if(1 == g_stThisLampInfo.stCurrStatus.ucSwitch)
				{
					OppLampCtrlOnOff(0);
				}
				jobIndex++;
			}
			else
			{
				OppLampCtrlSetDimLevel(pstPriPlanScheme->jobs[jobIndex].dimLevel);
				//on lamp
				if(0 == g_stThisLampInfo.stCurrStatus.ucSwitch)
				{
					OppLampCtrlOnOff(1);
				}
			}
		}
		else
		{
			temp = time.Hour*60 + time.Sencond;
			
			sscanf(cDate, "%02d:%02d", time.Hour, time.Min);		
			sscanf(sDate, "%02d:%02d", stAstTime.setH, stAstTime.setM);
			sscanf(eDate, "%02d:%02d", time.Hour < stAstTime.riseH, time.Min < stAstTime.riseM);
			n1 = CompareHM(cDate,sDate);
			n2 = CompareHM(cDate,eDate);
			/*cDate < sDate || cDate > sDate*/
			if(n1==0 || n2==1)
			{
				//off lamp
				if(1 == g_stThisLampInfo.stCurrStatus.ucSwitch)
				{
					OppLampCtrlOnOff(0);
				}
				jobIndex++;
			}		
			/*大于日出小于日落*/
			else
			{
				//on lamp
				onLamptime = temp;
				OppLampCtrlSetDimLevel(pstPriPlanScheme->jobs[jobIndex].dimLevel);
				if(0 == g_stThisLampInfo.stCurrStatus.ucSwitch)
				{
					OppLampCtrlOnOff(1);
				}
				if(temp - onLamptime >= pstPriPlanScheme->jobs[jobIndex].intervTime)
				{
					if(jobIndex > pstPriPlanScheme->jobNum)
						jobIndex = pstPriPlanScheme->jobNum;
					OppLampCtrlSetDimLevel(pstPriPlanScheme->jobs[jobIndex].dimLevel);
				}			
			}

		}
		
	}
	else
	{
		jobIndex = 0;
	}	

	return;
}
#else

#if 1
static U32 m_ulOnLastTime;
static ST_WORK_SCHEME *m_pstCurPlanScheme = NULL, *m_pstLastPlanScheme = NULL;
static U8 m_iPlanIndex = 0;	
static U8 m_ucJobIndex = 0;
static U8 m_ucPhase = 0; /*phase:0 find plan, phase:1 dimmer*/
static U8 m_ucAstInit = 0;
static U8 m_aucJobDone[JOB_MAX] = {0}; /*0:not do job, 1:already do job*/
static U8 m_ucInvalideTimeJobDone = 0;
#else
U32 m_ulOnLastTime;
ST_WORK_SCHEME *m_pstCurPlanScheme = NULL;
U8 m_iPlanIndex = 0;	
U8 m_ucJobIndex = 0;
U8 m_ucPhase = 0; /*phase:0 find plan, phase:1 dimmer*/
U8 m_ucAstInit = 0;
#endif

/*
*return: 1, no job done, 0, some job done
*/
int OppLampCtrlDimmerHasJobsDone()
{
	int i = 0;
	
	for(i=0;i<JOB_MAX;i++){
		if(1 == m_aucJobDone[i]){
			return 0;
		}
	}

	return 1;
}

int OppLampCtrlDimmerJobsInit()
{
	int i = 0;
	
	for(i=0;i<JOB_MAX;i++){
		m_aucJobDone[i] = 0;
	}

	return 0;
}

int OppLampCtrlDimmerJobsReset(void)
{
	OppLampCtrlDimmerJobsInit();
	m_ulOnLastTime = 0;
	m_ucInvalideTimeJobDone = 0;
	m_ucAstInit = 0;
	m_ucJobIndex = 0;

	return OPP_SUCCESS;
}

int OppLampCtrlDimmerPolicyReset(void)
{
	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlDimmerPolicyReset\r\n");
	//m_ulOnLastTime = 0;
	m_pstCurPlanScheme = NULL;
	m_iPlanIndex = 0;	
	//m_ucJobIndex = 0;
	m_ucPhase = 0;
	//m_ucAstInit = 0;
	//m_ucInvalideTimeJobDone = 0;
	OppLampCtrlDimmerJobsReset();

	return OPP_SUCCESS;
}


/*不允许24小时最大设置范围，00-23:59*/
int OppLampCtrlDimmerFindInterNight(ST_WORK_SCHEME *pstPlanScheme, int * idx)
{
	char sTime[6], eTime[6];
	int n, i;
	
	for(i=0;i<pstPlanScheme->jobNum;i++){
		sprintf(sTime, "%02d:%02d", pstPlanScheme->jobs[i].startHour, pstPlanScheme->jobs[i].startMin);
		sprintf(eTime, "%02d:%02d", pstPlanScheme->jobs[i].endHour, pstPlanScheme->jobs[i].endMin);
		n = CompareHM(sTime,eTime);
		//午夜点:stime>=etime
		if(1 == n || 2 == n){
			//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"%d is interNight\r\n", i);
			*idx = i;
			return OPP_SUCCESS;
		}
	}

	return OPP_FAILURE;
}

int OppLampCtrlDimmerFindIdx(ST_TIME *time,ST_WORK_SCHEME *pstPlanScheme, int * index)
{
	int ret, idx = -1;
	int i,n1,n2;
	char sTime[6], eTime[6], cTime[6];
	//int find = 0;
	
	ret = OppLampCtrlDimmerFindInterNight(pstPlanScheme, &idx);
	if(OPP_SUCCESS == ret){
		sprintf(cTime, "%02d:%02d", time->Hour, time->Min);
		sprintf(sTime, "%02d:%02d", pstPlanScheme->jobs[idx].startHour, pstPlanScheme->jobs[idx].startMin);
		sprintf(eTime, "%02d:%02d", pstPlanScheme->jobs[idx].endHour, pstPlanScheme->jobs[idx].endMin);
		n1 = CompareHM(cTime,sTime);
		n2 = CompareHM(cTime,eTime);
		//24进位
		if((1 == n1 || 2 == n1) || 0 == n2){
			//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"find in interNight %d\r\n",idx);
			*index = idx;
			return OPP_SUCCESS;
		}
	}
	for(i=0; i<pstPlanScheme->jobNum;i++){
		if(i == idx)
			continue;
		
		sprintf(cTime, "%02d:%02d", time->Hour, time->Min);
		sprintf(sTime, "%02d:%02d", pstPlanScheme->jobs[i].startHour, pstPlanScheme->jobs[i].startMin);
		sprintf(eTime, "%02d:%02d", pstPlanScheme->jobs[i].endHour, pstPlanScheme->jobs[i].endMin);
		/*DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"idx[%d]: c:%02d:%02d, s:%02d:%02d, e:%02d:%02d\r\n",i,
			time->Hour, time->Min,
			pstPlanScheme->jobs[i].startHour, pstPlanScheme->jobs[i].startMin,
			pstPlanScheme->jobs[i].endHour, pstPlanScheme->jobs[i].endMin
			);*/
		//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"idx[%d]: c:%s, s:%s, e:%s\r\n",i,cTime,sTime,eTime);
		n1 = CompareHM(cTime,sTime);
		n2 = CompareHM(cTime,eTime);
		//sTime<=cTime<eTime
		if((1 == n1 || 2 == n1) && (0 == n2/* || 2 == n2*/)){
			//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"find normal %d\r\n", i);
			*index = i;
			return OPP_SUCCESS;
		}
	}
	
	//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"not in job list\r\n",idx);

	return OPP_FAILURE;

}

void OppLampCtrlDimmerMakeLog(U8 plan, U8 job, U8 sw, U16 bri)
{
	APP_LOG_T log;

	log.id = WORKPLAN_LOGID;
	log.level = LOG_LEVEL_INFO;
	log.module = MC_MODULE_LAMP;
	log.len = 5;
	log.log[0] = plan;
	log.log[1] = job;
	log.log[2] = sw;
	*((U16 *)&log.log[3]) = bri;
	ApsDaemonLogReport(&log);

}
U8 OppLampCtrlGetPhase(void)
{
	return m_ucPhase;
}
U8 OppLampCtrlGetPlanIdx(void)
{
	if(m_pstCurPlanScheme)
		return (m_pstCurPlanScheme->hdr.index);
	return INVALIDE_PLANID;
}
U8 OppLampCtrlGetJobsIdx(void)
{
	return (m_ucJobIndex+1)> JOB_MAX ? INVALIDE_JOBID:(m_ucJobIndex+1);
}

U8 OppLampCtrlGetValidePlanId(int * paiPlanId, int * num)
{
	int i=0,j=0;
	for(i=0;i<PLAN_MAX;i++){
		if(1 == g_stPlanScheme[i].used){
			paiPlanId[j++]=i+1;
		}
	}

	*num = j;

	return OPP_SUCCESS;
}
void OppLampCtrlDimmerPolicy(void)
{
	ST_TIME time;
	U32 temp = 0, total = 0;
	int week = 0;
	ST_AST stAstTime;
	char sDate[20] = {0}, eDate[20] = {0}, cDate[20] = {0};
	char sTime[6] = {0}, eTime[6] = {0}, cTime[6] = {0};
	int n1 = 0, n2 = 0, i = 0;
	int ret = 0, idx = 0;
	ST_OPP_LAMP_LOCATION stLocationPara;
	U8 ucSwitch;
	long double lat,lng;
	ST_WORKPLAN_REPORT stWorkPlanR;
	U8 invalideTime = 0;
	
	if(NO_RECV_TIME == timeRecvTimeGet()){
		//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "no time here\r\n");
		return;
	}
	//1.获取时间
	Opple_Get_RTC(&time);
	//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"%04d-%02d-%02d-%02d-%02d-%02d\r\n", time.Year, time.Month, time.Day, time.Hour, time.Min, time.Sencond);
	sprintf(cDate, "%04d-%02d-%02d %02d:%02d:%02d", time.Year, time.Month, time.Day, time.Hour, time.Sencond, time.Sencond);
	//2.匹配时间
	if(0 == m_ucPhase)
	{
		/*DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"index %d mod %d used %d valid %d prio %d\r\n", 
			m_iPlanIndex, g_stPlanScheme[m_iPlanIndex].hdr.mode,
			g_stPlanScheme[m_iPlanIndex].used, g_stPlanScheme[m_iPlanIndex].hdr.valid,
			g_stPlanScheme[m_iPlanIndex].hdr.priority);*/
		
		if(g_stPlanScheme[m_iPlanIndex].used && g_stPlanScheme[m_iPlanIndex].hdr.valid)
		{
			if(YMD_MODE == g_stPlanScheme[m_iPlanIndex].hdr.mode 
				|| AST_MODE == g_stPlanScheme[m_iPlanIndex].hdr.mode)
			{
				sprintf(sDate, "%04d-%02d-%02d %02d:%02d:%02d", 
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.sDate.Year,
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.sDate.Month,
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.sDate.Day,
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.sDate.Hour,
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.sDate.Min,
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.sDate.Sencond);
				
				sprintf(eDate, "%04d-%02d-%02d %02d:%02d:%02d", 
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.eDate.Year,
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.eDate.Month,
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.eDate.Day,
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.eDate.Hour,
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.eDate.Min,
					g_stPlanScheme[m_iPlanIndex].u.stYmdPlan.eDate.Sencond);
				n1 = CompareDate(cDate,sDate);
				n2 = CompareDate(cDate,eDate);
				//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"ymd cDate %s sDate %s eDate %s n1 %d  n2 %d\r\n", cDate, sDate, eDate, n1, n2);
				/*sDate <= cDate <= eDate */
				if((n1==1 || n1==2) && (n2==0 || n2 ==2))
				{
					//first time
					if(NULL == m_pstCurPlanScheme)
					{
						m_pstCurPlanScheme = &g_stPlanScheme[m_iPlanIndex];
					}
					//high priority
					else if(m_pstCurPlanScheme->hdr.priority > g_stPlanScheme[m_iPlanIndex].hdr.priority)
					{
						m_pstCurPlanScheme = &g_stPlanScheme[m_iPlanIndex];
					}
				}
			}
			else if(WEEK_MODE == g_stPlanScheme[m_iPlanIndex].hdr.mode)
			{
				if(g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.dateValide)
				{
					sprintf(sDate, "%04d-%02d-%02d %02d:%02d:%02d", 
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.sDate.Year,
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.sDate.Month,
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.sDate.Day,
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.sDate.Hour,
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.sDate.Min,
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.sDate.Sencond);
					
					sprintf(eDate, "%04d-%02d-%02d %02d:%02d:%02d", 
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.eDate.Year,
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.eDate.Month,
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.eDate.Day,
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.eDate.Hour,
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.eDate.Min,
						g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.eDate.Sencond);
						n1 = CompareDate(cDate,sDate);
						n2 = CompareDate(cDate,eDate);
						//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"week cDate %s sDate %s eDate %s n1 %d  n2 %d\r\n", cDate, sDate, eDate, n1, n2);
						/*sDate <= cDate <= eDate */
						if((n1==1 || n1==2) && (n2==0 ||n2 ==2))
						{
							week = TimeCacWeek(time.Year, time.Month, time.Day);
							if(1<<week & g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.bitmapWeek)
							{
								//first time
								if(NULL == m_pstCurPlanScheme)
								{
									m_pstCurPlanScheme = &g_stPlanScheme[m_iPlanIndex];
								}
								//high priority
								else if(m_pstCurPlanScheme->hdr.priority > g_stPlanScheme[m_iPlanIndex].hdr.priority)
								{
									m_pstCurPlanScheme = &g_stPlanScheme[m_iPlanIndex];
								}
							}
						}
				}
				else{
					week = TimeCacWeek(time.Year, time.Month, time.Day);
					if(1<<week & g_stPlanScheme[m_iPlanIndex].u.stWeekPlan.bitmapWeek)
					{
						//first time
						if(NULL == m_pstCurPlanScheme)
						{
							m_pstCurPlanScheme = &g_stPlanScheme[m_iPlanIndex];
						}
						//high priority
						else if(m_pstCurPlanScheme->hdr.priority > g_stPlanScheme[m_iPlanIndex].hdr.priority)
						{
							m_pstCurPlanScheme = &g_stPlanScheme[m_iPlanIndex];
						}
					}
				}
			}			
		}
		m_iPlanIndex++;
	}

	if(m_iPlanIndex < PLAN_MAX){
		//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"\r\ngoto find policy~~~~~\r\n");
		return;
	}else{
		/*if not find invalide workplan then do invalide job*/
		if(NULL == m_pstCurPlanScheme){
			m_iPlanIndex = 0;
			return;
		}
		else{
			m_ucPhase = 1;
		}
	}

	//if invalide date judge force to stop, when doing the job
	if(m_pstCurPlanScheme->used && m_pstCurPlanScheme->hdr.valid)
	{
		if(YMD_MODE == m_pstCurPlanScheme->hdr.mode 
			|| AST_MODE == m_pstCurPlanScheme->hdr.mode)
		{
			sprintf(sDate, "%04d-%02d-%02d %02d:%02d:%02d", 
				m_pstCurPlanScheme->u.stYmdPlan.sDate.Year,
				m_pstCurPlanScheme->u.stYmdPlan.sDate.Month,
				m_pstCurPlanScheme->u.stYmdPlan.sDate.Day,
				m_pstCurPlanScheme->u.stYmdPlan.sDate.Hour,
				m_pstCurPlanScheme->u.stYmdPlan.sDate.Min,
				m_pstCurPlanScheme->u.stYmdPlan.sDate.Sencond);
			
			sprintf(eDate, "%04d-%02d-%02d %02d:%02d:%02d", 
				m_pstCurPlanScheme->u.stYmdPlan.eDate.Year,
				m_pstCurPlanScheme->u.stYmdPlan.eDate.Month,
				m_pstCurPlanScheme->u.stYmdPlan.eDate.Day,
				m_pstCurPlanScheme->u.stYmdPlan.eDate.Hour,
				m_pstCurPlanScheme->u.stYmdPlan.eDate.Min,
				m_pstCurPlanScheme->u.stYmdPlan.eDate.Sencond);
			n1 = CompareDate(cDate,sDate);
			n2 = CompareDate(cDate,eDate);
			//if date is invalide then force to stop
			//cDate < sDate || cDate > eDate 
			if((n1==0) || (n2==1))
			{
				invalideTime = 1;
			}
		}
		else if(WEEK_MODE == m_pstCurPlanScheme->hdr.mode)
		{
			if(m_pstCurPlanScheme->u.stWeekPlan.dateValide)
			{
				sprintf(sDate, "%04d-%02d-%02d %02d:%02d:%02d", 
					m_pstCurPlanScheme->u.stWeekPlan.sDate.Year,
					m_pstCurPlanScheme->u.stWeekPlan.sDate.Month,
					m_pstCurPlanScheme->u.stWeekPlan.sDate.Day,
					m_pstCurPlanScheme->u.stWeekPlan.sDate.Hour,
					m_pstCurPlanScheme->u.stWeekPlan.sDate.Min,
					m_pstCurPlanScheme->u.stWeekPlan.sDate.Sencond);
				
				sprintf(eDate, "%04d-%02d-%02d %02d:%02d:%02d", 
					m_pstCurPlanScheme->u.stWeekPlan.eDate.Year,
					m_pstCurPlanScheme->u.stWeekPlan.eDate.Month,
					m_pstCurPlanScheme->u.stWeekPlan.eDate.Day,
					m_pstCurPlanScheme->u.stWeekPlan.eDate.Hour,
					m_pstCurPlanScheme->u.stWeekPlan.eDate.Min,
					m_pstCurPlanScheme->u.stWeekPlan.eDate.Sencond);
					n1 = CompareDate(cDate,sDate);
					n2 = CompareDate(cDate,eDate);
					//if date is invalide then force to stop
					//cDate < sDate || cDate > eDate
					if((n1==0) || (n2==1))
					{
						invalideTime = 1;
					}
			}
		}		
	}

	//if invalide week judge force to stop, when doing the job
	if(WEEK_MODE == m_pstCurPlanScheme->hdr.mode){
		week = TimeCacWeek(time.Year, time.Month, time.Day);
		if(1<<week & ~(m_pstCurPlanScheme->u.stWeekPlan.bitmapWeek)){
			invalideTime = 1;
		}
	}
	//do invalide time job
	if(invalideTime){
		//do invalide time job
		if(0 == m_ucInvalideTimeJobDone){
			/*异步调光，不能实时获取switch状态*/
			OppLampCtrlSetDimLevel(TIME_SRC, BRI_MAX);
			OppLampCtrlGetSwitch(0,&ucSwitch);
			if(0 == ucSwitch){
				ucSwitch = 1;
			}
			//report  workplan		
			stWorkPlanR.planId = OppLampCtrlGetPlanIdx();
			stWorkPlanR.type = m_pstCurPlanScheme->hdr.mode;
			stWorkPlanR.jobId = INVALIDE_JOBID;
			stWorkPlanR.sw = ucSwitch;
			OppLampCtrlGetBri(0,&stWorkPlanR.bri);
			ApsCoapWorkPlanReport(CHL_NB, NULL, &stWorkPlanR);
			OppLampCtrlDimmerMakeLog(OppLampCtrlGetPlanIdx(),INVALIDE_JOBID,ucSwitch,stWorkPlanR.bri);
			OppLampCtrlDimmerJobsInit();
			m_ucInvalideTimeJobDone = 1;
		}				
		m_ucPhase = 0;
		m_iPlanIndex = 0;
		if(AST_MODE == m_pstCurPlanScheme->hdr.mode)
			m_ucAstInit = 0;
		m_pstCurPlanScheme = NULL;
		return;
	}

	//if find new entry, should do job reset
	if(m_pstLastPlanScheme != m_pstCurPlanScheme){
		OppLampCtrlDimmerJobsReset();
		m_pstLastPlanScheme = m_pstCurPlanScheme;
	}
	
	//if(m_pstCurPlanScheme->jobs[m_ucJobIndex].used)
		if(AST_MODE != m_pstCurPlanScheme->hdr.mode)
		{
			ret = OppLampCtrlDimmerFindIdx(&time, m_pstCurPlanScheme,&idx);
			if(OPP_SUCCESS == ret){
				//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "~~~~~~~idx %d\r\n", idx);
				m_ucJobIndex = idx;
				//do valide time job
				if(0 == m_aucJobDone[idx]){
					//invalide time job clean
					if(m_ucInvalideTimeJobDone)
						m_ucInvalideTimeJobDone = 0;

					/*异步调光，不能实时获取switch状态*/
					OppLampCtrlSetDimLevel(TIME_SRC, m_pstCurPlanScheme->jobs[idx].dimLevel);
					OppLampCtrlGetSwitch(0,&ucSwitch);
					if(0 == ucSwitch){
						//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"~~~~~~~~lamp on~~~~~~~~~\r\n");
						ucSwitch = 1;
						//OppLampCtrlOnOff(TIME_SRC, ucSwitch);
					}
					//report  workplan
					stWorkPlanR.planId = OppLampCtrlGetPlanIdx();
					stWorkPlanR.type = m_pstCurPlanScheme->hdr.mode;
					stWorkPlanR.jobId = OppLampCtrlGetJobsIdx();
					stWorkPlanR.sw = ucSwitch;
					OppLampCtrlGetBri(0,&stWorkPlanR.bri);
					ApsCoapWorkPlanReport(CHL_NB, NULL, &stWorkPlanR);
					OppLampCtrlDimmerMakeLog(OppLampCtrlGetPlanIdx(),OppLampCtrlGetJobsIdx(),ucSwitch,stWorkPlanR.bri);
					
					m_aucJobDone[idx] = 1;
				}
			}else{
				//do invalide time job
				if(0 == m_ucInvalideTimeJobDone){
					/*异步调光，不能实时获取switch状态*/
					OppLampCtrlSetDimLevel(TIME_SRC, BRI_MAX);
					OppLampCtrlGetSwitch(0,&ucSwitch);
					if(0 == ucSwitch){
						ucSwitch = 1;
					}
					//report  workplan					
					stWorkPlanR.planId = OppLampCtrlGetPlanIdx();
					stWorkPlanR.type = m_pstCurPlanScheme->hdr.mode;
					stWorkPlanR.jobId = INVALIDE_JOBID;
					stWorkPlanR.sw = ucSwitch;
					OppLampCtrlGetBri(0,&stWorkPlanR.bri);
					ApsCoapWorkPlanReport(CHL_NB, NULL, &stWorkPlanR);
					
					OppLampCtrlDimmerMakeLog(OppLampCtrlGetPlanIdx(),INVALIDE_JOBID,ucSwitch,stWorkPlanR.bri);
					OppLampCtrlDimmerJobsInit();
					m_ucInvalideTimeJobDone = 1;
				}				
				//re-find the entry
				m_ucPhase = 0;
				m_iPlanIndex = 0;
				m_pstCurPlanScheme = NULL;
			}
		}
		else
		{
			//LocationRead(GPS_LOC, &stLocationPara);
			LocGetLat(&lat);
			LocGetLng(&lng);
			Opple_Calc_Ast_Time(time.Year, time.Month, time.Day, time.Zone, lat, lng, &stAstTime);
			sprintf(cTime, "%02d:%02d", time.Hour, time.Min);		
			sprintf(sTime, "%02d:%02d", stAstTime.setH, stAstTime.setM);
			sprintf(eTime, "%02d:%02d", stAstTime.riseH, stAstTime.riseM);

			//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"ast on cTime:%s, set:%s, rise:%s\r\n", cTime, sTime,eTime);

			n1 = CompareHM(cTime,sTime);
			n2 = CompareHM(cTime,eTime);
			/*day eDate < cDate < sDate */
			if(n1==0 && n2==1)
			{
				// do invalide time job
				if(0 == m_ucInvalideTimeJobDone){
					DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"ast off light");
					//off lamp
					/*异步调光，不能实时获取switch状态*/
					OppLampCtrlSetDimLevel(TIME_SRC, BRI_MAX);
					OppLampCtrlGetSwitch(0,&ucSwitch);
					if(0 == ucSwitch){
						ucSwitch = 1;
					}					
					//report  workplan
					stWorkPlanR.planId = OppLampCtrlGetPlanIdx();
					stWorkPlanR.type = m_pstCurPlanScheme->hdr.mode;
					stWorkPlanR.jobId = INVALIDE_JOBID;
					stWorkPlanR.sw = ucSwitch;
					OppLampCtrlGetBri(0,&stWorkPlanR.bri);
					ApsCoapWorkPlanReport(CHL_NB, NULL, &stWorkPlanR);

					OppLampCtrlDimmerMakeLog(OppLampCtrlGetPlanIdx(),INVALIDE_JOBID,ucSwitch,stWorkPlanR.bri);
					m_ucInvalideTimeJobDone = 1;
					OppLampCtrlDimmerJobsInit();
				}
				//re-find the entry
				m_ucPhase = 0;
				m_ucAstInit = 0;
				m_iPlanIndex = 0;
				m_pstCurPlanScheme = NULL;
			}
			/*night*/
			else
			{
				//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"ast on set:%02d:%02d, rise:%02d:%02d\r\n",stAstTime.setH,stAstTime.setM, stAstTime.riseH,stAstTime.riseM);
				//cTime >= setTime
				if(m_ucInvalideTimeJobDone)
					m_ucInvalideTimeJobDone = 0;
				if(1==n1 || 2 ==n1){
					temp = (time.Hour*60 + time.Min) - (stAstTime.setH*60 + stAstTime.setM);
				}
				//cTime < setTime
				else if(0==n1){
					temp = ((time.Hour+24)*60 + time.Min) - (stAstTime.setH*60 + stAstTime.setM);
				}
				//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"temp %d  m_ucJobIndex %d interval %d \r\n", temp, m_ucJobIndex, m_pstCurPlanScheme->jobs[m_ucJobIndex].intervTime,m_ucAstInit);
				for(i=0; i<m_pstCurPlanScheme->jobNum;i++){
					total += m_pstCurPlanScheme->jobs[i].intervTime;
					if(temp <= total){
						m_ucJobIndex = i;
						break;
					}else{
						m_ucJobIndex = m_pstCurPlanScheme->jobNum-1;
					}
				}
				
				//do jobs
				if(0 == m_aucJobDone[m_ucJobIndex]){
					//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"ast on light");
					OppLampCtrlSetDimLevel(TIME_SRC, m_pstCurPlanScheme->jobs[m_ucJobIndex].dimLevel);
					OppLampCtrlGetSwitch(0,&ucSwitch);
					if(0 == ucSwitch)
					{
						//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"~~~~~~~~ast lamp on~~~~~~~~~\r\n");
						ucSwitch = 1;
						//OppLampCtrlOnOff(TIME_SRC, ucSwitch);
					}

					//report  workplan
					stWorkPlanR.planId = OppLampCtrlGetPlanIdx();
					stWorkPlanR.type = m_pstCurPlanScheme->hdr.mode;
					stWorkPlanR.jobId = OppLampCtrlGetJobsIdx();
					stWorkPlanR.sw = ucSwitch;
					OppLampCtrlGetBri(0,&stWorkPlanR.bri);
					ApsCoapWorkPlanReport(CHL_NB, NULL, &stWorkPlanR);
					
					OppLampCtrlDimmerMakeLog(OppLampCtrlGetPlanIdx(),OppLampCtrlGetJobsIdx(),ucSwitch,stWorkPlanR.bri);
					m_aucJobDone[m_ucJobIndex] = 1;
				}
			}
		}

	return;
}

#endif

void OppLampCtrlOnOff(EN_OP_SRC opSrc, U8 onOff)
{
	APP_LOG_T log;

	log.id = SWITCH_LOGID;
	log.level = LOG_LEVEL_INFO;
	log.module = MC_MODULE_LAMP;
	log.len = 2;
	log.log[0] = onOff;
	log.log[1] = opSrc;
	ApsDaemonLogReport(&log);
	if(0 == onOff)
	{
		LampOnOff(opSrc,0);
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlOnOff 0\r\n")
	}
	else
	{
		LampOnOff(opSrc,1);
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO,"OppLampCtrlOnOff 1\r\n")
	}

	return;
}



/*0-1000*/
void OppLampCtrlSetDimLevel(EN_OP_SRC opSrc, U16 percent)
{
	APP_LOG_T log;

/*
	if(g_stThisLampInfo.stObjInfo.enDimType == DIM_010_OUT)
	{
		LampDim(percent);
	}
	else
	{
		DimPwmDutyCycleSet(percent);
	}
*/
	/*if(percent < BRI_MIN)
		percent = BRI_MIN;
	
	if(percent > BRI_MAX)
		percent = BRI_MAX;
*/
	DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "OppLampCtrlSetDimLevel src %d %d\r\n", opSrc, percent);

	log.id = BRI_LOGID;
	log.level = LOG_LEVEL_INFO;
	log.module = MC_MODULE_LAMP;
	log.len = 3;
	*((U16 *)&log.log[0]) = percent;
	log.log[2] = opSrc;
	ApsDaemonLogReport(&log);

	//MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);	
	LampDim(opSrc,percent*10);
	//MUTEX_UNLOCK(g_stLampCtrlMutex);

}

U32 OppLampCtrlGetSwitchU32(U8 targetId, U32 * lampSwitch)
{
	int state, level;

	//MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	//*lampSwitch = g_stThisLampInfo.stCurrStatus.ucSwitch;
	LampStateRead(&state,&level);
	*lampSwitch = state;
	//MUTEX_UNLOCK(g_stLampCtrlMutex);
	return OPP_SUCCESS;
}

static U32 g_ulLampExepPwr = 0;
U32 OppLampCtrlGetExepPwr()
{
	return g_ulLampExepPwr;
}
U32 OppLampCtrlSetExepPwr(U32 pwr)
{
	g_ulLampExepPwr = pwr;
	return 0;
}

U32 OppLampCtrlGetOnExep(U8 targetId, U32 * exep)
{
	int state, level;
	unsigned int power;
	static unsigned int tick = 0, tick1 = 0;
	static unsigned char first = 1;
	int ret;
	
	if(first){
		tick = OppTickCountGet();
		first = 0;
	}
	if(0 != tick){
		if(OppTickCountGet()-tick<STARTUP_TIME){
			return OPP_FAILURE;
		}else{
			tick = 0;
		}
	}

	LampStateRead(&state,&level);
	if(0 == state){
		//*exep = 0;
		tick1 = 0;
		return OPP_FAILURE;
	}
	
	if(0 == tick1){
		tick1 = OppTickCountGet();
	}
	if(OppTickCountGet() - tick1 > READPOWER_PERIOD){
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "OppLampCtrlGetOnExep timeout tick %d***\r\n",OppTickCountGet());
		ret = ElecPowerGet(0,&power);
		if(OPP_SUCCESS != ret){
			//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "OppLampCtrlGetOnExep call ElecPowerGet err ret %d***\r\n",ret);
			return OPP_FAILURE;
		}
		OppLampCtrlSetExepPwr(power/10);
		//FIXME: max bri(10000) not equal max power(30)
		if(level == 10000){
			if(power < MAX_LAMP_PWR){
				*exep = 1;				
			}else{
				*exep = 0;
			}
		}else{
			if(power < EXEP_LOW_PWR){
				*exep = 1;
			}else if(power > EXEP_HIGH_PWR){
				*exep = 0;
			}
		}
		tick1 = 0;
		return OPP_SUCCESS;		
	}else{
		return OPP_FAILURE;
	}
	return OPP_SUCCESS;
}

U32 OppLampCtrlGetOffExep(U8 targetId, U32 * exep)
{
	int state, level;
	unsigned int power;
	static unsigned int tick = 0, tick1 = 0;
	static unsigned char first = 1;
	int ret;
	
	if(first){
		tick = OppTickCountGet();
		first = 0;
	}
	if(0 != tick){
		if(OppTickCountGet()-tick<STARTUP_TIME){
			return OPP_FAILURE;
		}else{
			tick = 0;
		}
	}

	LampStateRead(&state,&level);
	if(0 != state){
		//*exep = 0;
		tick1 = 0;
		return OPP_FAILURE;
	}
	if(0 == tick1){
		tick1 = OppTickCountGet();
	}
	if(OppTickCountGet() - tick1 > READPOWER_PERIOD){
		DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "OppLampCtrlGetOffExep timeout tick %d***\r\n",OppTickCountGet());
		ret = ElecPowerGet(0,&power);
		if(OPP_SUCCESS != ret){
			//DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "OppLampCtrlGetOffExep call ElecPowerGet err ret %d***\r\n",ret);
			return OPP_FAILURE;
		}
		OppLampCtrlSetExepPwr(power/10);
		if(power > EXEP_HIGH_PWR){
			*exep = 1;
		}else if(power < EXEP_LOW_PWR){
			*exep = 0;
		}
		tick1 = 0;
		return OPP_SUCCESS;		
	}else{
		return OPP_FAILURE;
	}
	return OPP_SUCCESS;
}

U32 OppLampCtrlGetSwitch(U8 targetId, U8 * lampSwitch)
{
	int state, level;

	//MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	LampStateRead(&state,&level);
	*lampSwitch = state;
	//MUTEX_UNLOCK(g_stLampCtrlMutex);
	return OPP_SUCCESS;
}
U32 OppLampCtrlSetSwitch(U8 targetId, U8 lampSwitch)
{
	//MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	//MUTEX_UNLOCK(g_stLampCtrlMutex);
	return OPP_SUCCESS;
}

U32 OppLampCtrlGetBriU32(U8 targetId, U32 * bri)
{
	int state, level;

	//MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	//*bri = g_stThisLampInfo.stCurrStatus.usBri;
	LampStateRead(&state,&level);
	*bri = level/10;
	//MUTEX_UNLOCK(g_stLampCtrlMutex);
	return OPP_SUCCESS;
}

U32 OppLampCtrlGetBri(U8 targetId, U16 * bri)
{
	int state, level;

	//MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	LampStateRead(&state,&level);
	*bri = level/10;
	//MUTEX_UNLOCK(g_stLampCtrlMutex);
	return OPP_SUCCESS;
}
U32 OppLampCtrlSetBri(U8 targetId, U16 bri)
{
	//MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	//MUTEX_UNLOCK(g_stLampCtrlMutex);
	return OPP_SUCCESS;
}
U32 OppLampCtrlGetDimType(U8 * dimType)
{
	//MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	*dimType = LampOuttypeGet();
	//MUTEX_UNLOCK(g_stLampCtrlMutex);
	return OPP_SUCCESS;
}
U32 OppLampCtrlSetDimType(U8 dimType)
{
	//MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	//MUTEX_UNLOCK(g_stLampCtrlMutex);
	return OPP_SUCCESS;
}

U32 OppLampCtrlGetRtime(U8 targetId, U32 * rtime)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	*rtime = g_stThisLampInfo.stCurrStatus.uwRunTime;
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;
}

U32 OppLampCtrlGetHtime(U8 targetId, U32 * htime)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	*htime = g_stThisLampInfo.stCurrStatus.uwHisTime;
	MUTEX_UNLOCK(g_stLampCtrlMutex);		
	return OPP_SUCCESS;
}
U32 OppLampCtrlSetRtime(U8 targetId, U32 rtime)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	g_stThisLampInfo.stCurrStatus.uwRunTime = rtime;
	MUTEX_UNLOCK(g_stLampCtrlMutex);			
	return OPP_SUCCESS;
}

U32 OppLampCtrlSetHtime(U8 targetId, U32 htime)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	g_stThisLampInfo.stCurrStatus.uwHisTime = htime;
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;
}

U32 OppLampCtrlHtimeAdd(U8 num)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	g_stThisLampInfo.stCurrStatus.uwHisTime += num;
	if(g_stThisLampInfo.stCurrStatus.uwHisTime > MAX_RUNTIME)
		g_stThisLampInfo.stCurrStatus.uwHisTime = g_stThisLampInfo.stCurrStatus.uwHisTime - MAX_RUNTIME - 1;
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;
}

U32 OppLampCtrlRtimeAdd(U8 num)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	g_stThisLampInfo.stCurrStatus.uwRunTime += num;
	if(g_stThisLampInfo.stCurrStatus.uwRunTime > MAX_RUNTIME)
		g_stThisLampInfo.stCurrStatus.uwRunTime = g_stThisLampInfo.stCurrStatus.uwRunTime - MAX_RUNTIME - 1;
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;
}

/*input time with crc, output time without crc*/
unsigned int LightTimeCrc8ToLightTime(unsigned int inLtime, unsigned int *outLtime)
{
	unsigned char *vptr,*vptr1;
	unsigned int temp,temp1;
	
	vptr = (unsigned char *)&inLtime;
	temp1 = vptr[0] | vptr[1]<<8 | vptr[2]<<16;
	temp = temp1;
	vptr1 = (unsigned char *)&temp1;
	vptr1[3] = cal_crc8(&vptr1[0],3);
	if(vptr1[3] != vptr[3]){
		temp = 0;
	}
	*outLtime = temp;
	return 0;

}
/*input time without crc, output time with crc*/
unsigned int LightTimeToLightTimeCrc8(unsigned int inLtime, unsigned int *outLtime)
{
	unsigned char *vptr,*vptr1;
	unsigned int temp;
	
	vptr1 = (unsigned char *)&inLtime;
	temp = vptr1[0] | vptr1[1]<<8 | vptr1[2]<<16;
	vptr = (unsigned char *)&temp;
	vptr[3] = cal_crc8(&vptr[0],3);
	*outLtime = temp;
	return 0;
}

/*判断crc是否相同，若不相同返回错误，否则返回成功*/
unsigned int LightTimeCrcJudge(unsigned int hltime)
{
	unsigned char *vptr,*vptr1;
	unsigned int temp;
	
	vptr1 = (unsigned char *)&hltime;
	temp = vptr1[0] | vptr1[1]<<8 | vptr1[2]<<16;
	vptr = (unsigned char *)&temp;
	vptr[3] = cal_crc8(&vptr[0],3);
	if(vptr[3] != vptr1[3])
		return -1;
	return 0;
}


U32 OppLampCtrlGetLtime(U8 targetId, U32 * ltime)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	LightTimeCrc8ToLightTime(g_stThisLampInfo.stCurrStatus.uwLightTime,ltime);
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;
}

U32 OppLampCtrlGetLtimeWithCrc8(U8 targetId, U32 * ltime)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	*ltime = g_stThisLampInfo.stCurrStatus.uwLightTime;
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;

}

U32 OppLampCtrlGetHLtime(U8 targetId, U32 * hltime)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	LightTimeCrc8ToLightTime(g_stThisLampInfo.stCurrStatus.uwHisLightTime,hltime);
	MUTEX_UNLOCK(g_stLampCtrlMutex);		
	return OPP_SUCCESS;
}

U32 OppLampCtrlGetHLtimeWithCrc8(U8 targetId, U32 * hltime)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	*hltime = g_stThisLampInfo.stCurrStatus.uwHisLightTime;
	MUTEX_UNLOCK(g_stLampCtrlMutex);		
	return OPP_SUCCESS;
}

U32 OppLampCtrlSetLtime(U8 targetId, U32 ltime)
{
	unsigned int temp;

	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	temp = ltime;
	if(temp > MAX_LIGHT_TIME)
		temp = temp - MAX_LIGHT_TIME - 1;
	LightTimeToLightTimeCrc8(temp,&g_stThisLampInfo.stCurrStatus.uwLightTime);
	MUTEX_UNLOCK(g_stLampCtrlMutex);			
	return OPP_SUCCESS;
}

U32 OppLampCtrlSetLtimeWithCrc8(U8 targetId, U32 ltime)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	g_stThisLampInfo.stCurrStatus.uwLightTime = ltime;
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;
}

U32 OppLampCtrlSetHLtime(U8 targetId, U32 hltime)
{
	unsigned int temp;
	
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	temp = hltime;
	if(temp > MAX_LIGHT_TIME)
		temp = temp - MAX_LIGHT_TIME - 1;
	LightTimeToLightTimeCrc8(temp,&g_stThisLampInfo.stCurrStatus.uwHisLightTime);
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;
}

U32 OppLampCtrlSetHLtimeWithCrc8(U8 targetId, U32 hltime)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	g_stThisLampInfo.stCurrStatus.uwHisLightTime = hltime;
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;

}

U32 OppLampCtrlHLtimeAdd(U8 num)
{
	unsigned int temp;
	
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	if(0 == m_iLightOnOff){
		MUTEX_UNLOCK(g_stLampCtrlMutex);		
		return OPP_SUCCESS;
	}
	LightTimeCrc8ToLightTime(g_stThisLampInfo.stCurrStatus.uwHisLightTime,&temp);
	temp += num;
	if(temp > MAX_LIGHT_TIME)
		temp = temp - MAX_LIGHT_TIME - 1;
	LightTimeToLightTimeCrc8(temp,&g_stThisLampInfo.stCurrStatus.uwHisLightTime);
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;
}

U32 OppLampCtrlLtimeAdd(U8 num)
{
	unsigned int temp;

	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	if(0 == m_iLightOnOff){
		MUTEX_UNLOCK(g_stLampCtrlMutex);	
		return OPP_SUCCESS;
	}
	LightTimeCrc8ToLightTime(g_stThisLampInfo.stCurrStatus.uwLightTime,&temp);
	temp += num;
	if(temp > MAX_LIGHT_TIME)
		temp = temp - MAX_LIGHT_TIME - 1;
	LightTimeToLightTimeCrc8(temp,&g_stThisLampInfo.stCurrStatus.uwLightTime);
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;
}


U32 OppLampCtrlLightOnOff(int onOff)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	if(onOff)
		m_iLightOnOff = 1;
	else
		m_iLightOnOff = 0;
	MUTEX_UNLOCK(g_stLampCtrlMutex);	
	return OPP_SUCCESS;
}

U8 g_ucDelaySwitch = 0;
U8 g_ucSrcChl = 0;
void OppLampDelayOnOffCallback(TimerHandle_t timer)
{
	U8 ucDelaySwitch,ucSrcChl;

	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	ucDelaySwitch = g_ucDelaySwitch;
	ucSrcChl = g_ucSrcChl;
	MUTEX_UNLOCK(g_stLampCtrlMutex);

	printf("OppLampDelayOnOffCallback chl %d sw %d\r\n", ucSrcChl, ucDelaySwitch);
	OppLampCtrlOnOff(ucSrcChl,ucDelaySwitch);
}
void OppLampDelayOnOff(U8 srcChl, U8 sw, U32 sec)
{
	printf("OppLampDelayOnOffCallback chl %d sw %d sec %d\r\n", srcChl, sw, sec);
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	g_ucDelaySwitch = sw;
	g_ucSrcChl = srcChl;
	MUTEX_UNLOCK(g_stLampCtrlMutex);
	
	if(g_stDelayTimer)
		xTimerDelete(g_stDelayTimer,0);
    g_stDelayTimer = xTimerCreate
                   ("One Sec TImer",
                   sec*1000,
                   pdFALSE,
                  ( void * ) 0,
                  OppLampDelayOnOffCallback);

     if( g_stDelayTimer != NULL ) {
        xTimerStart( g_stDelayTimer, 0 );
    }

}
void OppLampDelayInfo(U8 * srcChl, U8 * sw)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	*sw = g_ucDelaySwitch;
	*srcChl = g_ucSrcChl;
	MUTEX_UNLOCK(g_stLampCtrlMutex);
}

void OppLampActTimeGet(char *buf)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	strncpy(buf,g_aActTime,ACTTIME_LEN);
	MUTEX_UNLOCK(g_stLampCtrlMutex);
}

void OppLampActTimeSet(char *buf)
{
	MUTEX_LOCK(g_stLampCtrlMutex, MUTEX_WAIT_ALWAYS);
	strncpy(g_aActTime,buf,ACTTIME_LEN);
	MUTEX_UNLOCK(g_stLampCtrlMutex);
}

/*
*查询统计信息
*/
void OppLampCtrlLoop(void)
{
	OppLampCtrlDimmerPolicy();		
}

