#include "../PublicHeader/LightOnOff.h"
#include "../PrivateHeader/LightPrivateData.h"
#include <string.h>


static U8 g_light_index = 0;
t_onoff_data gOnOffContex[MAX_LIGHT_NUM];
T_MUTEX gMutex[MAX_LIGHT_NUM];



U8 isTimeOn(U32 moment, U32 now, U32 time)
{
	U32 t;
	t = now >= moment ? now - moment:(~(U32)0) - moment + now;
	if (t >= time) return 1;
	else return 0;
}

U8 OppLight_Init(const U8 index, const t_bsp_output_cb* sBspOutputCb, const pfunDimAlgorithm fun)
{
	CHECK_LIGHT_INDEX(index);
	//pLocalData[index] = contex;
	gOnOffContex[index].g_light.sBspOutputCb.pFunDim = sBspOutputCb->pFunDim;
	gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff = sBspOutputCb->pFunOnOff;
	gOnOffContex[index].g_light.pDimAlog = fun;

	return 0;
}

U8 OppLight_OnOff_Init(const U8 index, const t_bsp_output_cb* sBspOutputCb, const pfunDimAlgorithm fun)
{
	CHECK_LIGHT_INDEX(index);

	OppLight_Init(index,sBspOutputCb, fun);

	for (U8 i = 0; i < MAX_LIGHT_NUM;i++)
	{
		MUTEX_CREATE(gMutex[i]);
	}

	return 0;
}

U8 OppLight_OnOff_ProcLightTask(const U8 index, U32 u32SysTick)
{
	//static U32 moment = 0;
	CHECK_LIGHT_INDEX(index);
	
	if (gOnOffContex[index].g_blink.state == 1)
	{
		switch (gOnOffContex[index].g_blink.blinkState)
		{
		case BLINK_OFF:
			if (isTimeOn(gOnOffContex[index].g_blink.moment, u32SysTick, gOnOffContex[index].g_blink.offTimeMs))
			{
				gOnOffContex[index].g_blink.blinkState = BLINK_ON;
				gOnOffContex[index].g_blink.moment = u32SysTick;
				gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff(1); // light on
			}
			break;
		case BLINK_ON:
			if (isTimeOn(gOnOffContex[index].g_blink.moment, u32SysTick, gOnOffContex[index].g_blink.onTimeMs))
			{
				gOnOffContex[index].g_blink.blinkState = BLINK_OFF;
				gOnOffContex[index].g_blink.moment = u32SysTick;
				gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff(0); // light on
			}
			break;
		default:
			gOnOffContex[index].g_blink.blinkState = BLINK_OFF;
			break;
		}
	}
	else
	{
		gOnOffContex[index].g_light.sBspOutputCb.pFunOnOff(gOnOffContex[index].g_light.sOLStateSet.u8OnOffState);
		// 若是通信控制开关，由外层过滤重复动作
	}

	return 0;
}

U8 OppLight_OnOff_SetOnOff(const U8 index, U8 src, U8 u8OnOff,  U16 u16TransitionmMs)
{
	CHECK_LIGHT_INDEX(index);

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
	//g_light[index].sBspOutputCb.pFunOnOff(u8OnOff);
	gOnOffContex[index].g_light.sOLStateSet.u8LightSkillType = LIGHT_SKILL_ONOFF;
	gOnOffContex[index].g_light.sOLStateSet.u8OnOffState = u8OnOff;

	ONOFF_OBSERVER(index,EVENT_SET_ONOFF,src,&gOnOffContex[index].g_light.sOLStateSet);

	MUTEX_UNLOCK(gMutex[index]);

	return 0;
}

static U8 OppLight_OnOff_GetState(const U8 index, OPP_LIGHT_STATE * pOutState)
{
	CHECK_LIGHT_INDEX(index);
	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
	memcpy(pOutState,&(gOnOffContex[index].g_light.sOLStateCur),sizeof(OPP_LIGHT_STATE));
	MUTEX_UNLOCK(gMutex[index]);
	return 0;
}

static U8 OppLight_OnOff_SetState(const U8 index, U8 u8CmdResourceIndex, OPP_LIGHT_STATE * pState)
{
	CHECK_LIGHT_INDEX(index);

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
	memcpy(&(gOnOffContex[index].g_light.sOLStateCur), pState,sizeof(OPP_LIGHT_STATE));
	MUTEX_UNLOCK(gMutex[index]);

	return 0;
}

static U8 OppLight_RegisterStateObserver(const U8 index, U8 u8CmdResourceIndex, pfObserVer observer)
{
	CHECK_LIGHT_INDEX(index);
	gOnOffContex[index].observer = observer;
	return 0;
}

static U8 OppLight_UnRegisterStateObserver(const U8 index)
{
	CHECK_LIGHT_INDEX(index);
	gOnOffContex[index].observer = NULL;
	return 0;
}

static U8 OppLight_OnOff_StartBlink(const U8 index, U8 src, U16 onTimeMs, U16 offTimeMs)
{
	CHECK_LIGHT_INDEX(index);

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);

	gOnOffContex[index].g_blink.state = 1;
	gOnOffContex[index].g_blink.onTimeMs = onTimeMs;
	gOnOffContex[index].g_blink.offTimeMs = offTimeMs;
	gOnOffContex[index].g_blink.moment = 0;
  
	gOnOffContex[index].g_light.sOLStateCur.blink = 1;
	ONOFF_OBSERVER(index,EVENT_START_BLINK, src, &gOnOffContex[index].g_light.sOLStateSet);

	MUTEX_UNLOCK(gMutex[index]);
	return 0;
}

static U8 OppLight_OnOff_StopBlink(const U8 index, U8 src)
{
	CHECK_LIGHT_INDEX(index);

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
	gOnOffContex[index].g_blink.state = 0;
	
	gOnOffContex[index].g_light.sOLStateCur.blink = 0;
	ONOFF_OBSERVER(index,EVENT_STOP_BLINK, src, &gOnOffContex[index].g_light.sOLStateSet);
	MUTEX_UNLOCK(gMutex[index]);

	return 0;
}

U8 Create_I_OPP_LIGHT_ONOFF(I_OPP_LIGHT_ONOFF * pOP_Light_OnOff, U8 * uP8LightIndex)
{
	if (pOP_Light_OnOff == NULL)
		return 1;//Ptr Error

	if (g_light_index + 1 > MAX_LIGHT_NUM)
	{
		return 2;//Outof Memory
	}

	//I_OPP_LIGHT_ONOFF g_OLOnOff;
	//Not use malloc
	pOP_Light_OnOff->Init = OppLight_OnOff_Init;
	pOP_Light_OnOff->ProcLightTask = OppLight_OnOff_ProcLightTask;

	pOP_Light_OnOff->SetOnOff = OppLight_OnOff_SetOnOff;
	pOP_Light_OnOff->GetState = OppLight_OnOff_GetState;
	pOP_Light_OnOff->SetState = OppLight_OnOff_SetState;
	pOP_Light_OnOff->RegisterStateObserver = OppLight_RegisterStateObserver;
	pOP_Light_OnOff->UnRegisterStateObserver = OppLight_UnRegisterStateObserver;
	pOP_Light_OnOff->StartBlink = OppLight_OnOff_StartBlink;
	pOP_Light_OnOff->StopBlink = OppLight_OnOff_StopBlink;

	//Save
	*uP8LightIndex = g_light_index;
	g_light_index++;
	//g_light[g_light_index++].pOppLight = pOP_Light_OnOff;

	return 0;
}
