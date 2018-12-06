#ifndef __OPP_APS_LOG_H__
#define __OPP_APS_LOG_H__


/**********************************************************************************************
                                      ABOUT
**********************************************************************************************/
/*
Version 1.0
APS log caching service

*/
/**********************************************************************************************
                                      ENABLE CONFIG
**********************************************************************************************/
#define DEBUG_APSLOG_MULTITASK_SUPPORT_ENABLE 1

/**********************************************************************************************
                                      INCLUDES
**********************************************************************************************/
#if DEBUG_APSLOG_MULTITASK_SUPPORT_ENABLE==1
#include "OS.h"

#endif
#include "Includes.h"


/**********************************************************************************************
                                      RAM CONFIG
**********************************************************************************************/
#define APP_LOG_SIZE_MAX 12
#define APP_LOG_RAM_CACHE_DEPTH 6


/**********************************************************************************************
                                      MULTITASK SUPPORT CONFIG
**********************************************************************************************/
#if DEBUG_APSLOG_MULTITASK_SUPPORT_ENABLE==1
#define APSLOG_T_MUTEX                 T_MUTEX              // xSemaphoreHandle
#define APSLOG_MUTEX_CREATE(mutex)     MUTEX_CREATE(mutex)  // mutex=xSemaphoreCreateMutex()
#define APSLOG_MUTEX_LOCK(mutex)       MUTEX_LOCK((mutex),MUTEX_WAIT_ALWAYS)    // xSemaphoreTake((mutex),3)
#define APSLOG_MUTEX_UNLOCK(mutex)     MUTEX_UNLOCK((mutex))        // xSemaphoreGive((mutex))
#else
#define APSLOG_T_MUTEX                 unsigned int         
#define APSLOG_MUTEX_CREATE(mutex)     
#define APSLOG_MUTEX_LOCK(mutex)    
#define APSLOG_MUTEX_UNLOCK(mutex)   
#endif

/**********************************************************************************************
                                      TYPEDEFS
**********************************************************************************************/
#pragma pack(1)
typedef enum
{
    LOG_LEVEL_FETAL=0,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARN,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
}LOG_LEVEL_EN;

typedef struct
{
	unsigned int  seqnum;
    unsigned char module;
    unsigned char level;
    unsigned char time[6]; // Y-M-D H-M-S(Year=Y+2000)
    unsigned char id;  // log id
    unsigned char len; // length of log(except module and level)
    unsigned char log[APP_LOG_SIZE_MAX];
    unsigned int  saveid;
}APP_LOG_T;
#pragma pack()

typedef struct{
	unsigned int logDescId;
	char log[128];
}ST_LOG_DESC;

typedef int (*GetLogDescIdFunc)(APP_LOG_T *log, ST_LOG_DESC *logDesc);

typedef enum
{
	ALARM_LOGID=0,
	DEV_LOGID=1,
	NBNET_LOGID=2,
	SWITCH_LOGID=3,
	BRI_LOGID=4,
	NBDEV_LOGID=5,
	ELECDEV_LOGID=6,
	WORKPLAN_LOGID=7,
	LOC_LOGID=8,
	TIME_LOGID=9,
	REBOOT_LOGID=10,
	ERROR_LOGID=11,

/*****************************/
	LOG_ID_MAX,
}t_ad_log;



typedef struct{
	t_ad_log logId;
	GetLogDescIdFunc func;
}ST_LOGID_ACTION;

extern void OppApsLogInit(void);
extern void OppApsLogLoop(void);
extern int ApsLogRead(unsigned int saveid,APP_LOG_T* log);
extern int ApsLogWrite(APP_LOG_T* log,unsigned int* saveid);
extern int ApsLogSaveIdLatest(void);
extern int ApsLogAction(APP_LOG_T * log, ST_LOG_DESC * logDesc);
extern int MakeErrLog(unsigned char module, unsigned char errorno);

extern int OppApsRecoderRead(S32 id,U8* data,U32* len);
extern int OppApsRecoderWrite(U8* data,U32 len,U32* id);
extern S32 OppApsRecordIdLatest(void);
extern S32 OppApsRecordIdLast(S32 id_now);




#endif


