/******************************************************************************
* Version     : OPP_PLATFORM V100R001C01B001D001                              *
* File        : Opp_Timer.c                                                   *
* Description :                                                               *
*               OPPLE平台定时器机制源文件                                     *
* Note        : (none)                                                        *
* Author      : 智能控制研发部                                                *
* Date:       : 2013-08-09                                                    *
* Mod History : (none)                                                        *
******************************************************************************/



#ifdef __cplusplus
extern "C" {
#endif



/******************************************************************************
*                                Includes                                     *
******************************************************************************/
#include "Includes.h"
#include "DEV_Timer.h"
#include <string.h>
#include <time.h>


/******************************************************************************
*                                Defines                                      *
******************************************************************************/



/******************************************************************************
*                                Typedefs                                     *
******************************************************************************/



/******************************************************************************
*                                Globals                                      *
******************************************************************************/
/*OPPLE定时器管理表*/
ST_OPPLE_TIMER_INFO g_astOppTimerInfoTbl[OPP_TIMER_MAX_NUM];

/*定时器超时通知消息缓冲区*/
U8 g_aucTimerNotifyMsgBuf[OPP_TIMER_CUSTOM_DATA_MAX_SIZE];


static pfGetRtc OppApsTimerGetRtcCb=0;

/******************************************************************************
*                          Extern Declarations                                *
******************************************************************************/



/******************************************************************************
*                             Declarations                                    *
******************************************************************************/



/******************************************************************************
*                            Implementations                                  *
******************************************************************************/

/******************************************************************************
 Function    : Opp_Timer_Init
 Description : OPP定时器机制模块初始化
 Note        : (none)
 Input Para  : (none)
 Output Para : (none)
 Return      : OPP_SUCCESS - 成功
               其它        - 失败码
 Other       : (none)
******************************************************************************/
U32 Opp_Timer_Init(VOID) 
{
    /*初始化OPPLE定时器管理表*/
    memset(g_astOppTimerInfoTbl, 0, sizeof(g_astOppTimerInfoTbl));

    /*注册:周期滴答通知*/
    //OppRegAppSessionType(MSGID_OPP_DIDA_CLICK_TIMEOUT_MSG, OppTickTimerTask);

    return OPP_SUCCESS;
}

void Opp_Timer_Reset(void)
{
    /*初始化OPPLE定时器管理表*/
    memset(g_astOppTimerInfoTbl, 0, sizeof(g_astOppTimerInfoTbl));
}

/******************************************************************************
 Function    : OppTimeDlyHMSM
 Description : OPP平台系统延时的函数
 Note        : (none)
 Input Para  : ulHour - 小时数
               ulMin  - 分钟数
               ulSec  - 秒数
               ulMs   - 毫秒数
 Output Para : (none)
 Return      : (none)
 Other       : (none)
******************************************************************************/
/*
VOID OppTimeDlyHMSM(U32 ulHour, U32 ulMin, U32 ulSec, U32 ulMs)
{
    U32 ulDelayTicks = 0;
    U32 ulStartTick = OppGetSysTick();

    ulDelayTicks = (ulHour * 3600L + ulMin * 60L + ulSec) * OPP_TICKS_PER_SEC
        + OPP_TICKS_PER_SEC * ulMs / 1000L;

    for (; ; )
    {
        if (ulDelayTicks <= (OppGetSysTick() - ulStartTick))
        {
            break;
        }
    }
}
*/

/******************************************************************************
 Function    : OppTickTimerTaskTmrCallback
 Description : OPP定时器机制模块OS定时器超时回调函数
 Note        : (none)
 Input Para  : (none)
 Output Para : (none)
 Return      : (none)
 Other       : (none)
******************************************************************************/
void OppTickTimerTaskTmrCallback(VOID)
{
    //Printf("OppTickTimerTask(): Work Per 1 second.\r\n");
    
    /*轮询判断是否满足超时条件*/
    OppTickTimerCheckAllTimers();
}



/******************************************************************************
 Function    : OppTickTimerTask
 Description : OPP定时机制任务
 Note        : (none)
 Input Para  : pstInst    - 本端业务会话事务实例表项地址
               pstMsgBase - 消息收发信息结构指针
               pstMsg     - 应用消息包地址(从应用层开始)
               ulMsgLen   - 应用消息包长度
 Output Para : (none)
 Return      : OPP_SUCCESS - 成功
               其它        - 失败码
 Other       : 应用层提供给会话层调用,应用层必需保证会话事务及时释放
******************************************************************************/
//U32 OppTickTimerTask(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, 
//    ST_SESSION_MSG_BASE* pstMsgBase, ST_OPP_APP_LAYER* pstMsg, U32 ulMsgLen)
//{
//    Printf("OppTickTimerTask(): Work Per 1 second.\r\n");
//
//    /*轮询判断是否满足超时条件*/
//    OppTickTimerCheckAllTimers();
//
//    /*结束掉本会话事务实例,释放本会话事务实例资源*/
//    OPP_SESSION_INST_SET_STATUS(pstInst, OPP_SESSION_INST_STATUS_EXPIRED);
//    return OPP_SUCCESS;
//}

/******************************************************************************
 Function    : OppTickTimerCheckAllTimers
 Description : OPP定时机制任务检查所有定时器的处理
 Note        : (none)
 Input Para  : (none)
 Output Para : (none)
 Return      : OPP_SUCCESS - 成功
               其它        - 失败码
 Other       : (none)
******************************************************************************/
U32 OppTickTimerCheckAllTimers(VOID)
{
    ST_OPPLE_TIME stCurrTime;
    ST_OPPLE_TIMER_INFO* pstItem = NULL;
    U32 ulLoop = 0;

    /*获取当前时间*/
    OPPTimerGetCurrentTime(&stCurrTime);

    /*遍历处理每个定时器管理表项*/
    for (ulLoop = 0; OPP_TIMER_MAX_NUM > ulLoop; ulLoop++)
    {
        pstItem = &(g_astOppTimerInfoTbl[ulLoop]);
        if (IS_INVALID_VALUE(pstItem->ulUsedFlag))
        {
            continue;
        }


        /*判断是否超时*/
        if (OPP_SUCCESS != OppIsTimerTimeMatch(pstItem, &stCurrTime))
        {
            continue;
        }


        /*定时器超时处理*/
        if (EN_OPP_TIMER_CTRL_MSG == pstItem->enCtrlType)
        {
            /* Send a loopback message to link layer */
            //OppLinkSendMsg(OppLinkGetLinkAddr(), OLAP_PROTOCOL_VERSION, pstItem->aucData, pstItem->ulDataLen);
        }
        else/* if (EN_OPP_TIMER_CTRL_CALLBACK == pstItem->enCtrlType)*/
        {
            /*调用超时回调函数*/
            if (NULL != pstItem->fnTimeOutCallBack)
            {
                pstItem->fnTimeOutCallBack(pstItem->pstInst, pstItem->aucData, pstItem->ulDataLen);
            }
        }


        /*定时器管理表项后处理*/
        if (EN_OPP_TIMER_ATTR_NOLOOP == pstItem->enTimerAttr)
        {
            /*一次性定时器超时后,自动回收*/
            memset(pstItem, 0, sizeof(*pstItem));
        }
    }

    return OPP_SUCCESS;
}

/******************************************************************************
 Function    : OppIsTimerTimeMatch
 Description : OPP定时机制检查定时器是否到触发时间
 Note        : (none)
 Input Para  : pstItem     - 定时器管理表项指针
               pstCurrTime - 当前时间
 Output Para : (none)
 Return      : OPP_SUCCESS - 定时器到达触发条件
               OPP_FAILURE - 定时器未到达触发条件
 Other       : (none)
******************************************************************************/
U32 OppIsTimerTimeMatch(ST_OPPLE_TIMER_INFO* pstItem, ST_OPPLE_TIME* pstCurrTime)
{
    U32 ulDiffTimeSec = 0;
    U32 ulCompareFlag = 0;

    /*相对定时器,pstItem->stTimePara里存放的是2次超时之间的时间长度*/
    if (EN_OPP_TIMER_REL == pstItem->enTimerType)
    {
        ulDiffTimeSec = OPPTimerCalcDiffSec(pstCurrTime, &(pstItem->stStartTime), &ulCompareFlag);
        ulDiffTimeSec = OPPTimerCalcDiffSecEx(ulDiffTimeSec, &(pstItem->stTimePara), &ulCompareFlag);
        if (2 == ulCompareFlag)
        {
            return OPP_FAILURE;
        }

        if (EN_OPP_TIMER_ATTR_LOOP == pstItem->enTimerAttr)
        {
            /*周期性相对定时器超时后,更新启动定时器的当前时间*/
            memcpy(&(pstItem->stStartTime), pstCurrTime, sizeof(pstItem->stStartTime));
        }
    }
    /*绝对定时器,pstItem->stTimePara里存放的绝对时间*/
    else/* if (EN_OPP_TIMER_ABS == pstItem->enTimerType)*/
    {
        if ((1 <= pstItem->stTimePara.ucWDay) && (7 >= pstItem->stTimePara.ucWDay))
        {
            if (pstItem->stTimePara.ucWDay != pstCurrTime->ucWDay)
            {
                return OPP_FAILURE;
            }

            pstItem->stTimePara.uwYear = pstCurrTime->uwYear;
            pstItem->stTimePara.ucMon = pstCurrTime->ucMon;
            pstItem->stTimePara.ucDay = pstCurrTime->ucDay;
        }

        ulDiffTimeSec = OPPTimerCalcDiffSec(pstCurrTime, &(pstItem->stTimePara), &ulCompareFlag);

        if (3 < ulDiffTimeSec)
        {
            return OPP_FAILURE;
        }
    }

    return OPP_SUCCESS;
}


/******************************************************************************
 Function    : OppStartRelTimer
 Description : OPP定时机制启动相对定时器
 Note        : (none)
 Input Para  : pstInst     - 启动该定时器的会话事务实例(也是接收该定时超时通知消息的会话事务实例)
               pucData     - 超时时的通知消息内容(从应用层开始)地址
               ulDataLen   - 超时时的通知消息内容长度
               pstTimePara - 时间参数(相对时间)
               enTimerAttr - 指定是一次性定时还是周期性定时
 Output Para : *pulTimerID - 启动成功后定时器句柄
 Return      : OPP_SUCCESS - 成功
               其它        - 失败码
 Other       : (none)
******************************************************************************/
U32 OppStartRelTimer(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, U32* pulTimerID)
{
    return OppStartTimer(pstInst, pucData, ulDataLen, pstTimePara, EN_OPP_TIMER_REL, 
        enTimerAttr, NULL, pulTimerID);
}

/******************************************************************************
 Function    : OppStartRelTimer
 Description : OPP定时机制启动相对定时器
 Note        : (none)
 Input Para  : pstInst           - 启动该定时器的会话事务实例(也是接收该定时超时通知消息的会话事务实例)
               pucData           - 超时时的通知消息内容(从应用层开始)地址
               ulDataLen         - 超时时的通知消息内容长度
               pstTimePara       - 时间参数(相对时间)
               enTimerAttr       - 指定是一次性定时还是周期性定时
               fnTimeOutCallBack - 超时时的回调通知函数指针
 Output Para : *pulTimerID - 启动成功后定时器句柄
 Return      : OPP_SUCCESS - 成功
               其它        - 失败码
 Other       : (none)
******************************************************************************/
U32 OppStartRelTimerCallback(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, OPP_TIMER_TIMEOUT_CALLBACK fnTimeOutCallBack, 
    U32* pulTimerID)
{
    if (NULL == fnTimeOutCallBack)
    {
        return OPP_FAILURE;
    }

    return OppStartTimer(pstInst, pucData, ulDataLen, pstTimePara, EN_OPP_TIMER_REL, 
        enTimerAttr, fnTimeOutCallBack, pulTimerID);
}


/******************************************************************************
 Function    : OppStartAbsTimer
 Description : OPP定时机制启动绝对定时器
 Note        : (none)
 Input Para  : pstInst     - 启动该定时器的会话事务实例(也是接收该定时超时通知消息的会话事务实例)
               pucData     - 超时时的通知消息内容(从应用层开始)地址
               ulDataLen   - 超时时的通知消息内容长度
               pstTimePara - 时间参数(绝对时间)
               enTimerAttr - 指定是一次性定时还是周期性定时
 Output Para : *pulTimerID - 启动成功后定时器句柄
 Return      : OPP_SUCCESS - 成功
               其它        - 失败码
 Other       : (none)
******************************************************************************/
U32 OppStartAbsTimer(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, U32* pulTimerID)
{
    return OppStartTimer(pstInst, pucData, ulDataLen, pstTimePara, EN_OPP_TIMER_ABS, 
        enTimerAttr, NULL, pulTimerID);
}

/******************************************************************************
 Function    : OppStartAbsTimer
 Description : OPP定时机制启动绝对定时器
 Note        : (none)
 Input Para  : pstInst           - 启动该定时器的会话事务实例(也是接收该定时超时通知消息的会话事务实例)
               pucData           - 超时时的通知消息内容(从应用层开始)地址
               ulDataLen         - 超时时的通知消息内容长度
               pstTimePara       - 时间参数(绝对时间)
               enTimerAttr       - 指定是一次性定时还是周期性定时
               fnTimeOutCallBack - 超时时的回调通知函数指针
 Output Para : *pulTimerID - 启动成功后定时器句柄
 Return      : OPP_SUCCESS - 成功
               其它        - 失败码
 Other       : (none)
******************************************************************************/
U32 OppStartAbsTimerCallback(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, OPP_TIMER_TIMEOUT_CALLBACK fnTimeOutCallBack, 
    U32* pulTimerID)
{
    if (NULL == fnTimeOutCallBack)
    {
        return OPP_FAILURE;
    }

    return OppStartTimer(pstInst, pucData, ulDataLen, pstTimePara, EN_OPP_TIMER_ABS, 
        enTimerAttr, fnTimeOutCallBack, pulTimerID);
}

/******************************************************************************
 Function    : OppStartTimer
 Description : OPP定时机制启动定时器
 Note        : (none)
 Input Para  : pstInst           - 启动该定时器的会话事务实例(也是接收该定时超时通知消息的会话事务实例)
               pucData           - 超时时的通知消息内容(从应用层开始)地址
               ulDataLen         - 超时时的通知消息内容长度
               pstTimePara       - 定时时间参数
               enTimerType       - 指定定时器是相对定时器还是绝对定时器
               enTimerAttr       - 指定是一次性定时还是周期性定时
               fnTimeOutCallBack - 超时时的回调通知函数指针
 Output Para : *pulTimerID - 启动成功后定时器句柄
 Return      : OPP_SUCCESS - 成功
               其它        - 失败码
 Other       : (none)
******************************************************************************/
U32 OppStartTimer(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, 
    U32 ulDataLen, ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_TYPE enTimerType, 
    EN_OPP_TIMER_ATTR_TYPE enTimerAttr, OPP_TIMER_TIMEOUT_CALLBACK fnTimeOutCallBack, 
    U32* pulTimerID)
{
    ST_OPPLE_TIME stCurrTime;
    ST_OPPLE_TIMER_INFO* pstItem = NULL;
    U32 ulLoop = 0;

    /*获取当前时间*/
    OPPTimerGetCurrentTime(&stCurrTime);

    if ((EN_OPP_TIMER_BUTT <= enTimerType) || (EN_OPP_TIMER_ATTR_BUTT <= enTimerAttr))
    {
        return OPP_FAILURE;
    }
    if (/*(NULL == pstInst) || */(NULL == pucData) || (NULL == pstTimePara))
    {
        return OPP_FAILURE;
    }
    if ((OPP_TIMER_CUSTOM_DATA_MAX_SIZE /*- sizeof(ST_OPP_SESSION_LAYER)*/) < ulDataLen)
    {
        return OPP_FAILURE;
    }

    /*检查时间参数合法性*/
    if (OPP_SUCCESS != OppTimerCheckTimeValid(pstTimePara, enTimerType, &stCurrTime))
    {
        return OPP_FAILURE;
    }

    /*查找分配管理表项*/
    for (ulLoop = 0; OPP_TIMER_MAX_NUM > ulLoop; ulLoop++)
    {
        pstItem = &(g_astOppTimerInfoTbl[ulLoop]);
        if (IS_VALID_VALUE(pstItem->ulUsedFlag))
        {
            continue;
        }

        /*分配新定时器管理表项*/
        pstItem->ulUsedFlag = VALID_VALUE;
        pstItem->ulTimerID = ulLoop;
        pstItem->enTimerType = enTimerType;
        pstItem->enTimerAttr = enTimerAttr;
        memcpy(&(pstItem->stTimePara), pstTimePara, sizeof(pstItem->stTimePara));
        memcpy(&(pstItem->stStartTime), &stCurrTime, sizeof(pstItem->stStartTime));
        pstItem->fnTimeOutCallBack = fnTimeOutCallBack;  /*超时回调处理函数指针*/
        if (NULL == pstItem->fnTimeOutCallBack)
        {
            pstItem->enCtrlType = EN_OPP_TIMER_CTRL_MSG;
        }
        else
        {
            pstItem->enCtrlType = EN_OPP_TIMER_CTRL_CALLBACK;
        }
        /*用户数据区*/
        pstItem->pstInst = pstInst;
        pstItem->ulDataLen = ulDataLen;
        if (0 < ulDataLen)
        {
            memcpy(pstItem->aucData, pucData, ulDataLen);
        }

        if (NULL != pulTimerID)
        {
            *pulTimerID = ulLoop;
        }
        return OPP_SUCCESS;
    }

    return OPP_OVERFLOW;
}


/******************************************************************************
 Function    : OppStopTimer
 Description : OPP定时机制停止定时器
 Note        : (none)
 Input Para  : ulTimerID - 定时器句柄
 Output Para : (none)
 Return      : OPP_SUCCESS - 成功
               其它        - 失败码
 Other       : 周期性定时器需要主动调用该函数关闭
******************************************************************************/
U32 OppStopTimer(U32 ulTimerID)
{
    if (OPP_TIMER_MAX_NUM <= ulTimerID)
    {
        return OPP_FAILURE;
    }

    if (IS_INVALID_VALUE(g_astOppTimerInfoTbl[ulTimerID].ulUsedFlag))
    {
        return OPP_SUCCESS;
    }

    memset(&g_astOppTimerInfoTbl[ulTimerID], 0, sizeof(g_astOppTimerInfoTbl[ulTimerID]));
    return OPP_SUCCESS;
}

/******************************************************************************
 Function    : OPPTimerGetCurrentTime
 Description : OPP定时机制获取当前时间
 Note        : (none)
 Input Para  : (none)
 Output Para : pstCurrTime - 当前时间
 Return      : OPP_SUCCESS - 成功
               其它        - 失败码
 Other       : (none)
******************************************************************************/
extern U32 DS1302_GET_TIME(ST_OPPLE_TIME* time);
U32 OPPTimerGetCurrentTime(ST_OPPLE_TIME* pstCurrTime)
{
#if (1)
    /*调用I2C时间芯片的驱动函数接口*/
    // return DS1302_GET_TIME(pstCurrTime);
    return OppApsTimerGetRtcCb(pstCurrTime);
#else
    pstCurrTime->uwYear = 2013;
    pstCurrTime->ucMon = 8;
    pstCurrTime->ucDay = 30;
    pstCurrTime->ucWDay = 5;
    pstCurrTime->ucHour = 9;
    pstCurrTime->ucMin = 45;
    pstCurrTime->ucSec = 10;
    return OPP_SUCCESS;
#endif
}


/******************************************************************************
 Function    : OppTimerCheckTimeValid
 Description : OPP定时机制检查时间是否合法
 Note        : (none)
 Input Para  : pstTime     - 时间结构指针
               enTimerType - 定时器类型:相对还是绝对
               pstCurrTime - 当前时间
 Output Para : (none)
 Return      : OPP_SUCCESS - 合法
               OPP_FAILURE - 非法
 Other       : (none)
******************************************************************************/
U32 OppTimerCheckTimeValid(ST_OPPLE_TIME* pstTime, EN_OPP_TIMER_TYPE enTimerType, 
    ST_OPPLE_TIME* pstCurrTime)
{
    U32 ulDiffTimeSec = 0;
    U32 ulCompareFlag = 0;

    //相对定时器
    if (EN_OPP_TIMER_REL == enTimerType)
    {
        return OPP_SUCCESS;
    }

    //绝对定时器

    //星期值无效时
    if ((1 > pstTime->ucWDay) || (7 < pstTime->ucWDay))
    {
        if ((1 > pstTime->ucMon) || (12 < pstTime->ucMon))
        {
            return OPP_FAILURE;
        }
        if ((1 > pstTime->ucDay) || (31 < pstTime->ucDay))
        {
            return OPP_FAILURE;
        }

        /*绝对定时器,要保证指定的定时时间要大于当前时间5秒以上*/
        ulDiffTimeSec = OPPTimerCalcDiffSec(pstTime, pstCurrTime, &ulCompareFlag);
        if ((0 == ulCompareFlag) || (2 == ulCompareFlag) || (5 > ulDiffTimeSec))
        {
            return OPP_FAILURE;
        }
    }

    if (23 < pstTime->ucHour)
    {
        return OPP_FAILURE;
    }
    if (59 < pstTime->ucMin)
    {
        return OPP_FAILURE;
    }
    if (59 < pstTime->ucSec)
    {
        return OPP_FAILURE;
    }

    return OPP_SUCCESS;
}


/******************************************************************************
 Function    : OPPTimerCalcDiffSec
 Description : OPP定时机制计算2个时间之间的差值(秒数)
 Note        : (none)
 Input Para  : pstTime1       - 时间1结构指针,这里时间1指绝对时间(时刻)
               pstTime2       - 时间2结构指针,这里时间2指绝对时间(时刻)
 Output Para : pulCompareFlag - 时间1和时间2的比较结果:
                   0:  时间1  ==  时间2
                   1:  时间1  >   时间2
                   2:  时间1  <   时间2
 Return      : 差值秒数
 Other       : (none)
******************************************************************************/
U32 OPPTimerCalcDiffSec(ST_OPPLE_TIME* pstTime1, ST_OPPLE_TIME* pstTime2, 
    U32* pulCompareFlag)
{
    struct tm stTime;
    time_t time1 = 0;
    time_t time2 = 0;

    memset(&stTime, 0, sizeof(stTime));

    stTime.tm_year = (int)(pstTime1->uwYear - 1900);
    stTime.tm_mon = (int)(pstTime1->ucMon - 1);
    stTime.tm_mday = (int)pstTime1->ucDay;
    stTime.tm_hour = (int)pstTime1->ucHour;
    stTime.tm_min = (int)pstTime1->ucMin;
    stTime.tm_sec = (int)pstTime1->ucSec;
    time1 = mktime(&stTime);

    stTime.tm_year = (int)(pstTime2->uwYear - 1900);
    stTime.tm_mon = (int)(pstTime2->ucMon - 1);
    stTime.tm_mday = (int)pstTime2->ucDay;
    stTime.tm_hour = (int)pstTime2->ucHour;
    stTime.tm_min = (int)pstTime2->ucMin;
    stTime.tm_sec = (int)pstTime2->ucSec;
    time2 = mktime(&stTime);

    if (time1 == time2)
    {
        *pulCompareFlag = 0;
        return 0;
    }

    if (time1 > time2)
    {
        *pulCompareFlag = 1;
        return (time1 - time2);
    }

    *pulCompareFlag = 2;
    return (time2 - time1);
}

/******************************************************************************
 Function    : OPPTimerCalcDiffSecEx
 Description : OPP定时机制计算2个时间之间的差值(秒数)
 Note        : (none)
 Input Para  : ulTime1        - 时间1,这里时间1指时间长度(单位:second)
               pstTime2       - 时间2结构指针,这里时间2指时间长度(最低单位:second)
 Output Para : pulCompareFlag - 时间1和时间2的比较结果:
                   0:  时间1长度  ==  时间2长度
                   1:  时间1长度  >   时间2长度
                   2:  时间1长度  <   时间2长度
 Return      : 差值秒数
 Other       : (none)
******************************************************************************/
U32 OPPTimerCalcDiffSecEx(U32 ulTime1, ST_OPPLE_TIME* pstTime2, 
    U32* pulCompareFlag)
{
    U32 ulTime2 = 0;

    ulTime2 += 365 * pstTime2->uwYear * 24 * 3600;
    ulTime2 += 30 * pstTime2->ucMon * 24 * 3600;
    ulTime2 += pstTime2->ucDay * 24 * 3600;
    ulTime2 += pstTime2->ucHour * 3600;
    ulTime2 += pstTime2->ucMin * 60;
    ulTime2 += pstTime2->ucSec;

    if (ulTime1 == ulTime2)
    {
        *pulCompareFlag = 0;
        return 0;
    }

    if (ulTime1 > ulTime2)
    {
        *pulCompareFlag = 1;
        return (ulTime1 - ulTime2);
    }

    *pulCompareFlag = 2;
    return (ulTime2 - ulTime1);
}
#if 0
//wantao定时器
U64 OppTickCountGet(void)
{
	U64 tick;
	tick = xTaskGetTickCount;
	return tick;
	//OppTickCountGet();
}
#endif
void RegOPPTimerGetRtc(pfGetRtc fun)
{
	OppApsTimerGetRtcCb = fun;
}

void OppDevTimerInit(void)
{
	/*初始化OPPLE定时器管理表*/
    memset(g_astOppTimerInfoTbl, 0, sizeof(g_astOppTimerInfoTbl));
}

void OppDevTimerLoop(void)
{
	OppTickTimerCheckAllTimers();
}



#ifdef __cplusplus
}
#endif

