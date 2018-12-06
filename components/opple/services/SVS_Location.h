/******************************************************************************
* Version     : OPP_LOCATION V100R001C01B001D001                              *
* File        : Opp_Mutex.h                                              *
* Description :                                                               *
*               OPPLE产品小类定义头文件                                       *
* Note        : (none)                                                        *
* Author      : 智能控制研发部                                                *
* Date:       : 2013-08-09                                                    *
* Mod History : (none)                                                        *
******************************************************************************/
#ifndef __SVS_LOCATION_H__
#define __SVS_LOCATION_H__


#ifdef __cplusplus
extern "C" {
#endif


#include <Typedef.h>
//#include <APP_LampCtrl.h>

#define ALL_LOC    	 0
#define GPS_LOC    	 1
#define NBIOT_LOC    2
#define CFG_LOC    	 3
#define UNKNOW_LOC   4

#define DEFAULT_LATITUDE    (31.040786)
#define DEFAULT_LONGITUDE   (120.80235)
#define LOC_LOOP_PERIOD     (60000)/*(180000)*/
#define INVALIDE_LAT        (0.0)
#define INVALIDE_LNG        (0.0)


#define LOC_ID    0
#define LOCSRC_ID    1

#pragma pack(1)
typedef struct
{
	long double ldLatitude;  //维度
	long double ldLongitude;  //经度
}ST_OPP_LAMP_LOCATION;

typedef struct
{
	U8 locSrc;
}ST_LOCSRC;

#pragma pack()

U32 LocSetSrc(U8 ucLocalSrc);
U8 LocGetSrc(void);
U32 OppLocationFromGps(void);
U32 OppLocationFromNbiot(void);
U32 OppLocationGet();
int LocationRead(U8 localSrc, ST_OPP_LAMP_LOCATION * pstLocationPara);
void LocationLoop(void);
void LocationInit(void);
int LocGetLng(long double * lng);
int LocGetLat(long double * lat);
int LocSetLng(long double lng);
int LocSetLat(long double lat);
U32 LocChg(U8 targetid,U32 *chg);
int LocGetLastLng(long double * lng);
int LocGetLastLat(long double * lat);
int LocGetCfgLng(long double * lng);
int LocGetCfgLat(long double * lat);
int LocSetCfgLng(long double lng);
int LocSetCfgLat(long double lat);
U32 LocSrcGetFromFlash(ST_LOCSRC *pstLocSrc);
U32 LocSetSrcToFlash(ST_LOCSRC *pstLocSrc);
U32 LocCfgSetToFlash(ST_OPP_LAMP_LOCATION *pstLoc);
U32 LocCfgGetFromFlash(ST_OPP_LAMP_LOCATION *pstLoc);
U32 LocRestore(void);

	
#ifdef __cplusplus
}
#endif

#endif

