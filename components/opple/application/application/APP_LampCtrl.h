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
//本灯具的分组数(个)规格
#define OPP_LAMP_GROUP_MAX_NUM         (1)

//本灯具的场景数(个)规格
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

//PWM周期的频率(单位:Hz)
//#define OPP_LAMP_PWMPERIOD_FREQ_HZ     (3000)
#define OPP_LAMP_PWMPERIOD_FREQ_HZ     (200)
#define PRODUCTNAME                    "ROADLAMP"
#define PROTID                         "1.0"
#define MODULE                         "NBIOT"
#define MANU                           "OPPLE"

//配置数据的版本号
#define OPP_LAMP_CTRL_CFG_DATA_VER     (786)
#define OPP_LAMP_CTRL_CFG_DATA_VER_STR "786"
#define OPP_LAMP_CTRL_CFG_DATA_MAX_VER  (0xFFFF)
        
#define PRODUCTCLASS                 0xF011                
#define PRODUCTSKU                   0x0085
/*色温参数*/
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

//获取百位数
#define OPP_HUNDRED_OF_NUMBER(x)                    ((x) / 100)
//获取十位数
#define OPP_DECADE_OF_NUMBER(x)                     (((x) % 100) / 10)
//获取个位数
#define OPP_UNIT_OF_NUMBER(x)                       ((x) % 10)

#pragma pack(1)

typedef enum
{
	DIM_010_OUT = 0,	
	DIM_PWM_OUT,
	DIM_NONE,
}EN_DIM_TYPE;

/*欧普设备信息结构定义*/
typedef struct
{
    U16 uwAgentFlag;                     /*代理标识:0-对象本身,其他-具体代理实体*/
	U8 productName[24];					 /*产品名称*/
	U8 protId[6];						 /*HUAWEI 协议号*/
	U8 model[20];                        /*设备型号*/
	U8 manu[10];						 /*设备制造商*/
    U16 uwProductClass;                  /*产品类型,取值范围:Opp_ProductClass.h*/
    U16 uwProductSku;                    /*产品型号,取值范围:Opp_ProductSku.h*/
    U16 uwSourceType;                    /*光源类型,取值范围:Opp_SourceType.h*/
    U8 aucObjectSN[16];                   /*产品序列号后半段(操作密钥)*/
    U16 uwObjectType;                    /*IOT ObjectType*/
    U32 ObjectIDL;                       /*IOT ID Low*/
    U32 ObjectIDH;                       /*IOT ID High*/
    U32 ulVersion;                       /*智能设备的软件版本号*/
    U32 ulLocalIP;                       /*局域网本地IP*/
    U16 uwLocalPort;                     /*本地服务UDP端口*/
    U8 aucDesc[16];                      /*设备描述信息*/
    U8 aucOpPwd[16];                     /*产品操作密码*/	
	U8 ucHasRelay;						 /*有无继电器*/
	EN_DIM_TYPE enDimType;               /*调光方式*/
}ST_OPP_OBJ_INFO;

/*色坐标表示结构定义*/
typedef struct
{
    FLOAT cx;                            //当前颜色的色坐标x值
    FLOAT cy;                            //当前颜色的色坐标y值
}ST_OPP_COLOR_CXY;

/*直线斜率结构定义*/
typedef struct
{
    FLOAT fSlope;                        /*直线斜率*/
    U8 ucSlopeFlag;                      /*斜率无穷大标志:0-斜率正常,1-斜率无穷大*/
}ST_OPP_SLOPE;

/*色坐标直线线段结构定义*/
typedef struct
{
    ST_OPP_COLOR_CXY Pt1;                //线段起始点色坐标x,y值
    ST_OPP_COLOR_CXY Pt2;                //线段结束点色坐标x,y值
    ST_OPP_SLOPE stSlope;                //直线斜率
}ST_OPP_COLOR_LINE;

/*颜色控制维度枚举定义*/
typedef enum
{
    EN_OPP_COLOR_RED = 0, 
    EN_OPP_COLOR_GREEN,
    EN_OPP_COLOR_BLUE,
    EN_OPP_COLOR_WHITE,
    EN_OPP_COLOR_BUTT
}EN_OPP_COLOR_NAME;

/*调光控制方式*/
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



/*本灯具当前状态结构定义*/
typedef struct
{
    //U8  ucSwitch;                        /*开关状态:0-关,其他-开*/
    //U16  usBri;                           /*0-关, 其他(0~255)-开着时的亮度等级*/ 
	U32 uwRunTime;
	U32 uwHisTime;
	U32 uwLightTime;
	U32 uwHisLightTime;
#if 0	
    EN_OPP_CTRL_TYPE enCtrlType;         /*调光控制方式*/
    U8  aucHue[3];                       /*颜色(R-G-B)*/  
    U16 uwCCT;                           /*色温值(K)*/
    U16 uwHue;                           /*色相:0~360*/ 
    U8  ucSat;                           /*饱和度:0~255*/
    ST_OPP_COLOR_CXY stTargetCxy;        /*目标(当前)点的色坐标*/
    U32 ulPwmFreq;                       /*PWM周期的频率(Hz)*/
    U32 aulCurrPwms[EN_OPP_COLOR_BUTT];  /*目标(当前)点的控制输出PWM值*/
#endif	
}ST_OPP_LAMP_CURR_STATUS;

/*本灯具场景结构定义*/
typedef struct
{
    U8 ucInvalidFlag;                    /*有效标志:0-无效,其他-有效*/
    U8 aucSceneName[16];                 /*场景名称*/
    ST_OPP_LAMP_CURR_STATUS stStatus;    /*灯具在该场景下的表现状态*/
}ST_OPP_LAMP_SCENE_ITEM;


/*三原色三角形类型的枚举定义*/
typedef enum
{
    EN_OPP_TRIANG_RGB = 0, 
    EN_OPP_TRIANG_RGW, 
    EN_OPP_TRIANG_WGB, 
    EN_OPP_TRIANG_RWB, 
    EN_OPP_TRIANG_BUTT
}EN_OPP_TRIANG_NAME;


/*颜色控制维度信息结构定义*/
typedef struct
{
    ST_OPP_COLOR_CXY stCxy;              /*颜色点的色坐标x,y值*/
    FLOAT fLightEffect;                  /*该颜色控制通路的光效(单位:lm/W)*/
    FLOAT fCurrentLimit;                 /*该颜色控制通路的峰值电流(单位:A,安培)*/
    FLOAT fVoltageForward;               /*该颜色控制通路的前向电压(单位:V,伏特)*/
    FLOAT fTotalLmLimit;                 /*该颜色控制通路的最大输出光通量(单位:lm,流明),在某环境温度下的最大输出光通量FluxPeak@Tc*/
}ST_OPP_CTRL_ITEM;

/*本灯具信息结构定义*/
typedef struct
{
    //本灯具的对象信息
    ST_OPP_OBJ_INFO stObjInfo;
    //本灯具的分组信息
    U32 ulGroupNum; 
    U32 aulGroupItem[OPP_LAMP_GROUP_MAX_NUM];
    //参考点R[0]G[1]B[2]W[3]的色坐标
    ST_OPP_CTRL_ITEM astRefPoint[EN_OPP_COLOR_BUTT];
    //混光算法选择:0-RGB三色混光,1-白色参与的三色混光
    U32 ulAlgorithm;
    //RGB混光辅助点的色坐标
    ST_OPP_COLOR_CXY stAssistCxy;
	//位置信息
	//ST_OPP_LAMP_LOCATION stLocation;
	//当前电能信息
	//ST_OPP_LAMP_CURR_ELECTRIC_INFO	stCurrElecric;
	//NBIOT设备状态
	ST_NEUL_DEV stNeulSignal;
    //本灯具的当前状态
    ST_OPP_LAMP_CURR_STATUS stCurrStatus;//本灯具的当前状态
    //本灯具的场景信息
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
	//char ocIp[16]; /*IP地址xxx.xxx.xxx.xxx*/
	//U16 ocPort;
	//U8 band;
	//U8 scram; /*扰码*/	
	U32 period; /*分钟*/
	//U32 hisTime; /*历史时间*/
	//U32 hisConsumption; /*历史功耗*/
	U8 dimType;
	//char apn[128]; /*域名*/
	U32 plansNum;
	U32 hbPeriod; /*单位s*/
}ST_CONFIG_PARA;

typedef struct
{
	U32 hisTime; /*历史时间*/
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
