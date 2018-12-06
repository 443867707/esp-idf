/******************************************************************************
* Version     :									                              *
* File        : LightDef.h                                                    *
* Description :                                                               *
*               OPPLE 灯抽象结构定义头文件                                    *
* Note        : (none)                                                        *
* Author      : 智能控制部                                                    *
* Date:       : 2018-03-13                                                    *
* Mod History : (none)                                                        *
******************************************************************************/

#ifndef __LightDef_H__
#define __LightDef_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "LightData.h"
#include "LightPrivateData.h"
#include "OS.h"


#define SOURCE_INDEX_LOCAL 0

	//#define not struct contain struct, will be access member quickly
/******************************************************************************
*                            OnOff_Struct_Body                                *
******************************************************************************/
#define OnOff_Struct_Body																												 \
																																		 \
	U8(*Init)						(const U8 u8LightIndex, const t_bsp_output_cb* sBspOutputCb, const pfunDimAlgorithm pCalLvl);		 \
	U8(*ProcLightTask)				(const U8 u8LightIndex, U32 u32SysTick);															 \
																																		 \
	U8(*SetOnOff)					(const U8 u8LightIndex, U8 u8CmdResourceIndex,U8 u8OnOff,  U16 u16TransitionmMs);					 \
	U8(*GetState)					(const U8 u8LightIndex, OPP_LIGHT_STATE * pOutState);						 						 \
	U8(*SetState)					(const U8 u8LightIndex, U8 u8CmdResourceIndex,OPP_LIGHT_STATE * pInState);	                         \
	U8(*RegisterStateObserver)		(const U8 u8LightIndex, U8 u8CmdResourceIndex,pfObserVer fun);								         \
	U8(*UnRegisterStateObserver)	(const U8 u8LightIndex);														                     \
	U8(*StartBlink)					(const U8 u8LightIndex, U8 u8CmdResourceIndex, U16 onTimeMs, U16 offTimeMs);                      	 \
	U8(*StopBlink)                  (const U8 u8LightIndex, U8 u8CmdResourceIndex);                                                      \
	U8(*SetTickCircle)				(const U8 u8LightIndex, U16 u16TickCicle);															 \

	
struct _I_OPP_LIGHT_ONOFF 
{  
	OnOff_Struct_Body;
};  

/******************************************************************************
*                            Bri_Struct_Body                                  *
******************************************************************************/
#define Bri_Struct_Body																													 \
	OnOff_Struct_Body;																													 \
																																		 \
	U8(*SetBrightLvl)				(const U8 u8LightIndex, U16 u16Level, U8 u8CmdSourceIndex, U16 u16TransitionMs, U8 u8IsWithOnOff);	 \
	U16(*SetDevLvlOutput)			(const U8 u8LightIndex, U8 u8CmdSourceIndex,OPP_LIGHT_DEV_LVL * pLvlOutput, U16 u16TransitionMs, U8 u8IsWithOnOff);	 \
	U8(*GetCurDevLvl)				(const U8 u8LightIndex, OPP_LIGHT_DEV_LVL * pOutDevLvl);\
  U8(*SetLimit)                   (const U8 index, t_level_limit* limit)

struct _I_OPP_LIGHT_BRI 
{ 
	Bri_Struct_Body;
	U8 (*SetDevLvlOutputLimit)(const U8 index, t_level_limit* limit);
	U8 (*UpdateOutput)(const U8 u8LightIndex);                                                                             \
};  

/******************************************************************************
*                            CCT_Struct_Body                                  *
******************************************************************************/
#define CCT_Struct_Body																													 \
	Bri_Struct_Body;																													 \
																																		 \
	U8(*SetCCT)						(const U8 u8LightIndex, U16 u16CCT, U8 u8CmdSourceIndex, U16 u16TransitionMs, U8 u8IsWithOnOff)

struct _I_OPP_LIGHT_CCT
{  
	CCT_Struct_Body;	
};

/******************************************************************************
*                            RGB_Struct_Body                                  *
******************************************************************************/
#define RGB_Struct_Body																													 \
	CCT_Struct_Body;																													 \
																																		 \
	U8(*SetRGB)						(const U8 u8LightIndex, U16 * u8PRGB, U8 u8CmdSourceIndex, U16 u16TransitionMs, U8 u8IsWithOnOff)	 \
	
struct _I_OPP_LIGHT_RGB
{  
	RGB_Struct_Body;	
};  

/*
#define MULTITASK_SUPPORT_ENABLE 0
#if MULTITASK_SUPPORT_ENABLE==1
#define T_MUTEX                 unsigned int              // xSemaphoreHandle
#define MUTEX_CREATE(mutex)     LOS_MuxCreate(&mutex)     // mutex=xSemaphoreCreateMutex()
#define MUTEX_LOCK(mutex)       LOS_MuxPend(mutex,100)    // xSemaphoreTake((mutex),3)
#define MUTEX_UNLOCK(mutex)     LOS_MuxPost(mutex)        // xSemaphoreGive((mutex))
#else
#define T_MUTEX                 unsigned int         
#define MUTEX_CREATE(gMutex)     
#define MUTEX_LOCK(gMutex)    
#define MUTEX_UNLOCK(gMutex)   
#endif
*/


#ifdef __cplusplus
}
#endif

#endif/*__LightDef_H__*/
