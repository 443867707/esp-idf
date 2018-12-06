#include "cli-interpreter.h"
#include "LightDef.h"
#include "LightData.h"
#include <stdio.h>
#include "LightOnOff.h"
#include "cli-interpreter.h"
#include "LightRgb.h"

//#include "stm32f4xx_hal.h"

//#define LIGHT_CLI_GET_SYSTICK() HAL_GetTick()
#define LIGHT_CLI_GET_SYSTICK()  0

I_OPP_LIGHT_ONOFF light_onoff;
U8 light_onoff_index;
I_OPP_LIGHT_RGB light_rgb;
U8 light_rgb_index;
I_OPP_LIGHT_BRI light_bri;
U8 light_bri_index;



extern void ledon(unsigned char led);
extern void ledoff(unsigned char led);
void lightlib_create(void);
void lightlib_onoff(void);
void lightlib_startblink(void);
void lightlib_stopblink(void);

void lightlib_rgb_onoff(void);
void lightlib_rgb_startblink(void);
void lightlib_rgb_stopblink(void);
void lightlib_rgb_setlimit(void);
void lightlib_rgb_setrgb(void);

void lightlib_bri_onoff(void);
void lightlib_bri_startblink(void);
void lightlib_bri_stopblink(void);
void lightlib_bri_setlimit(void);
void lightlib_bri_setbri(void);
void lightlib_bri_getbri(void);


const char* const arg_startblink[] = {
  "On time(ms)",
	"Off time(ms)"
};

const char* const arg_setrgb[] = {
  "RGB array,{r,g,b,w}",
	"Transaction time(ms)",
	"With OnOff(0/1)"
};

const char* const arg_rgbonoff[] = {
  "OnOff(0:off,1:on)",
	"Transaction time(ms)"
};

const char* const arg_rgb_setlimit[] = {
  "High limit {H,H,H,H}",
	"Low limit {L,L,L,L}"
};

const char* const arg_setbri[] = {
  "Bri 0-10000",
	"Transaction time(ms)",
	"With OnOff(0/1)"
};

CommandEntry CommandTableLightlibOnoff[]=
{
	//CommandEntryActionWithDetails("create", lightlib_create, "", "LightLib create...", (const char* const *)NULL),
  CommandEntryActionWithDetails("onoff", lightlib_onoff, "u", "LightLib setonoff...", (const char* const *)NULL),
	CommandEntryActionWithDetails("startblink", lightlib_startblink, "uu", "LightLib start blink...", arg_startblink),
	CommandEntryActionWithDetails("stopblink", lightlib_stopblink, "", "LightLib stop blink...", (const char* const *)NULL),
	
	
	CommandEntryTerminator()
};

CommandEntry CommandTableLightlibRgb[]=
{
	//CommandEntryActionWithDetails("create", lightlib_create, "", "LightLib create...", (const char* const *)NULL),
  CommandEntryActionWithDetails("onoff", lightlib_rgb_onoff, "uu", "LightLib setonoff...", arg_rgbonoff),
	CommandEntryActionWithDetails("startblink", lightlib_rgb_startblink, "uu", "LightLib start blink...", arg_startblink),
	CommandEntryActionWithDetails("stopblink", lightlib_rgb_stopblink, "", "LightLib stop blink...", (const char* const *)NULL),
	CommandEntryActionWithDetails("setlimit", lightlib_rgb_setlimit, "aa", "LightLib set limit(HL)...", arg_rgb_setlimit),
	CommandEntryActionWithDetails("setrgb", lightlib_rgb_setrgb, "auu", "LightLib set rgb...", arg_setrgb),
	CommandEntryTerminator()
};

CommandEntry CommandTableLightlibBri[]=
{
	//CommandEntryActionWithDetails("create", lightlib_create, "", "LightLib create...", (const char* const *)NULL),
  CommandEntryActionWithDetails("onoff", lightlib_bri_onoff, "uu", "LightLib setonoff...", arg_rgbonoff),
	CommandEntryActionWithDetails("startblink", lightlib_bri_startblink, "uu", "LightLib start blink...", arg_startblink),
	CommandEntryActionWithDetails("stopblink", lightlib_bri_stopblink, "", "LightLib stop blink...", (const char* const *)NULL),
	CommandEntryActionWithDetails("setlimit", lightlib_bri_setlimit, "aa", "LightLib set limit(HL)...", arg_rgb_setlimit),
	CommandEntryActionWithDetails("setbri", lightlib_bri_setbri, "uuu", "LightLib set rgb...", arg_setbri),
	CommandEntryActionWithDetails("getbri", lightlib_bri_getbri, "", "LightLib get rgb...", (const char* const *)NULL),
	CommandEntryTerminator()
};

CommandEntry CommandTableLightlib[]=
{
	CommandEntrySubMenu("onoff", CommandTableLightlibOnoff, ""),
	CommandEntrySubMenu("rgb", CommandTableLightlibRgb, ""),
	CommandEntrySubMenu("bri", CommandTableLightlibBri, ""),

	CommandEntryTerminator()
};

U8 bsp_level_output(OPP_LIGHT_DEV_LVL *sBspOutputCb);
U8 bsp_onoff_output(U8 u8OnOffFlag);
U8 bsp_level_onoff_output(U8 u8OnOffFlag);

t_bsp_output_cb bsp_cb={
	.pFunDim = NULL,
	.pFunOnOff = bsp_onoff_output,
};

t_bsp_output_cb bsp_level_cb={
	.pFunDim = bsp_level_output,
	.pFunOnOff = bsp_level_onoff_output,
};

U8 bsp_level_output(OPP_LIGHT_DEV_LVL *sBspOutputCb)
{
	CLI_PRINTF("Bsp_level_output:[");
	for(U8 i = 0;i < sBspOutputCb->u8ChannelTotalNum;i++)
	{
		CLI_PRINTF("%d ",sBspOutputCb->u16ChannelLvlArry[i]);
	}
	CLI_PRINTF("]\r\n");
	
	return 0;
}

U8 bsp_level_onoff_output(U8 u8OnOffFlag)
{
	//CLI_PRINTF("Bsp_onoff_output:%d\r\n",u8OnOffFlag);
	if(u8OnOffFlag == 0)
	{
		//ledoff(1);
		ledoff(2);
	}
	else{
		//ledon(1);
		ledon(2);
	}
	
	return 0;
}

U8 bsp_onoff_output(U8 u8OnOffFlag)
{
	//CLI_PRINTF("Bsp_onoff_output:%d\r\n",u8OnOffFlag);
	if(u8OnOffFlag == 0)
	{
		//ledoff(1);
	}
	else{
		//ledon(1);
	}
	
	return 0;
}

U8 DimAlgorithm(OPP_LIGHT_STATE * pTarget, OPP_LIGHT_DEV_LVL * pOutLevel)
{
	U8 i;
	// bri/cct/rgb
	if(pTarget->u8LightSkillType == LIGHT_SKILL_RGB){
		pOutLevel->u8ChannelTotalNum = 4;
		for(i = 0;i <pOutLevel->u8ChannelTotalNum;i++)
		{
		    pOutLevel->u16ChannelLvlArry[i] = pTarget->strCCTRGB.unCCTRGB.u8RGBArry[i];
		}
  }else if(pTarget->u8LightSkillType == LIGHT_SKILL_BRI)
	{
		pOutLevel->u8ChannelTotalNum = 4;
		for(i = 0;i <pOutLevel->u8ChannelTotalNum;i++)
		{
		    pOutLevel->u16ChannelLvlArry[i] = pTarget->u16BriLvl;
		}
	}else{
	}
	
	return 0;
}

void observer(t_event event,U8 src, const OPP_LIGHT_STATE * const state)
{
	CLI_PRINTF("Observer: event:%d,src:%d,state:%d\r\n",event,src,state->u8OnOffState);
}

void lightlib_create(void)
{
	U8 res;
	// create
	res = Create_I_OPP_LIGHT_ONOFF(&light_onoff,&light_onoff_index);
	if(res != 0) CLI_PRINTF("Lightlib create fail!");
	res = Create_I_OPP_LIGHT_RGB(&light_rgb,&light_rgb_index);
	if(res != 0) CLI_PRINTF("Lightlib create fail!");
	res = Create_I_OPP_LIGHT_BRI(&light_bri,&light_bri_index);
	if(res != 0) CLI_PRINTF("Lightlib create fail!");
	
	// init
	light_onoff.Init(light_onoff_index,&bsp_cb,DimAlgorithm);
	light_rgb.Init(light_rgb_index,&bsp_level_cb,DimAlgorithm);
	light_bri.Init(light_bri_index,&bsp_level_cb,DimAlgorithm);
	
	light_rgb.SetOnOff(light_rgb_index,1,0,0);
	
	// observer
	light_onoff.RegisterStateObserver(light_onoff_index,1,observer);
}
void lightlib_cli_loop(void)
{
	if(light_onoff.ProcLightTask != NULL)
		{
	    light_onoff.ProcLightTask(light_onoff_index,LIGHT_CLI_GET_SYSTICK());
	}
	if(light_rgb.ProcLightTask != NULL)
	{
		light_rgb.ProcLightTask(light_rgb_index,LIGHT_CLI_GET_SYSTICK());
	}
	if(light_bri.ProcLightTask != NULL)
	{
		light_bri.ProcLightTask(light_bri_index,LIGHT_CLI_GET_SYSTICK());
	}
}
/***************************************************************************************/
void lightlib_onoff(void)
{
	light_onoff.SetOnOff(light_onoff_index,1,CliIptArgList[0][0],0);
}
void lightlib_startblink(void)
{
	  light_onoff.StartBlink(light_onoff_index,1,CliIptArgList[0][0],CliIptArgList[1][0]);
}
void lightlib_stopblink(void)
{
		light_onoff.StopBlink(light_onoff_index,1);
}

/*******************************************************************************************/
void lightlib_rgb_onoff(void)
{
	light_rgb.SetOnOff(light_rgb_index,1,CliIptArgList[0][0],CliIptArgList[1][0]);
}
void lightlib_rgb_startblink(void)
{
	light_rgb.StartBlink(light_rgb_index,1,CliIptArgList[0][0],CliIptArgList[1][0]);
}
void lightlib_rgb_stopblink(void)
{
	light_rgb.StopBlink(light_rgb_index,1);
}

void lightlib_rgb_setlimit(void)
{
	t_level_limit limit;
	
	for(U8 i = 0;i < 4;i++)
	{
		limit.high[i] = CliIptArgList[0][i];
		limit.low[i] = CliIptArgList[1][i];
	}
	
	light_rgb.SetLimit(light_rgb_index,&limit);
}

void lightlib_rgb_setrgb(void)
{
	U16 rgb[4];
	for(U8 i = 0;i < 4;i++)
	{
		rgb[i] = CliIptArgList[0][i];
	}
	// static U8 OppLight_RGB_SetRGB(const U8 index, U8 * u8PRGB, U8 src, U16 u16TransitionMs, U8 u8IsWithOnOff)
	light_rgb.SetRGB(light_rgb_index,rgb,1,CliIptArgList[1][0],CliIptArgList[2][0]);
}

/*******************************************************************************************/
void lightlib_bri_onoff(void)
{
	light_bri.SetOnOff(light_bri_index,1,CliIptArgList[0][0],CliIptArgList[1][0]);
}
void lightlib_bri_startblink(void)
{
	light_bri.StartBlink(light_bri_index,1,CliIptArgList[0][0],CliIptArgList[1][0]);
}
void lightlib_bri_stopblink(void)
{
	light_bri.StopBlink(light_bri_index,1);
}

void lightlib_bri_setlimit(void)
{
	t_level_limit limit;
	
	for(U8 i = 0;i < 4;i++)
	{
		limit.high[i] = CliIptArgList[0][i];
		limit.low[i] = CliIptArgList[1][i];
	}
	
	light_bri.SetLimit(light_bri_index,&limit);
}

void lightlib_bri_setbri(void)
{
	// static U8 OppLight_RGB_SetRGB(const U8 index, U8 * u8PRGB, U8 src, U16 u16TransitionMs, U8 u8IsWithOnOff)
	light_bri.SetBrightLvl(light_bri_index,CliIptArgList[0][0],1,CliIptArgList[1][0],CliIptArgList[2][0]);
}

void lightlib_bri_getbri(void)
{
	OPP_LIGHT_STATE state;
	
	light_bri.GetState(light_bri_index,&state);
	CLI_PRINTF("Current Bri:%d\r\n",state.u16BriLvl);
}
/**************************************************************************************/

void lightlib_cli_init(void)
{
	lightlib_create();
}
