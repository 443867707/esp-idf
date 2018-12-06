
#include "Includes.h"
#include "Opp_ErrorCode.h"
#include "OPP_Debug.h"
#include "SVS_Para.h"
#include "SVS_Exep.h"
#include "rom/rtc.h"
#include "nvs.h"
#include "DRV_Bc28.h"
#include "esp_partition.h"

/*
inline U32 isResetReasonPowerOn(RESET_REASON reason)
{
	if((reason == RTCWDT_RTC_RESET)
	|| (reason == POWERON_RESET) )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}*/

extern esp_err_t print_panic_saved(int core_id, char *outBuf);
extern esp_err_t print_panic_occur_saved(int8_t *occur);
extern esp_err_t save_panic_occur(int8_t occur);

static char core0Panic[1024] = {0};
static char core1Panic[1024] = {0};

U32 ApsExepInit()
{
	int ret;
	U32 normalRst;
	ST_EXEP_INFO stExepInfo;
	RESET_REASON reason_cpu0;
	unsigned int len;
	int8_t occur;
	
	ret = print_panic_occur_saved(&occur);
	if(OPP_SUCCESS == ret && occur){
		printf("******* backtrace**************\r\n");		
		ret = print_panic_saved(0,core0Panic);
		if(OPP_SUCCESS == ret)
			printf("%s\r\n",core0Panic);
		ret = print_panic_saved(1,core1Panic);
		if(OPP_SUCCESS == ret)
			printf("%s\r\n",core1Panic);
		save_panic_occur(0);		
	}
	
	reason_cpu0 = rtc_get_reset_reason(0);
	//if(RTCWDT_RTC_RESET != rtc_get_reset_reason(0)){
	if(isResetReasonPowerOn(reason_cpu0) == 0){
		NeulBc28SetDisEnableWithoutMutex(DIS_DISABLE);
	}
	ret = ApsExepReadNormalReboot(&normalRst);
	printf("ApsExepInit normalRst %x ret %d\r\n",normalRst,ret);
	if((OPP_SUCCESS != ret && ESP_ERR_NVS_NOT_FOUND == ret) || (OPP_SUCCESS == ret && ABNORMAL_REBOOT == normalRst)){		
		stExepInfo.rst0Type = rtc_get_reset_reason(0);
		stExepInfo.rst1Type = rtc_get_reset_reason(1);
		//if(RTCWDT_RTC_RESET == stExepInfo.rst0Type){
		if(isResetReasonPowerOn(reason_cpu0) == 1){
			stExepInfo.oppleRstType = POWER_RST;
		}else{
			stExepInfo.oppleRstType = UNKNOW_RST;
		}
		stExepInfo.tick = OppTickCountGet();
		ApsExepWriteExep(&stExepInfo);
	}
	ApsExepWriteNormalReboot(ABNORMAL_REBOOT);
	
	return OPP_SUCCESS;
}

U32 ApsExepGetPanic(int core_id, char **outBuf)
{
	if(0 == core_id){
		*outBuf = core0Panic;
	}else{
		*outBuf = core1Panic;
	}
	
	return OPP_SUCCESS;
}

U32 ApsExepWriteCurExep(ST_EXEP_INFO *pstExepInfo)
{
	int ret;
	
	ret = AppParaWrite(APS_PARA_MODULE_APS_EXEP, CUR_EXEP_ID, (unsigned char *)pstExepInfo, sizeof(ST_EXEP_INFO));
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "ApsExepWriteCurExep fail ret %d\r\n",ret);
		return ret;
	}
	return OPP_SUCCESS;
}

U32 ApsExepReadCurExep(ST_EXEP_INFO *pstExepInfo)
{
	int ret;
	unsigned int len = sizeof(ST_EXEP_INFO);
	
	ret = AppParaRead(APS_PARA_MODULE_APS_EXEP, CUR_EXEP_ID, (unsigned char *)pstExepInfo, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "ApsExepReadCurExep fail ret %d\r\n",ret);
		return ret;
	}
	return OPP_SUCCESS;
}

U32 ApsExepWritePreExep(ST_EXEP_INFO *pstExepInfo)
{
	unsigned int ret;
	
	ret = AppParaWrite(APS_PARA_MODULE_APS_EXEP, PRE_EXEP_ID, (unsigned char *)pstExepInfo, sizeof(ST_EXEP_INFO));
	if(OPP_SUCCESS != ret){
		return ret;
	}
	
	return OPP_SUCCESS;
}

U32 ApsExepReadPreExep(ST_EXEP_INFO *pstExepInfo)
{
	int ret;
	unsigned int len = sizeof(ST_EXEP_INFO);
	
	ret = AppParaRead(APS_PARA_MODULE_APS_EXEP, PRE_EXEP_ID, (unsigned char *)pstExepInfo, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "ApsExepReadPreExep fail ret %d\r\n",ret);
		return ret;
	}
	return OPP_SUCCESS;
}

U32 ApsExepWriteExep(ST_EXEP_INFO *pstExepInfo)
{
	ST_EXEP_INFO stExepInfo,stPreExepInfo;
	unsigned int ret;
	
	ret = ApsExepReadCurExep(&stExepInfo);
	if(OPP_SUCCESS != ret){
		if(ESP_ERR_NVS_NOT_FOUND == ret){
			ret = ApsExepWriteCurExep(pstExepInfo);
			if(OPP_SUCCESS != ret){
				DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "ApsExepWriteExep call ApsExepWriteCurExep fail ret %d\r\n",ret);
			}
		}
		return ret;
	}

	ret = ApsExepReadPreExep(&stPreExepInfo);
	if(OPP_SUCCESS == ret){
		/*now cur and pre oppleRstType is same no need to save flash*/
		if(pstExepInfo->oppleRstType == stExepInfo.oppleRstType && stExepInfo.oppleRstType == stPreExepInfo.oppleRstType){
			DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "ApsExepWriteExep now cur and pre oppleRstType is %d\r\n", pstExepInfo->oppleRstType);
			return OPP_SUCCESS;
		}
	}
	ret = ApsExepWritePreExep(&stExepInfo);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "ApsExepWriteExep call ApsExepWritePreExep fail ret %d\r\n",ret);
		return ret;
	}
	ret = ApsExepWriteCurExep(pstExepInfo);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "ApsExepWriteExep call ApsExepWriteCurExep fail ret %d\r\n",ret);
		return ret;
	}
	
	return OPP_SUCCESS;
}


U32 ApsExepWriteNormalReboot(U32 valide)
{
	int ret;

	ret = AppParaWriteU32(APS_PARA_MODULE_APS_EXEP, NORMAL_REBOOT_ID, valide);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "ApsExepWriteNormalReboot fail ret %d\r\n",ret);
		return ret;
	}
	return OPP_SUCCESS;
}

U32 ApsExepReadNormalReboot(U32 * pvalide)
{
	int ret;

	ret = AppParaReadU32(APS_PARA_MODULE_APS_EXEP, NORMAL_REBOOT_ID, pvalide);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "ApsExepReadNormalReboot fail ret %d\r\n",ret);
		return ret;
	}
	return OPP_SUCCESS;
}

U32 ApsExepRestore()
{
	int ret;
	
	ST_EXEP_INFO stExepInfo;

	memset(&stExepInfo, 0, sizeof(ST_EXEP_INFO));
	ret = ApsExepWriteCurExep(&stExepInfo);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "restore cur exep fail ret %d\r\n",ret);
		return OPP_FAILURE;
	}
	ret = ApsExepWritePreExep(&stExepInfo);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "restore pre exep fail ret %d\r\n",ret);
		return OPP_FAILURE;
	}

	ret = ApsExepWriteNormalReboot(ABNORMAL_REBOOT);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_EXEP, DLL_ERROR, "restore exep normal reboot type fail ret %d\r\n",ret);
		return OPP_FAILURE;
	}
	return OPP_SUCCESS;
}

