#ifndef _APP_LAMPCTRL_H
#define _APP_LAMPCTRL_H

#include <DRV_Bc28.h>
#include <SVS_Time.h>
/******************************************************************************
*                                Defines                                      *
******************************************************************************/
#define DEBUG
#define JOB_MAX             /*0x03*/0x06
#define PLAN_MAX            /*0x03*/0x08

#define INVALID_SCN_NUM   (-1)
//���ƾߵķ�����(��)���
#define OPP_LAMP_GROUP_MAX_NUM         (1)

//���ƾߵĳ�����(��)���
#define OPP_LAMP_SCENE_MAX_NUM         (10)

#define OPP_MIN_VAL(a, b)              (((a) > (b)) ? (b) : (a))
#define OPP_MAX_VAL(a, b)              (((a) < (b)) ? (b) : (a))


#define OPP_APP_INC(M, A, a)           \
    do {\
        if ((M) <= ((A) + (a)))\
        {\
            (A) = (M);\
        }\
        else\
        {\
            (A) += (a);\
        }\
    }while(0)

#define OPP_APP_DEC(A, a)              \
    do {\
        if ((A) <= (a))\
        {\
            (A) = 0;\
        }\
        else\
        {\
            (A) -= (a);\
        }\
    }while(0)


#define IS_EQUAL_OF_TWO_POINTS(a, b)   (((a).cx == (b)->cx) && ((a).cy == (b)->cy))

//PWM���ڵ�Ƶ��(��λ:Hz)
//#define OPP_LAMP_PWMPERIOD_FREQ_HZ     (3000)
#define OPP_LAMP_PWMPERIOD_FREQ_HZ     (200)
#define PRODUCTNAME                    "ROADLAMP"
#define PROTID                         "1.0"
#define MODULE                         "NBIOT"
#define MANU                           "OPPLE"

//�������ݵİ汾��
#define OPP_LAMP_CTRL_CFG_DATA_VER     (786)
#define OPP_LAMP_CTRL_CFG_DATA_VER_STR "786"
#define OPP_LAMP_CTRL_CFG_DATA_MAX_VER  (0xFFFF)
        
#define PRODUCTCLASS                 0xF011                
#define PRODUCTSKU                   0x0085
/*ɫ�²���*/
#define  WARMINTERVAL     (4600-2700)
#define  BRIGHTINTERVAL    (255-10)
#define  COOLINTERVAL      (6500-4600)
#define  WARMMAXRATIO     (5.0)
#define  COOLMAXRATIO    (1.0)
#define  APPMINCCTVALUE     2700
#define  APPMAXCCTVALUE     6500
#define  BRIGHTESTCCTVALUE     4600
#define  BRIGHTFRONTCCTVALUE  4400
#define  BRIGHTENDCCTVALUE      4800
#define  APPMINBRIGHTVALUE   10
#define  APPMAXBRIGHTVALUE   255
#define  PWMMINRATIO      0.3
#define  PWMMAXPRTIO      1.2

#define  CUTOFFLINERATION   (0.26)

#define LAMPCONFIG_ID     0
#define WORKSCHEME_ID     1
#define RUNTIME_ID        2
#define INVALIDE_JOBID    0
#define INVALIDE_PLANID    0
#define MAX_RUNTIME       0x7FFFFFFF
#define STARTUP_TIME    20000
#define READPOWER_PERIOD    60000
#define MAX_LIGHT_TIME    (16777215)
#define ACTTIME_LEN        18
//100mW
#define EXEP_LOW_PWR        35
//100mW
#define EXEP_HIGH_PWR       40
#define MAX_LAMP_PWR        300

//��ȡ��λ��
#define OPP_HUNDRED_OF_NUMBER(x)                    ((x) / 100)
//��ȡʮλ��
#define OPP_DECADE_OF_NUMBER(x)                     (((x) % 100) / 10)
//��ȡ��λ��
#define OPP_UNIT_OF_NUMBER(x)                       ((x) % 10)

#pragma pack(1)

typedef enum
{
	DIM_010_OUT = 0,	
	DIM_PWM_OUT,
	DIM_NONE,
}EN_DIM_TYPE;

/*ŷ���豸��Ϣ�ṹ����*/
typedef struct
{
    U16 uwAgentFlag;                     /*�����ʶ:0-������,����-�������ʵ��*/
	U8 productName[24];					 /*��Ʒ����*/
	U8 protId[6];						 /*HUAWEI Э���*/
	U8 model[20];                        /*�豸�ͺ�*/
	U8 manu[10];						 /*�豸������*/
    U16 uwProductClass;                  /*��Ʒ����,ȡֵ��Χ:Opp_ProductClass.h*/
    U16 uwProductSku;                    /*��Ʒ�ͺ�,ȡֵ��Χ:Opp_ProductSku.h*/
    U16 uwSourceType;                    /*��Դ����,ȡֵ��Χ:Opp_SourceType.h*/
    U8 aucObjectSN[16];                   /*��Ʒ���кź���(������Կ)*/
    U16 uwObjectType;                    /*IOT ObjectType*/
    U32 ObjectIDL;                       /*IOT ID Low*/
    U32 ObjectIDH;                       /*IOT ID High*/
    U32 ulVersion;                       /*�����豸������汾��*/
    U32 ulLocalIP;                       /*����������IP*/
    U16 uwLocalPort;                     /*���ط���UDP�˿�*/
    U8 aucDesc[16];                      /*�豸������Ϣ*/
    U8 aucOpPwd[16];                     /*��Ʒ��������*/	
	U8 ucHasRelay;						 /*���޼̵���*/
	EN_DIM_TYPE enDimType;               /*���ⷽʽ*/
}ST_OPP_OBJ_INFO;

/*ɫ�����ʾ�ṹ����*/
typedef struct
{
    FLOAT cx;                            //��ǰ��ɫ��ɫ����xֵ
    FLOAT cy;                            //��ǰ��ɫ��ɫ����yֵ
}ST_OPP_COLOR_CXY;

/*ֱ��б�ʽṹ����*/
typedef struct
{
    FLOAT fSlope;                        /*ֱ��б��*/
    U8 ucSlopeFlag;                      /*б��������־:0-б������,1-б�������*/
}ST_OPP_SLOPE;

/*ɫ����ֱ���߶νṹ����*/
typedef struct
{
    ST_OPP_COLOR_CXY Pt1;                //�߶���ʼ��ɫ����x,yֵ
    ST_OPP_COLOR_CXY Pt2;                //�߶ν�����ɫ����x,yֵ
    ST_OPP_SLOPE stSlope;                //ֱ��б��
}ST_OPP_COLOR_LINE;

/*��ɫ����ά��ö�ٶ���*/
typedef enum
{
    EN_OPP_COLOR_RED = 0, 
    EN_OPP_COLOR_GREEN,
    EN_OPP_COLOR_BLUE,
    EN_OPP_COLOR_WHITE,
    EN_OPP_COLOR_BUTT
}EN_OPP_COLOR_NAME;

/*������Ʒ�ʽ*/
typedef enum
{
    EN_OPP_CTRL_TYPE_RGB = 0, 
    EN_OPP_CTRL_TYPE_CCT, 
    EN_OPP_CTRL_TYPE_HSB, 
    EN_OPP_CTRL_TYPE_CXY, 
    EN_OPP_CTRL_TYPE_PWM, 
    EN_OPP_CTRL_TYPE_BRI,
    EN_OPP_CTRL_TYPE_SHOW,
    EN_OPP_CTRL_TYPE_BUTT
}EN_OPP_CTRL_TYPE;



/*���ƾߵ�ǰ״̬�ṹ����*/
typedef struct
{
    //U8  ucSwitch;                        /*����״̬:0-��,����-��*/
    //U16  usBri;                           /*0-��, ����(0~255)-����ʱ�����ȵȼ�*/ 
	U32 uwRunTime;
	U32 uwHisTime;
	U32 uwLightTime;
	U32 uwHisLightTime;
#if 0	
    EN_OPP_CTRL_TYPE enCtrlType;         /*������Ʒ�ʽ*/
    U8  aucHue[3];                       /*��ɫ(R-G-B)*/  
    U16 uwCCT;                           /*ɫ��ֵ(K)*/
    U16 uwHue;                           /*ɫ��:0~360*/ 
    U8  ucSat;                           /*���Ͷ�:0~255*/
    ST_OPP_COLOR_CXY stTargetCxy;        /*Ŀ��(��ǰ)���ɫ����*/
    U32 ulPwmFreq;                       /*PWM���ڵ�Ƶ��(Hz)*/
    U32 aulCurrPwms[EN_OPP_COLOR_BUTT];  /*Ŀ��(��ǰ)��Ŀ������PWMֵ*/
#endif	
}ST_OPP_LAMP_CURR_STATUS;

/*���ƾ߳����ṹ����*/
typedef struct
{
    U8 ucInvalidFlag;                    /*��Ч��־:0-��Ч,����-��Ч*/
    U8 aucSceneName[16];                 /*��������*/
    ST_OPP_LAMP_CURR_STATUS stStatus;    /*�ƾ��ڸó����µı���״̬*/
}ST_OPP_LAMP_SCENE_ITEM;


/*��ԭɫ���������͵�ö�ٶ���*/
typedef enum
{
    EN_OPP_TRIANG_RGB = 0, 
    EN_OPP_TRIANG_RGW, 
    EN_OPP_TRIANG_WGB, 
    EN_OPP_TRIANG_RWB, 
    EN_OPP_TRIANG_BUTT
}EN_OPP_TRIANG_NAME;


/*��ɫ����ά����Ϣ�ṹ����*/
typedef struct
{
    ST_OPP_COLOR_CXY stCxy;              /*��ɫ���ɫ����x,yֵ*/
    FLOAT fLightEffect;                  /*����ɫ����ͨ·�Ĺ�Ч(��λ:lm/W)*/
    FLOAT fCurrentLimit;                 /*����ɫ����ͨ·�ķ�ֵ����(��λ:A,����)*/
    FLOAT fVoltageForward;               /*����ɫ����ͨ·��ǰ���ѹ(��λ:V,����)*/
    FLOAT fTotalLmLimit;                 /*����ɫ����ͨ·����������ͨ��(��λ:lm,����),��ĳ�����¶��µ���������ͨ��FluxPeak@Tc*/
}ST_OPP_CTRL_ITEM;

/*���ƾ���Ϣ�ṹ����*/
typedef struct
{
    //���ƾߵĶ�����Ϣ
    ST_OPP_OBJ_INFO stObjInfo;
    //���ƾߵķ�����Ϣ
    U32 ulGroupNum; 
    U32 aulGroupItem[OPP_LAMP_GROUP_MAX_NUM];
    //�ο���R[0]G[1]B[2]W[3]��ɫ����
    ST_OPP_CTRL_ITEM astRefPoint[EN_OPP_COLOR_BUTT];
    //����㷨ѡ��:0-RGB��ɫ���,1-��ɫ�������ɫ���
    U32 ulAlgorithm;
    //RGB��⸨�����ɫ����
    ST_OPP_COLOR_CXY stAssistCxy;
	//λ����Ϣ
	//ST_OPP_LAMP_LOCATION stLocation;
	//��ǰ������Ϣ
	//ST_OPP_LAMP_CURR_ELECTRIC_INFO	stCurrElecric;
	//NBIOT�豸״̬
	ST_NEUL_DEV stNeulSignal;
    //���ƾߵĵ�ǰ״̬
    ST_OPP_LAMP_CURR_STATUS stCurrStatus;//���ƾߵĵ�ǰ״̬
    //���ƾߵĳ�����Ϣ
    ST_OPP_LAMP_SCENE_ITEM astSceneItem[OPP_LAMP_SCENE_MAX_NUM];
}ST_OPP_LAMP_INFO;


typedef struct
{
	/*S32 rsrp;
	S32 rsrq;
	*/
	U32 rsrp;
	U32 rsrq;
	U32 signalEcl;
	U32 cellId;
	U32 signalPCI;
	U32 snr;
}ST_NBIOT_SIGNAL;

//config parameter
typedef struct
{
	//char ocIp[16]; /*IP��ַxxx.xxx.xxx.xxx*/
	//U16 ocPort;
	//U8 band;
	//U8 scram; /*����*/	
	U32 period; /*����*/
	//U32 hisTime; /*��ʷʱ��*/
	//U32 hisConsumption; /*��ʷ����*/
	U8 dimType;
	//char apn[128]; /*����*/
	U32 plansNum;
	U32 hbPeriod; /*��λs*/
}ST_CONFIG_PARA;

typedef struct
{
	U32 hisTime; /*��ʷʱ��*/
}ST_RUNTIME_CONFIG;
////////////plan/////////////
typedef enum
{
	AST_MODE = 0,
	YMD_MODE,
	WEEK_MODE,
	SENSOR_MODE,
	NONE_MODE
}EN_PLAN_MODE;

typedef struct
{
	U8 index;
	U8 priority;
	U8 valid;
	EN_PLAN_MODE mode;
}ST_PLAN_HDR;

typedef struct
{
	U8 used;
	U8 startHour;
	U8 startMin;
	U8 endHour;
	U8 endMin;
	U16 intervTime;
	U16 dimLevel;
}ST_JOB;

typedef struct
{
/*
	U16 startYear;
	U8 startMonth;
	U8 startDay;
	U16 endYear;
	U8 endMonth;
	U8 endDay;
	*/
	ST_TIME sDate;
	ST_TIME eDate;
}ST_YMD_PLAN;
/*
typedef struct
{
	ST_YMD_PLAN hdr;
	U8 jobNum;
	ST_JOB[JOB_MAX];
}ST_YMD_PLAN;
*/
typedef struct
{
	U8 dateValide;
	ST_TIME sDate;
	ST_TIME eDate;
	U8 bitmapWeek;
}ST_WEEK_PLAN;

/*
typedef struct
{
	ST_WEEK_PLAN hdr;
	U8 jobNum;
	ST_JOB[JOB_MAX];
}ST_WEEK_PLAN;
*/
/*
typedef struct
{
	U8 jobNum;
	ST_JOB[JOB_MAX];
}ST_AST_PLAN; //astronomical time
*/

typedef struct
{
	U8 used;
	ST_PLAN_HDR hdr;
	union{
		ST_YMD_PLAN stYmdPlan;
		ST_WEEK_PLAN stWeekPlan;
	}u;	
	U8 jobNum;
	ST_JOB jobs[JOB_MAX];
}ST_WORK_SCHEME;

typedef enum{
	NB_SRC = 0,
	WIFI_SRC,
	CLI_SRC,
	TIME_SRC,
	BOOT_SRC,
	TEST_SRC,
	UNKNOW_SRC
}EN_OP_SRC;
#pragma pack()

extern ST_OPP_LAMP_INFO g_stThisLampInfo;
extern ST_CONFIG_PARA g_stConfigPara, g_stConfigParaDefault;
extern ST_WORK_SCHEME g_stPlanScheme[PLAN_MAX];


void OppLampCtrlInit(void);
void OppLampCtrlVar(void);
void Print_ProductClass_Sku_Ver(void);
void OppLampCtrlParaDump(ST_CONFIG_PARA *pstConfigPara);
void OppLampCtrlOnOff(EN_OP_SRC opSrc, U8 onOff);
void OppLampCtrlSetDimLevel(EN_OP_SRC opSrc, U16 percent);
ST_WORK_SCHEME * OppLampCtrlGetEmptyWorkSchemeElement(void);
ST_WORK_SCHEME * OppLampCtrlGetWorkSchemeElementByIdx(U8 index);
int OppLampCtrlGetWorkSchemeElementByIdxOut(U8 index, ST_WORK_SCHEME * pstPlanScheme);
U32 OppLampCtrlAddWorkScheme(ST_WORK_SCHEME *pstPlanScheme);
U32 OppLampCtrlAddWorkSchemeById(U8 id, ST_WORK_SCHEME *pstPlanScheme);
U32 OppLampCtrlDelWorkScheme(U8 idx);
void OppLampCtrlWorkSchDump(ST_WORK_SCHEME * pstPlanScheme);
U32 OppLampCtrlSetConfigPara(ST_CONFIG_PARA *pstConfigPara);
U32 OppLampCtrlGetConfigParaFromFlash();
U32 OppLampCtrlGetConfigPara(ST_CONFIG_PARA *pstConfigPara);
U32 OppLampCtrlGetRuntimeParaFromFlash(ST_RUNTIME_CONFIG * pstRuntime);
U32 OppLampCtrlSetRuntimeParaToFlash(ST_RUNTIME_CONFIG * pstRuntime);
void OppLampCtrlAstTimerCallback(U32 arg);
void OppLampCtrlHeartCallback(void);
int OppLampCtrlDimmerPolicyReset(void);
void OppLampCtrlDimmerPolicy(void);
void OppCoapTask(void);
void OppCoapMsgDispatch(unsigned char *pucVal, int nLen);
U32 OppLampCtrlCreateHeartTimer(void);
void OppLampCtrlLoop(void);
void bubble_sort(ST_JOB array[], const int num);
void OppLampCtrlReadWorkSchemeFromFlash(void);
U32 OppLampCtrlRestoreFactory(void);
int CompareDate(const char * str1,const char * str2);
int CompareHM(const char * str1,const char * str2);
U32 OppLampCtrlGetSwitchU32(U8 targetId, U32 * lampSwitch);
U32 OppLampCtrlGetOnExep(U8 targetId, U32 * exep);
U32 OppLampCtrlGetOffExep(U8 targetId, U32 * exep);
U32 OppLampCtrlGetSwitch(U8 targetId, U8 * lampSwitch);
U32 OppLampCtrlSetSwitch(U8 targetId, U8 lampSwitch);
U32 OppLampCtrlGetBriU32(U8 targetId, U32 * bri);
U32 OppLampCtrlGetBri(U8 targetId, U16 * bri);
U32 OppLampCtrlSetBri(U8 targetId, U16 bri);
U32 OppLampCtrlGetDimType(U8 * dimType);
U32 OppLampCtrlSetDimType(U8 dimType);
U32 OppLampCtrlGetRtime(U8 targetId, U32 * rtime);
U32 OppLampCtrlGetHtime(U8 targetId, U32 * htime);
U32 OppLampCtrlSetRtime(U8 targetId, U32 rtime);
U32 OppLampCtrlSetHtime(U8 targetId, U32 htime);
U32 OppLampCtrlHtimeAdd(U8 num);
U32 OppLampCtrlRtimeAdd(U8 num);
U8 OppLampCtrlGetPhase(void);
U8 OppLampCtrlGetPlanIdx(void);
U8 OppLampCtrlGetJobsIdx(void);
U8 OppLampCtrlGetValidePlanId(int * paiPlanId, int * num);
void OppLampDelayOnOff(U8 srcChl, U8 sw, U32 sec);
U32 OppLampCtrlGetLtime(U8 targetId, U32 * ltime);
U32 OppLampCtrlGetLtimeWithCrc8(U8 targetId, U32 * ltime);
U32 OppLampCtrlGetHLtime(U8 targetId, U32 * hltime);
U32 OppLampCtrlGetHLtimeWithCrc8(U8 targetId, U32 * hltime);
U32 OppLampCtrlSetLtime(U8 targetId, U32 ltime);
U32 OppLampCtrlSetLtimeWithCrc8(U8 targetId, U32 ltime);
U32 OppLampCtrlSetHLtime(U8 targetId, U32 hltime);
U32 OppLampCtrlSetHLtimeWithCrc8(U8 targetId, U32 hltime);
unsigned int LightTimeCrc8ToLightTime(unsigned int inLtime, unsigned int *outLtime);
unsigned int LightTimeToLightTimeCrc8(unsigned int inLtime, unsigned int *outLtime);
unsigned int LightTimeCrcJudge(unsigned int hltime);
U32 OppLampCtrlHLtimeAdd(U8 num);
U32 OppLampCtrlLtimeAdd(U8 num);
U32 OppLampCtrlLightOnOff(int onOff);
void OppLampDelayInfo(U8 * srcChl, U8 * sw);
void OppLampActTimeGet(char *buf);
void OppLampActTimeSet(char *buf);
U32 OppLampCtrlGetExepPwr();

#endif
