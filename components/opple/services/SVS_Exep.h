#ifndef _SVS_EXEP_H
#define _SVS_EXEP_H

#include "rom/rtc.h"

#pragma pack(1)

#define CUR_EXEP_ID    0
#define PRE_EXEP_ID    1
#define NORMAL_REBOOT_ID    2

#define NORMAL_REBOOT  0xAABB1122
#define ABNORMAL_REBOOT 0xCCDD3344

typedef enum{
	COAP_RST = 0,
	OTA_RST,
	CMD_RST,
	POWER_RST,
	UNKNOW_RST
};

typedef struct{
	U8 oppleRstType;
	U8 rst0Type;
	U8 rst1Type;
	U32 tick;
	U8 data[512];
}ST_EXEP_INFO;

#pragma pack()

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
}

U32 ApsExepInit();
U32 ApsExepGetPanic(int core_id, char **outBuf);
U32 ApsExepWriteCurExep(ST_EXEP_INFO *pstExepInfo);
U32 ApsExepReadCurExep(ST_EXEP_INFO *pstExepInfo);
U32 ApsExepWritePreExep(ST_EXEP_INFO *pstExepInfo);
U32 ApsExepReadPreExep(ST_EXEP_INFO *pstExepInfo);
U32 ApsExepWriteExep(ST_EXEP_INFO *pstExepInfo);
U32 ApsExepWriteNormalReboot(U32 valide);
U32 ApsExepReadNormalReboot(U32 * pvalide);
U32 ApsExepRestore();

#endif
