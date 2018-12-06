#include <Includes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <DRV_Gps.h>
#include <Opp_ErrorCode.h>
#include <SVS_Location.h>
#include <SVS_Para.h>
#include <SVS_Config.h>
#include <APP_LampCtrl.h>
#include <OPP_Debug.h>
#include <DRV_Bc28.h>
#include <SVS_Exep.h>

static U8 g_ucSrcLoc = GPS_LOC;
static T_MUTEX g_stLocMutex, g_stLastLocMutex;
static T_MUTEX g_stLocCfgMutex, g_stLocCfgCacheMutex, g_stLocSrcMutex;
static U8 m_ucInit = 0;

static ST_OPP_LAMP_LOCATION g_stLoc={
	.ldLatitude = DEFAULT_LATITUDE,
	.ldLongitude = DEFAULT_LONGITUDE
};

static ST_OPP_LAMP_LOCATION g_stLastLoc={
	.ldLatitude = 0.0,
	.ldLongitude = 0.0
};

static ST_OPP_LAMP_LOCATION g_stCfgLoc={
	.ldLatitude = 0.0,
	.ldLongitude = 0.0
};

/*GPS ddmm转换到百度ddmmss坐标*/
int ddmm2dd(const char *ddmm, char *dd)  
{
    int lenSrc = strlen(ddmm)+1;  
    int lenMm = 0;  
    int flag = 1;  
    char *pcMm;  
    double dMm;  
    int iMm;  

    if (NULL == ddmm || NULL == dd)  
    {  
        return -1;  
    }  
  
    memcpy(dd,ddmm,lenSrc);
	
    /* 把pcMm定位到小数点位置 */  
    pcMm = strstr(dd,".");  
  
    if (pcMm == NULL) /* 不含小数点的情况 */  
    {  
        pcMm = dd+strlen(dd)-2;  
        iMm = atoi(pcMm);  
        dMm = iMm /60.0;  
    }  
    else /* 含有小数点的情况 */  
    {     
        /* 有度 */  
        if (pcMm - dd > 2)  
        {  
            pcMm = pcMm - 2;  
        }  
        else /* 没有度,只有分 */  
        {  
            pcMm = dd;  
            flag = 0;  
        }  
        /* 将字符串转换为浮点数 */  
        dMm = atof(pcMm);
        /* 将分转换为度 */  
        dMm /= 60.0;  
    }  
    /* 把转换后的浮点数转换为字符串 */  
    sprintf(pcMm,"%lf",dMm);
    if ( flag )  
    {  
        /* 去掉小数点前面的0 */  
        strcpy(pcMm,pcMm+1);
    }  
    /* 保留小数点后6位 */  
    pcMm = strstr(dd,".");
    lenMm = strlen(pcMm);
    if ( lenMm > (6+2))  
    {  
        memset(pcMm+6+2,0,lenMm-6-2);
    }  
      
    return 1;  
} 

U32 LocSetSrc(U8 ucLocalSrc)
{
	g_ucSrcLoc = ucLocalSrc;
	return OPP_SUCCESS;
}
U8 LocGetSrc(void)
{
	return g_ucSrcLoc;
}

U32 OppLocationFromGps(void)
{
	gps_location_info_t info;
	char dd[10] = {0};
	char ddd[11] = {0};
    char Latitude[11] = {0};                               //纬度  
    char Longitude[12] = {0};                          //经度  
	int ret = OPP_SUCCESS;
	ST_OPP_LAMP_LOCATION stLocation;
#if 1
	ret = GpsLocationInfoGet(&info,100);
	DEBUG_LOG(DEBUG_MODULE_LOC, DLL_INFO, "OppLocationFromGps ret %d\r\n", ret);
	if(0 == ret){
		strncpy((char *)Latitude, (char *)info.Latitude, 11/*strlen(info.Latitude)*/);
		DEBUG_LOG(DEBUG_MODULE_LOC, DLL_INFO, "Latitude: %s\r\n", Latitude);
		ddmm2dd(Latitude, dd);
		stLocation.ldLatitude = atof(dd);
		if(info.NS_Indicator == 'S'){
			stLocation.ldLatitude = 0 - stLocation.ldLatitude;
		}
		strncpy((char *)Longitude, (char *)info.Longitude, 12/*strlen(info.Longitude)*/);
		ddmm2dd(Longitude, ddd);
		stLocation.ldLongitude = atof(ddd);
		if(info.EW_Indicator == 'W'){
			stLocation.ldLongitude = 0 - stLocation.ldLongitude;
		}
		LocSetLat(stLocation.ldLatitude);
		LocSetLng(stLocation.ldLongitude);
		DEBUG_LOG(DEBUG_MODULE_LOC, DLL_INFO, "dd %Lf ddd %Lf\r\n", stLocation.ldLatitude, stLocation.ldLongitude);
		return OPP_SUCCESS;
	}
#else
#ifdef DEBUG
			strcpy(info.Latitude, "3102.2102");
			info.NS_Indicator = 'N';
			strcpy(info.Longitude, "12047.5853");
			info.EW_Indicator = 'E';
			ret = 0;
#else
			ret = GpsLocationInfoGet(&info,100);
#endif
		if(0 == ret){
			strncpy((char *)&Latitude[len], (char *)info.Latitude, strlen(info.Latitude));
			DEBUG_LOG(DEBUG_MODULE_LOC, DLL_INFO, "Latitude: %s\r\n", Latitude);
			ddmm2dd(Latitude, dd);
			stLocation.ldLatitude = atof(dd);
			if(info.NS_Indicator == 'S'){
				stLocation.ldLatitude = 0 - stLocation.ldLatitude;
			}
			strncpy((char *)&Longitude[len], (char *)info.Longitude, strlen(info.Longitude));
			ddmm2dd(Longitude, ddd);
			stLocation.ldLongitude = atof(ddd);
			if(info.EW_Indicator == 'W'){
				stLocation.ldLongitude = 0 - stLocation.ldLongitude;
			}
			LocSetLat(stLocation.ldLatitude);
			LocSetLng(stLocation.ldLongitude);
			DEBUG_LOG(DEBUG_MODULE_LOC, DLL_INFO, "dd %Lf ddd %Lf\r\n", stLocation.ldLatitude, stLocation.ldLongitude);
			return OPP_SUCCESS;
		}
#endif

	return OPP_FAILURE;
}

U32 OppLocationFromNbiot(void)
{
	return OPP_FAILURE;
}

U32 LocGetFromCfg(void)
{
	long double lat;
	long double lng;

	LocGetCfgLat(&lat);
	LocGetCfgLng(&lng);
	if(INVALIDE_LAT == lat && INVALIDE_LNG == lng)
		return OPP_FAILURE;
	LocSetLat(lat);
	LocSetLng(lng);

	return OPP_SUCCESS;
}

U32 LocCfgGetFromFlash(ST_OPP_LAMP_LOCATION *pstLoc)
{
	int ret = OPP_SUCCESS;
	unsigned int len = sizeof(ST_OPP_LAMP_LOCATION);
	ST_OPP_LAMP_LOCATION stLocCfg;

	MUTEX_LOCK(g_stLocCfgCacheMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaRead(APS_PARA_MODULE_APS_LOC, LOC_ID, (unsigned char *)&stLocCfg, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_LOC, DLL_INFO, "OppCfgLocFromFlash fail ret %d\r\n",ret);
		MUTEX_UNLOCK(g_stLocCfgCacheMutex);
		return OPP_FAILURE;
	}
	
	memcpy(pstLoc,&stLocCfg,sizeof(ST_OPP_LAMP_LOCATION));
	MUTEX_UNLOCK(g_stLocCfgCacheMutex);
	return OPP_SUCCESS;
}

U32 LocRestore(void)
{
	int ret;
	ST_OPP_LAMP_LOCATION stLoc;
	ST_LOCSRC stLocSrc;

	LocSetCfgLng(INVALIDE_LNG);
	LocSetCfgLat(INVALIDE_LAT);
	stLoc.ldLatitude = INVALIDE_LAT;
	stLoc.ldLongitude = INVALIDE_LNG;
	ret = LocCfgSetToFlash(&stLoc);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_LOC, DLL_ERROR, "LocRestore restore cfg loc fail ret %d\r\n",ret);
		return OPP_FAILURE;
	}	
	LocSetLng(DEFAULT_LONGITUDE);
	LocSetLat(DEFAULT_LATITUDE);
	
	stLocSrc.locSrc = GPS_LOC;
	LocSetSrcToFlash(&stLocSrc);
	ret = LocSetSrc(GPS_LOC);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_LOC, DLL_ERROR, "LocRestore restore loc src fail ret %d\r\n",ret);
		return OPP_FAILURE;
	}
	return OPP_SUCCESS;
}

U32 LocCfgSetToFlash(ST_OPP_LAMP_LOCATION *pstLoc)
{
	int ret = OPP_SUCCESS;
	
	MUTEX_LOCK(g_stLocCfgMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaWrite(APS_PARA_MODULE_APS_LOC, LOC_ID, (unsigned char *)pstLoc, sizeof(ST_OPP_LAMP_LOCATION));
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_LOC, DLL_INFO, "OppCfgLocToFlash fail ret %d\r\n",ret);
		MUTEX_UNLOCK(g_stLocCfgMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stLocCfgMutex);
	return OPP_SUCCESS;
}

U32 LocSrcGetFromFlash(ST_LOCSRC *pstLocSrc)
{
	int ret = OPP_SUCCESS;
	unsigned int len = sizeof(ST_LOCSRC);
	ST_LOCSRC stLocSrc;

	if(NULL == pstLocSrc){
		DEBUG_LOG(DEBUG_MODULE_LOC, DLL_ERROR, "LocSrcGetFromFlash pstLocSrc is null\r\n");
		return OPP_FAILURE;
	}

	MUTEX_LOCK(g_stLocSrcMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaRead(APS_PARA_MODULE_APS_LOC, LOCSRC_ID, (unsigned char *)&stLocSrc, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_LOC, DLL_ERROR, "LocSrcGetFromFlash fail ret %d\r\n",ret);
		MUTEX_UNLOCK(g_stLocSrcMutex);
		return OPP_FAILURE;
	}
	memcpy(pstLocSrc,&stLocSrc,sizeof(ST_LOCSRC));
	MUTEX_UNLOCK(g_stLocSrcMutex);
	
	return OPP_SUCCESS;
}

U32 LocSetSrcToFlash(ST_LOCSRC *pstLocSrc)
{
	int ret = OPP_SUCCESS;
	
	MUTEX_LOCK(g_stLocSrcMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaWrite(APS_PARA_MODULE_APS_LOC, LOCSRC_ID, (unsigned char *)pstLocSrc, sizeof(ST_LOCSRC));
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_LOC, DLL_INFO, "LocSetSrcToFlash fail ret %d\r\n",ret);
		MUTEX_UNLOCK(g_stLocSrcMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stLocSrcMutex);
	return OPP_SUCCESS;
}

U32 OppLocationGet(void)
{
	U32 ret = OPP_SUCCESS;
	int support;
	
	if(ALL_LOC == g_ucSrcLoc)
	{
		ret = LocGetFromCfg();
		if(OPP_SUCCESS != ret){
			ret = ApsDevFeaturesGet(GPS_FEATURE,&support);
			if(OPP_SUCCESS == ret && support == 1){
				ret = OppLocationFromGps();
				if(OPP_SUCCESS != ret){
					ret = OppLocationFromNbiot();
				}				
			}else{
				ret = OppLocationFromNbiot();
			}
		}
	}
	else if(GPS_LOC == g_ucSrcLoc)
	{
		ret = ApsDevFeaturesGet(GPS_FEATURE,&support);
		if(OPP_SUCCESS == ret && support == 1){
			ret = OppLocationFromGps();
		}
	}
	else if(NBIOT_LOC == g_ucSrcLoc)
	{
		ret = OppLocationFromNbiot();
	}
	else if(CFG_LOC == g_ucSrcLoc)
	{
		ret = LocGetFromCfg();
	}
	
	return ret;
}

int LocationRead(U8 localSrc, ST_OPP_LAMP_LOCATION * pstLocationPara)
{
	U32 ret = OPP_SUCCESS;
	int support;
	
	if(ALL_LOC == localSrc)
	{
		//[code review] add gps feature judgement		
		ret = LocGetFromCfg();
		if(OPP_SUCCESS != ret)
		{
			ret = ApsDevFeaturesGet(GPS_FEATURE,&support);
			if(OPP_SUCCESS == ret && 1 == support){
				ret = OppLocationFromGps();
				if(OPP_SUCCESS != ret){
					ret = OppLocationFromNbiot();
				}
			}else{
				ret = OppLocationFromNbiot();
			}
		}
	}
	else if(GPS_LOC == localSrc)
	{
		ret = ApsDevFeaturesGet(GPS_FEATURE,&support);
		if(OPP_SUCCESS == ret && 1 == support){
			ret = OppLocationFromGps();
		}
	}
	else if(NBIOT_LOC == localSrc)
	{
		ret = OppLocationFromGps();
	}
	else if(CFG_LOC == localSrc)
	{
		ret = LocGetFromCfg();
	}	
	else
	{
		//return OPP_FAILURE;
	}
	MUTEX_LOCK(g_stLocMutex, MUTEX_WAIT_ALWAYS);
	pstLocationPara->ldLatitude = g_stLoc.ldLatitude;
	pstLocationPara->ldLongitude = g_stLoc.ldLongitude;
	MUTEX_UNLOCK(g_stLocMutex);
	return ret;
}

int LocGetCfgLng(long double * lng)
{
	/*MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stLocMutex);*/

	MUTEX_LOCK(g_stLocCfgMutex,MUTEX_WAIT_ALWAYS);
	*lng = g_stCfgLoc.ldLongitude;
	MUTEX_UNLOCK(g_stLocCfgMutex);
	return OPP_SUCCESS;
}
int LocGetCfgLat(long double * lat)
{
	/*MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stLocMutex);*/

	MUTEX_LOCK(g_stLocCfgMutex,MUTEX_WAIT_ALWAYS);
	*lat = g_stCfgLoc.ldLatitude;
	MUTEX_UNLOCK(g_stLocCfgMutex);
	
	return OPP_SUCCESS;
}

int LocSetCfgLng(long double lng)
{
	/*MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stLocMutex);*/

	MUTEX_LOCK(g_stLocCfgMutex, MUTEX_WAIT_ALWAYS);
	g_stCfgLoc.ldLongitude = lng;	
	MUTEX_UNLOCK(g_stLocCfgMutex);
	return OPP_SUCCESS;
}

int LocSetCfgLat(long double lat)
{
	/*MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stLocMutex);*/

	MUTEX_LOCK(g_stLocCfgMutex,MUTEX_WAIT_ALWAYS);
	g_stCfgLoc.ldLatitude = lat;	
	MUTEX_UNLOCK(g_stLocCfgMutex);
	return OPP_SUCCESS;
}

int LocGetLng(long double * lng)
{
	MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}
	*lng = g_stLoc.ldLongitude;
	MUTEX_UNLOCK(g_stLocMutex);
	
	return OPP_SUCCESS;
}

int LocGetLat(long double * lat)
{
	MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}	
	*lat = g_stLoc.ldLatitude;
	MUTEX_UNLOCK(g_stLocMutex);
	
	return OPP_SUCCESS;
}

int LocSetLng(long double lng)
{
	MUTEX_LOCK(g_stLocMutex, MUTEX_WAIT_ALWAYS);
	/*if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}*/
	g_stLoc.ldLongitude = lng;	
	MUTEX_UNLOCK(g_stLocMutex);
	return OPP_SUCCESS;
}

int LocSetLat(long double lat)
{
	MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	/*if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}*/
	g_stLoc.ldLatitude = lat;	
	MUTEX_UNLOCK(g_stLocMutex);
	return OPP_SUCCESS;
}

int LocGetLastLng(long double * lng)
{
	MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stLocMutex);

	MUTEX_LOCK(g_stLastLocMutex,MUTEX_WAIT_ALWAYS);
	*lng = g_stLastLoc.ldLongitude;
	MUTEX_UNLOCK(g_stLastLocMutex);
	return OPP_SUCCESS;
}

int LocGetLastLat(long double * lat)
{
	MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stLocMutex);

	MUTEX_LOCK(g_stLastLocMutex,MUTEX_WAIT_ALWAYS);
	*lat = g_stLastLoc.ldLatitude;
	MUTEX_UNLOCK(g_stLastLocMutex);
	return OPP_SUCCESS;
}

int LocSetLastLng(long double lng)
{
	/*MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stLocMutex);*/

	MUTEX_LOCK(g_stLastLocMutex,MUTEX_WAIT_ALWAYS);
	g_stLastLoc.ldLongitude = lng;
	MUTEX_UNLOCK(g_stLastLocMutex);
	return OPP_SUCCESS;
}

int LocSetLastLat(long double lat)
{
	/*MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	if(!m_ucInit){
		MUTEX_UNLOCK(g_stLocMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stLocMutex);*/

	MUTEX_LOCK(g_stLastLocMutex,MUTEX_WAIT_ALWAYS);
	g_stLastLoc.ldLatitude = lat;
	MUTEX_UNLOCK(g_stLastLocMutex);
	return OPP_SUCCESS;
}

U32 LocChg(U8 targetid,U32 *chg)
{
	long double lat,lng;
	long double lastLat,lastLng;
	static U8 first = 1;
	static U32 timeout = 0;
	static U32 tick = 0;
	RESET_REASON reason_cpu0;
	ST_DISCRETE_SAVE_PARA stDisSavePara;
	
	if(first){
		reason_cpu0 = rtc_get_reset_reason(0);		
		if(isResetReasonPowerOn(reason_cpu0)){
			NeulBc28GetDiscretePara(&stDisSavePara);
			if(stDisSavePara.udwDiscreteWin > DIS_WINDOW_MAX_SECOND)
				stDisSavePara.udwDiscreteWin = DIS_WINDOW_MAX_SECOND;
			timeout = (stDisSavePara.udwDiscreteWin + NeulBc28DisOffsetSecondGet())*1000;
			tick = OppTickCountGet();
		}
		first = 0;
	}
	if(tick > 0){
		if(OppTickCountGet()- tick < timeout){
			*chg = 0;
			return OPP_SUCCESS;
		}else{
			//printf("loc timeout\r\n");
			tick = 0;
		}
	}
		
	LocGetLat(&lat);
	LocGetLng(&lng);
	LocGetLastLat(&lastLat);
	LocGetLastLng(&lastLng);

	if(lat != lastLat || lng != lastLng){
		//printf("LocChg 1\r\n");
		LocSetLastLat(lat);
		LocSetLastLng(lng);
		*chg = 1;
	}else{
		//printf("LocChg 0\r\n");
		*chg = 0;
	}
	
	return OPP_SUCCESS;
}

void LocationInit(void)
{
	ST_OPP_LAMP_LOCATION stLoc;
	ST_LOCSRC stLocSrc;	
	int ret;
	
	MUTEX_CREATE(g_stLocMutex);
	MUTEX_CREATE(g_stLastLocMutex);
	MUTEX_CREATE(g_stLocCfgMutex);
	MUTEX_CREATE(g_stLocCfgCacheMutex);
	MUTEX_CREATE(g_stLocSrcMutex);

	
	ret = LocSrcGetFromFlash(&stLocSrc);
	if(OPP_SUCCESS == ret){
		LocSetSrc(stLocSrc.locSrc);
	}else{
		LocSetSrc(GPS_LOC);
	}

	ret = LocCfgGetFromFlash(&stLoc);
	printf("LocCfgGetFromFlash %Lf %Lf ret %d\r\n", stLoc.ldLatitude,stLoc.ldLongitude, ret);	
	if(OPP_SUCCESS == ret && (stLoc.ldLatitude != INVALIDE_LAT || stLoc.ldLongitude != INVALIDE_LAT)){
		LocSetCfgLat(stLoc.ldLatitude);
		LocSetCfgLng(stLoc.ldLongitude);
		LocSetLat(stLoc.ldLatitude);
		LocSetLng(stLoc.ldLongitude);
	}else{
		LocSetLat(DEFAULT_LATITUDE);
		LocSetLng(DEFAULT_LONGITUDE);
	}

	MUTEX_LOCK(g_stLocMutex,MUTEX_WAIT_ALWAYS);
	m_ucInit = 1;
	MUTEX_UNLOCK(g_stLocMutex);

}
void LocationLoop(void)
{
	static U8 rLoc = 0;
	int ret;
	static U32 tick = 0;
	
	if(0 == rLoc){
		if(0 == tick){
			ret = OppLocationGet();
			if(OPP_SUCCESS == ret){
				rLoc = 1;
			}else{
				tick = OppTickCountGet();
			}
		}else if(OppTickCountGet() - tick > LOC_LOOP_PERIOD){
			tick = 0;
		}
	}
}

