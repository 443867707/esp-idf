#include <Includes.h>
#include <DRV_Gps.h>
#include <DRV_SpiMeter.h>
#include <DRV_DimCtrl.h>
#include <DRV_Bc28.h>
#include <Opp_Module.h>
#include <APP_LampCtrl.h>
#include <SVS_Time.h>
#include <APP_Main.h>
#include <DEV_Utility.h>
#include <SVS_Para.h>
#include <SVS_Coap.h>
#include <SVS_Lamp.h>
#include <SVS_Elec.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <OPP_Debug.h>
#include "DEV_LogMmt.h"
#include "SVS_Log.h"
#include "opple_main.h"
#include <SVS_Config.h>

#define  MAX_ENERGY_REG_VALUE_PER_30S 80

T_MUTEX g_stElecMutex;
U8 m_ucElecInit = 0;
U16 g_u16LastEnergyReg = 0;

TimerHandle_t  ElecTimer;
ST_OPP_LAMP_CURR_ELECTRIC_INFO g_stCurrElecric={
	.current = 0,
	.voltage = 0,
	.power = 0,
	.consumption = 0,
	.hisConsumption = 0,	
	.delta = 0,
	.factor = 0
};
ST_ELEC_CONFIG stElecConfig;


int ElecInit(void)
{
	MUTEX_CREATE(g_stElecMutex);
	return OPP_SUCCESS;
}

void RuntimeLoop(void)
{
	static U32 tick_start, tick_start1;
	static U8 init = 0;
	//U32 rTime = 0,hTime = 0;	
	ST_ELEC_CONFIG stRuntime;
	int ret;
	
	if(!init){
		tick_start1 = tick_start = OppTickCountGet();
		init = 1; // add by Liqi
	}
	
	//一分钟加一
	if ((OppTickCountGet() - tick_start) >= RUNTIME_SCAN_TO)
	{
		OppLampCtrlHtimeAdd(1);
		OppLampCtrlRtimeAdd(1);
		tick_start = OppTickCountGet();
	}
	//10分钟写一次flash
	if ((OppTickCountGet() - tick_start1) >= RUNTIME_SAVE_TO)
	{
		stRuntime.hisConsumption = ElecHisConsumptionGetInner();		
		OppLampCtrlGetHtime(0, &stRuntime.hisTime);
		ret = ElecWriteFlash(&stRuntime);
		if(OPP_SUCCESS == ret){
			DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "ElecWriteFlash err %d\r\n", ret);
		}
		tick_start1 = OppTickCountGet();
	}
	

}

/*返回值表示增量值*/
uint16_t ElecCalcIncEnergy(uint16_t energy)
{
	uint16_t u16EnergyReg = 0;
	uint16_t u16IncEnergy = 0;

	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	u16EnergyReg = energy;
	if (u16EnergyReg >= g_u16LastEnergyReg) {
		u16IncEnergy = u16EnergyReg - g_u16LastEnergyReg;
	} else {
		u16IncEnergy = MAX_ENERGY_REG_VALUE - g_u16LastEnergyReg + u16EnergyReg;
	}

    g_u16LastEnergyReg = u16EnergyReg;

    if (u16IncEnergy > MAX_ENERGY_REG_VALUE_PER_30S) {
        u16IncEnergy = 0;
    }

	MUTEX_UNLOCK(g_stElecMutex);

	return u16IncEnergy;
}

/*get elec info from driver immediately*/
int ElecGetElectricInfoImm(ST_OPP_LAMP_CURR_ELECTRIC_INFO * pElecInfo)
{
	int ret;
	meter_info_t meter_info;
	//ST_ELEC_CONFIG stElecInfo;
	uint16_t u16IncEnergy = 0;

	if(NULL == pElecInfo){
		return OPP_NULL_POINTER;
	}
	
	ret = MeterInfoGet(&meter_info);
	if(OPP_SUCCESS != ret) {
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "MeterInfoGet fail ret %d\r\n", ret);
		return ret;
	}
	//数据不一致
	pElecInfo->current = meter_info.u32Current;//(mA)
	pElecInfo->voltage = meter_info.u32Voltage; //(V)
	pElecInfo->power = meter_info.u32Power;	
	pElecInfo->factor= meter_info.u32PowerFactor;//单位千分之一
	u16IncEnergy = ElecCalcIncEnergy(meter_info.u16Energy);
	pElecInfo->consumption = u16IncEnergy;
	pElecInfo->delta = u16IncEnergy;
	ElecSetElectricInfo(pElecInfo);
	//wangtao TODO
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "W = %u, P = %u, U = %u, I = %u\r\n", meter_info.u16Energy, meter_info.u32Power, meter_info.u32Voltage, meter_info.u32Current);
	
	return OPP_SUCCESS;
}


/*get elec info from driver immediately*/
int ElecGetElectricInfoCustom(ST_OPP_LAMP_CURR_ELECTRIC_INFO * pElecInfo)
{
	int ret;
	meter_info_t meter_info;

	if(NULL == pElecInfo){
		return OPP_NULL_POINTER;
	}
	
	ret = MeterInstantInfoGet(&meter_info);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "ElecGetElectricInfoCustom call MeterInstantInfoGet fail ret %d\r\n", ret);
		return ret;
	}
	pElecInfo->current = meter_info.u32Current;//(mA)
	pElecInfo->voltage = meter_info.u32Voltage; //(V)
	pElecInfo->power = meter_info.u32Power;	
	pElecInfo->factor= meter_info.u32PowerFactor;//单位千分之一
	ElecSetElectricInfoCustom(pElecInfo);
	//wangtao TODO
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "P = %u, U = %u, I = %u F = %u\r\n", meter_info.u32Power, meter_info.u32Voltage, meter_info.u32Current, meter_info.u32PowerFactor);

	return OPP_SUCCESS;
}

//[code review module not init return error !!!]
void ElecLoop(void)
{
	static U8 init = 0, init1 = 0;
	static U32 tick_start = 0, tick1_start = 0;
	int ret;
	ST_OPP_LAMP_CURR_ELECTRIC_INFO elecInfo;
	ST_ELEC_CONFIG stElecInfo;

	if(!init){
		tick_start = OppTickCountGet();
		tick1_start = OppTickCountGet();
		init = 1;
	}

	if (!init1 ||(OppTickCountGet() - tick_start >= ELEC_SCAN_TO)/*10000*//*600000*/)
	{
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "[tick:%d]ElecThread run here init1 %d\r\n", OppTickCountGet(), init1);
		ret = ElecGetElectricInfoImm(&elecInfo);
		if(OPP_SUCCESS != ret){
			DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "ElecLoop ElecGetElectricInfoImm fail ret %d\r\n", ret);
		}
		tick_start = OppTickCountGet();
		if(!init1){
			init1 = 1;
			MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);			
			m_ucElecInit = 1;
			MUTEX_UNLOCK(g_stElecMutex);
		}
	}
	
	if(OppTickCountGet() - tick1_start >= ELEC_SAVE_TO)
	{
		stElecInfo.hisConsumption = ElecHisConsumptionGetInner();
		OppLampCtrlGetHtime(0,&stElecInfo.hisTime);
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "[tick:%d]write hisCons %u his hisTime %u to flash\r\n", OppTickCountGet(), stElecInfo.hisConsumption, stElecInfo.hisTime);
		ret = ElecWriteFlash(&stElecInfo);
		if(OPP_SUCCESS != ret){
			DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "ElecWriteFlash fail ret %d\r\n", ret);
		}
		tick1_start = OppTickCountGet();
	}
}

int ElecDoReboot(void)
{
	int ret;
	ST_ELEC_CONFIG stElecInfo;
	ST_OPP_LAMP_CURR_ELECTRIC_INFO elecInfo;

	ret = ElecGetElectricInfoImm(&elecInfo);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "ElecDoReboot ElecGetElectricInfoImm fail ret %d\r\n", ret);
	}
	stElecInfo.hisConsumption = ElecHisConsumptionGetInner();
	OppLampCtrlGetHtime(0,&stElecInfo.hisTime);
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "[tick:%d]write hisCons %u his hisTime %u to flash\r\n", OppTickCountGet(), stElecInfo.hisConsumption, stElecInfo.hisTime);
	ret = ElecWriteFlash(&stElecInfo);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "ElecWriteFlash fail ret %d\r\n", ret);
	}
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "ElecDoReboot do reboot succ\r\n");
	return 0;
}

void ElecPrintInfoDelay(int timeout)
{
	static U32 tick = 0;

	if(0 == tick){
		tick = OppTickCountGet();
	}else if(OppTickCountGet() - tick > timeout){
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "ElecThread run..........\r\n");
		tick = 0;
	}
}

void ElecThread(void *pvParameter)
{
	int ret;
	ST_ELEC_CONFIG stElecInfo;
	int support;
	//ElecInit();

	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "ElecThread init\r\n");
	ret = ElecReadFlash(&stElecInfo);
	if(OPP_SUCCESS == ret){
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "elec read hisConsumption %d\r\n",stElecInfo.hisConsumption);
		ElecHisConsumptionSetInner(stElecInfo.hisConsumption);
		OppLampCtrlSetHtime(0,stElecInfo.hisTime);
	}else{
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "ElecReadFlash fail ret %d\r\n", ret);
		ElecHisConsumptionSetInner(0);
		OppLampCtrlSetHtime(0,0);
	}
	ElecConsumptionSetInner(0);
	OppLampCtrlSetRtime(0,0);

	while(1){
		//ApsDevFeaturesGet(METER_FEATURE,&support);
		//if(support){
			ElecLoop();
		//}
		LocationLoop();
		RuntimeLoop();
		ElecPrintInfoDelay(60000);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        taskWdtReset();
	}
	
}

int ElecWriteFlash(ST_ELEC_CONFIG * pstElecInfo)
{
	int ret = OPP_SUCCESS;
	U32 id;

	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "ElecWriteFlash\r\n");
	//ret = AppParaWrite(APS_PARA_MODULE_APS_METER, ELEC_ID, (unsigned char *)pstElecInfo, sizeof(ST_ELEC_CONFIG));
	//ret = LogMmtWrite(LOG_MMT_PARTITION_METER,(unsigned char *)pstElecInfo, sizeof(ST_ELEC_CONFIG),&id);
	ret = OppApsRecoderWrite((unsigned char *)pstElecInfo, sizeof(ST_ELEC_CONFIG),&id);
	if(OPP_SUCCESS != ret){
		return OPP_FAILURE;
	}
	
	return OPP_SUCCESS;
}

static inline int ElecParaLoad(ST_ELEC_CONFIG* stElecInfo)
{
	S32 id_latest,id_last,id=0,cnt=0;
	int res;
	unsigned int len = sizeof(ST_ELEC_CONFIG);

	id_latest= OppApsRecordIdLatest();
	res = OppApsRecoderRead(id_latest,(unsigned char *)stElecInfo, &len);
	if(res == 0) return 0;

	id = id_latest;
	while(1)
	{
		id_last = OppApsRecordIdLast(id);
		res = OppApsRecoderRead(id_last,(unsigned char *)stElecInfo, &len);
		if(res == 0){
			return 0;
		}else{
			id=id_last;
			cnt++;
			if(cnt >= 100) return 1;
			else continue;
		}
	}
}

int ElecReadFlash(ST_ELEC_CONFIG * pstElecInfo)
{
	int ret = OPP_SUCCESS;
	//unsigned int len = sizeof(ST_ELEC_CONFIG);
	ST_ELEC_CONFIG stElecInfo;
	//S32 id;
	
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "ElecReadFlash\r\n");
	//ret = AppParaRead(APS_PARA_MODULE_APS_METER, ELEC_ID, (unsigned char *)&stElecInfo, &len);
	//ret = LogMmtReadLatestItem(LOG_MMT_PARTITION_METER,&id,(unsigned char *)&stElecInfo, &len);
	ret = ElecParaLoad(&stElecInfo);
	//ret = 1;
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "ElecReadFlash fail ret %d\r\n",ret);
		return OPP_FAILURE;
	}
	
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "ElecReadFlash succ\r\n");
	memcpy(pstElecInfo,&stElecInfo,sizeof(ST_ELEC_CONFIG));
	return OPP_SUCCESS;
}
int ElecSetElectricInfo(ST_OPP_LAMP_CURR_ELECTRIC_INFO *pstElecInfo)
{
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	g_stCurrElecric.current = pstElecInfo->current;
	g_stCurrElecric.voltage = pstElecInfo->voltage;
	g_stCurrElecric.power = pstElecInfo->power;
	g_stCurrElecric.hisConsumption += pstElecInfo->consumption;
	if(g_stCurrElecric.hisConsumption>MAX_INNER_EC)
		g_stCurrElecric.hisConsumption = g_stCurrElecric.hisConsumption - MAX_INNER_EC - 1;
	g_stCurrElecric.consumption += pstElecInfo->consumption;
	if(g_stCurrElecric.consumption>MAX_INNER_EC)
		g_stCurrElecric.consumption = g_stCurrElecric.consumption - MAX_INNER_EC - 1;
	g_stCurrElecric.delta = pstElecInfo->delta;
	g_stCurrElecric.factor = pstElecInfo->factor;
	MUTEX_UNLOCK(g_stElecMutex);	
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "current = %u, voltage = %u, power = %u, consumption = %u hisConsumption = %u delta = %u factor = %u\r\n", 
		g_stCurrElecric.current, g_stCurrElecric.voltage, g_stCurrElecric.power, g_stCurrElecric.consumption, g_stCurrElecric.hisConsumption, g_stCurrElecric.delta, g_stCurrElecric.factor);
	return OPP_SUCCESS;
}

int ElecSetElectricInfoCustom(ST_OPP_LAMP_CURR_ELECTRIC_INFO *pstElecInfo)
{
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	g_stCurrElecric.current = pstElecInfo->current;
	g_stCurrElecric.voltage = pstElecInfo->voltage;
	g_stCurrElecric.power = pstElecInfo->power;
	g_stCurrElecric.factor = pstElecInfo->factor;
	MUTEX_UNLOCK(g_stElecMutex);	
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "current = %u, voltage = %u, power = %u, factor = %u\r\n", 
		g_stCurrElecric.current, g_stCurrElecric.voltage, g_stCurrElecric.power, g_stCurrElecric.factor);
	return OPP_SUCCESS;
}

int ElecGetElectricInfoNow(ST_OPP_LAMP_CURR_ELECTRIC_INFO *pstElecInfo)
{
	int ret;
	
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	//[code review power adjust !!!]	
	if(!m_ucElecInit){
		MUTEX_UNLOCK(g_stElecMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stElecMutex);
	
	ret = ElecGetElectricInfoCustom(pstElecInfo);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "ElecGetElectricInfoImm fail ret %d\r\n", ret);
		return ret;
	}
	
    //pstElecInfo->power = pstElecInfo->power / 10000;	
    //pstElecInfo->consumption = ElecConsumptionGetInner()  /12;
	//pstElecInfo->hisConsumption = ElecHisConsumptionGetInner()/12;

	return OPP_SUCCESS;
}

int ElecGetElectricInfo(ST_OPP_LAMP_CURR_ELECTRIC_INFO *pstElecInfo)
{
	//int ret;
	
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	//[code review power adjust !!!]	
	if(!m_ucElecInit){
		MUTEX_UNLOCK(g_stElecMutex);
		return OPP_FAILURE;
	}	
	memcpy(pstElecInfo, &g_stCurrElecric, sizeof(ST_OPP_LAMP_CURR_ELECTRIC_INFO));
	MUTEX_UNLOCK(g_stElecMutex);

	pstElecInfo->voltage = pstElecInfo->voltage/10;
    pstElecInfo->power = pstElecInfo->power / 10000;	
    pstElecInfo->consumption = ElecConsumptionGetInner()  /12;
	pstElecInfo->hisConsumption = ElecHisConsumptionGetInner()/12;

	return OPP_SUCCESS;
}

int ElecVoltageGet(int target, unsigned int * voltage)
{
	//int ret;
	//ST_OPP_LAMP_CURR_ELECTRIC_INFO elecInfo;

	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	if(!m_ucElecInit){
		MUTEX_UNLOCK(g_stElecMutex);
		return OPP_FAILURE;
	}
	/*MUTEX_UNLOCK(g_stElecMutex);
	
	ret = ElecGetElectricInfoImm(&elecInfo);
	if(OPP_SUCCESS != ret){
		return OPP_FAILURE;
	}
	*voltage = elecInfo.voltage;*/
	//精确小数点	
	*voltage = g_stCurrElecric.voltage/10;
	MUTEX_UNLOCK(g_stElecMutex);
	return OPP_SUCCESS;
}

int ElecVoltageSet(int target, int voltage)
{
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	if(!m_ucElecInit){
		MUTEX_UNLOCK(g_stElecMutex);
		return OPP_FAILURE;
	}	
	//精确小数点
	g_stCurrElecric.voltage = voltage;
	MUTEX_UNLOCK(g_stElecMutex);
	
	return OPP_SUCCESS;
}

int ElecCurrentGet(int target, unsigned int * current)
{
	//int ret;
	//ST_OPP_LAMP_CURR_ELECTRIC_INFO elecInfo;

	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	if(!m_ucElecInit){
		MUTEX_UNLOCK(g_stElecMutex);
		return OPP_FAILURE;
	}
	/*MUTEX_UNLOCK(g_stElecMutex);
	
	ret = ElecGetElectricInfoImm(&elecInfo);
	if(OPP_SUCCESS != ret){
		return OPP_FAILURE;
	}
	*current = elecInfo.current;*/
	
	//精确小数点
	*current =  g_stCurrElecric.current; //mA
	MUTEX_UNLOCK(g_stElecMutex);
	
	return OPP_SUCCESS;
}
int ElecCurrentSet(int target, int current)
{
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	if(!m_ucElecInit){
		MUTEX_UNLOCK(g_stElecMutex);
		return OPP_FAILURE;
	}	
	//精确小数点
	g_stCurrElecric.current = current; //mA
	
	MUTEX_UNLOCK(g_stElecMutex);
	
	return OPP_SUCCESS;
}

int ElecConsumptionSet(unsigned int consumption)
{
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	if(!m_ucElecInit){
		MUTEX_UNLOCK(g_stElecMutex);
		return OPP_FAILURE;
	}	
	g_stCurrElecric.consumption = consumption*12;
	MUTEX_UNLOCK(g_stElecMutex);
	
	return OPP_SUCCESS;
}

int ElecConsumptionSetInner(U32 csp)
{
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	g_stCurrElecric.consumption = csp;
	MUTEX_UNLOCK(g_stElecMutex);
	
	return csp;
}

U32 ElecHisConsumptionGetInner(void)
{
	U32 csp;
	
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	csp = g_stCurrElecric.hisConsumption;
	MUTEX_UNLOCK(g_stElecMutex);
	
	return csp;
}

U32 ElecConsumptionGetInner(void)
{
	U32 csp;
	
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	csp = g_stCurrElecric.consumption;
	MUTEX_UNLOCK(g_stElecMutex);
	
	return csp;
}

int ElecHisConsumptionSetInner(U32 csp)
{
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);
	g_stCurrElecric.hisConsumption = csp;
	if(g_stCurrElecric.hisConsumption>MAX_INNER_EC){
		g_stCurrElecric.hisConsumption = g_stCurrElecric.hisConsumption - MAX_INNER_EC - 1;
	}
	MUTEX_UNLOCK(g_stElecMutex);
	
	return csp;
}

int ElecHisConsumptionSet(unsigned int hisConsumption)
{
	MUTEX_LOCK(g_stElecMutex, MUTEX_WAIT_ALWAYS);	
	if(!m_ucElecInit){
		MUTEX_UNLOCK(g_stElecMutex);
		return OPP_FAILURE;
	}
	g_stCurrElecric.hisConsumption = hisConsumption*12;
	if(g_stCurrElecric.hisConsumption > MAX_INNER_EC)
		g_stCurrElecric.hisConsumption = g_stCurrElecric.hisConsumption -  MAX_INNER_EC - 1;
	MUTEX_UNLOCK(g_stElecMutex);
	
	return OPP_SUCCESS;

}

