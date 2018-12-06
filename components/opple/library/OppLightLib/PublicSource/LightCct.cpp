#include "../PublicHeader/LightCct.h"
#include "../PrivateHeader/LightPrivateData.h"
#include "../PublicHeader/LightBri.h"


extern t_onoff_data gOnOffContex[MAX_LIGHT_NUM];
extern t_bri gBriContex[MAX_LIGHT_NUM];
extern T_MUTEX gMutex[MAX_LIGHT_NUM];

static U8 OppLight_CCT_SetCCT(const U8 index, U16 u16Cct, U8 src, U16 u16TransitionMs, U8 u8IsWithOnOff)
{  
	CHECK_LIGHT_INDEX(index);
	OPP_LIGHT_DEV_LVL  level;

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);

	// 更新设置值
	gOnOffContex[index].g_light.sOLStateSet.u8LightSkillType = LIGHT_SKILL_CCT;
	gOnOffContex[index].g_light.sOLStateSet.strCCTRGB.unCCTRGB.u16CCT = u16Cct;
	gOnOffContex[index].g_light.sOLStateSet.strCCTRGB.u8CCTRGBValid = 1;//1:cct,2:rgb

	// 更新当前值
	gOnOffContex[index].g_light.sOLStateCur.u8ValidType = 1;
	gOnOffContex[index].g_light.sOLStateCur.u8LightSkillType = LIGHT_SKILL_CCT;
	gOnOffContex[index].g_light.sOLStateCur.strCCTRGB.unCCTRGB.u16CCT = u16Cct;
	gOnOffContex[index].g_light.sOLStateCur.strCCTRGB.u8CCTRGBValid = 1; //1:cct,2:rgb

	// 使用调光算法
	gOnOffContex[index].g_light.pDimAlog(&gOnOffContex[index].g_light.sOLStateSet, &level);

	// 观察者钩子
	ONOFF_OBSERVER(index,EVENT_SET_CCT, src, &gOnOffContex[index].g_light.sOLStateSet);
	MUTEX_UNLOCK(gMutex[index]);

	// Level输出
	OppLight_LocalSetDevLvlOutput(index, src, &level, u16TransitionMs, u8IsWithOnOff);

	
	return 0;
}

static U8 OppLight_CCT_SetOnOff(const U8 index, U8 u8CmdResourceIndex,U8 u8OnOff,  U16 u16TransitionmMs)
{
	CHECK_LIGHT_INDEX(index);

	OppLight_Bri_SetOnOff(index, u8CmdResourceIndex,u8OnOff,   u16TransitionmMs);

	return 0;
}

U8 OppLight_CCT_Init(const U8 index, const t_bsp_output_cb* sBspOutputCb, const pfunDimAlgorithm fun)
{
	OppLight_Bri_Init(index, sBspOutputCb, fun);

	return 0;
}

U8 Create_I_OPP_LIGHT_CCT(I_OPP_LIGHT_CCT * pOL_CCT, U8 * uP8LightIndex)
{  
	if (pOL_CCT == NULL)
	{
		return 1;
	}

	U8 u8Ret = Create_I_OPP_LIGHT_BRI((I_OPP_LIGHT_BRI*)pOL_CCT, uP8LightIndex);

	pOL_CCT->SetCCT = OppLight_CCT_SetCCT;
	pOL_CCT->Init = OppLight_CCT_Init;
	pOL_CCT->SetOnOff = OppLight_CCT_SetOnOff;
	
	return u8Ret;
} 

