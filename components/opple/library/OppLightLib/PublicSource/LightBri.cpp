#include "../PublicHeader/LightBri.h"
#include "../PrivateHeader/LightPrivateData.h"
#include <string.h>


extern t_onoff_data gOnOffContex[MAX_LIGHT_NUM];
extern T_MUTEX gMutex[MAX_LIGHT_NUM];

t_bri gBriContex[MAX_LIGHT_NUM];
U8 OppLight_Bri_SetOnOff_Local(const U8 index, U8 u8OnOff, U16 u16TransitionmMs);

static void OppDevLvValueMap(U8 index,OPP_LIGHT_DEV_LVL* pIn, OPP_LIGHT_DEV_LVL *pOut)
{
	U8 i;
	U32 total,value;
   
	// in 0 -> x 0 else
	// (in - 1)/(10000-1) = (x-low)/(high - low)
	// x= (( (in-1) * (high-low)) / (10000-1) )+ low 
	for (i = 0; i < DEV_CHANNEL_SIZE; i++)
	{
		if (pIn->u16ChannelLvlArry[i] == 0)
		{
			pOut->u16ChannelLvlArry[i] = 0;
		}
		else
		{
			total = gBriContex[index].limit.high[i] - gBriContex[index].limit.low[i];
			value = pIn->u16ChannelLvlArry[i] - 1;
			value *= total;
			value /= (DIM_DEV_LEVEL_MAX - 1);
			value += gBriContex[index].limit.low[i];
			pOut->u16ChannelLvlArry[i] = (U16)value;
		}
		
	}
}

static U8 OppBriDimTask(const U8 index, U32 u32SysTick)
{
	U32 stepNums;
	S32 sv,cv;
	U8 j,num,endFlag=0,valueChangedFlag,allOffFlag;
	OPP_LIGHT_DEV_LVL level;

	CHECK_LIGHT_INDEX(index);

	// Add 2018-8-13 only for nblampctrl program
	// if (gBriContex[index].g_blink.state == 1) return 0;

	// Changing curValue
	switch (gBriContex[index].dim.state)
	{
	case DIM_STATE_IDLE:
		return 0;
		break;
	case DIM_STAE_START:
		//OppDevLvValueMap(index,&gBriContex[index].dim.curValue,&level);
		//gOnOffContex[index].g_light.sBspOutputCb.pFunDim(&level);
		
		if (gBriContex[index].dim.trsMs == 0)
		{
			//gOnOffContex[index].g_light.sBspOutputCb.pFunDim(&(gBriContex[index].dim.setValue));
			memcpy(&gBriContex[index].dim.curValue, &gBriContex[index].dim.setValue,sizeof(OPP_LIGHT_DEV_LVL));
			gBriContex[index].dim.state = DIM_STATE_IDLE;
		}
		else
		{
			gBriContex[index].dim.curValue.u8ChannelTotalNum = gBriContex[index].dim.setValue.u8ChannelTotalNum;
			num = gBriContex[index].dim.curValue.u8ChannelTotalNum;
			// 当前值到目标值根据固定的tick每次执行step直到目标值
			gBriContex[index].dim.moment = u32SysTick;
			stepNums = (gBriContex[index].dim.trsMs+ DIM_STEP_TIME_MS_DELT) / DIM_STEP_TIME_MS; 
			for (U8 i = 0; i < num; i++)
			{
				sv =  (S32)gBriContex[index].dim.setValue.u16ChannelLvlArry[i];
				cv =  (S32)gBriContex[index].dim.curValue.u16ChannelLvlArry[i];
				gBriContex[index].dim.step[i] = (sv - cv) / (S32)stepNums;
				//if (gBriContex[index].dim.step[i] == 0)
				//{
				//	gBriContex[index].dim.step[i] = sv > cv ? 1 : -1;
				//}
			}
			gBriContex[index].dim.state = DIM_STATE_DIM;
		}
		break;
	case DIM_STATE_DIM:
		if (isTimeOn(gBriContex[index].dim.moment, u32SysTick, DIM_STEP_TIME_MS))
		{
			gBriContex[index].dim.moment = u32SysTick;
			num = gBriContex[index].dim.curValue.u8ChannelTotalNum;
			for (U8 i = 0; i < num; i++)  //gBriContex[index].dim.setValue.u8ChannelTotalNum;
			{
				cv = (S32)gBriContex[index].dim.curValue.u16ChannelLvlArry[i];
				cv += gBriContex[index].dim.step[i];
				if (cv <= (S32)gBriContex[index].dim.setValue.u16ChannelLvlArry[i]
					&& gBriContex[index].dim.step[i] < 0)
				{ 
					//cv = (S32)gBriContex[index].dim.setValue.u16ChannelLvlArry[i];
					//gBriContex[index].dim.state = DIM_STATE_END;
					//if(cv < 0) cv = 0;
					cv = gBriContex[index].dim.setValue.u16ChannelLvlArry[i];
					gBriContex[index].dim.step[i] = 0;
				}
				else if (cv >= (S32)gBriContex[index].dim.setValue.u16ChannelLvlArry[i]
					&& gBriContex[index].dim.step[i] > 0)
				{
					//cv = (S32)gBriContex[index].dim.setValue.u16ChannelLvlArry[i];
					//gBriContex[index].dim.state = DIM_STATE_END;
					//if(cv > DIM_DEV_LEVEL_MAX) cv = DIM_DEV_LEVEL_MAX;
          cv = gBriContex[index].dim.setValue.u16ChannelLvlArry[i];
					gBriContex[index].dim.step[i] = 0;
				}
				else
				{

				}
				gBriContex[index].dim.curValue.u16ChannelLvlArry[i] = (U16)cv;
			}
			endFlag = 0;
			for (U8 i = 0; i < num; i++)
			{
				if (gBriContex[index].dim.step[i] != 0) endFlag = 1;
			}
			if (endFlag == 0)
			{
				gBriContex[index].dim.state = DIM_STATE_END;
			}
		}
		break;

	case DIM_STATE_END:
		memcpy(&gBriContex[index].dim.curValue, &gBriContex[index].dim.setValue, sizeof(OPP_LIGHT_DEV_LVL));
		gBriContex[index].dim.state = DIM_STATE_IDLE;
		break;

	default:
		gBriContex[index].dim.state = DIM_STATE_IDLE;
	  return 0;
		break;
	}

	// 输出变化的当前值
	// 判断当前值是否变化
	num = gBriContex[index].dim.curValue.u8ChannelTotalNum;
	valueChangedFlag = 0;
	allOffFlag = 1;
	for (j = 0; j < num; j++)
	{
		if (gBriContex[index].dim.outValue.u16ChannelLvlArry[j] != gBriContex[index].dim.curValue.u16ChannelLvlArry[j])
		{
			valueChangedFlag = 1;
		}
		if (gBriContex[index].dim.curValue.u16ChannelLvlArry[j] != 0) 
		{
			allOffFlag = 0;
		}
	}
	// valuechange
	if (valueChangedFlag != 0)
	{
		// map
		memcpy(&level, &gBriContex[index].dim.curValue, sizeof(OPP_LIGHT_DEV_LVL));
		OppDevLvValueMap(index,&gBriContex[index].dim.curValue,&level);
		// level
		gOnOffContex[index].g_light.sBspOutputCb.pFunDim(&level);
		memcpy(&gBriContex[index].dim.outValue, &gBriContex[index].dim.curValue, sizeof(OPP_LIGHT_DEV_LVL));
		// OnOff
		if (allOffFlag == 1){
			gOnOffContex[index].g_light.sOLStateCur.u8OnOffState = 0;
		}
		else {
			gOnOffContex[index].g_light.sOLStateCur.u8OnOffState = 1;
		}

		// Update Curstate
		
	}
	//gOnOffContex[index].g_light.sBspOutputCb.pFunDim(&level);
	
	// 輰支欠摔远
	if (gBriContex[index].dim.isWithOnOff == 1)
	{
		//if (allOffFlag && gOnOffContex[index].g_light.sOLStateCur.u8OnOffState!=0)
		if (allOffFlag)
		{
			gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff(0);
			gOnOffContex[index].g_light.sOLStateCur.u8OnOffState = 0;
		}
		//else if(allOffFlag==0 && gOnOffContex[index].g_light.sOLStateCur.u8OnOffState == 0)
		else if(allOffFlag==0)
		{
			gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff(1);
			gOnOffContex[index].g_light.sOLStateCur.u8OnOffState = 1;
		}
		else { /*Keep*/ }
	}

	return 0;
} 

U8 OppBriBlinkTask(const U8 index, U32 u32SysTick)
{
	//static U32 moment = 0;
	CHECK_LIGHT_INDEX(index);

	if (gBriContex[index].g_blink.state == 1)
	{
		switch (gBriContex[index].g_blink.blinkState)
		{
		case BLINK_OFF:
			if (isTimeOn(gBriContex[index].g_blink.moment, u32SysTick, gBriContex[index].g_blink.offTimeMs))
			{
				gBriContex[index].g_blink.blinkState = BLINK_ON;
				gBriContex[index].g_blink.moment = u32SysTick;
				//gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff(1); // light on
				OppLight_Bri_SetOnOff_Local(index,1,0);
			}
			break;
		case BLINK_ON:
			if (isTimeOn(gBriContex[index].g_blink.moment, u32SysTick, gBriContex[index].g_blink.onTimeMs))
			{
				gBriContex[index].g_blink.blinkState = BLINK_OFF;
				gBriContex[index].g_blink.moment = u32SysTick;
				//gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff(0); // light on
				OppLight_Bri_SetOnOff_Local(index, 0, 0);
				
			}
			break;
		default:
			gBriContex[index].g_blink.blinkState = BLINK_OFF;
			break;
		}
	}
	else
	{
		//gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff(gOnOffContex[index].g_light.sOLStateSet.u8OnOffState);
		// 若是通信控制开关，由外层过滤重复动作
	}

	return 0;
}

static U8 OppLight_Bri_ProcLightTask(const U8 index, U32 u32SysTick)
{
	//OppLight_OnOff_ProcLightTask(index, u32SysTick);
	OppBriBlinkTask(index, u32SysTick);
	OppBriDimTask(index, u32SysTick);
	return 0;
}

U8 OppLight_SetDevLvlOutputLimit(const U8 index, t_level_limit* limit)
{
	CHECK_LIGHT_INDEX(index);

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
	memcpy(&gBriContex[index].limit,limit,sizeof(t_level_limit));
	MUTEX_UNLOCK(gMutex[index]);

	return 0;
}

static U8 OppLight_Bri_StartBlink(const U8 index, U8 src, U16 onTimeMs, U16 offTimeMs)
{
	CHECK_LIGHT_INDEX(index);

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);

	gBriContex[index].g_blink.state = 1;
	gBriContex[index].g_blink.onTimeMs = onTimeMs;
	gBriContex[index].g_blink.offTimeMs = offTimeMs;
	gBriContex[index].g_blink.moment = 0;
  
	gOnOffContex[index].g_light.sOLStateCur.blink = 1;
	ONOFF_OBSERVER(index,EVENT_START_BLINK, src, &gOnOffContex[index].g_light.sOLStateSet);

	MUTEX_UNLOCK(gMutex[index]);
	return 0;
}

static U8 OppLight_Bri_StopBlink(const U8 index, U8 src)
{
	CHECK_LIGHT_INDEX(index);

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
	gBriContex[index].g_blink.state = 0;
	gOnOffContex[index].g_light.sOLStateCur.blink = 0;
	ONOFF_OBSERVER(index,EVENT_STOP_BLINK, src, &gOnOffContex[index].g_light.sOLStateSet);
	MUTEX_UNLOCK(gMutex[index]);

	return 0;
}

U16 OppLight_LocalSetDevLvlOutput(const U8 index,U8 src,  OPP_LIGHT_DEV_LVL * pLvlOutput, U16 u16TransitionMs, U8 u8IsWithOnOff)
{
	CHECK_LIGHT_INDEX(index);

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
	memcpy(&gBriContex[index].dim.setValue, pLvlOutput, sizeof(OPP_LIGHT_DEV_LVL));
	// gOnOffContex[index].g_light.sOLStateSet.u8LightSkillType = LIGHT_SKILL_LEVEL;
	gBriContex[index].dim.trsMs = u16TransitionMs;
	gBriContex[index].dim.state = DIM_STAE_START;
	gBriContex[index].dim.isWithOnOff = u8IsWithOnOff;
	MUTEX_UNLOCK(gMutex[index]);

	//gOnOffContex[index].observer(EVENT_SET_LEVEL, src, &gOnOffContex[index].g_light.sOLStateSet);
	return 0;
}
// 输出pwm
U16 OppLight_SetDevLvlOutput(const U8 index, U8 src, OPP_LIGHT_DEV_LVL * pLvlOutput, U16 u16TransitionMs, U8 u8IsWithOnOff)
{
	CHECK_LIGHT_INDEX(index);

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
	memcpy(&gBriContex[index].dim.setValue,pLvlOutput,sizeof(OPP_LIGHT_DEV_LVL));
	gOnOffContex[index].g_light.sOLStateSet.u8LightSkillType = LIGHT_SKILL_LEVEL;
	gBriContex[index].dim.trsMs = u16TransitionMs;
	gBriContex[index].dim.state = DIM_STAE_START;
	gBriContex[index].dim.isWithOnOff = u8IsWithOnOff;

	gOnOffContex[index].g_light.sOLStateCur.u8ValidType = 0;

	if (src != SOURCE_INDEX_LOCAL) {
		ONOFF_OBSERVER(index,EVENT_SET_LEVEL, src, &gOnOffContex[index].g_light.sOLStateSet);
	}
	MUTEX_UNLOCK(gMutex[index]);

	return 0;
}

// 获取pwm
static U8 OppLight_GetCurDevLvl(const U8 index, OPP_LIGHT_DEV_LVL * pOutDevLvl)
{
	CHECK_LIGHT_INDEX(index);
	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
	memcpy(pOutDevLvl, &gBriContex[index].dim.curValue, sizeof(OPP_LIGHT_DEV_LVL));
	MUTEX_UNLOCK(gMutex[index]);
	return 0;
}

// 设置亮度
/*
  Switch      DevLevel      Driver-PWM(LevelLimit)
	On <------  10000(SetOn)  10000
	On        	.      -
	On          .        -     (On)
	On        	.          ->  Limit_H(On)
	On          .                .
	On        	.                .
	On        	.          ->  Limit_l(Min)
	On        	.        -     
	On        	.      -       (Off)
	On        	.   -
	On  <------ 1
	Off <------ 0(SetOff)---->  0
*/
static U8 OppLight_Bri_SetBrightLvl(const U8 index, U16 u16Level, U8 src, U16 u16TransitionMs, U8 u8IsWithOnOff)
{  
	CHECK_LIGHT_INDEX(index);
	OPP_LIGHT_DEV_LVL  level;

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
	
	// 更新设置值
	gOnOffContex[index].g_light.sOLStateSet.u8LightSkillType = LIGHT_SKILL_BRI;
	gOnOffContex[index].g_light.sOLStateSet.u16BriLvl = u16Level;

	// 更新当前值
	gOnOffContex[index].g_light.sOLStateCur.u8ValidType = 1;
	gOnOffContex[index].g_light.sOLStateCur.u8LightSkillType = LIGHT_SKILL_BRI;
	gOnOffContex[index].g_light.sOLStateCur.u16BriLvl = u16Level;

	// 调用调光算法
	gOnOffContex[index].g_light.pDimAlog(&gOnOffContex[index].g_light.sOLStateSet,&level);

	// 调用观察者回调
	ONOFF_OBSERVER(index,EVENT_SET_BRI, src, &gOnOffContex[index].g_light.sOLStateSet);
	MUTEX_UNLOCK(gMutex[index]);

	// Level输出
	OppLight_LocalSetDevLvlOutput(index, src,&level, u16TransitionMs, u8IsWithOnOff);

	return 0;
}  

U8 OppLight_Bri_Init(const U8 index, const t_bsp_output_cb* sBspOutputCb, const pfunDimAlgorithm fun)
{
	OppLight_OnOff_Init(index, sBspOutputCb, fun);

	for (U8 i = 0; i < DEV_CHANNEL_SIZE; i++)
	{
		gBriContex[index].limit.low[i] = 1;
		gBriContex[index].limit.high[i] = DIM_DEV_LEVEL_MAX;
		gBriContex[index].dim.setValue.u8ChannelTotalNum = RGB_ARRY_SIZE;
		gBriContex[index].dim.curValue.u8ChannelTotalNum = RGB_ARRY_SIZE;
		gBriContex[index].dim.outValue.u8ChannelTotalNum = RGB_ARRY_SIZE;

		gBriContex[index].dim.setValue.u16ChannelLvlArry[0] = 0;
		gBriContex[index].dim.outValue.u16ChannelLvlArry[0] = 0;
		gBriContex[index].dim.curValue.u16ChannelLvlArry[0] = DIM_DEV_LEVEL_MAX;
	}

	return 0;
}

U8 OppLight_Bri_SetOnOff_Local(const U8 index, U8 u8OnOff,U16 u16TransitionmMs)
{
	OPP_LIGHT_DEV_LVL level;

	if (u8OnOff == 0)
	{
		MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
		level.u8ChannelTotalNum = DEV_CHANNEL_SIZE;
		for (U8 i = 0; i < DEV_CHANNEL_SIZE; i++) {
			level.u16ChannelLvlArry[i] = 0;
		}
		MUTEX_UNLOCK(gMutex[index]);

		//OppLight_SetDevLvlOutput(index, SOURCE_INDEX_LOCAL, &level, u16TransitionmMs, 1);
		gOnOffContex[index].g_light.sBspOutputCb.pFunDim(&level);
		gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff(0);
	}
	else
	{
		MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
		level.u8ChannelTotalNum = DEV_CHANNEL_SIZE;
		for (U8 i = 0; i < DEV_CHANNEL_SIZE; i++) {
			level.u16ChannelLvlArry[i] = DIM_DEV_LEVEL_MAX;
		}
		MUTEX_UNLOCK(gMutex[index]);

		//OppLight_SetDevLvlOutput(index, SOURCE_INDEX_LOCAL, &level, u16TransitionmMs, 1);
		gOnOffContex[index].g_light.sBspOutputCb.pFunDim(&level);
		gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff(1);
	}

	return 0;
}

U8 OppLight_Bri_SetOnOff(const U8 index,  U8 src, U8 u8OnOff,U16 u16TransitionmMs)
{
	//U8 res;
	OPP_LIGHT_DEV_LVL level;

	if (u8OnOff == 0)  // 关
	{
		MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
		level.u8ChannelTotalNum = DEV_CHANNEL_SIZE;
		for (U8 i = 0; i < DEV_CHANNEL_SIZE; i++) {
			level.u16ChannelLvlArry[i] = 0;
		}
		gOnOffContex[index].g_light.sOLStateCur.u16BriLvl = 0;
        gOnOffContex[index].g_light.sOLStateSet.u16BriLvl = 0;
        gOnOffContex[index].g_light.sOLStateCur.u8OnOffState = 0;
        gOnOffContex[index].g_light.sOLStateSet.u8OnOffState = 0;
		MUTEX_UNLOCK(gMutex[index]);

		OppLight_SetDevLvlOutput(index, SOURCE_INDEX_LOCAL, &level, u16TransitionmMs, 1);
	}
	else  // 开
	{
		MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
		level.u8ChannelTotalNum = DEV_CHANNEL_SIZE;
		//for (U8 i = 0; i < DEV_CHANNEL_SIZE; i++) {
		//	level.u16ChannelLvlArry[i] = DIM_DEV_LEVEL_MAX;
		//}
		// gOnOffContex[index].g_light.sOLStateCur.u16BriLvl = ;
        gOnOffContex[index].g_light.sOLStateCur.u8OnOffState = 1;
        gOnOffContex[index].g_light.sOLStateSet.u8OnOffState = 1;
		MUTEX_UNLOCK(gMutex[index]);

		//OppLight_SetDevLvlOutput(index, SOURCE_INDEX_LOCAL, &level, u16TransitionmMs, 1);
        gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff(1);
	}

	ONOFF_OBSERVER(index,EVENT_SET_ONOFF, src, &gOnOffContex[index].g_light.sOLStateSet);
	return 0;
}

U8 OppLight_Bri_Update_output(const U8 index)
{
    OPP_LIGHT_DEV_LVL level;
    
	// map
	OppDevLvValueMap(index,&gBriContex[index].dim.curValue,&level);
	// level
	gOnOffContex[index].g_light.sBspOutputCb.pFunDim(&level);
	return 0;
}

U8 Create_I_OPP_LIGHT_BRI(I_OPP_LIGHT_BRI * pOL_Bri, U8 * uP8LightIndex)
{  
	if (pOL_Bri == NULL)
	{
		return 1;
	} 

	U8 u8Ret = Create_I_OPP_LIGHT_ONOFF((I_OPP_LIGHT_ONOFF*)pOL_Bri, uP8LightIndex);
 
	pOL_Bri->SetBrightLvl = OppLight_Bri_SetBrightLvl;
	pOL_Bri->SetDevLvlOutput = OppLight_SetDevLvlOutput;
	pOL_Bri->GetCurDevLvl = OppLight_GetCurDevLvl;
	pOL_Bri->Init = OppLight_Bri_Init;
	pOL_Bri->ProcLightTask = OppLight_Bri_ProcLightTask;
	pOL_Bri->SetOnOff = OppLight_Bri_SetOnOff;
	pOL_Bri->SetLimit = OppLight_SetDevLvlOutputLimit;
	pOL_Bri->StartBlink = OppLight_Bri_StartBlink;
	pOL_Bri->StopBlink = OppLight_Bri_StopBlink;
	pOL_Bri->SetDevLvlOutputLimit = OppLight_SetDevLvlOutputLimit;
	pOL_Bri->UpdateOutput = OppLight_Bri_Update_output;

	return u8Ret;
}








