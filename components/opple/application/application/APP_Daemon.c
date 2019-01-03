/**
******************************************************************************
* @file    APP_Daemon.c
* @author  Liqi
* @version V1.0.0
* @date    2018-6-22
* @brief   
******************************************************************************

******************************************************************************
*/ 

/*****************************************************************
                              How it works
******************************************************************/




/*****************************************************************
                              Includes
******************************************************************/
#include "APP_Daemon.h"
#include "string.h"
#include "SVS_Para.h"
#include "stdio.h"
#include "Includes.h"
#include "OPP_Debug.h"
#include "SVS_Coap.h"
#include "SVS_MsgMmt.h"
#include "DEV_Queue.h"
#include "SVS_Time.h"
#include "DRV_Bc28.h"
//#include "Opp_ErrorCode.h"


/*****************************************************************
                              Defines
******************************************************************/
#define DAEMON_PARA_ID_LOG_CONTROL        0    ///< 参数ID-日志控制
#define DAEMON_PARA_ID_LOG_SEQNUM         1    ///< 参数ID-日志序列号
#define DAEMON_PARA_ID_LOG_CONFIG_START   2    ///< 参数ID-日志配置起始ID
#define DAEMON_PARA_ID_ACTION_MMT         109  ///< 参数ID-动作管理
#define DAEMON_PARA_ID_ACTION_START       110  ///< 参数ID-动作起始ID

/*****************************************************************
                              Configs
******************************************************************/
#define DAEMON_PARA_LOG_DIR_DEFAULT LOG_DIR_NB  ///< Default log dir
#define DAEMON_PARA_LOG_PERIOD_DEFAULT   (30*60*1000)  ///< ms
#define DAEMON_STR_LOG_CHAR_MAX 100             ///< 未使用
#define DAEMON_STR_LOG_CODE_CHAR_MAX 50         ///< 未使用
#define DAEMON_REPORT_INTERVAL_MIN  3000        ///< ms
#define DAEMON_LOG_QUEUE_MAX   20               ///< 日志队列大小
#define DAEMON_LOG_SAVING_QUEUE_MAX   40        ///< 日志存储队列大小

#define DAEMON_LOG_SYNC_QUEUE_LIMIT 10          ///< 日志到达需要上报的深度
#define DAEMON_LOG_SAVING_PERIOD  (10*60*1000)  ///< 日志存储时间间隔ms

#define BOUNCE_ALARM_OVER_VOLT     10 // 过压防抖范围,V,单位与资源获取回调接口返回值一致
#define BOUNCE_ALARM_UNDER_VOLT    10 // 欠压防抖范围,V,单位与资源获取回调接口返回值一致
#define BOUNCE_ALARM_OVER_CURRENT  50   // 过流防抖范围,mA,单位与资源获取回调接口返回值一致
#define BOUNCE_ALARM_UNDER_CURRENT 5     // 欠流防抖范围,mA,单位与资源获取回调接口返回值一致


#define DAEMON_DEBUG 0

#define AD_GET_SYSTICK()   OppTickCountGet()
#define AD_REPORT_SYSTICK() ApsCoapHeartDisTick()     // 用于心跳上报随机推迟
#define AD_LOGREPORT_SYSTICK() NeulBc28AttachDisTick()  // 用于Log上报随机推迟

#define DAEMON_INIT_CHECK(v) \
				if((v)==0){\
					/*MakeErrLog(DEBUG_MODULE_DAEMON,OPP_UNINIT);*/ \
					DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_WARN,"uninit!\r\n");\
					return OPP_UNINIT;}
				
#define DAEMON_P_CHECK(p) \
				if((p)==(void*)0){\
					/*MakeErrLog(DEBUG_MODULE_DAEMON,OPP_NULL_POINTER);*/ \
					DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_ERROR,"null pointer!\r\n");\
					return OPP_NULL_POINTER;}
					

/*****************************************************************
                              Typedefs
******************************************************************/
typedef U8 action_mmt_t[MODULE_STATUS_ITEM_MAX];

const char* levelName[5]={
	"Fetal",
	"Error",
	"Warn",
	"Info",
	"Debug",
};

MODULE_DESC_T moduleDesc[MC_MODULE_MAX] ={ \
	{MC_MODULE_GENERAL, "GENERAL"},
	{MC_MODULE_NB, "NBIOT"},
	{MC_MODULE_WIFI, "WIFI"},
	{MC_MODULE_METER, "METER"},
	{MC_MODULE_LAMP, "LAMP"},
	{MC_MODULE_LOCATION, "LOCATION"},
	{MC_MODULE_AUS, "AUS"},
	{MC_MODULE_PARA, "PARA"},
	{MC_MODULE_DAEMON, "DAEMON"},
	{MC_MODULE_MAIN, "MAIN"},	
};

/// 对动作表进行定制化分区
const itemIdCustom_t itemIdCustom[2]={
    {.start=0,.end=9},   // alarm
    {.start=10,.end=19}, // report
};
/******************************************************************
                              Desp
******************************************************************/




/*****************************************************************
                              Local Data
******************************************************************/
/// 动作配置表
static AD_CONFIG_T config[MODULE_STATUS_ITEM_MAX];
/// 动作配置表默认参数
const AD_CONFIG_T config_default[MODULE_STATUS_ITEM_MAX]=\
MC_CONFIG_LIST_DEFAULT;

/// 日志配置表
static LOG_CONFIG_T log_config[LOG_ID_MAX];
/// 日志配置表默认参数
const LOG_CONFIG_T log_config_default[LOG_ID_MAX]=\
MC_LOG_CONFIG_DEFAULT;

/// 资源表
static RST_FUN_TABLE_T resource[RSC_MAX]=\
MC_RSC_FUN_LIST;

static U8 value_cache_valid[MODULE_STATUS_ITEM_MAX]={0}; ///< 资源属性值获取有效性缓存
static S32 value_cache[MODULE_STATUS_ITEM_MAX]={0};      ///< 资源属性值缓存，用于计算两次值之间的变化值
static U8 condition_cache[MODULE_STATUS_ITEM_MAX]={0};   ///< 动作发生条件匹配缓存，用于判断重复发生
static U32 tick_cache[MODULE_STATUS_ITEM_MAX]={0};       ///< 动作发生的时刻缓存，用于多个动作执行对齐到同一时刻

/// 动作表管理数组，每个元素对应动作表的一个动作是否有效(涉及添加删除)
static action_mmt_t action_mmt;
/// 动作管理数组默认值
const action_mmt_t action_mmt_default =\
ACTIOM_MMT_DEFAULT;

#define LS_STATE_IDLE   0   ///< 日志上报状态-空闲
#define LS_STATE_SEND   1   ///< 日志上报状态-发送
#define LS_STATE_RESEND 2   ///< 日志上报状态-重发
#define LS_STATE_NEXT   3   ///< 日志上报状态-下一个

static struct
{    
    U32 init;   ///< 初始化标识
    S32 index;  /// 处理动作配置索引，一次只处理一条配置
    LOG_CONTROL_T logctrl; ///< 日志控制信息
    //T_MUTEX mutex;         ///< 互斥锁用于多任务中使用本模块接口
    //T_MUTEX mutexCustom;   ///< 互斥锁用于多任务中使用本模块接口
    //T_MUTEX mutexAction;   ///< 互斥锁用于多任务中使用本模块接口
	U32 logSyncTick;       ///< 日志上报任务用计数值
	U8 logSyncState;       ///< 日志上报任务用状态
	U8 logSyncNum;         ///< 日志上报任务用本次上报数量总数
	U8 logSyncIndex;       ///< 日志上报任务用本次上报计数
	U32 logSeqNum;         ///< 日志上报任务用日志序列号
	T_MUTEX mutexConfig;   ///< 配置相关资源锁
	T_MUTEX mutexLog;      ///< 日志相关资源锁
}sDaemon;

static t_queue queue_log; ///< 日志上传队列
static U8 queue_log_buf[sizeof(APP_LOG_T)*DAEMON_LOG_QUEUE_MAX]; ///< 日志队列数据区

static t_queue queue_log_saving; ///< 日志保存队列
static U8 queue_log_saving_buf[sizeof(APP_LOG_T)*DAEMON_LOG_SAVING_QUEUE_MAX]; ///< 日志队列数据区


/*****************************************************************
                              Interfaces
******************************************************************/
U32 dfun_null(unsigned char targetid,U32* para)
{
  return 0;
}

U32 ApsDaemonLogLevelGet(int levelId, char * lName)
{
	DAEMON_P_CHECK(lName);
	if(levelId >= 5 || levelId < 0)
		return 1;
	strcpy(lName, levelName[levelId]);
	return 0;
}

U32 ApsDaemonLogModuleNameGet(int moduleId, char * mName)
{
	int i = 0;

	DAEMON_P_CHECK(mName);
	if(moduleId >= MC_MODULE_MAX || moduleId < 0)
		return 1;

	for(i=0;i<MC_MODULE_MAX;i++){
		if(moduleDesc[i].moduleId == moduleId){
			strcpy(mName, moduleDesc[i].moduleName);
			return 0;
		}
	}
	return 1;
}


/*****************************************************************
                              Debug
******************************************************************/
/**
   调试区接口，用于所使用的接口未准备好的情况下独立测试本模块
   正常情况下不使用以下接口
*/
extern U8 cli_daemon_lrb;
extern U8 cli_daemon_lrs;

U32 ApsCoapLogReport_debug(U32 chl,APP_LOG_T* log)
{
DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"ApsCoapLogReport log,logid:%d",log->id);
return 0;
}

U32 ApsCoapStateReport_debug(U32 chl,EN_MC_RESOURCE resource,S32 value)
{
DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"ApsCoapStateReport,resource:%d,value:%d",resource,value);
return 0;
}

U32 ApsCoapAlarmReport_debug(unsigned short alarmId,U32 status,S32 value)
{
DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"ApsCoapAlarmReport,alarmid:%d,status:%d,value:%d",alarmId,status,value);
return 0;
}

U32 ApsCopaLogReporterIsBusy_debug(void) // 0:idle,1:busy
{
return cli_daemon_lrb;
}

U32 ApsCoapLogBatchAppend_debug(APP_LOG_T* log) // 0:success,1:full,2:fail
{
DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"OppCoapLogBatchAppend log,logid:%d",log->id);
return 0;
}

U32 ApsCoapLogBatchStop_debug(void)
{
DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"OppCoapLogBatchStop");
return 0;
}

U32  ApsCoapLogBatchReportStatus_debug(void) // 0:success,1:fail,2:sending
{
return cli_daemon_lrs;
}



/**********************************************************************************************************
          Config
**********************************************************************************************************/
/***************************ActionMmt*****************************************/
/**
	@brief 从存储器加载动作管理表
	@attention none
	@param mmt 动作管理表
	@retval 0:success
*/
U32 ApsDaemonActionMmtLoad(action_mmt_t* mmt)
{
    U32 len = MODULE_STATUS_ITEM_MAX;
    DAEMON_P_CHECK(mmt);
    
    return AppParaRead(APS_PARA_MODULE_DAEMON,DAEMON_PARA_ID_ACTION_MMT,*mmt,&len);
}

/**
	@brief 设置动作管理表，成功后保存到存储器
	@attention none
	@param mmt 动作管理表
	@retval 0:success
*/
U32 ApsDaemonActionMmtSet(action_mmt_t* mmt)
{
    U32 res;
    u32 len = MODULE_STATUS_ITEM_MAX;

    //MUTEX_LOCK(sDaemon.mutexAction,MUTEX_WAIT_ALWAYS);
    DAEMON_P_CHECK(mmt);

    res =  AppParaWrite(APS_PARA_MODULE_DAEMON,DAEMON_PARA_ID_ACTION_MMT,*mmt,len);
    if(res ==0)
    {
        memcpy(action_mmt,*mmt,sizeof(action_mmt_t));
    }
    //MUTEX_UNLOCK(sDaemon.mutexAction);

    return res;
}

/**
	@brief 动作管理表置标识
	@attention 对相应的动作配置条目设置为有效
	@param index 动作配置编号
	@retval 0:success
*/
U32 ApsDaemonActionMmtSetIndex(U8 index)
{
    action_mmt_t mmt;

    if(index >= MODULE_STATUS_ITEM_MAX) return 1;

    memcpy(mmt,action_mmt,sizeof(action_mmt_t));
    mmt[index] = 1;
    if(ApsDaemonActionMmtSet(&mmt)==0)
    {
        action_mmt[index] = 1;
        return 0;
    }else
    {
        return 1;
    }
}

/**
	@brief 动作管理表清标识
	@attention 对相应的动作配置条目设置为无效
	@param index 动作配置编号
	@retval 0:success
*/
U32 ApsDaemonActionMmtClearIndex(U8 index)
{
    action_mmt_t mmt;
    U32 res;
    
    if(index >= MODULE_STATUS_ITEM_MAX) return 1;

	//MUTEX_LOCK(sDaemon.mutexAction,MUTEX_WAIT_ALWAYS);
    memcpy(mmt,action_mmt,sizeof(action_mmt_t));
    mmt[index] = 0;
    if(ApsDaemonActionMmtSet(&mmt)==0)
    {
        action_mmt[index] = 0;
        res = 0;
    }
    else
    {
        res = 1;
    }
	//MUTEX_UNLOCK(sDaemon.mutexAction);
	
    return res;
}

/***************************ActionItem*****************************************/
/**
	@brief 从存储器加载动作配置
	@attention none
	@param index 动作配置编号
	@param code 动作配置
	@retval 0:success
*/
U32 ApsDaemonActionItemParaLoad(S32 index,AD_CONFIG_T* code)
{
    U32 len = sizeof(AD_CONFIG_T);

    DAEMON_P_CHECK(code);
    
	return AppParaRead(APS_PARA_MODULE_DAEMON,index+DAEMON_PARA_ID_ACTION_START,(U8*)code,&len);
    //return 1;  // return 0:success,1:fail
}

/**
	@brief 加载默认动作配置
	@attention none
	@param index 动作配置编号
	@param code 动作配置
	@retval 0:success
*/
U32 ApsDaemonActionItemParaLoadDefault(S32 index,AD_CONFIG_T* code)
{
	DAEMON_P_CHECK(code);
	
    memcpy(code,&config_default[index],sizeof(AD_CONFIG_T));
    return 0;  // return 0:success
}

/**
	@brief 设置动作配置
	@attention 自动将动作配置为有效
	@param index 动作配置编号
	@param code 动作配置
	@retval 0:success
*/
U32 ApsDaemonActionItemParaSetInner(U32 index,AD_CONFIG_T* code) // Save para to flash and ram
{
    U32 res;

    DAEMON_P_CHECK(code);
    if(index >= MODULE_STATUS_ITEM_MAX) return 100;
    
    
    // Save code
    res = AppParaWrite(APS_PARA_MODULE_DAEMON,index+DAEMON_PARA_ID_ACTION_START,(U8*)code,sizeof(AD_CONFIG_T));
    if(res == 0){
        // Copy code to ram
        //MUTEX_LOCK(sDaemon.mutex,MUTEX_WAIT_ALWAYS);
        memcpy(&config[index],code,sizeof(AD_CONFIG_T));
        if(action_mmt[index] == 0){
            res = ApsDaemonActionMmtSetIndex(index);
        }else{
            
        }
        //MUTEX_UNLOCK(sDaemon.mutex);
    }

    return res;    
}

/**
	@brief 读取动作配置
	@attention none
	@param index 动作配置编号
	@param code 动作配置
	@retval 0:success
*/
U32 ApsDaemonActionItemParaGetInner(U32 index,AD_CONFIG_T* code)
{
   U32 res = 0;

   DAEMON_P_CHECK(code);
   if(index >= MODULE_STATUS_ITEM_MAX) return 100;
    //MUTEX_LOCK(sDaemon.mutex,MUTEX_WAIT_ALWAYS);
    if(action_mmt[index] != 0)
    { 
        memcpy(code,&config[index],sizeof(AD_CONFIG_T));
        res = 0;
    }
    else
    {
        res = 1;
    }
    //MUTEX_UNLOCK(sDaemon.mutex);
    return res;
}

/**
	@brief 添加一个动作配置
	@attention none
	@param index 成功后的动作配置编号
	@param code 动作配置
	@retval 0:success
*/
U32 ApsDaemonActionItemParaAddInner(U32 *index,AD_CONFIG_T* code)
{
    // Assign a index;
	int i;
	U32 id=MODULE_STATUS_ITEM_MAX;
	U8 tmp;

	DAEMON_P_CHECK(index);
	DAEMON_P_CHECK(code);
	
	for(i = 0;i < MODULE_STATUS_ITEM_MAX;i++)
	{	
		//MUTEX_LOCK(sDaemon.mutex,MUTEX_WAIT_ALWAYS);
		tmp = action_mmt[i];
		//MUTEX_UNLOCK(sDaemon.mutex);
		if(tmp == 0)
		{
		    id = i;
		    break;
		}
	}
	if(id < MODULE_STATUS_ITEM_MAX)
	{
		// Save config
		*index = id;
		return ApsDaemonActionItemParaSetInner(id,code);
	}
	else
	{
		return 1;
	}
}

/**
	@brief 删除一个动作配置
	@attention none
	@param index 动作配置编号
	@retval 0:success
*/
U32 ApsDaemonActionItemParaDelInner(U32 index)
{
   if(index >= MODULE_STATUS_ITEM_MAX) {return 1;}
    else {
		// save action_mmt
    	//action_mmt[index] = 0;
      return ApsDaemonActionMmtClearIndex(index);
    }
    return 0;
}

/**
	@brief 获取动作配置表的管理表信息
	@attention none
	@param p 动作配置管理表
	@param outLen 总长度
	@retval 0:success
*/
U32 ApsDaemonActionItemIndexValidityInner(const const unsigned char** p,unsigned int *outLen)
{
	*p = (const unsigned char*)&action_mmt;
	*outLen = MODULE_STATUS_ITEM_MAX;
	return 0;
}

/**************************************************************************************************
                                            Custom ares
**************************************************************************************************/
/**
	@brief 获取动作配置表中的全局id
	@attention none
	@param type 定制化分区，例如：
	       #define CUSTOM_ALARM    0
           #define CUSTOM_REPORT   1
	@param index 定制化分区下的编号
	@retval 0:success
*/
U32 ApsDaemonActiionItemIdCustomInner(U32 type,U32 index)
{
    U32 id;
    if(type >= 2 ) return MODULE_STATUS_ITEM_MAX;
    if(index+ itemIdCustom[type].start > itemIdCustom[type].end) return MODULE_STATUS_ITEM_MAX;
    id = itemIdCustom[type].start + index;
    return id;
}

/**
	@brief 获取动作表定制化分区中的配置信息
	@attention none
	@param type 定制化分区，例如：
	       #define CUSTOM_ALARM    0
           #define CUSTOM_REPORT   1
	@param index 定制化分区下的编号
	@code 配置信息
	@retval 0:success
*/
U32 ApsDaemonActionItemParaGetCustomInner(U32 type,U32 index,AD_CONFIG_T* code)
{
    U32 res = 0;
    U32 id  = ApsDaemonActiionItemIdCustomInner(type,index);

    DAEMON_P_CHECK(code);
   
    if(id >= MODULE_STATUS_ITEM_MAX) return 100;
    //MUTEX_LOCK(sDaemon.mutexCustom,MUTEX_WAIT_ALWAYS);
    if(action_mmt[id] != 0)
    { 
        memcpy(code,&config[id],sizeof(AD_CONFIG_T));
        res = 0;
    }
    else
    {
        res = 1;
    }
    //MUTEX_UNLOCK(sDaemon.mutexCustom);
    return res;
}

/**
	@brief 设置动作表定制化分区中的配置信息
	@attention 若该编号无效则设置为有效
	@param type 定制化分区，例如：
	       #define CUSTOM_ALARM    0
           #define CUSTOM_REPORT   1
	@param index 定制化分区下的编号
	@code 配置信息
	@retval 0:success
*/
U32 ApsDaemonActionItemParaSetCustomInner(U32 type,U32 index,AD_CONFIG_T* code)
{
    U32 res;
    U32 id  = ApsDaemonActiionItemIdCustomInner(type,index);

    DAEMON_P_CHECK(code);
    if(id >= MODULE_STATUS_ITEM_MAX) return 100;
    
    //MUTEX_LOCK(sDaemon.mutexCustom,MUTEX_WAIT_ALWAYS);
    // Save code
    res = AppParaWrite(APS_PARA_MODULE_DAEMON,id+DAEMON_PARA_ID_ACTION_START,(U8*)code,sizeof(AD_CONFIG_T));

    if(res == 0){
        // Copy code to ram
        memcpy(&config[id],code,sizeof(AD_CONFIG_T));
        if(action_mmt[id] == 0){
            res = ApsDaemonActionMmtSetIndex(id);
        }else{
        }
    }
    //MUTEX_UNLOCK(sDaemon.mutexCustom);

    return res; 
}

/**
	@brief 添加一个动作表定制化分区中的配置信息
	@attention none
	@param type 定制化分区，例如：
	       #define CUSTOM_ALARM    0
           #define CUSTOM_REPORT   1
	@param index 成功后分配的定制化分区下的编号
	@code 配置信息
	        typedef struct
			{
				EN_MC_RESOURCE resource; // 必选
				unsigned char targetid;  // 必选
				EN_MC_ENABLE enable;     // 必选
				unsigned short alarmId;  // 告警必选
			    EN_ALARM_IF alarmIf;     // 告警必选
			    S32 alarmIfPara1;        // 告警必选
			    S32 alarmIfPara2;        // 告警必选
			    EN_REPORT_IF reportIf;   // 状态上报必选
			    U32 reportIfPara;        // 状态上报必选
			}AD_CONFIG_T;
	@retval 0:success
*/
U32 ApsDaemonActionItemParaAddCustomInner(U32 type,U32* index,AD_CONFIG_T* code)
{
  int i;
  U32 id;
  U32 start,end;

  DAEMON_P_CHECK(code);
  DAEMON_P_CHECK(index);
  if(type >= 2) return 200;
  
  start = itemIdCustom[type].start;
  end = itemIdCustom[type].end;
  
  id=end+1;
	for(i = start;i <= end;i++)
	{
		if(action_mmt[i] == 0)
		{
		    id = i;
		    break;
		}
	}
	if(id < end+1)
	{
		// Save config
		*index = id-start;
		return ApsDaemonActionItemParaSetCustomInner(type,*index,code);
	}
	else
	{
		return 1;
	}
}

/**
	@brief 删除动作表定制化分区中的配置信息
	@attention 若该编号无效则设置为有效
	@param type 定制化分区，例如：
	       #define CUSTOM_ALARM    0
           #define CUSTOM_REPORT   1
	@param index 定制化分区下的编号
	@retval 0:success
*/
U32 ApsDaemonActionItemParaDelCustomInner(U32 type,U32 index)
{
    U32 id  = ApsDaemonActiionItemIdCustomInner(type,index);
    if(id >= MODULE_STATUS_ITEM_MAX) {return 1;}
    else {
		// save action_mmt
    	return ApsDaemonActionMmtClearIndex(id);
    }
    return 0;
}

/**
	@brief 恢复出厂设置
	@attention
	@param type 0/1:告警/上报
	@retval 0:success
*/
U32 ApsDaemonActionItemParaSetDefaultCustomInner(U32 type)
{
	U32 id,num,res;
    if(type > CUSTOM_REPORT) {return 1;}

    num = itemIdCustom[type].end - itemIdCustom[type].start + 1;
	for(int i=0;i<num;i++)
	{
		id = ApsDaemonActiionItemIdCustomInner(type,i);
		if(id > MODULE_STATUS_ITEM_MAX) return 2;
		
		if(action_mmt_default[id] == 0)
		{
			res = ApsDaemonActionItemParaDelInner(id);
			if(res != 0) return 3;
		}
		else
		{
			res = ApsDaemonActionItemParaSetInner(id,&config_default[id]);
			if(res != 0) return 4;
		}
	}
	return 0;
}

/**
	@brief 获取动作配置管理表信息
	@attention none
	@param p 动作配置管理表
	@param outLen 总长度
	@retval 0:success
*/
U32 ApsDaemonActionItemIndexValidityCustomInner(U32 type,const const unsigned char** p,unsigned int *outLen)
{
    U32 id = ApsDaemonActiionItemIdCustomInner(type,0);
    U32 len = itemIdCustom[type].end - itemIdCustom[type].start +1;
    *p = (const unsigned char*)&action_mmt[id];
    //memcpy(*p,&action_mmt[id],len);
	  *outLen = len;
	  return 0;
}



/***************************LogControl*****************************************/
/**
	@brief 从存储器加载日志控制信息
	@attention none
	@param control 日志控制信息
	@retval 0:success
*/
U32 ApsDaemonLogControlParaLoad(LOG_CONTROL_T* control)
{
	U32 len = sizeof(LOG_CONTROL_T);
	
	DAEMON_P_CHECK(control);
	
    return AppParaRead(APS_PARA_MODULE_DAEMON,DAEMON_PARA_ID_LOG_CONTROL,(U8*)control,&len);
}

/**
	@brief 加载默认日志控制信息
	@attention none
	@param control 日志控制信息
	@retval 0:success
*/
U32 ApsDaemonLogControlParaLoadDefault(LOG_CONTROL_T* control)
{
	DAEMON_P_CHECK(control);
	
    control->dir = DAEMON_PARA_LOG_DIR_DEFAULT;
	control->period = DAEMON_PARA_LOG_PERIOD_DEFAULT;
	return 0;
}

/**
	@brief 设置日志控制信息
	@attention none
	@param control 日志控制信息
	@retval 0:success
*/
U32 ApsDaemonLogControlParaSetInner(LOG_CONTROL_T* control)
{
    U32 res;

    DAEMON_P_CHECK(control);
    //MUTEX_LOCK(sDaemon.mutex,MUTEX_WAIT_ALWAYS);
    res =  AppParaWrite(APS_PARA_MODULE_DAEMON,DAEMON_PARA_ID_LOG_CONTROL,(U8*)control,sizeof(LOG_CONTROL_T));
    if(res ==0)
    {
        memcpy(&sDaemon.logctrl,control,sizeof(LOG_CONTROL_T));
    }
    //MUTEX_UNLOCK(sDaemon.mutex);
    
    return res;
}

/**
	@brief 获取日志控制信息
	@attention none
	@param control 日志控制信息
	@retval 0:success
*/
U32 ApsDaemonLogControlParaGetInner(LOG_CONTROL_T* control)
{
	DAEMON_P_CHECK(control);
	//MUTEX_LOCK(sDaemon.mutex,MUTEX_WAIT_ALWAYS);
    memcpy(control,&sDaemon.logctrl,sizeof(LOG_CONTROL_T));
    //MUTEX_UNLOCK(sDaemon.mutex);
	return 0;
}

/**
	@brief 从存储器加载日志配置信息，参照：
	        typedef struct
			{
			    t_ad_log logid;
				U8 isLogReport;
				U8 isLogSave;
			}LOG_CONFIG_T;
	@attention none
	@param cfg 日志配置信息
	@retval 0:success
*/
U32 ApsDaemonLogIdParaLoad(LOG_CONFIG_T* cfg)
{
	U32 len = sizeof(LOG_CONFIG_T);

	DAEMON_P_CHECK(cfg);
	
	if(cfg->logid >= LOG_ID_MAX || cfg->logid < 0) return 1;
	return AppParaRead(APS_PARA_MODULE_DAEMON,DAEMON_PARA_ID_LOG_CONFIG_START+cfg->logid,(U8*)cfg,&len);
}

/**
	@brief 加载日志配置默认配置
	@attention none
	@param cfg 日志配置信息
	@retval 0:success
*/
U32 ApsDaemonLogIdParaLoadDefault(LOG_CONFIG_T* cfg)
{
	DAEMON_P_CHECK(cfg);
	
    if(cfg->logid >= LOG_ID_MAX || cfg->logid < 0) return 1;
    memcpy(cfg,&log_config_default[cfg->logid],sizeof(LOG_CONFIG_T));
	return 0;
}

/**
	@brief 设置日志配置信息
	@attention none
	@param cfg 日志配置信息
	@retval 0:success
*/
U32 ApsDaemonLogIdParaSetInner(LOG_CONFIG_T* cfg)
{
    U32 res;

    DAEMON_P_CHECK(cfg);
    //MUTEX_LOCK(sDaemon.mutex,MUTEX_WAIT_ALWAYS);
    res =  AppParaWrite(APS_PARA_MODULE_DAEMON,DAEMON_PARA_ID_LOG_CONFIG_START+cfg->logid,(U8*)cfg,sizeof(LOG_CONFIG_T));
	if(res ==0)
    {
		if(cfg->logid >=0 && cfg->logid < LOG_ID_MAX){
        	memcpy(&log_config[cfg->logid],cfg,sizeof(LOG_CONFIG_T));
        }else{
        	return 1;
        }
    }
    //MUTEX_UNLOCK(sDaemon.mutex);
    
    return res;
}

/**
	@brief 获取日志配置信息
	@attention none
	@param cfg 日志配置信息
	@retval 0:success
*/
U32 ApsDaemonLogIdParaGetInner(LOG_CONFIG_T* cfg)
{
	DAEMON_P_CHECK(cfg);
	
	//MUTEX_LOCK(sDaemon.mutex,MUTEX_WAIT_ALWAYS);
	if(cfg->logid >= LOG_ID_MAX || cfg->logid < 0) return 1;
    memcpy(cfg,&log_config[cfg->logid],sizeof(LOG_CONFIG_T));
    //MUTEX_UNLOCK(sDaemon.mutex);
	return 0;
}

/*return op fail num*/
U32 ApsDaemonLogConfigSetDefaultInner(void)
{
	U32 res;
	U32 ret = 0,ret_logctrl;
	LOG_CONTROL_T control;

	control.dir = DAEMON_PARA_LOG_DIR_DEFAULT;
	control.period = DAEMON_PARA_LOG_PERIOD_DEFAULT;
	ret_logctrl = ApsDaemonLogControlParaSetInner(&control);
	for(int i=0;i<LOG_ID_MAX;i++)
	{
		res = ApsDaemonLogIdParaSetInner(&log_config_default[i]);
		if(res != 0) {ret++; continue;}
	}
	return ret+ret_logctrl;
}


/***************************LogId*****************************************/



/**********************************************************************************************************/
// Interfaces


/**********************************************************************************************************/
/**
	@brief 通过logid找到在logid配置表中的索引号
	@attention None
	@param id logid
	@retval -1：error，else：index
*/
S32 findIndexInLogConfigTable(t_ad_log id)
{
    U32 i;

	for(i = 0;i<LOG_ID_MAX;i++)
	{
		if(log_config[i].logid == id)
		{
		    return i;
		}
	}
	return -1;
}

/**
	@brief 日志生成接口
	@attention 为日志分配一个序列号，添加时间信息
	@param log 日志信息
	@retval 0:success
*/
U32 ApsDaemonLogGen(APP_LOG_T* log)
{
	U32 seqnum;
	ST_TIME time;

	DAEMON_P_CHECK(log);
	
	if(AppParaReadU32(APS_PARA_MODULE_DAEMON,DAEMON_PARA_ID_LOG_SEQNUM,&sDaemon.logSeqNum) != 0)
	{
		// Do nothing
		DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_WARN,"Read log seqnum fail!\r\n");
	}

	seqnum = sDaemon.logSeqNum+1;
	if(AppParaWriteU32(APS_PARA_MODULE_DAEMON,DAEMON_PARA_ID_LOG_SEQNUM,seqnum) == 0)
	{
		sDaemon.logSeqNum++;
		log->seqnum = seqnum;
		//time;
		Opple_Get_RTC(&time);
		log->time[0] = (U8)(time.Year-2000);  // Year
		log->time[1] = time.Month;
		log->time[2] = time.Day;
		log->time[3] = time.Hour;
		log->time[4] = time.Min;
		log->time[5] = time.Sencond;
		return 0;
	}
	else
	{
		return 1;
	}
	
}

/**
	@brief 判断日志logid是否配置为有效动作(上报或存储)
	@attention 
	@param logid 日志id
	@retval 0:success
*/
U32 ApsIsLogConfigedValid(unsigned char logid)
{
	S32 logindex;
	
	logindex = findIndexInLogConfigTable(logid);
	if(logindex < 0) {
		return 0;
	}
	
	// save or report
	if(log_config[logindex].isLogSave || log_config[logindex].isLogReport){
		return 1;
	}else{
		return 0;
	}
}

/**
	@brief 日志上报（压入队列）
	@attention None
	@param log 日志信息
	@retval 0:success
*/
U32 ApsDaemonLogReportInner(APP_LOG_T* log)
{  
	U32 res;
	S32 logindex;

	DAEMON_P_CHECK(log);
	
	//if(ApsIsLogConfigedValid(log->id) == 0)
	//{
	//	res = 1;
	//}

	logindex = findIndexInLogConfigTable(log->id);
	if(logindex < 0) {
		return 1;
	}

	if(log_config[logindex].isLogSave==0 && log_config[logindex].isLogReport==0)
	{
		return 2;
	}
	
	if(ApsDaemonLogGen(log) == 0)
	{
		if(log_config[logindex].isLogSave)
		{
			res = pushQueue(&queue_log_saving,(U8*)log, 0);
			if(res != 0)
			{
				res += 100;
				DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_WARN,"Push log saving queue fail\r\n");
			}
		}
		else
		{
			log->saveid = 0;
			res = pushQueue(&queue_log, (U8*)log, 0);
			if(res != 0)
			{
				res += 10;
				DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_WARN,"Push log report queue fail\r\n");
			}
		}
	}
	else
	{
		DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_INFO,"Write log seq fail\r\n");
		res = 3;
	}

	return res;
}

/**
	@brief 动作匹配接口
	@attention None
	@param value 动作值
	@action 需要匹配的动作值
	@retval 0:success
*/
static U32 isMcActionMatch(U8 value,EN_MC_ENABLE action)
{
	// 0b0000 0111 & 0b0000 0011
	// ob0000 0001 & 0b0000 0011
	if(((U32)value & action) != 0) return 1;
	else return 0;
	//return (value & action);
}

/**
	@brief 状态上报接口
	@attention None
	@param index 动作配置表的编号
	@param res 动作配置对应的属性值
	@param status 发生状态 发生/恢复
	@retval 0:success
*/
static void ApsDaemonStatusReport(U32 index,S32 res,U32 status)
{
    #if DAEMON_DEBUG==0
    ST_REPORT_PARA para;
	para.chl = CHL_NB;
	para.resId = config[index].resource;
	para.value = res;
    ApsCoapStateReport(&para);
	DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_INFO,"ApsDaemonStatusReport index:%d,res:%d,status:%d\r\n",\
    index,res,status);
    #else
    DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_INFO,"ApsDaemonStatusReport index:%d,res:%d,status:%d\r\n",\
    index,res,status);
    #endif
}

/**
	@brief 告警上报接口
	@attention None
	@param index 动作配置表的编号
	@param res 动作配置对应的属性值
	@param status 发生状态 1:发生/0:恢复
	@retval 0:success
*/
static void ApsDaemonAlarmReport(U32 index,S32 res,U32 status)
{
	APP_LOG_T log;
	ST_ALARM_PARA para;
	U32 saveid,ret;
	ST_LOG_PARA reportPara;
	
    #if DAEMON_DEBUG==0
    DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"ApsDaemonAlarmReport index:%d,res:%d,status:%d\r\n",\
    index,res,status);

	para.id = ApsDaemonActiionItemIdCustomInner(CUSTOM_ALARM,index);
	para.dstChl = CHL_NB;
	para.alarmId = config[index].alarmId;
	para.status = status;
	para.value = res;	
	memset(para.dstInfo,0,sizeof(para.dstInfo));
    ApsCoapAlarmReport(&para);

    #else
    DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"ApsDaemonAlarmReport index:%d,res:%d,status:%d\r\n",\
    index,res,status);
    #endif

    if(isMcActionMatch(config[index].enable,MCA_LOG))
    {
		// save
		/*FIXME*/
		/*log[0-1]:alarm id
		**log[2]:status
		**log[3-6]:value*/
		log.id = ALARM_LOGID;
		log.level = LOG_LEVEL_WARN;
		log.module = MC_MODULE_DAEMON;
		log.len =7;
		*(U16*)log.log = config[index].alarmId;
		*(U8*)(log.log+2) = (U8)status;
		*(U32*)(log.log+3) = res;
		if(ApsDaemonLogGen(&log) == 0)
		{
            ret = ApsLogWrite(&log,&saveid);
            if(ret == 0)
	        {
	        	// report        	
				reportPara.chl = CHL_NB;
				reportPara.type = ACTIVE_REPROT;
				reportPara.leftItems = 0;
				reportPara.reqId = 0;
				reportPara.logSaveId = saveid;
				memset(reportPara.dstInfo,0,sizeof(reportPara.dstInfo));
				memcpy(&reportPara.log, &log, sizeof(APP_LOG_T));
				ApsCoapLogReport(&reportPara);
	        }
        }
        else
        {
          // Log gen fail
        }
        
    }
}

// 绝对值
inline S32 diff(S32 a,S32 b)
{
    return a>b?a-b:b-a;
}

/**
	@brief 动作发生状态判断
	@attention None
	@param c_now 目前的状态
	@param c_cache 上一次状态
	@retval 0:重复发生，1：首次发生，2：消除
*/
static U32 ApdDaemonStatusMath(U32 c_now,U32 c_cache)
{
    if(c_now)
    {
        if(c_cache)
        {
            return 0;  // Repeated
        }
        else
        {
            return 1;  // Appear
        }
    }
    else
    {
        if(c_cache)
        {
            return 2;  // Disappear
        }
        else
        {
            return 0;
        }
    }
}

/**
	@brief 通过资源ID找到它在资源表中对应的编号
	@attention None
	@param rscid 资源ID
	@retval -1：error，else：index
*/
int findIndexWithResource(U8 rscid)
{
    int i;
    for(i = 0;i < RSC_MAX;i++)
    {
        if(rscid == resource[i].resource) return i;
    }
    return -1;
}

// 时差
U32 tickDiff(OS_TICK_T last,OS_TICK_T now)
{
	return now-last;
}

U32 secDiff(U32 last,U32 now)
{
	if (now >= last) return now-last;
	else return (~((OS_TICK_T)0))/1000 - last + now;
}

/**
	@brief 通过上报配置信息和当前属性值执行相应动作
	@attention None
	@param index 动作配置表编号
	@param res 属性值
	@param config 配置信息
	@retval 1：match,0:unmatch
*/
static U32 ApdDaemonConditionMathReport(S32 index,S32 res,AD_CONFIG_T* config)
{
    EN_REPORT_IF ri = config->reportIf;
    U32 tick,sec;
    //S32 pindex = findIndexWithResource(config[index].resource);

    DAEMON_P_CHECK(config);
    
    if(ri == RI_CHANGE)
    {
		if(config->reportIfPara == 0) return 0;
        if(diff(res,value_cache[index]) >= (U32)config->reportIfPara)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else if(ri == RI_TIMING)// RI_TIMING    // 时间要求严格，需要单独开一个任务
    {
        //tick = AD_GET_SYSTICK();
        tick = AD_REPORT_SYSTICK();   // For randomly reporting purpose
        //DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"TICK=%d,tick_cache=%d",tick,tick_cache[index]);
        sec = tick/1000;
        if( (secDiff(tick_cache[index],sec) >= (U32)config->reportIfPara)    // 定时时间到
          /*&&(sec % config->reportIfPara <= DAEMON_REPORT_INTERVAL_MIN)*/ ) // 且对齐到时刻
        {
             tick_cache[index] = sec - (sec % (U32)config->reportIfPara);     // 对齐到整点时刻
             return 1;
        }
        else
        {
            return 0;
        }
    }
    else if(ri == RI_APPEAR)
    {
        if(res == config->reportIfPara)
        {
        	return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

inline U32 getBounceValue(AD_CONFIG_T* config,U32* res)
{
	if(config->resource == RSC_VOLT)
	{
		if(config->alarmIf == AI_GT){
			*res = BOUNCE_ALARM_OVER_VOLT;
			return 0;
		}else if(config->alarmIf == AI_LT){
			*res = BOUNCE_ALARM_UNDER_VOLT;
			return 0;
		}else{
			return 1;
		}
	}
	else if(config->resource == RSC_CURRENT)
	{
		if(config->alarmIf == AI_GT){
			*res = BOUNCE_ALARM_OVER_CURRENT;
			return 0;
		}else if(config->alarmIf == AI_LT){
			*res = BOUNCE_ALARM_UNDER_CURRENT;
			return 0;
		}else{
			return 2;
		}
	}
	else
	{
		//return 3;
		*res = 0;
		return 0;
	}
}

/**
	@brief 通过告警配置信息和当前属性值执行相应动作
	@attention None
	@param index 动作配置表编号
	@param res 属性值
	@param config 配置信息
	@retval 0：success
*/
static U32 ApdDaemonConditionMathAlarm(S32 index,S32 res,AD_CONFIG_T* config)
{
    U32 condition,status,cache_condition;
    EN_ALARM_IF ai = config->alarmIf;
    U32 bounceValue;
    
	DAEMON_P_CHECK(config);

	cache_condition = condition_cache[index];
	
    //DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"ApdDaemonConditionMathAlarm,index=%d,ai=%d",index,ai);
    if(ai == AI_EQUAL) // ==
    {
        if(res == config->alarmIfPara1) condition = 1;
        else condition = 0;
    }
    else if(ai == AI_LT) // <
    {
		if(getBounceValue(config,&bounceValue)!=0)
		{
			DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_ERROR,"Get bounceValue fail!\r\n");
			return 1;
		}
		
        if(res < config->alarmIfPara1)
        {
        	condition = 1; // happened
        }
        else
        {
        	if(cache_condition != 0) // Try to recovery alarm
        	{
        		if(res >= config->alarmIfPara1 + bounceValue)
        		{
        			condition = 0; // recovery
        		}
        		else
        		{
        			condition = 1; // keep happened
        		}
        	}
        	else
        	{
        		condition = 0; // keep unhappened
        	}
        }
    }
    else if(ai == AI_GT) // >
    {
        //DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"AI_GT,res=%d,cpara=%d",res,config->alarmIfPara1);
        if(getBounceValue(config,&bounceValue)!=0)
		{
			DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_ERROR,"Get bounceValue fail!\r\n");
			return 1;
		}
		
        if(res > config->alarmIfPara1)
        {
        	condition = 1; // happened
        }
        else
        {
        	if(cache_condition != 0) // Try to recovery alarm
        	{
        		if(res  + bounceValue <= config->alarmIfPara1)
        		{
        			condition = 0; // recovery
        		}
        		else
        		{
        			condition = 1; // keep happened
        		}
        	}
        	else
        	{
        		condition = 0; // keep unhappened
        	}
        }
    }
    else if(ai == AI_LT_GT_OUT) // ***|  |****
    {
        if((res) < config->alarmIfPara1 || (res) > config->alarmIfPara2)  // 加防抖与恢复判断
        {
            condition = 1;
        }else{
            condition = 0;
        }
    }
    else if(ai == AI_LT_GT_IN) // |******|
    {
        if((res) > config->alarmIfPara1 && (res) < config->alarmIfPara2)  // 加防抖与恢复判断
        {
            condition = 1;
        }else{
            condition = 0;
        }
    }
    else
    {
        return 0;
    }
    
    status = ApdDaemonStatusMath(condition,condition_cache[index]); // 重复、发生、消除
    condition_cache[index] = condition;
    if(status != 0)
    {
        return status;
    }
    else
    {
        return 0;
    }
}

/**
	@brief 通过配置信息和当前属性值执行相应动作
	@attention None
	@param index 动作配置表编号
	@param res 属性值
	@param config 配置信息
	@retval 0：success
*/
void ApsDaemonAction(S32 index,AD_CONFIG_T* config,S32 res)
{
    U32 condition;
    U32 sta=0;

	DAEMON_P_CHECK(config);
	
    if(isMcActionMatch(config->enable,MCA_REPORT))
    {
        condition = ApdDaemonConditionMathReport(index,res,config);
        if(condition != 0){
            sta = condition==1?1:0;
            ApsDaemonStatusReport(index,res,sta);
        }
    }
    if(isMcActionMatch(config->enable,MCA_ALARM))
    {
        condition = ApdDaemonConditionMathAlarm(index,res,config); // 0:repeat/none,1:appear,2:disappear
        if(condition != 0){
            sta = condition==1?1:0;
            ApsDaemonAlarmReport(index,res,sta);
        }
    }
}

/**
	@brief 获取日志模块最新的saveid
	@attention None
	@retval saveid
*/
S32 ApsDamemonLogCurrentSaveIndexGetInner(void)
{
    return ApsLogSaveIdLatest();
}

/**
	@brief 通过saveid查询日志信息
	@attention None
	@param saveIndex 日志saveid
	@param log 日志信息
	@retval 0：success
*/
U32 ApsDaemonLogQueryInner(U32 saveIndex,APP_LOG_T* log)
{
	DAEMON_P_CHECK(log);
    return ApsLogRead(saveIndex, log);
}

/**
	@brief 日志缓存任务
	@attention 将需要上报和存储的日志缓存在ram中，达到一定条件后再上报或存储
	@retval none
*/
void ApsDaemonLoopLogSync(void)
{
    /*
    1.若定时时间到或者队列达到一定程度，则开始，否则什么也不干
    2.首先从队列中取出log，存在本地，并上报
    3.当处理完本地的长度后，继续等待条件
    */
	U32 tick;
	U32 len;
	static APP_LOG_T log;
	static U32 saveid;
	U32 ret;
	S32 logindex;
	ST_LOG_APPEND_PARA appendPara;	
	ST_LOG_STOP_PARA stopPara;
	
	MUTEX_LOCK(sDaemon.mutexLog,MUTEX_WAIT_ALWAYS);
	switch(sDaemon.logSyncState)
	{
	    case LS_STATE_IDLE:
			//tick = AD_GET_SYSTICK();
			tick = AD_LOGREPORT_SYSTICK(); // For randomly reporting purpose
	        len = lengthQueue(&queue_log);
			if(((tickDiff(sDaemon.logSyncTick,tick) >= sDaemon.logctrl.period) // Liqi,18-9-20,>= to  >
				&&(len > 0))||  /* 时间到 */
	  		 (len >= DAEMON_LOG_SYNC_QUEUE_LIMIT) )            /* 或者log数量达到 */
			{
			    sDaemon.logSyncNum = len;
				sDaemon.logSyncIndex = 0;
				sDaemon.logSyncTick = tick;
				sDaemon.logSyncState = LS_STATE_SEND;
				// DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"Log state -> 1\r\n");
			}
			else
			{
			    // DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"idle,qlen=%d,period=%d\r\n",len,sDaemon.logctrl.period);
			}
			break;
		case LS_STATE_SEND:
		    #if DAEMON_DEBUG==0
			if(ApsCopaLogReporterIsBusy() == 1){ 
				DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"Reporter is busy!\r\n");
				break;
			}
			#else
			if(ApsCopaLogReporterIsBusy_debug() == 1) break;
			#endif
			
			
			//MUTEX_LOCK(sDaemon.mutexLog,MUTEX_WAIT_ALWAYS);
			ret = pullQueue(&queue_log, (U8*)&log, sizeof(APP_LOG_T));
			//MUTEX_UNLOCK(sDaemon.mutexLog);
			if(ret != 0){
			    sDaemon.logSyncState = LS_STATE_NEXT;
				// DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_ERROR,"Queue is empty\r\n");
			}else{
				logindex = findIndexInLogConfigTable(log.id);
				if(logindex < 0) {
					//sDaemon.logSyncState = LS_STATE_IDLE;
					//return;
					sDaemon.logSyncState = LS_STATE_NEXT;
					break;
				}
				
			    // save
			    /*
			    if(log_config[logindex].isLogSave){
				    ret = ApsLogWrite(&log,&saveid);
			    }else{
					saveid = 0;
			    }
			    */
			    saveid = log.saveid;
				
				// report
				if(log_config[logindex].isLogReport){
					#if DAEMON_DEBUG==0
					appendPara.dstChl = CHL_NB;
					appendPara.logSaveId = saveid;
					memcpy(&appendPara.log, &log, sizeof(APP_LOG_T));
					ret = ApsCoapLogBatchAppend(&appendPara);
					#else
					ret = ApsCoapLogBatchAppend_debug(&log);
					#endif
					if(ret == 0) // success
					{
					    sDaemon.logSyncState = LS_STATE_NEXT;	
					}else if(ret == 1){ // full
						sDaemon.logSyncState = LS_STATE_RESEND; // resend;
					}else{ // fail
						sDaemon.logSyncState = LS_STATE_NEXT;
					}
				}else{
					sDaemon.logSyncState = LS_STATE_NEXT;
				}
			}
			break;
		case LS_STATE_RESEND:
			#if DAEMON_DEBUG==0
			if(ApsCopaLogReporterIsBusy() == 1) break;
			appendPara.dstChl = CHL_NB;
			appendPara.logSaveId = saveid;
			memcpy(&appendPara.log, &log, sizeof(APP_LOG_T));
			ret = ApsCoapLogBatchAppend(&appendPara);
			DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"Log reappend,ret=%d!\r\n",ret);
			#else
			if(ApsCopaLogReporterIsBusy_debug() == 1) break;
			ret = ApsCoapLogBatchAppend_debug(&log);
			#endif
			sDaemon.logSyncState = LS_STATE_NEXT;	
		    break;
		case LS_STATE_NEXT:
			DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"Log report next!\r\n");
			sDaemon.logSyncIndex++;
			if(sDaemon.logSyncIndex >= sDaemon.logSyncNum)
			{
				sDaemon.logSyncState = LS_STATE_IDLE;
				stopPara.dstChl = CHL_NB;
				stopPara.leftItems = 0;
				stopPara.reqId = 0;
				stopPara.type = ACTIVE_REPROT;
				ApsCoapLogBatchStop(&stopPara);///////////////////////
				DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_DEBUG,"Log append stop()!\r\n");
			}
			else
			{
			    sDaemon.logSyncState = LS_STATE_SEND;
			}
			break;
		default:
			sDaemon.logSyncState = LS_STATE_IDLE;
			break;
	}
	MUTEX_UNLOCK(sDaemon.mutexLog);
}

void ApsDaemonLoopLogSaving(void)
{
	static APP_LOG_T log;
	static U32 saveid;
	U32 ret;
	S32 logindex;
	U32 tick,len;
	static U32 sTick=0,sLogSavingState=0;
	static U32 sLogSavingNum=0,sLogSavingIndex;

	MUTEX_LOCK(sDaemon.mutexLog,MUTEX_WAIT_ALWAYS);

	switch(sLogSavingState)
	{
		case 0:
			tick = AD_GET_SYSTICK();
		    len = lengthQueue(&queue_log_saving);
			if(((tickDiff(sTick,tick) >= DAEMON_LOG_SAVING_PERIOD)
				&&(len > 0)))  /* 时间到 */
			{
				// Continue;
				sLogSavingIndex = 0;
				sLogSavingNum = len;
				sLogSavingState = 1;
				sTick = tick;
				DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_INFO,"Log saving period time on\r\n");
			}
			else
			{
				// Do nothing
			}
		break;

		case 1:
			ret = pullQueue(&queue_log_saving, (U8*)&log, sizeof(APP_LOG_T));
	
			if(ret == 0)
			{
				logindex = findIndexInLogConfigTable(log.id);
				if(logindex < 0) 
				{
					//return;
				}
				else
				{
				    // save
				    if(log_config[logindex].isLogSave)
				    {
					    ret = ApsLogWrite(&log,&saveid);
					    if(ret == 0)
					    {
					    	if(log_config[logindex].isLogReport)
					    	{
								log.saveid = saveid;
					    		ret = pushQueue(&queue_log, (U8*)&log, 0);
					    	}
					    }
					    // [code review]统一加到FLASH故障里
					    // if(ret != 0) saveid = 0;
				    }
			    }
				sLogSavingIndex++;
				if(sLogSavingIndex >= sLogSavingNum)
				{
					sLogSavingState = 0;
				}
			}
			else
			{
				sLogSavingState = 0;
			}
		break;

		default:
			sLogSavingState = 0;
		break;
	}
	
	MUTEX_UNLOCK(sDaemon.mutexLog);
}

/// 状态监控主任务
void AppDaemonLoop(void)
{
    U8 i=0;
    U32 result;
    pfGetResourceValue fun;
    int index;
    int err;
    
    /******************************************************************************************/
    // Act according to the para
    MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
    if((action_mmt[sDaemon.index] != 0)&&
       (isMcActionMatch(config[sDaemon.index].enable,MCA_ALARM+MCA_REPORT)) )
    {
        index = findIndexWithResource(config[sDaemon.index].resource);
        if((index >= 0 )&&(index < RSC_MAX)&&(resource[index].fun != 0))
        {
            fun = resource[index].fun;
            if(fun(config[sDaemon.index].targetid,&result) == 0)
            {
				if(RI_APPEAR == config[sDaemon.index].reportIf)
				{
                    ApsDaemonAction(sDaemon.index,&config[sDaemon.index],result);
				}
				else
				{
					if(value_cache_valid[sDaemon.index]!=0)
	                {
	                    ApsDaemonAction(sDaemon.index,&config[sDaemon.index],result);
	                    value_cache[sDaemon.index] = result;
	                }
	                else
	                {
	                    value_cache_valid[sDaemon.index] = 1;
	                    value_cache[sDaemon.index] = result;
	                }
				}
            }
        }
    }
    sDaemon.index = (sDaemon.index+1)%MODULE_STATUS_ITEM_MAX; /* next round */
    MUTEX_UNLOCK(sDaemon.mutexConfig);
    
	/******************************************************************************************/
    // Report Local Log
    ApsDaemonLoopLogSync();

    // Saving Loacl Log
    ApsDaemonLoopLogSaving();
    
}

/// 状态监控初始化
void AppDaemonInit(void)
{
    U8 i=0;
    int err;

    sDaemon.init = 0;
    sDaemon.index = 0;
    sDaemon.logSyncTick = 0;
    //MUTEX_CREATE(sDaemon.mutex);
    //MUTEX_CREATE(sDaemon.mutexCustom);
    //MUTEX_CREATE(sDaemon.mutexAction);
    
	MUTEX_CREATE(sDaemon.mutexConfig);
    MUTEX_CREATE(sDaemon.mutexLog);
	initQueue(&queue_log,queue_log_buf,sizeof(APP_LOG_T)*DAEMON_LOG_QUEUE_MAX,DAEMON_LOG_QUEUE_MAX,sizeof(APP_LOG_T));
	initQueue(&queue_log_saving,queue_log_saving_buf,sizeof(APP_LOG_T)*DAEMON_LOG_SAVING_QUEUE_MAX,DAEMON_LOG_SAVING_QUEUE_MAX,sizeof(APP_LOG_T));
	
	// Load para from Flash,if failed then load the default para
    if(sDaemon.init == 0)
    {
		/******************************************************************************************/
		// Load action_mmt
        if(ApsDaemonActionMmtLoad(&action_mmt) != 0)
        {
            for(U32 i=0;i< MODULE_STATUS_ITEM_MAX;i++){
                action_mmt[i] = action_mmt_default[i];
            }
        }
		
        // Load action item para
		for(i = 0;i < MODULE_STATUS_ITEM_MAX;i++)
        {
            if(ApsDaemonActionItemParaLoad(i,&config[i]) != 0)
            {
				DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_INFO,"ActionItemPara default index=%u\r\n",i);
                ApsDaemonActionItemParaLoadDefault(i,&config[i]);
            }
        }
        
        // Load log dir control
        if(ApsDaemonLogControlParaLoad(&sDaemon.logctrl) != 0)
        {
            ApsDaemonLogControlParaLoadDefault(&sDaemon.logctrl);
        }
		// Load log config
		for(i = 0;i < LOG_ID_MAX;i++)
		{
			log_config[i].logid = i;
		    if(ApsDaemonLogIdParaLoad(&log_config[i]) != 0)
		    {
				log_config[i].logid = i;
		        ApsDaemonLogIdParaLoadDefault(&log_config[i]);
		    }
		}
		// Load load seqnum
		err = AppParaReadU32(APS_PARA_MODULE_DAEMON,DAEMON_PARA_ID_LOG_SEQNUM,&sDaemon.logSeqNum);
		if(err != 0)
		{
			DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_ERROR,"Load log seq fail,err=%d\r\n");
		    sDaemon.logSeqNum = 0;
		}
		else
		{
			DEBUG_LOG(DEBUG_MODULE_DAEMON,DLL_INFO,"Load log seq success,seq=%u\r\n",sDaemon.logSeqNum);
		}
        sDaemon.init = 1;  
    }

}


/********************************************************************************************
    Interfaces
********************************************************************************************/
/*告警和上报分开的接口函数*/

U32 ApsDaemonActiionItemIdCustom(U32 type,U32 index)
{
	U32 res;
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActiionItemIdCustomInner(type,index);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;
}
U32 ApsDaemonActionItemParaGetCustom(U32 type,U32 index,AD_CONFIG_T* code)
{
	U32 res;
	DAEMON_P_CHECK(code);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActionItemParaGetCustomInner(type,index,code);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;

}
U32 ApsDaemonActionItemParaSetCustom(U32 type,U32 index,AD_CONFIG_T* code)
{
	U32 res;
	DAEMON_P_CHECK(code);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActionItemParaSetCustomInner(type,index,code);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;
}
U32 ApsDaemonActionItemParaAddCustom(U32 type,U32* index,AD_CONFIG_T* code)
{
	U32 res;
	DAEMON_P_CHECK(code);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActionItemParaAddCustomInner(type,index,code);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;
}
U32 ApsDaemonActionItemParaDelCustom(U32 type,U32 index)
{
	U32 res;
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActionItemParaDelCustomInner(type,index);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;
}
U32 ApsDaemonActionItemParaSetDefaultCustom(U32 type)
{
	U32 res;
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActionItemParaSetDefaultCustomInner(type);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;
}
U32 ApsDaemonActionItemIndexValidityCustom(U32 type,const const unsigned char** p,unsigned int *outLen)
{
	U32 res;
	DAEMON_P_CHECK(p);
	DAEMON_P_CHECK(outLen);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActionItemIndexValidityCustomInner(type,p,outLen);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;
}

/* 告警和上报不分开的接口函数 */

U32 ApsDaemonActionItemParaSet(U32 index,AD_CONFIG_T* code)
{
	U32 res;
	DAEMON_P_CHECK(code);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActionItemParaSetInner(index,code);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;
}
U32 ApsDaemonActionItemParaGet(U32 index,AD_CONFIG_T* code)
{
	U32 res;
	DAEMON_P_CHECK(code);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActionItemParaGetInner(index,code);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;
}
U32 ApsDaemonActionItemParaAdd(U32 *index,AD_CONFIG_T* code)
{
	U32 res;
	DAEMON_P_CHECK(index);
	DAEMON_P_CHECK(code);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActionItemParaAddInner(index,code);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;
}
U32 ApsDaemonActionItemParaDel(U32 index)
{
	U32 res;
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActionItemParaDelInner(index);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;
}
U32 ApsDaemonActionItemIndexValidity(const const unsigned char** p,unsigned int *outLen)
{
	U32 res;
	DAEMON_P_CHECK(p);
	DAEMON_P_CHECK(outLen);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexConfig, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonActionItemIndexValidityInner(p,outLen);
	MUTEX_UNLOCK(sDaemon.mutexConfig);
	return res;
}


/* Log接口函数 */
/* 注意：必须在Daemon模块Loop初始化后才能调用成功 */
U32 ApsDaemonLogReport(APP_LOG_T* log)
{
	U32 res;
	DAEMON_P_CHECK(log);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexLog, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonLogReportInner(log);
	MUTEX_UNLOCK(sDaemon.mutexLog);
	return res;
}
S32 ApsDamemonLogCurrentSaveIndexGet(void)
{
	S32 res;
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexLog, MUTEX_WAIT_ALWAYS);
	res = ApsDamemonLogCurrentSaveIndexGetInner();
	MUTEX_UNLOCK(sDaemon.mutexLog);
	return res;
}
U32 ApsDaemonLogQuery(U32 saveIndex,APP_LOG_T* log) 
{
	U32 res;
	DAEMON_P_CHECK(log);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexLog, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonLogQueryInner(saveIndex,log);
	MUTEX_UNLOCK(sDaemon.mutexLog);
	return res;
}
U32 ApsDaemonLogControlParaSet(LOG_CONTROL_T* control)
{
	U32 res;
	DAEMON_P_CHECK(control);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexLog, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonLogControlParaSetInner(control);
	MUTEX_UNLOCK(sDaemon.mutexLog);
	return res;
}
U32 ApsDaemonLogControlParaGet(LOG_CONTROL_T* control)
{
	U32 res;
	DAEMON_P_CHECK(control);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexLog, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonLogControlParaGetInner(control);
	MUTEX_UNLOCK(sDaemon.mutexLog);
	return res;
}
U32 ApsDaemonLogIdParaSet(LOG_CONFIG_T* config)  
{
	U32 res;
	DAEMON_P_CHECK(config);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexLog, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonLogIdParaSetInner(config);
	MUTEX_UNLOCK(sDaemon.mutexLog);
	return res;
}
U32 ApsDaemonLogIdParaGet(LOG_CONFIG_T* config) 
{
	U32 res;
	DAEMON_P_CHECK(config);
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexLog, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonLogIdParaGetInner(config);
	MUTEX_UNLOCK(sDaemon.mutexLog);
	return res;
}
U32 ApsDaemonLogConfigSetDefault(void)
{
	U32 res;
	DAEMON_INIT_CHECK(sDaemon.init);
	MUTEX_LOCK(sDaemon.mutexLog, MUTEX_WAIT_ALWAYS);
	res = ApsDaemonLogConfigSetDefaultInner();
	MUTEX_UNLOCK(sDaemon.mutexLog);
	return res;
}






