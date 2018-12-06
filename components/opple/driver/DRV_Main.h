#ifndef __DRV_MAIN_H__
#define __DRV_MAIN_H__

#include <DRV_DimCtrl.h>
#include <DRV_SpiMeter.h>
#include <DRV_Gps.h>
#include <DRV_Bc28.h>
#include <DRV_Led.h>
#include "DRV_LightSensor.h"

#define DRV_MAIN_INIT() do{\
	int ret; \
	System_Led_Init(); \
	ret = NB_Status_Led_Init(); \
	if(0 != ret){ \
		MakeErrLog(DEBUG_MODULE_MAIN,OPP_NBLED_INIT_ERR); \
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_ERROR, "NB_Status_Led_Init err ret %d\r\n", ret); \
	} \
	System_Led_On(); \
	ret = DimCtrlInit(); \
	if(0 != ret){ \
		MakeErrLog(DEBUG_MODULE_MAIN,OPP_DIMCTRL_INIT_ERR); \
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_ERROR, "DimCtrlInit err ret %d\r\n", ret); \
	} \
	ret = MeterInit(); \
	if(0 != ret){ \
		MakeErrLog(DEBUG_MODULE_MAIN,OPP_METER_INIT_ERR); \
		ApsDevFeaturesSet(METER_FEATURE,0); \
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_ERROR, "MeterInit err ret %d\r\n", ret); \
	}else{ \
		ApsDevFeaturesSet(METER_FEATURE,1); \
	} \
	ret = GpsUartInit(); \
	if(0 != ret){ \
		MakeErrLog(DEBUG_MODULE_MAIN,OPP_GPS_INIT_ERR); \
		ApsDevFeaturesSet(GPS_FEATURE,0); \
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_ERROR, "Thers is no gps module\r\n"); \
	}else{ \
		ApsDevFeaturesSet(GPS_FEATURE,1); \
	} \
	ret = NeulBc28Init(); \
	if(0 != ret){ \
		MakeErrLog(DEBUG_MODULE_MAIN,OPP_BC28_INIT_ERR); \
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_ERROR, "NeulBc28Init err ret %d\r\n", ret); \		
	} \
	ret = BspLightSensorInit();\
	if(0 != ret){ \
		MakeErrLog(DEBUG_MODULE_MAIN,OPP_LIGHT_SENSOR_INIT_ERR); \
		DEBUG_LOG(DEBUG_MODULE_MAIN, DLL_ERROR, "BspLightSensorInit err ret %d\r\n", ret); \		
	} \	
	/*EmuInitial();*/ \
}while(0)

#define DRV_MAIN_LOOP() \
	/*NeulBc28Loop();*/ \
	LedLoop(); \

/*
#define DRV_MAIN_LOOP() \
	LedLoop(); \
*/





#endif


