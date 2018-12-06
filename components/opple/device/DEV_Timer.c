/******************************************************************************
* Version     : OPP_PLATFORM V100R001C01B001D001                              *
* File        : Opp_Timer.c                                                   *
* Description :                                                               *
*               OPPLEƽ̨��ʱ������Դ�ļ�                                     *
* Note        : (none)                                                        *
* Author      : ���ܿ����з���                                                *
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
/*OPPLE��ʱ�������*/
ST_OPPLE_TIMER_INFO g_astOppTimerInfoTbl[OPP_TIMER_MAX_NUM];

/*��ʱ����ʱ֪ͨ��Ϣ������*/
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
 Description : OPP��ʱ������ģ���ʼ��
 Note        : (none)
 Input Para  : (none)
 Output Para : (none)
 Return      : OPP_SUCCESS - �ɹ�
               ����        - ʧ����
 Other       : (none)
******************************************************************************/
U32 Opp_Timer_Init(VOID) 
{
    /*��ʼ��OPPLE��ʱ�������*/
    memset(g_astOppTimerInfoTbl, 0, sizeof(g_astOppTimerInfoTbl));

    /*ע��:���ڵδ�֪ͨ*/
    //OppRegAppSessionType(MSGID_OPP_DIDA_CLICK_TIMEOUT_MSG, OppTickTimerTask);

    return OPP_SUCCESS;
}

void Opp_Timer_Reset(void)
{
    /*��ʼ��OPPLE��ʱ�������*/
    memset(g_astOppTimerInfoTbl, 0, sizeof(g_astOppTimerInfoTbl));
}

/******************************************************************************
 Function    : OppTimeDlyHMSM
 Description : OPPƽ̨ϵͳ��ʱ�ĺ���
 Note        : (none)
 Input Para  : ulHour - Сʱ��
               ulMin  - ������
               ulSec  - ����
               ulMs   - ������
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
 Description : OPP��ʱ������ģ��OS��ʱ����ʱ�ص�����
 Note        : (none)
 Input Para  : (none)
 Output Para : (none)
 Return      : (none)
 Other       : (none)
******************************************************************************/
void OppTickTimerTaskTmrCallback(VOID)
{
    //Printf("OppTickTimerTask(): Work Per 1 second.\r\n");
    
    /*��ѯ�ж��Ƿ����㳬ʱ����*/
    OppTickTimerCheckAllTimers();
}



/******************************************************************************
 Function    : OppTickTimerTask
 Description : OPP��ʱ��������
 Note        : (none)
 Input Para  : pstInst    - ����ҵ��Ự����ʵ�������ַ
               pstMsgBase - ��Ϣ�շ���Ϣ�ṹָ��
               pstMsg     - Ӧ����Ϣ����ַ(��Ӧ�ò㿪ʼ)
               ulMsgLen   - Ӧ����Ϣ������
 Output Para : (none)
 Return      : OPP_SUCCESS - �ɹ�
               ����        - ʧ����
 Other       : Ӧ�ò��ṩ���Ự�����,Ӧ�ò���豣֤�Ự����ʱ�ͷ�
******************************************************************************/
//U32 OppTickTimerTask(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, 
//    ST_SESSION_MSG_BASE* pstMsgBase, ST_OPP_APP_LAYER* pstMsg, U32 ulMsgLen)
//{
//    Printf("OppTickTimerTask(): Work Per 1 second.\r\n");
//
//    /*��ѯ�ж��Ƿ����㳬ʱ����*/
//    OppTickTimerCheckAllTimers();
//
//    /*���������Ự����ʵ��,�ͷű��Ự����ʵ����Դ*/
//    OPP_SESSION_INST_SET_STATUS(pstInst, OPP_SESSION_INST_STATUS_EXPIRED);
//    return OPP_SUCCESS;
//}

/******************************************************************************
 Function    : OppTickTimerCheckAllTimers
 Description : OPP��ʱ�������������ж�ʱ���Ĵ���
 Note        : (none)
 Input Para  : (none)
 Output Para : (none)
 Return      : OPP_SUCCESS - �ɹ�
               ����        - ʧ����
 Other       : (none)
******************************************************************************/
U32 OppTickTimerCheckAllTimers(VOID)
{
    ST_OPPLE_TIME stCurrTime;
    ST_OPPLE_TIMER_INFO* pstItem = NULL;
    U32 ulLoop = 0;

    /*��ȡ��ǰʱ��*/
    OPPTimerGetCurrentTime(&stCurrTime);

    /*��������ÿ����ʱ���������*/
    for (ulLoop = 0; OPP_TIMER_MAX_NUM > ulLoop; ulLoop++)
    {
        pstItem = &(g_astOppTimerInfoTbl[ulLoop]);
        if (IS_INVALID_VALUE(pstItem->ulUsedFlag))
        {
            continue;
        }


        /*�ж��Ƿ�ʱ*/
        if (OPP_SUCCESS != OppIsTimerTimeMatch(pstItem, &stCurrTime))
        {
            continue;
        }


        /*��ʱ����ʱ����*/
        if (EN_OPP_TIMER_CTRL_MSG == pstItem->enCtrlType)
        {
            /* Send a loopback message to link layer */
            //OppLinkSendMsg(OppLinkGetLinkAddr(), OLAP_PROTOCOL_VERSION, pstItem->aucData, pstItem->ulDataLen);
        }
        else/* if (EN_OPP_TIMER_CTRL_CALLBACK == pstItem->enCtrlType)*/
        {
            /*���ó�ʱ�ص�����*/
            if (NULL != pstItem->fnTimeOutCallBack)
            {
                pstItem->fnTimeOutCallBack(pstItem->pstInst, pstItem->aucData, pstItem->ulDataLen);
            }
        }


        /*��ʱ������������*/
        if (EN_OPP_TIMER_ATTR_NOLOOP == pstItem->enTimerAttr)
        {
            /*һ���Զ�ʱ����ʱ��,�Զ�����*/
            memset(pstItem, 0, sizeof(*pstItem));
        }
    }

    return OPP_SUCCESS;
}

/******************************************************************************
 Function    : OppIsTimerTimeMatch
 Description : OPP��ʱ���Ƽ�鶨ʱ���Ƿ񵽴���ʱ��
 Note        : (none)
 Input Para  : pstItem     - ��ʱ���������ָ��
               pstCurrTime - ��ǰʱ��
 Output Para : (none)
 Return      : OPP_SUCCESS - ��ʱ�����ﴥ������
               OPP_FAILURE - ��ʱ��δ���ﴥ������
 Other       : (none)
******************************************************************************/
U32 OppIsTimerTimeMatch(ST_OPPLE_TIMER_INFO* pstItem, ST_OPPLE_TIME* pstCurrTime)
{
    U32 ulDiffTimeSec = 0;
    U32 ulCompareFlag = 0;

    /*��Զ�ʱ��,pstItem->stTimePara���ŵ���2�γ�ʱ֮���ʱ�䳤��*/
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
            /*��������Զ�ʱ����ʱ��,����������ʱ���ĵ�ǰʱ��*/
            memcpy(&(pstItem->stStartTime), pstCurrTime, sizeof(pstItem->stStartTime));
        }
    }
    /*���Զ�ʱ��,pstItem->stTimePara���ŵľ���ʱ��*/
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
 Description : OPP��ʱ����������Զ�ʱ��
 Note        : (none)
 Input Para  : pstInst     - �����ö�ʱ���ĻỰ����ʵ��(Ҳ�ǽ��ոö�ʱ��ʱ֪ͨ��Ϣ�ĻỰ����ʵ��)
               pucData     - ��ʱʱ��֪ͨ��Ϣ����(��Ӧ�ò㿪ʼ)��ַ
               ulDataLen   - ��ʱʱ��֪ͨ��Ϣ���ݳ���
               pstTimePara - ʱ�����(���ʱ��)
               enTimerAttr - ָ����һ���Զ�ʱ���������Զ�ʱ
 Output Para : *pulTimerID - �����ɹ���ʱ�����
 Return      : OPP_SUCCESS - �ɹ�
               ����        - ʧ����
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
 Description : OPP��ʱ����������Զ�ʱ��
 Note        : (none)
 Input Para  : pstInst           - �����ö�ʱ���ĻỰ����ʵ��(Ҳ�ǽ��ոö�ʱ��ʱ֪ͨ��Ϣ�ĻỰ����ʵ��)
               pucData           - ��ʱʱ��֪ͨ��Ϣ����(��Ӧ�ò㿪ʼ)��ַ
               ulDataLen         - ��ʱʱ��֪ͨ��Ϣ���ݳ���
               pstTimePara       - ʱ�����(���ʱ��)
               enTimerAttr       - ָ����һ���Զ�ʱ���������Զ�ʱ
               fnTimeOutCallBack - ��ʱʱ�Ļص�֪ͨ����ָ��
 Output Para : *pulTimerID - �����ɹ���ʱ�����
 Return      : OPP_SUCCESS - �ɹ�
               ����        - ʧ����
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
 Description : OPP��ʱ�����������Զ�ʱ��
 Note        : (none)
 Input Para  : pstInst     - �����ö�ʱ���ĻỰ����ʵ��(Ҳ�ǽ��ոö�ʱ��ʱ֪ͨ��Ϣ�ĻỰ����ʵ��)
               pucData     - ��ʱʱ��֪ͨ��Ϣ����(��Ӧ�ò㿪ʼ)��ַ
               ulDataLen   - ��ʱʱ��֪ͨ��Ϣ���ݳ���
               pstTimePara - ʱ�����(����ʱ��)
               enTimerAttr - ָ����һ���Զ�ʱ���������Զ�ʱ
 Output Para : *pulTimerID - �����ɹ���ʱ�����
 Return      : OPP_SUCCESS - �ɹ�
               ����        - ʧ����
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
 Description : OPP��ʱ�����������Զ�ʱ��
 Note        : (none)
 Input Para  : pstInst           - �����ö�ʱ���ĻỰ����ʵ��(Ҳ�ǽ��ոö�ʱ��ʱ֪ͨ��Ϣ�ĻỰ����ʵ��)
               pucData           - ��ʱʱ��֪ͨ��Ϣ����(��Ӧ�ò㿪ʼ)��ַ
               ulDataLen         - ��ʱʱ��֪ͨ��Ϣ���ݳ���
               pstTimePara       - ʱ�����(����ʱ��)
               enTimerAttr       - ָ����һ���Զ�ʱ���������Զ�ʱ
               fnTimeOutCallBack - ��ʱʱ�Ļص�֪ͨ����ָ��
 Output Para : *pulTimerID - �����ɹ���ʱ�����
 Return      : OPP_SUCCESS - �ɹ�
               ����        - ʧ����
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
 Description : OPP��ʱ����������ʱ��
 Note        : (none)
 Input Para  : pstInst           - �����ö�ʱ���ĻỰ����ʵ��(Ҳ�ǽ��ոö�ʱ��ʱ֪ͨ��Ϣ�ĻỰ����ʵ��)
               pucData           - ��ʱʱ��֪ͨ��Ϣ����(��Ӧ�ò㿪ʼ)��ַ
               ulDataLen         - ��ʱʱ��֪ͨ��Ϣ���ݳ���
               pstTimePara       - ��ʱʱ�����
               enTimerType       - ָ����ʱ������Զ�ʱ�����Ǿ��Զ�ʱ��
               enTimerAttr       - ָ����һ���Զ�ʱ���������Զ�ʱ
               fnTimeOutCallBack - ��ʱʱ�Ļص�֪ͨ����ָ��
 Output Para : *pulTimerID - �����ɹ���ʱ�����
 Return      : OPP_SUCCESS - �ɹ�
               ����        - ʧ����
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

    /*��ȡ��ǰʱ��*/
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

    /*���ʱ������Ϸ���*/
    if (OPP_SUCCESS != OppTimerCheckTimeValid(pstTimePara, enTimerType, &stCurrTime))
    {
        return OPP_FAILURE;
    }

    /*���ҷ���������*/
    for (ulLoop = 0; OPP_TIMER_MAX_NUM > ulLoop; ulLoop++)
    {
        pstItem = &(g_astOppTimerInfoTbl[ulLoop]);
        if (IS_VALID_VALUE(pstItem->ulUsedFlag))
        {
            continue;
        }

        /*�����¶�ʱ���������*/
        pstItem->ulUsedFlag = VALID_VALUE;
        pstItem->ulTimerID = ulLoop;
        pstItem->enTimerType = enTimerType;
        pstItem->enTimerAttr = enTimerAttr;
        memcpy(&(pstItem->stTimePara), pstTimePara, sizeof(pstItem->stTimePara));
        memcpy(&(pstItem->stStartTime), &stCurrTime, sizeof(pstItem->stStartTime));
        pstItem->fnTimeOutCallBack = fnTimeOutCallBack;  /*��ʱ�ص�������ָ��*/
        if (NULL == pstItem->fnTimeOutCallBack)
        {
            pstItem->enCtrlType = EN_OPP_TIMER_CTRL_MSG;
        }
        else
        {
            pstItem->enCtrlType = EN_OPP_TIMER_CTRL_CALLBACK;
        }
        /*�û�������*/
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
 Description : OPP��ʱ����ֹͣ��ʱ��
 Note        : (none)
 Input Para  : ulTimerID - ��ʱ�����
 Output Para : (none)
 Return      : OPP_SUCCESS - �ɹ�
               ����        - ʧ����
 Other       : �����Զ�ʱ����Ҫ�������øú����ر�
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
 Description : OPP��ʱ���ƻ�ȡ��ǰʱ��
 Note        : (none)
 Input Para  : (none)
 Output Para : pstCurrTime - ��ǰʱ��
 Return      : OPP_SUCCESS - �ɹ�
               ����        - ʧ����
 Other       : (none)
******************************************************************************/
extern U32 DS1302_GET_TIME(ST_OPPLE_TIME* time);
U32 OPPTimerGetCurrentTime(ST_OPPLE_TIME* pstCurrTime)
{
#if (1)
    /*����I2Cʱ��оƬ�����������ӿ�*/
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
 Description : OPP��ʱ���Ƽ��ʱ���Ƿ�Ϸ�
 Note        : (none)
 Input Para  : pstTime     - ʱ��ṹָ��
               enTimerType - ��ʱ������:��Ի��Ǿ���
               pstCurrTime - ��ǰʱ��
 Output Para : (none)
 Return      : OPP_SUCCESS - �Ϸ�
               OPP_FAILURE - �Ƿ�
 Other       : (none)
******************************************************************************/
U32 OppTimerCheckTimeValid(ST_OPPLE_TIME* pstTime, EN_OPP_TIMER_TYPE enTimerType, 
    ST_OPPLE_TIME* pstCurrTime)
{
    U32 ulDiffTimeSec = 0;
    U32 ulCompareFlag = 0;

    //��Զ�ʱ��
    if (EN_OPP_TIMER_REL == enTimerType)
    {
        return OPP_SUCCESS;
    }

    //���Զ�ʱ��

    //����ֵ��Чʱ
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

        /*���Զ�ʱ��,Ҫ��ָ֤���Ķ�ʱʱ��Ҫ���ڵ�ǰʱ��5������*/
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
 Description : OPP��ʱ���Ƽ���2��ʱ��֮��Ĳ�ֵ(����)
 Note        : (none)
 Input Para  : pstTime1       - ʱ��1�ṹָ��,����ʱ��1ָ����ʱ��(ʱ��)
               pstTime2       - ʱ��2�ṹָ��,����ʱ��2ָ����ʱ��(ʱ��)
 Output Para : pulCompareFlag - ʱ��1��ʱ��2�ıȽϽ��:
                   0:  ʱ��1  ==  ʱ��2
                   1:  ʱ��1  >   ʱ��2
                   2:  ʱ��1  <   ʱ��2
 Return      : ��ֵ����
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
 Description : OPP��ʱ���Ƽ���2��ʱ��֮��Ĳ�ֵ(����)
 Note        : (none)
 Input Para  : ulTime1        - ʱ��1,����ʱ��1ָʱ�䳤��(��λ:second)
               pstTime2       - ʱ��2�ṹָ��,����ʱ��2ָʱ�䳤��(��͵�λ:second)
 Output Para : pulCompareFlag - ʱ��1��ʱ��2�ıȽϽ��:
                   0:  ʱ��1����  ==  ʱ��2����
                   1:  ʱ��1����  >   ʱ��2����
                   2:  ʱ��1����  <   ʱ��2����
 Return      : ��ֵ����
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
//wantao��ʱ��
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
	/*��ʼ��OPPLE��ʱ�������*/
    memset(g_astOppTimerInfoTbl, 0, sizeof(g_astOppTimerInfoTbl));
}

void OppDevTimerLoop(void)
{
	OppTickTimerCheckAllTimers();
}



#ifdef __cplusplus
}
#endif

