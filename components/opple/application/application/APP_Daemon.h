#ifndef __OPP_APP_DAEMON_H__
#define __OPP_APP_DAEMON_H__

/*****************************************************************
                              Includes
******************************************************************/
#include "SVS_Log.h"
#include "DRV_Bc28.h"
#include "SVS_Elec.h"
#include "SVS_Location.h"
#include "APP_LampCtrl.h"
#include "Includes.h"

/*****************************************************************
                              Defines
******************************************************************/
#define MODULE_STATUS_ITEM_MAX  20
#define MODULE_STATUS_DESP_LEN_MAX  APP_LOG_SIZE_MAX

#define CUSTOM_ALARM    0
#define CUSTOM_REPORT   1
#define CUSTOM_ALL      2

extern U32 CliGetTest1(U8 targetid,U32* res);
extern U32 CliGetTest2(U8 targetid,U32* res);
extern U32 dfun_null(unsigned char targetid,U32* para);
extern unsigned int getGIU_RSC0(unsigned char target,unsigned int*res);
extern unsigned int getGIU_RSC1(unsigned char target,unsigned int*res);


typedef struct
{
    unsigned int start;
    unsigned int end;
}itemIdCustom_t;

#define ACTIOM_MMT_DEFAULT {\
1,1,1,1,0,0,0,0,0,0,\
1,1,1,1,1,1,1,1,1,1,\
}

#define MC_CONFIG_LIST_DEFAULT {\
		{RSC_VOLT,		0, MCA_ALARM+MCA_LOG,10000,AI_GT,274,0,0,0},\
		{RSC_VOLT,		0, MCA_ALARM+MCA_LOG,10001,AI_LT,166,0,0,0},\
		{RSC_CURRENT,	0, MCA_ALARM+MCA_LOG,10002,AI_GT,1000,0,0,0},\
		{RSC_CURRENT,	0, MCA_NONE,10003,AI_LT,0,0,0,0},\
    	{RSC_TEST,    	0, MCA_NONE,20000,AI_GT,300,0,RI_CHANGE,1},\
    	{RSC_TEST,      0, MCA_NONE,20000,AI_GT,300,0,RI_CHANGE,1},\
    	{RSC_TEST,      0, MCA_NONE,20000,AI_GT,300,0,RI_CHANGE,1},\
    	{RSC_TEST,   	0, MCA_NONE,20000,AI_GT,300,0,RI_CHANGE,1},\
    	{RSC_TEST,   	0, MCA_NONE,20000,AI_GT,300,0,RI_CHANGE,1},\
    	{RSC_TEST,   	0, MCA_NONE,20000,AI_GT,300,0,RI_CHANGE,1},\
		{RSC_TEST,		0, MCA_NONE,0,0,0,0,RI_CHANGE,10},\
		{RSC_TEST,	    0, MCA_NONE,0,0,0,0,RI_CHANGE,100},\
		{RSC_TEST,      0, MCA_NONE,0,0,0,0,RI_TIMING,60},\
		{RSC_TEST,      0, MCA_NONE,0,0,0,0,RI_CHANGE,1},\
		{RSC_TEST,      0, MCA_NONE,0,0,0,0,RI_CHANGE,1},\
		{RSC_TEST,	    0, MCA_NONE,0,0,0,0,RI_CHANGE,1},\
		{RSC_HEART, 	0, MCA_REPORT,0,0,0,0,RI_TIMING,1800},\
		{RSC_TEST,	    0, MCA_NONE,0,0,0,0,RI_TIMING,60},\
		{RSC_TEST,	    0, MCA_NONE,0,0,0,0,RI_TIMING,60},\
		{RSC_LOC,		0, MCA_REPORT,0,0,0,0,RI_APPEAR,1},\
	}

#define MC_RSC_FUN_LIST {\
    {RSC_NB_SIGNAL ,MC_MODULE_NB      ,NeulBc28QueryRsrp},\
    {RSC_NB_NETWORK,MC_MODULE_NB      ,NeulBc28QueryRegCon},\
    {RSC_VOLT      ,MC_MODULE_METER   ,ElecVoltageGet/*getGIU_RSC0*/},\
    {RSC_CURRENT   ,MC_MODULE_METER   ,ElecCurrentGet/*getGIU_RSC1*/},\
    {RSC_LAMP_ONOFF,MC_MODULE_LAMP    ,OppLampCtrlGetSwitchU32},\
    {RSC_LAMP_BRI  ,MC_MODULE_LAMP    ,OppLampCtrlGetBriU32},\
    {RSC_RUNTIME   ,MC_MODULE_LAMP    ,OppLampCtrlGetRtime},\
    {RSC_HISTIME   ,MC_MODULE_LAMP    ,OppLampCtrlGetHtime},\
    {RSC_HEART     ,MC_MODULE_GENERAL ,dfun_null},\
    {RSC_LOC       ,MC_MODULE_LOCATION,LocChg},\
    {RSC_TEST      ,MC_MODULE_GENERAL ,CliGetTest1},\
}

#define MC_LOG_CONFIG_DEFAULT {\
	{ ALARM_LOGID,               1,  1    },\
    { DEV_LOGID,                 1,  1    },\
    { NBNET_LOGID,               1,  1    },\
    { SWITCH_LOGID,              1,  1    },\
    { BRI_LOGID,                 1,  1    },\
    { NBDEV_LOGID,               1,  1    },\
    { ELECDEV_LOGID,             1,  1    },\
    { WORKPLAN_LOGID,            1,  1    },\
    { LOC_LOGID,            	 1,  1    },\
    { TIME_LOGID,            	 1,  1    },\
    { REBOOT_LOGID,            	 1,  1    },\
    { ERROR_LOGID,            	 0,  1    },\    
}

/*****************************************************************
                              Typedefs
******************************************************************/
typedef enum
{
    MC_MODULE_GENERAL=0,
    MC_MODULE_NB,
    MC_MODULE_WIFI,
    MC_MODULE_METER,
    MC_MODULE_LAMP,
    MC_MODULE_LOCATION,
    MC_MODULE_AUS,
    MC_MODULE_PARA,
    MC_MODULE_DAEMON,
	MC_MODULE_MAIN,
/********************************/
    MC_MODULE_MAX,
}EN_MC_MODULE;

typedef enum
{
    RSC_VOLT=0,         /* valtage */
    RSC_CURRENT=1,      /* current */
    RSC_LAMP_ONOFF=2,	/* lamp onoff status */
    RSC_LAMP_BRI=3, 	/* lamp brightness level */
    RSC_RUNTIME=4,		/* runtime */
    RSC_HISTIME=5,		/* history run time */
    RSC_NB_SIGNAL = 6,  /* NB signal */
    RSC_NB_NETWORK=7,   /* NB Network connecting status */
    RSC_HEART=8,        /* heart beat */
    RSC_LOC=9,			/* loc */   
    RSC_TEST=10,         /* cli test */
/***************************/
    RSC_MAX,
}EN_MC_RESOURCE;

typedef enum
{
    MCA_NONE=0,
    MCA_REPORT=0x01,
    MCA_ALARM=0x02,
    MCA_LOG=0x04,
}EN_MC_ENABLE;

typedef enum
{
    AI_EQUAL=1,    /* var == Condition */
    AI_LT,         /* var < Condition */
    AI_GT,         /* var > Condition */
    AI_LT_GT_OUT,  /* var < condition1 || var > condition2 */
    AI_LT_GT_IN,   /* condition1 < var < condition2 */
}EN_ALARM_IF;

typedef enum
{
    RI_CHANGE=1,
    RI_TIMING,
    RI_APPEAR,
}EN_REPORT_IF;

typedef enum
{
    LOG_DIR_NONE=0,  // disable log
    LOG_DIR_NB,      // nb only
    LOG_DIR_WIFI,    // wifi only
}EN_LOG_DIR;


typedef struct{
	EN_MC_MODULE moduleId;
	char *moduleName;
}MODULE_DESC_T;


// return 0:success,1:fail
typedef U32 (*pfGetResourceValue)(U8 targetid,U32* res);
typedef U32 (*pfGetLogString)(t_ad_log id,U8* despId,char* string);

#pragma pack(1)
typedef struct
{
	EN_MC_RESOURCE resource; /* 资源ID */
	unsigned char targetid;  /* 目标ID(资源ID下有多路目标ID) */
	EN_MC_ENABLE enable;     /* 使能 */
	unsigned short alarmId;  /* 告警ID(全局) */
    EN_ALARM_IF alarmIf;     /* 告警条件 */
    S32 alarmIfPara1;        /* 告警配置参数1 */
    S32 alarmIfPara2;        /* 告警配置参数2 */
    EN_REPORT_IF reportIf;   /* 状态上报条件 */
    U32 reportIfPara;        /* 状态上报配置参数 */
}AD_CONFIG_T;

typedef struct
{
    EN_MC_RESOURCE resource; /* 资源ID */
    EN_MC_MODULE module;     /* 模块名 */
    pfGetResourceValue fun;  /* 获取资源属性值的回调函数 */
}RST_FUN_TABLE_T;

typedef struct
{
    t_ad_log id;            /* Log id */
	pfGetLogString fun;     /* 解析Log id的回调函数 */
}LOG_TABLE_T;

typedef struct
{
    t_ad_log logid;
	U8 isLogReport;
	U8 isLogSave;
}LOG_CONFIG_T;

typedef struct
{
    U32 period; // ms
    EN_LOG_DIR dir;
}LOG_CONTROL_T;

#pragma pack()
/*****************************************************************
                              Interfaces
******************************************************************/
extern void AppDaemonInit(void);
extern void AppDaemonLoop(void);

/*告警和上报分开的接口函数*/
extern U32 ApsDaemonActiionItemIdCustom(U32 type,U32 index);
extern U32 ApsDaemonActionItemParaGetCustom(U32 type,U32 index,AD_CONFIG_T* code);
extern U32 ApsDaemonActionItemParaSetCustom(U32 type,U32 index,AD_CONFIG_T* code);
extern U32 ApsDaemonActionItemParaAddCustom(U32 type,U32* index,AD_CONFIG_T* code);
extern U32 ApsDaemonActionItemParaDelCustom(U32 type,U32 index);
extern U32 ApsDaemonActionItemParaSetDefaultCustom(U32 type);
extern U32 ApsDaemonActionItemIndexValidityCustom(U32 type,const const unsigned char** p,unsigned int *outLen);

/* 告警和上报不分开的接口函数 */
extern U32 ApsDaemonActionItemParaSet(U32 index,AD_CONFIG_T* code);
extern U32 ApsDaemonActionItemParaGet(U32 index,AD_CONFIG_T* code);
extern U32 ApsDaemonActionItemParaAdd(U32 *index,AD_CONFIG_T* code);
extern U32 ApsDaemonActionItemParaDel(U32 index);
extern U32 ApsDaemonActionItemIndexValidity(const const unsigned char** p,unsigned int *outLen);

/* Log接口函数 */
extern U32 ApsDaemonLogReport(APP_LOG_T* log);
extern S32 ApsDamemonLogCurrentSaveIndexGet(void);
extern U32 ApsDaemonLogQuery(U32 saveIndex,APP_LOG_T* log); /* 0:success */
extern U32 ApsDaemonLogControlParaSet(LOG_CONTROL_T* control);
extern U32 ApsDaemonLogControlParaGet(LOG_CONTROL_T* control);
extern U32 ApsDaemonLogIdParaSet(LOG_CONFIG_T* config);  /* Input: config->logid */
extern U32 ApsDaemonLogIdParaGet(LOG_CONFIG_T* config);  /* Input: config->logid */
extern U32 ApsDaemonLogConfigSetDefault(void);

/* Log模块名称和级别名称获取工具 */
extern U32 ApsDaemonLogLevelGet(int levelId, char * lName);
extern U32 ApsDaemonLogModuleNameGet(int moduleId, char * mName);









#endif


