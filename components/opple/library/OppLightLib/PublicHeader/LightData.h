/******************************************************************************
* Version     :									                              *
* File        : LightData.h                                                       *
* Description :                                                               *
*               OPPLE 灯状态数据结构定义头文件                                    *
* Note        : (none)                                                        *
* Author      : 智能控制部                                                    *
* Date:       : 2018-03-13                                                    *
* Mod History : (none)                                                        *
******************************************************************************/

#ifndef __LightData_H__
#define __LightData_H__

//#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "Opp_Std.h" 

/* MUTEX */
//#define USE_T_MUTEX
#ifdef USE_T_MUTEX
#include "los_mux.h"
//T_MUTEX mutex_Light;
//MUTEX_CREATE(mutex_Light);
//MUTEX_LOCK(mutex_Light);
//MUTEX_UNLOCK(mutex_Light);
#endif

/* Define NULL pointer value */
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */

	/******************************************************************************
	*                                CCT&RGB                                      *
	******************************************************************************/
#define CR_CCT 1
#define CR_RGB 2
#define RGB_ARRY_SIZE 4
	typedef union _CCTRGB_UNION
	{
		U16				u8RGBArry[RGB_ARRY_SIZE];
		U16				u16CCT;
	}CCTRGB_UNION;

	typedef struct _CCTRGB_STRUCT
	{
		U8				u8CCTRGBValid;// 1-CCT 2-RGB //Type???
		CCTRGB_UNION	unCCTRGB;

		/*_CCTRGBStruct operator=(_CCTRGBStruct& other)
		{
		u8CCTRGBValid = other.u8CCTRGBValid;

		unCCTRGB = other.unCCTRGB;

		return *this;
		};*/
	}CCTRGB_STRUCT;

	/******************************************************************************
	*                              OppLightState                                  *
	******************************************************************************/
typedef enum
{
	LIGHT_SKILL_ONOFF=0,
	LIGHT_SKILL_LEVEL,
	LIGHT_SKILL_BRI,
	LIGHT_SKILL_CCT,
	 LIGHT_SKILL_RGB,
}t_light_skill;

	typedef struct _OPP_LIGHT_STATE
	{
		U8				u8LightSkillType;//OnOff Bri CCT RGB
		U8				u8ValidType;//Whether it is changing, valid,consistence with dev lvl
		U8        blink; // 0:normal,1:blinking
		U8				u8OnOffState;
		U16				u16BriLvl;//Levle 1k
		CCTRGB_STRUCT	strCCTRGB;
	} OPP_LIGHT_STATE;

	/******************************************************************************
	*                             OppLightDevLvl                                  *
	******************************************************************************/
#define DEV_CHANNEL_SIZE 4
	typedef struct _OPP_LIGHT_DEV_LVL
	{
		U8				u8ChannelValidFlag;
		U8				u8ChannelTotalNum;
		U16				u16ChannelLvlArry[DEV_CHANNEL_SIZE];//for 4 channel[4] lvl == 0 means off
	} OPP_LIGHT_DEV_LVL;

	typedef struct
	{
		U16 low[DEV_CHANNEL_SIZE];
		U16 high[DEV_CHANNEL_SIZE];
	}t_level_limit;;

	/******************************************************************************
	*                            OppLightDimState                                 *
	******************************************************************************/
	//typedef struct _OPP_LIGHT_DIM_STATE
	////{
	//	U16				u16BriLvl;
	//	CCTRGB_STRUCT	strCCTRGB;
	//} OPP_LIGHT_DIM_STATE;

	///////////////////////////////////////////////////////////////////////////////

	/******************************************************************************
	*                              I_DevLevel                                     *
	******************************************************************************/
	typedef struct _I_DevLevel
	{
		U8(*pFunDim)		(OPP_LIGHT_DEV_LVL *sBspOutputCb);
		U8(*pFunOnOff)		(U8 u8OnOffFlag);
	} t_bsp_output_cb;

	/******************************************************************************
	*                            I_DimAlogrithm                                   *
	******************************************************************************/
	typedef U8(*pfunDimAlgorithm)		(OPP_LIGHT_STATE * pTarget, OPP_LIGHT_DEV_LVL * pOutLevel);


	/*
	#define ONOFF_EVENT \
	EVENT_SET_ONOFF=0,

	#define BRI_EVENT \
	ONOFF_EVENT \
	EVENT_SET_BRI,

	#define CCT_EVENT \
	BRI_EVENT \
	EVENT_SET_CCT,

	#define RGB_EVENT \
	CCT_EVENT \
	EVENT_SET_RGB,

	typedef enum
	{
	ONOFF_EVENT
	}t_event_onoff;
	typedef enum
	{
	BRI_EVENT
	}t_event_bri;
	typedef enum
	{
	CCT_EVENT
	}t_event_cct;
	typedef enum
	{
	RGB_EVENT
	}t_event_rgb;
	*/
	typedef enum
	{
		EVENT_SET_ONOFF = 0,
		EVENT_SET_LEVEL,
		EVENT_SET_BRI,
		EVENT_SET_CCT,
		EVENT_SET_RGB,
		EVENT_ONOFF_CHANGED,
		EVENT_START_BLINK,
		EVENT_STOP_BLINK,

	}t_event;
	typedef void(*pfObserVer)(t_event event,U8 src, const OPP_LIGHT_STATE * const state);

#ifdef __cplusplus
}
#endif

#endif/*__LightData_H__*/
