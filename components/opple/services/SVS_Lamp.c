/*********************************************************************************
                                Config
*********************************************************************************/
#define APS_LAMP_DEBUG 0


/*********************************************************************************
                                Includes
*********************************************************************************/
#include "SVS_Lamp.h"
#include "LightDef.h"
#include "LightData.h"
#include "LightBri.h"
//#include "stm32f4xx_hal.h"
#include "SVS_Para.h"
#include "OPP_Debug.h"
#include "Opp_ErrorCode.h"
#include "APP_LampCtrl.h"


#if APS_LAMP_DEBUG==0
#include "DRV_DimCtrl.h"
#endif

/*********************************************************************************
                                Para description
*********************************************************************************/
// num 0 : levelOutputType;  //0:0-10V,1:PWM
// num 1 : briSet
#define LAMP_PARA_NUM_OUTTYPE 0
#define LAMP_PARA_NUM_BRISET  2  // 1 - bad 

#define LAMP_PARA_OUTTYPE_DEFAULT 1
#define LAMP_PARA_BRISET_DEFAULT  DIM_DEV_LEVEL_MAX//((DIM_DEV_LEVEL_MAX)>>1)

//#define LAMP_GET_SYSTICK() HAL_GetTick()
#define LAMP_GET_SYSTICK()  OppTickCountGet()

#define APSLAMP_P_CHECK(p) \
		if((p)==(void*)0){\
		DEBUG_LOG(DEBUG_MODULE_APSLAMP,DLL_ERROR,"null pointer!\r\n");\
		return OPP_NULL_POINTER;}



unsigned char LampBspLevelOutput(OPP_LIGHT_DEV_LVL *sBspOutputCb);
unsigned char LampBspLevelOnOffOutput(unsigned char onoff);

typedef enum
{
	BLINK_IDLE=0,
	BLINK_LOW,
	BLINK_LOW_KEEP,
	BLINK_HIGH,
	BLINK_HIGH_KEEP,
}BLINK_EN;

typedef struct
{
	BLINK_EN status;
	U32 en;
	U32 lowLevel;
	U32 highLevel;
	U32 lowTime;
	U32 highTime;
	U32 lastTime; // 0xFFFFFFFF:Forever
}BLINK_T;

static struct
{
    unsigned char init;
    I_OPP_LIGHT_BRI bri;
    unsigned char index;
    unsigned char levelOutputType;  //0:0-10V,1:PWM
    unsigned int briSet;
    BLINK_T blink;
}sLamp;


t_bsp_output_cb bsp_bri_cb={
    .pFunDim = LampBspLevelOutput,
    .pFunOnOff = LampBspLevelOnOffOutput,
};

#if APS_LAMP_DEBUG==1
extern void ledon(unsigned char led);
extern void ledoff(unsigned char led);
#endif

unsigned char LampBspLevelOutput(OPP_LIGHT_DEV_LVL *sBspOutputCb)
{
    unsigned short level;
	unsigned int temp;

	APSLAMP_P_CHECK(sBspOutputCb);
	
	if(sLamp.levelOutputType == 0) // 0-10V
    {
        #if APS_LAMP_DEBUG==0
		level = sBspOutputCb->u16ChannelLvlArry[0];
		//level = (U32)level*255/DIM_DEV_LEVEL_MAX;
        DimDacLevelSet(level);
    DEBUG_LOG(DEBUG_MODULE_APSLAMP,DLL_INFO,"DimDacLevelSet level: %d\r\n",sBspOutputCb->u16ChannelLvlArry[0]);
        #else
        printf("DimDacLevelSet level: %d\r\n",sBspOutputCb->u16ChannelLvlArry[0]);
        #endif
    }
    else
    {
        #if APS_LAMP_DEBUG==0
		level = sBspOutputCb->u16ChannelLvlArry[0];
		//level = (U32)level*255/DIM_DEV_LEVEL_MAX;
        DimPwmDutyCycleSet(level);
    DEBUG_LOG(DEBUG_MODULE_APSLAMP,DLL_INFO,"DimDacLevelSet level: %d\r\n",sBspOutputCb->u16ChannelLvlArry[0]);
        #else
        printf("DimPwmDutyCycleSet level: %d\r\n",sBspOutputCb->u16ChannelLvlArry[0]);
        #endif
    }
    return 0;
}

unsigned char LampBspLevelOnOffOutput(unsigned char onoff)
{
    if(onoff == 0)
    {
        #if APS_LAMP_DEBUG==0
        DimRelayOff();
        #else
        //ledoff(1);
        #endif
    }
    else
    {
        #if APS_LAMP_DEBUG==0
        DimRelayOn();
        #else
        //ledon(1);
        #endif
    }
    return 0;
}

unsigned char LampDimAlgorithm(OPP_LIGHT_STATE * pTarget, OPP_LIGHT_DEV_LVL * pOutLevel)
{
	APSLAMP_P_CHECK(pTarget);
	APSLAMP_P_CHECK(pOutLevel);
	
    // bri/cct/rgb
    if(pTarget->u8LightSkillType == LIGHT_SKILL_BRI)
    {
        pOutLevel->u8ChannelTotalNum = 1;
        for(U8 i = 0;i <pOutLevel->u8ChannelTotalNum;i++)
        {
            pOutLevel->u16ChannelLvlArry[i] = pTarget->u16BriLvl;
        }
    }
    
    return 0;
}

U32 LampOnOff(unsigned char src,int flag)
{
    unsigned short level;
    
    if(flag == 0){
        return sLamp.bri.SetOnOff(sLamp.index,src,flag,0);
    }else{
        level = sLamp.briSet;
        //if(level==0){
        //    level = LAMP_PARA_BRISET_DEFAULT;
        //}
        //sLamp.bri.SetBrightLvl(sLamp.index,level,src,0,0);
        //sLamp.bri.SetOnOff(sLamp.index,src,flag,0);
        return sLamp.bri.SetBrightLvl(sLamp.index,level,src,0,1);
    }
}

/*
    level:0-10000
*/
U32 LampDim(unsigned char src,int level)
{
    return sLamp.bri.SetBrightLvl(sLamp.index,level,src,0,1);
}

/*
    level:0-10000
*/
U32 LampMoveToLevel(unsigned char src,int level,int ms)
{
    return sLamp.bri.SetBrightLvl(sLamp.index,level,src,ms,1);
}

int LampStateRead(int* state,int* level)
{
    OPP_LIGHT_STATE briState;

	APSLAMP_P_CHECK(state);
	APSLAMP_P_CHECK(level);
	
    sLamp.bri.GetState(sLamp.index,&briState);
    *state = briState.u8OnOffState;
    *level = briState.u16BriLvl;
    return 0;
}

void LampObserver(t_event event,unsigned char src, const OPP_LIGHT_STATE * const state)
{
    unsigned int bri;
    int res;
    
    DEBUG_LOG(DEBUG_MODULE_APSLAMP,DLL_INFO,"Observer event:%d,src:%d,onoff:%s,level:%d\r\n",event,src,state->u8OnOffState==0?"off":"on",state->u16BriLvl);
    // LampObserverNotifyWifi();
    // LampObserverNotifyNb();
    
    if(event == EVENT_SET_BRI && state->u16BriLvl != 0)
    {
        bri = state->u16BriLvl;
        if(sLamp.briSet != bri)
        {
            //res = AppParaWriteU32(APS_PARA_MODULE_APS_LAMP,LAMP_PARA_NUM_BRISET,bri);
            //if(res != 0)
            //{
                // Save fail
            //}
            sLamp.briSet = bri;
        }
    }
    
}

void ApsLampInit(void)
{
     unsigned char res;
     
     sLamp.init = 0;
     sLamp.blink.en = BLINK_IDLE;
     sLamp.blink.status = 0;
    
     // Create bri lamp
     res = Create_I_OPP_LIGHT_BRI(&sLamp.bri,&sLamp.index);
     if(res != 0) {
        // Create fail
        return ;
     }

     // Init bri lamp
     sLamp.bri.Init(sLamp.index,&bsp_bri_cb,LampDimAlgorithm);

     // Set Limit
	 

     // Register observer
     sLamp.bri.RegisterStateObserver(sLamp.index,0,LampObserver);

	if(sLamp.init == 0)
	{
		sLamp.init = 1;
		/*res = AppParaReadU8(APS_PARA_MODULE_APS_LAMP,LAMP_PARA_NUM_OUTTYPE,&sLamp.levelOutputType);
		if(res != 0){
			sLamp.levelOutputType = LAMP_PARA_OUTTYPE_DEFAULT;
		}*/

		/*
		res = AppParaReadU32(APS_PARA_MODULE_APS_LAMP,LAMP_PARA_NUM_BRISET,&sLamp.briSet);
		if(res != 0){
			DEBUG_LOG(DEBUG_MODULE_APSLAMP,DLL_ERROR,"Briset save fail!\r\n")
			sLamp.briSet = LAMP_PARA_BRISET_DEFAULT;
		}else{
			if(sLamp.briSet == 0) sLamp.briSet = LAMP_PARA_BRISET_DEFAULT;
		}
		*/
		sLamp.briSet = LAMP_PARA_BRISET_DEFAULT;
		
		 if(sLamp.levelOutputType == 0) // 0-10V
		 {
			#if APS_LAMP_DEBUG==0
			DimCtrlSwitchToDac();
			#else
			//printf("DimCtrlSwitchToDac\r\n");
			#endif
		 }
		 else
		 {
			 #if APS_LAMP_DEBUG==0
			 DimCtrlSwitchToPwm();
			 #else
			 //printf("DimCtrlSwitchToPwm\r\n");
			 #endif
		 }
	}	 
}

void ApsLampLoop(void)
{
    int res;
    static OS_TICK_T sSec=0;
    static OS_TICK_T sLastSec=0;
    OS_TICK_T sec=0;
    
    if(sLamp.bri.ProcLightTask != NULL)
	{
		sLamp.bri.ProcLightTask(sLamp.index,LAMP_GET_SYSTICK());
	}
	
	switch(sLamp.blink.status)
	{
		case BLINK_IDLE:
		if(sLamp.blink.en != 0)
		{
			sLamp.blink.status=BLINK_LOW;
			sLastSec= OS_GET_TICK()/1000;
		}
		break;

		case BLINK_LOW:
		sLamp.bri.SetBrightLvl(sLamp.index,sLamp.blink.lowLevel,TEST_SRC,0,1);
		sSec = OS_GET_TICK()/1000;
		sLamp.blink.status=BLINK_LOW_KEEP;
		DEBUG_LOG(DEBUG_MODULE_APSLAMP,DLL_INFO,"Lamp blink low,level=%u!\r\n",sLamp.blink.lowLevel);
		break;

		case BLINK_LOW_KEEP:
		sec = OS_GET_TICK()/1000;
		if(sec-sSec >= sLamp.blink.lowTime)
		{
			sLamp.blink.status=BLINK_HIGH;
		}
		break;

		case BLINK_HIGH:
		sLamp.bri.SetBrightLvl(sLamp.index,sLamp.blink.highLevel,TEST_SRC,0,1);
		sSec = OS_GET_TICK()/1000;
		sLamp.blink.status=BLINK_HIGH_KEEP;
		DEBUG_LOG(DEBUG_MODULE_APSLAMP,DLL_INFO,"Lamp blink high,level=%u!\r\n",sLamp.blink.highLevel);
		break;

		case BLINK_HIGH_KEEP:
		sec = OS_GET_TICK()/1000;
		if(sec-sSec >= sLamp.blink.highTime)
		{
			sLamp.blink.status=BLINK_LOW;
		}
		break;

		default:
		sLamp.blink.status = BLINK_IDLE;
		break;
	}
	if(sLamp.blink.status != BLINK_IDLE)
	{
		sec = OS_GET_TICK()/1000;
		if((sec-sLastSec)>sLamp.blink.lastTime)
		{
			sLamp.blink.status = BLINK_IDLE;
			sLamp.blink.en = 0;
			DEBUG_LOG(DEBUG_MODULE_APSLAMP,DLL_INFO,"Lamp blink end\r\n");
		}
	}
	if(sLamp.blink.en == 0)
	{
		sLamp.blink.status = BLINK_IDLE;
	}
	
}

int LampOuttypeSet(EN_LAMP_OUT type)
{
    //int res;
    int state,level;
    
    //res = AppParaWriteU8(APS_PARA_MODULE_APS_LAMP,LAMP_PARA_NUM_OUTTYPE,type);
    //if(res == 0)
    //{
	    sLamp.levelOutputType = type;
		
		if(sLamp.levelOutputType == LAMP_OUT_DAC) // 0-10V
		{
		   #if APS_LAMP_DEBUG==0
			   DimCtrlSwitchToDac();
		   #else
			   //printf("DimCtrlSwitchToDac\r\n");
		   #endif
		}
		else
		{
			#if APS_LAMP_DEBUG==0
				DimCtrlSwitchToPwm();
			#else
				//printf("DimCtrlSwitchToPwm\r\n");
			#endif
		}
		
		//LampStateRead(&state,&level);
		//if(state == 0) LampOnOff(0,0);
		//else LampDim(0, level);
		sLamp.bri.UpdateOutput(sLamp.index);
		
    //}
    return 0;
}

unsigned char LampOuttypeGet(void)
{
    return sLamp.levelOutputType;
}

/*
state:0:off,else:on
lowLevel:0-10000
highLevel:0-10000
lowTime:seconds 
highTime:seconds
lastTime:seconds
*/
int LampBlink(U32 state,U32 lowLevel,U32 highLevel,U32 lowTime,U32 highTime,U32 lastTime)
{
	if(lowLevel > DIM_DEV_LEVEL_MAX) return 1;
	if(highLevel < lowLevel) return 2;

	sLamp.blink.en=state;
	sLamp.blink.lowLevel = lowLevel;
	sLamp.blink.highLevel = highLevel;
	sLamp.blink.lowTime = lowTime;
	sLamp.blink.highTime = highTime;
	sLamp.blink.lastTime = lastTime;
	return 0;
}














