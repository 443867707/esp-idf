/******************************************************************************
* Version     : OPP_PLATFORM V100R001C01B001D001                              *
* File        : Opp_Timer.h                                                   *
* Description :                                                               *
*               OPPLEƽ̨��ʱ������ͷ�ļ�                                     *
* Note        : (none)                                                        *
* Author      : ���ܿ����з���                                                *
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
	
/*OPP��ʱ����֧�ֵĶ�ʱ������*/
#define OPP_TIMER_MAX_NUM                             (1)
/*OPP��ʱ����֧�ֵĶ�ʱ���û��Զ���������󳤶�(��λ:Byte)*/
#define OPP_TIMER_CUSTOM_DATA_MAX_SIZE                (64)
/* Define session instance tbl item as void */
#define     ST_OPP_SESSION_INST_TBL_ITEM            void
/*ÿ���SYSTICK��*/
#define OPP_TICKS_PER_SEC                             (1000)



/******************************************************************************
*                                Typedefs                                     *
******************************************************************************/

#pragma pack(1)

/*OPPLEʱ�����Ͷ���*/
typedef struct
{
    U16 uwYear;                                    /*���(��: 2013)*/
    U8  ucMon;                                     /*�·�(1~12)*/
    U8  ucDay;                                     /*����(1~31)*/
    U8  ucWDay;                                    /*���ڼ�(1~7)*/
    U8  ucHour;                                    /*ʱ(0~23)*/
    U8  ucMin;                                     /*��(0~59)*/
    U8  ucSec;                                     /*��(0~59)*/
	U16 usMs;                                      /*����(0~999)*/
}ST_OPPLE_TIME;


/*��ʱ�������*/
typedef enum
{
    EN_OPP_TIMER_REL = 0,                          /*��Զ�ʱ��*/
    EN_OPP_TIMER_ABS,                              /*���Զ�ʱ��*/
    EN_OPP_TIMER_BUTT 
}EN_OPP_TIMER_TYPE;

/*��ʱ�����Զ���*/
typedef enum
{
    EN_OPP_TIMER_ATTR_NOLOOP = 0,                  /*һ���Զ�ʱ��*/
    EN_OPP_TIMER_ATTR_LOOP,                        /*����ѭ����ʱ��*/
    EN_OPP_TIMER_ATTR_BUTT 
}EN_OPP_TIMER_ATTR_TYPE;


/*��ʱ����ʱ�������Ͷ���*/
typedef enum
{
    EN_OPP_TIMER_CTRL_MSG = 0,                     /*��ʱ����ʱ��Ϣ֪ͨ*/
    EN_OPP_TIMER_CTRL_CALLBACK,                    /*��ʱ����ʱ�ص�����֪ͨ*/
    EN_OPP_TIMER_CTRL_BUTT 
}EN_OPP_TIMER_CTRL_TYPE;

typedef int (*pfGetRtc)(ST_OPPLE_TIME* time);
/******************************************************************************
 Function    : OPP_TIMER_TIMEOUT_CALLBACK����
 Description : OPPLE��ʱ����ʱ�ص�������
 Note        : (none)
 Input Para  : pstInst   - ���ոó�ʱ����ĻỰ����ָ��
               pucData   - ��ʱ���û����Ʋ���
               ulDataLen - ��ʱ���û����Ʋ�������
 Output Para : (none)
 Return      : OPP_SUCCESS - �ɹ�
               ����        - ʧ����
 Other       : (none)
******************************************************************************/
typedef U32 (*OPP_TIMER_TIMEOUT_CALLBACK)(ST_OPP_SESSION_INST_TBL_ITEM* pstInst,
    U8* pucData, U32 ulDataLen);


/*OPPLE��ʱ���������ṹ����*/
typedef struct
{
    U32 ulUsedFlag;                                /*ռ�ñ�־:VALID_VALUE-ռ��,����-����*/
    U32 ulTimerID;                                 /*��ʱ��ID*/
    EN_OPP_TIMER_TYPE      enTimerType;            /*��� or ����*/
    EN_OPP_TIMER_ATTR_TYPE enTimerAttr;            /*һ���� or ����ѭ��*/
    ST_OPPLE_TIME stTimePara;                      /*��ʱʱ�����*/
    ST_OPPLE_TIME stStartTime;                     /*������ʱ���ĵ�ǰʱ��*/
    EN_OPP_TIMER_CTRL_TYPE enCtrlType;             /*��ʱʱ����Ϣ֪ͨ  or ��ʱʱ�ص�����֪ͨ*/
    OPP_TIMER_TIMEOUT_CALLBACK fnTimeOutCallBack;  /*��ʱ�ص�������ָ��*/
    /*�û�������*/
    ST_OPP_SESSION_INST_TBL_ITEM* pstInst;
    U32 ulDataLen;                                 /*aucData�������ĳ���(��λ:Byte)*/
    U8  aucData[OPP_TIMER_CUSTOM_DATA_MAX_SIZE];   /*��Ϣ����(��Ӧ�ò㿪ʼ) or �ص������Ĳ�������*/
}ST_OPPLE_TIMER_INFO;


#pragma pack()



/******************************************************************************
*                           Extern Declarations                               *
******************************************************************************/
void RegOPPTimerGetRtc(pfGetRtc fun);
void OppDevTimerInit(void);
void OppDevTimerLoop(void);
void Opp_Timer_Reset(void);


/*��ȡϵͳSYSTICK�ĺ���*/
//U32 OppGetSysTick(void);
/*OPPLEƽ̨ϵͳ��ʱ����*/
//void OppTimeDlyHMSM(U32 ulHour, U32 ulMin, U32 ulSec, U32 ulMs);


/*��Ӧ�ò��ṩ��������Զ�ʱ���Ľӿ�*/
U32 OppStartRelTimer(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, U32* pulTimerID);

/*��Ӧ�ò��ṩ��������Զ�ʱ���Ľӿ�*/
U32 OppStartRelTimerCallback(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, OPP_TIMER_TIMEOUT_CALLBACK fnTimeOutCallBack, 
    U32* pulTimerID);


/*��Ӧ�ò��ṩ���������Զ�ʱ���Ľӿ�*/
U32 OppStartAbsTimer(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, U32* pulTimerID);

/*��Ӧ�ò��ṩ���������Զ�ʱ���Ľӿ�*/
U32 OppStartAbsTimerCallback(ST_OPP_SESSION_INST_TBL_ITEM* pstInst, U8* pucData, U32 ulDataLen, 
    ST_OPPLE_TIME* pstTimePara, EN_OPP_TIMER_ATTR_TYPE enTimerAttr, OPP_TIMER_TIMEOUT_CALLBACK fnTimeOutCallBack, 
    U32* pulTimerID);


/*��Ӧ�ò��ṩ�Ĺرն�ʱ���Ľӿ�*/
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


