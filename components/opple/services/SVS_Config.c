
#include <Includes.h>
#include <string.h>
#include "DRV_Nb.h"
#include <SVS_MsgMmt.h>
#include <APP_Communication.h>
#include <APP_LampCtrl.h>
#include <SVS_Coap.h>
#include <SVS_Config.h>
#include <SVS_Para.h>
#include <APP_Main.h>
#include <DEV_Utility.h>
#include <OPP_Debug.h>
#include <DRV_Gps.h>
#include <DRV_Bc28.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//int g_iIotInit = 0;
T_MUTEX g_stIotMutex;
T_MUTEX g_stDevCfgMutex;
T_MUTEX g_stDevCacheMutex;

ST_IOT_CONFIG g_stIotConfigDefault={
	.ocIp = COM_PLT,
	.ocPort = 5683,
};
ST_DEV_CONFIG g_stDevConfigDefault={
	.nb = 1,
	.wifi = 1,
	.gps = 1,
	.meter = 1,
	.sensor = 1,
	.rs485 = 1,
	.light = 1,
};

ST_IOT_CONFIG g_stIotConfig;

ST_DEV_CONFIG g_stDevConfig;
static U32 m_udwIotTick = 0;

void OppCoapIOTParaDump(ST_IOT_CONFIG *pstConfigPara)
{
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"module_config config para ocIp:%s\r\n" \
		"ocPort:%d\r\n",
		pstConfigPara->ocIp,
		pstConfigPara->ocPort);
}

int OppCoapIOTSetConfig(ST_IOT_CONFIG *pstConfigPara)
{	
	if(NULL == pstConfigPara)
		return OPP_FAILURE;
	MUTEX_LOCK(g_stIotMutex,MUTEX_WAIT_ALWAYS);	
	memcpy(&g_stIotConfig, pstConfigPara, sizeof(ST_IOT_CONFIG));
	MUTEX_UNLOCK(g_stIotMutex);
	return OPP_SUCCESS;
}

int OppCoapIOTGetConfig(ST_IOT_CONFIG *pstConfigPara)
{	
	if(NULL == pstConfigPara)
		return OPP_FAILURE;
	MUTEX_LOCK(g_stIotMutex,MUTEX_WAIT_ALWAYS);	
	memcpy(pstConfigPara, &g_stIotConfig, sizeof(ST_IOT_CONFIG));
	MUTEX_UNLOCK(g_stIotMutex);
	return OPP_SUCCESS;
}

int OppCoapIOTGetConfigFromFlash(ST_IOT_CONFIG *pstConfigPara)
{
	int ret = OPP_SUCCESS;
	unsigned int len = sizeof(ST_IOT_CONFIG);
	ST_IOT_CONFIG stConfigPara;

	//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"NeulBc28GetConfigFromFlash\r\n");
	MUTEX_LOCK(g_stIotMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaRead(APS_PARA_MODULE_APS_IOT, IOTCONFIG_ID, (unsigned char *)&stConfigPara, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"iot config is empty, use default config\r\n");
		memcpy(pstConfigPara,&g_stIotConfigDefault,sizeof(ST_IOT_CONFIG));
	}else{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "iot read config from flash success\r\n");
		memcpy(pstConfigPara,&stConfigPara,sizeof(ST_IOT_CONFIG));
	}
	OppCoapIOTParaDump(&stConfigPara);
	MUTEX_UNLOCK(g_stIotMutex);
	return OPP_SUCCESS;
}

int OppCoapIOTSetConfigToFlash(ST_IOT_CONFIG *pstConfigPara)
{
	int ret = OPP_SUCCESS;

	//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"OppCoapIOTSetConfigToFlash\r\n");
	MUTEX_LOCK(g_stIotMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaWrite(APS_PARA_MODULE_APS_IOT, IOTCONFIG_ID, (unsigned char *)pstConfigPara, sizeof(ST_IOT_CONFIG));
	if(OPP_SUCCESS != ret){
		//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO,"OppCoapIOTSetConfigToFlash write flash ret %d\r\n", ret);
		MUTEX_UNLOCK(g_stIotMutex);
		return ret;
	}
	
	MUTEX_UNLOCK(g_stIotMutex);
	return OPP_SUCCESS;
}

int OppCoapIOTRestoreFactory(void)
{
	int ret;
	
	ret = OppCoapIOTSetConfigToFlash(&g_stIotConfigDefault);
	if(OPP_SUCCESS != ret){
		return OPP_FAILURE;
	}

	OppCoapIOTSetConfig(&g_stIotConfigDefault);
	
	return OPP_SUCCESS;
}
int OppCoapIOTStateChg(void)
{
	//int ret;
	signed char iotStatus;
	signed char iotLastStatus;

	NeulBc28QueryIOTstats(&iotStatus);
	NeulBc28QueryLastIOTstats(&iotLastStatus);
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "OppCoapIOTStateChg lastStatus %d status %d\r\n", iotLastStatus, iotStatus);

	if(IOT_NOWORK == iotStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "iot nowork\r\n");
		printf("iot nowork\r\n");
	}
	else if(IOT_WAIT_MSG == iotStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "iot wait msg\r\n");
		printf("iot wait msg\r\n");		
	}
	else if(IOT_START_REG == iotStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "iot start reg\r\n");
		printf("iot start reg\r\n");		
	}
	else if(IOT_REG == iotStatus)/*FIXME*/
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "Successful registration indication\r\n");
	}
	else if(IOT_DEREG == iotStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "Deregister complete\r\n");

	}
	else if(IOT_1900_OBSERVE == iotStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "IoT platform has observed the data object 19\r\n");		
	}
	else if(IOT_503_OBSERVE  == iotStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "IoT platform has observed the firmware object 5\r\n");
	}
	else if(IOT_UPDATE_URL == iotStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "Notify the device to receive update package URL\r\n");
		NeulBc28DfotaSet(2);
	}
	else if(IOT_UPDATE_MSG == iotStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "Notify the device to receive update message\r\n");
		NeulBc28DfotaSet(4);
	}
	else if(IOT_CANCEL_1900 == iotStatus)
	{
		DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "Cancel object 19/0/0 observe\r\n");	
		//deregister
		//NeulBc28HuaweiIotRegistration(1);
		//register
		//NeulBc28HuaweiIotRegistration(0);
		/*ret = NbiotHardReset(5000);
		if(0 == ret){
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "OppCoapIOTStateChg hard reset fail\r\n");
		}else{
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR, "OppCoapIOTStateChg hard reset ok\r\n");
		}*/
		NbiotHardResetInner();
	}

	if(IOT_WAIT_MSG == iotStatus || IOT_START_REG == iotStatus || IOT_REG == iotStatus){
		m_udwIotTick = OppTickCountGet();
	}else{
		m_udwIotTick = 0;
	}
	
	return 0;
}

void OppCoapConfigInit(void)
{
	ST_IOT_CONFIG stConfigPara;

	NeulBc28ProtoInit(COAP_PRO);
	NeulBc28IotChgReg(OppCoapIOTStateChg);
	MUTEX_CREATE(g_stIotMutex);
	OppCoapIOTGetConfigFromFlash(&stConfigPara);
	DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "OppCoapConfigInit: %s, %d\r\n", stConfigPara.ocIp, stConfigPara.ocPort);
	OppCoapIOTSetConfig(&stConfigPara);
}


void OppCoapConfig(void)
{
	//ST_IOT_CONFIG stConfigPara;
	//NeulBc28ProtoInit(COAP_PRO);
	//NeulBc28IotChgReg(OppCoapIOTStateChg);
	
	//NeulBc28SetCdpServer("180.101.147.115");
	//if(!g_iIotInit){
		//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "OppCoapConfig111 g_ucIotInit %d\r\n", g_iIotInit);
		//OppCoapIOTGetConfig(&stConfigPara);
		//NeulBc28SetCdpServer(stConfigPara.ocIp);
		//NeulBc28HuaweIotSetRegmod(0);
		//NeulBc28HuaweiIotQueryReg();
		//NeulBc28HuaweiDataEncryption();
		NeulBc28HuaweiIotRegistration(0);
		NeulBc28SetLastIotState(NeulBc28GetIotState());
		NeulBc28SetIotState(IOT_START_REG);
		//NeulBc28HuaweiIotRegistration(2);
		//g_iIotInit = 1;
		//DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "OppCoapConfig222 g_ucIotInit %d\r\n", g_iIotInit);
	//}
}

void OppCoapLoop(void)
{
	int ret;
	ST_IOT_CONFIG stIotConfigPara;
	int type;
	
	if(m_udwIotTick > 0){
		if(OppTickCountGet() - m_udwIotTick > IOTSTATE_DEFAULT_TO){
			if(IOT_WAIT_MSG == NeulBc28GetIotState() || IOT_START_REG == NeulBc28GetIotState()){
				OppCoapIOTGetConfig(&stIotConfigPara);
				ret = NeulBc28SetCdpServer(stIotConfigPara.ocIp);
				if(0 == ret){
					DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28SetCdpServer fail ret %d\r\n", OppTickCountGet(), ret);	  
					return;
				}
				NeulBc28Sleep(30);		
				//if iot regmod=auto chang to manu and hard reboot
				ret = NeulBc28HuaweiIotQueryReg(&type);
				if(0 == ret && 1 == type){
					ret = NeulBc28HuaweIotSetRegmod(0);
					if(0 != ret){
						DEBUG_LOG(DEBUG_MODULE_BC28, DLL_ERROR, "[%d]NeulBc28HuaweIotSetRegmod fail ret %d\r\n", OppTickCountGet(), ret);	  
						return;
					}
				}
				NeulBc28SetLastIotState(NeulBc28GetIotState());
				NeulBc28SetIotState(IOT_INIT);
			}
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "\r\n~~~~~~~~~~~\r\nOppCoapLoop steady iotStatus 0 greate than 60s\r\n~~~~~~~~~\r\n");
			//deregister
			//NeulBc28HuaweiIotRegistration(1);
			//register
			//NeulBc28HuaweiIotRegistration(0);
			ret = NbiotHardReset(5000);
			if(0 == ret){
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "OppCoapIOTStateChg hard reset fail\r\n");
			}else{
				DEBUG_LOG(DEBUG_MODULE_COAP, DLL_INFO, "OppCoapIOTStateChg hard reset ok\r\n");
			}
			m_udwIotTick  = 0;
		}
	}
}

int ApsDevConfigInit()
{
	//ST_DEV_CONFIG stDevConfig;
	//int ret;
	
	MUTEX_CREATE(g_stDevCfgMutex);
	MUTEX_CREATE(g_stDevCacheMutex);
	/*ret = ApsDevConfigReadFromFlash(&stDevConfig);
	if(OPP_SUCCESS != ret){
		ApsDevConfigWrite(&g_stDevConfigDefault);
	}else{
		ApsDevConfigWrite(&stDevConfig);
	}*/
	return OPP_SUCCESS;
}

int ApsDevConfigRead(ST_DEV_CONFIG * pstDevConfig)
{
	MUTEX_LOCK(g_stDevCacheMutex, MUTEX_WAIT_ALWAYS);
	memcpy(pstDevConfig, &g_stDevConfig,sizeof(ST_DEV_CONFIG));
	MUTEX_UNLOCK(g_stDevCacheMutex);
	return OPP_SUCCESS;
}

int ApsDevConfigWrite(ST_DEV_CONFIG * pstDevConfig)
{
	MUTEX_LOCK(g_stDevCacheMutex, MUTEX_WAIT_ALWAYS);
	memcpy(&g_stDevConfig, pstDevConfig,sizeof(ST_DEV_CONFIG));
	MUTEX_UNLOCK(g_stDevCacheMutex);
	return OPP_SUCCESS;
}

int ApsDevConfigReadFromFlash(ST_DEV_CONFIG * pstDevConfig)
{
	int ret;
	ST_DEV_CONFIG stDevConfig;
	unsigned int len = sizeof(ST_DEV_CONFIG);
	
	MUTEX_LOCK(g_stDevCfgMutex, MUTEX_WAIT_ALWAYS);
	ret = AppParaRead(APS_PARA_MODULE_APS_DEV, DEVCONFIG_ID, (unsigned char *)&stDevConfig, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "ElecReadFlash fail ret %d\r\n",ret);
		MUTEX_UNLOCK(g_stDevCfgMutex);
		return OPP_FAILURE;
	}

	memcpy(pstDevConfig, &stDevConfig, sizeof(ST_DEV_CONFIG));
	MUTEX_UNLOCK(g_stDevCfgMutex);
	return OPP_SUCCESS;
}
int ApsDevConfigWriteToFlash(ST_DEV_CONFIG * pstDevConfig)
{
	int ret;
	
	MUTEX_LOCK(g_stDevCfgMutex, MUTEX_WAIT_ALWAYS);
	ret = AppParaWrite(APS_PARA_MODULE_APS_DEV, DEVCONFIG_ID, (unsigned char *)pstDevConfig, sizeof(ST_DEV_CONFIG));
	if(OPP_SUCCESS != ret){
		MUTEX_UNLOCK(g_stDevCfgMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stDevCfgMutex);
	return OPP_SUCCESS;
}

int ApsDevFeaturesSet(EN_FEATURE feature, int value)
{
	int ret;
	ST_DEV_CONFIG stDevConfig;

	ApsDevConfigRead(&stDevConfig);
	if(NB_FEATURE == feature)
		stDevConfig.nb = value;
	else if(WIFI_FEATURE == feature)
		stDevConfig.wifi = value;
	else if(GPS_FEATURE == feature)
		stDevConfig.gps= value;
	else if(METER_FEATURE == feature)
		stDevConfig.meter= value;
	else if(SENSOR_FEATURE == feature)
		stDevConfig.sensor = value;
	else if(RS485_FEATURE == feature)
		stDevConfig.rs485 = value;
	else if(LIGHT_FEATURE == feature)
		stDevConfig.light = value;
	else
		return OPP_FAILURE;
	//[code view] no need to save flash
	/*ret = ApsDevConfigWriteToFlash(&stDevConfig);
	if(OPP_SUCCESS != ret){
		return OPP_FAILURE;
	}*/
	ret = ApsDevConfigWrite(&stDevConfig);
	if(OPP_SUCCESS != ret){
		return OPP_FAILURE;
	}

	return OPP_SUCCESS;
}

int ApsDevFeaturesGet(EN_FEATURE feature, int * value)
{
	ST_DEV_CONFIG stDevConfig;

	if(feature >= UNKNOW_FEATURE)
		return OPP_FAILURE;
	
	ApsDevConfigRead(&stDevConfig);
	if(NB_FEATURE == feature)
		*value = stDevConfig.nb;
	else if(WIFI_FEATURE == feature)
		*value = stDevConfig.wifi;
	else if(GPS_FEATURE == feature)
		*value = stDevConfig.gps;
	else if(METER_FEATURE == feature)
		*value = stDevConfig.meter;
	else if(SENSOR_FEATURE == feature)
		*value = stDevConfig.sensor;
	else if(RS485_FEATURE == feature)
		*value = stDevConfig.rs485;
	else if(LIGHT_FEATURE == feature)
		*value = stDevConfig.light;
	else
		return OPP_FAILURE;

	return OPP_SUCCESS;
}
