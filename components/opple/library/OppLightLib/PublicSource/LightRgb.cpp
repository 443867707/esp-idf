#include "../PublicHeader/LightRgb.h"
#include "../PrivateHeader/LightPrivateData.h"
#include <string.h>

extern t_onoff_data gOnOffContex[MAX_LIGHT_NUM];
extern t_bri gBriContex[MAX_LIGHT_NUM];
extern T_MUTEX gMutex[MAX_LIGHT_NUM];


static U8 OppLight_RGB_SetRGB(const U8 index, U16 * u8PRGB, U8 src, U16 u16TransitionMs, U8 u8IsWithOnOff)
{ 
	CHECK_LIGHT_INDEX(index);
	OPP_LIGHT_DEV_LVL  level;
	U16 *prgb;

	MUTEX_LOCK(gMutex[index],MUTEX_WAIT_ALWAYS);
	
	// 更新设置值
	gOnOffContex[index].g_light.sOLStateSet.u8LightSkillType = LIGHT_SKILL_RGB;
	prgb = gOnOffContex[index].g_light.sOLStateSet.strCCTRGB.unCCTRGB.u8RGBArry;
	memcpy(prgb, u8PRGB, RGB_ARRY_SIZE*2);
	gOnOffContex[index].g_light.sOLStateSet.strCCTRGB.u8CCTRGBValid = 2; //1:cct,2:rgb

	// 更新当前值
	gOnOffContex[index].g_light.sOLStateCur.u8ValidType = 1;
	gOnOffContex[index].g_light.sOLStateCur.u8LightSkillType = LIGHT_SKILL_RGB;
	prgb = gOnOffContex[index].g_light.sOLStateCur.strCCTRGB.unCCTRGB.u8RGBArry;
	memcpy(prgb, u8PRGB, RGB_ARRY_SIZE*2);
	gOnOffContex[index].g_light.sOLStateCur.strCCTRGB.u8CCTRGBValid = 2;//1:cct,2:rgb

	// 调用调光算法
	gOnOffContex[index].g_light.pDimAlog(&gOnOffContex[index].g_light.sOLStateSet, &level);

	// 观察者回调
	ONOFF_OBSERVER(index,EVENT_SET_RGB, src, &gOnOffContex[index].g_light.sOLStateSet);
	MUTEX_UNLOCK(gMutex[index]);

	// Level输出
	OppLight_LocalSetDevLvlOutput(index, src, &level, u16TransitionMs, u8IsWithOnOff);

	return 0;
}  

static U8 OppLight_RGB_SetOnOff(const U8 index, U8 u8CmdResourceIndex, U8 u8OnOff, U16 u16TransitionmMs)
{
	CHECK_LIGHT_INDEX(index);

	OppLight_Bri_SetOnOff(index, u8CmdResourceIndex,u8OnOff,  u16TransitionmMs);

	return 0;
}

U8 OppLight_RGB_Init(const U8 index, const t_bsp_output_cb* sBspOutputCb, const pfunDimAlgorithm fun)
{
	OppLight_CCT_Init(index, sBspOutputCb, fun);

	return 0;
}

U8 Create_I_OPP_LIGHT_RGB(I_OPP_LIGHT_RGB * pOL_RGB, U8 * uP8LightIndex)
{  
	if (pOL_RGB == NULL)
	{
		return 1;
	}

	U8 u8Ret = Create_I_OPP_LIGHT_CCT((I_OPP_LIGHT_CCT*)pOL_RGB, uP8LightIndex);
	pOL_RGB->SetRGB = OppLight_RGB_SetRGB;
	pOL_RGB->SetOnOff = OppLight_RGB_SetOnOff;
	pOL_RGB->Init = OppLight_RGB_Init;

	return u8Ret;
} 


















