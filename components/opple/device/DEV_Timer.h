/******************************************************************************
* Version     : OPP_PLATFORM V100R001C01B001D001                              *
* File        : Opp_Timer.h                                                   *
* Description :                                                               *
*               OPPLE平台定时器机制头文件                                     *
* Note        : (none)                                                        *
* Author      : 智能控制研发部                                                *
* Date:       : 2013-08-09                                                    *
* Mod History : (none)                                                        *
******************************************************************************/



#ifndef __OPP_DEV_TIMER_H__
#define __OPP_DEV_TIMER_H__



#ifdef __cplusplus
extern "C" {
#endif



/******************************************************************************
*                                Includes                                     *
******************************************************************************/
//#include "Opp_std.h"
#include "Typedef.h"
#include "Opp_ErrorCode.h"


/******************************************************************************
*                                Defines                                      *
******************************************************************************/
//#define OPP_SUCCESS 0
//#define OPP_FAILURE 1
//#define OPP_OVERFLOW 2
	
/*OPP定时机制支持的定时器个数*/
#define OPP_TIMER_MAX_NUM                             (1)
/*OPP定时机制支持的定时器用户自定义数据最大长度(单位:Byte)*/
#define OPP_TIMER_CUSTOM_DATA_MAX_SIZE                (64)
/* Define session instance tbl item as void */
#define     ST_OPP_SESSION_INST_TBL_ITEM            void
/*每秒的SYSTICK数*/
#define OPP_TICKS_PER_SEC                             (1000)



/******************************************************************************
*                                Typedefs                                     *
******************************************************************************/

#pragma pack(1)

/*OPPLE时间类型定义*/
typedef struct
{
    U16 uwYear;                                    /*年份(如: 2013)*/
    U8  ucMon;                                     /*月份(1~12)*/
    U8  ucDay;                                     /*日期(1~31)*/
    U8  ucWDay;                                    /*星期几(1~7)*/
    U8  ucHour;                                    /*时(0~23)*/
    U8  ucMin;                                     /*分(0~59)*/
    U8  ucSec;                                     /*秒(0~59)*/
	U16 usMs;                                      /*毫秒(0~999)*/
}ST_OPPLE_TIME;


/*定时器类别定义*/
typedef enum
{
    EN_OPP_TIMER_REL = 0,                          /*相对定时器*/
    EN_OPP_TIMER_ABS,                              /*绝对定时器*/
    EN_OPP_TIMER_BUTT 
}EN_OPP_TIMER_TYPE;

/*定时器属性定义*/
typedef enum
{
    EN_OPP_TIMER_ATTR_NOLOOP = 0,                  /*一次性定时器*/
    EN_OPP_TIMER_ATTR_LOOP,                        /*周期循环定时器*/
    EN_OPP_TIMER_ATTR_BUTT 
}EN_OPP_TIMER_ATTR_TYPE;


/*定时器超时处理类型定义*/
typedef enum
{
    EN_OPP_TIMER_CTRL_MSG = 0,                     /*定时器超时消息通知*/
    EN_OPP_TIMER_CTRL_CALLBACK,                    /*定时器超时回调函数通知*/
    EN_OPP_TIMER_CTRL_BUTT 
}EN_OPP_TIMER_CTRL_TYPE;

typedef int (*pfGetRtc)(ST_OPPLE_TIME* time);
/******************************************************************************
 Function    : OPP_TIMER_TIMEOUT_CALLBACK类型
 Description : OPPLE定时器超时回调处理函数
 Note        : (none)
 Input Para  : pstInst   - 接收该超时处理的会话事务指针
               pucData   - 定时器用户定制参数
               ulDataLen - 定时器用户定制参数长度
 Output Para : (none)
 Return      : OPP_SUCCESS - 成功
               其它        - 失败码
 Other       : (none)
******************************************************************************/
typedef U32 (*OPP_TIMER_TIMEOUT_CALLBACK)(ST_OPP_SESSION_INST_TBL_ITEM* pstInst,
    U8* pucData, U32 ulDataLen);


/*OPPLE定时器管理表项结构定义*/
typedef struct
{
    U32 ulUsedFlag;                                /*占用标志:VALID_VALUE-占用,其它-空闲*/
    U32 ulTimerID;                                 /*定时器ID*/
    EN_OPP_TIMER_TYPE      enTimerType;            /*相对 or 绝对*/
    EN_OPP_TIMER_ATTR_TYPE enTimerAttr;            /*一次性 or 周期循环*/
    ST_OPPLE_TIME stTimePara;                      /*定时时间参数*/
    ST_OPPLE_TIME stStartTime;                     /*启动定时器的当前时间*/
    EN_OPP_TIMER_CTRL_TYPE enCtrlType;             /*超时时发消息通知  or 超时时回调函数通知*/
    OPP_TIMER_TIMEOUT_CALLBACK fnTimeOutCallBack;  /*超时回调处理函数指针*/
    /*用户数据区*/
    ST_OPP_SESSION_INST_TBL_ITEM* pstInst;
    U32 ulDataLen;                                 /*aucData数据区的长度(单位:Byte)*/
    U8  aucData[OPP_TIMER_CUSTOM_DATA_MAX_SIZE];   /*消息内容(从应用层开始) or 回调函数的参数内容*/
}ST_OPPLE_TIMER_INFO;


#pragma pack()



/******************************************************************************
*                           Extern Declarations                               *
******************************************************************************/
void RegOPPTimerGetRtc(pfGetRtc fun);
void OppDevTimerInit(void);
void OppDevTimerLoop(void);
void Opp_Timer_Reset(void);


/*获取系统SYSTICK的函数*/
//U32 OppGetSysTick(void);
/*OPPLE平台系统延时函数*/
//void OppTimeDlyHMSM(U32 ulHour, U32 ulMin, U32 ulSec, U32 ulMs);


/*向应用层提供的启动相对定时器的接口*/
U32 OppStartRelTimer(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, U32* pulTimerID);

/*向应用层提供的启动相对定时器的接口*/
U32 OppStartRelTimerCallback(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, OPP_TIMER_TIMEOUT_CALLBACK fnTimeOutCallBack, 
    U32* pulTimerID);


/*向应用层提供的启动绝对定时器的接口*/
U32 OppStartAbsTimer(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, U32* pulTimerID);

/*向应用层提供的启动绝对定时器的接口*/
U32 OppStartAbsTimerCallback(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, OPP_TIMER_TIMEOUT_CALLBACK fnTimeOutCallBack, 
    U32* pulTimerID);


/*向应用层提供的关闭定时器的接口*/
U32 OppStopTimer(U32 ulTimerID);



/******************************************************************************
*                              Declarations                                   *
******************************************************************************/
U32 OppStartTimer(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, 
    U32 ulDataLen, ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_TYPE enTimerType, 
    EN_OPP_TIMER_ATTR_TYPE enTimerAttr, OPP_TIMER_TIMEOUT_CALLBACK fnTimeOutCallBack, 
    U32* pulTimerID);

U32 OPPTimerGetCurrentTime(ST_OPPLE_TIME* pstCurrTime);
void OppTickTimerTaskTmrCallback(VOID);
/*U32 OppTickTimerTask(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, 
     ST_SESSION_MSG_BASE* pstMsgBase, ST_OPP_APP_LAYER* pstMsg, U32 ulMsgLen); */
U32 OppTickTimerCheckAllTimers(VOID);
U32 OppIsTimerTimeMatch(ST_OPPLE_TIMER_INFO* pstItem, ST_OPPLE_TIME* pstCurrTime);
U32 OppTimerCheckTimeValid(ST_OPPLE_TIME* pstTime, EN_OPP_TIMER_TYPE enTimerType, 
    ST_OPPLE_TIME* pstCurrTime);
U32 OPPTimerCalcDiffSec(ST_OPPLE_TIME* pstTime1, ST_OPPLE_TIME* pstTime2, 
    U32* pulCompareFlag);
U32 OPPTimerCalcDiffSecEx(U32 ulTime1, ST_OPPLE_TIME* pstTime2, 
    U32* pulCompareFlag);




#ifdef __cplusplus
}
#endif



#endif /*__OPP_TIMER_H__*/


