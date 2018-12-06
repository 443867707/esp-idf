/******************************************************************************
* Version     :									                              *
* File        : LightOnOff.h                                                  *
* Description :                                                               *
*               OPPLE 具备OnOff功能的灯抽象结构定义头文件                     *
* Note        : (none)                                                        *
* Author      : 智能控制部                                                    *
* Date:       : 2018-03-13                                                    *
* Mod History : (none)                                                        *
******************************************************************************/
#ifndef __OppLight_OnOff_H__
#define __OppLight_OnOff_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "LightDef.h"
//#include "LightPrivateData.h"



#define ONOFF_OBSERVER(index,event,src,state)\
if(gOnOffContex[index].observer!=0)\
{\
	gOnOffContex[index].observer(event,src,state);\
}



/******************************************************************************
*                             OppLight_OnOff                                  *
******************************************************************************/
typedef struct _I_OPP_LIGHT_ONOFF I_OPP_LIGHT_ONOFF;   

typedef enum
{
	BLINK_OFF = 0,
	BLINK_ON,
}t_blink_state;

typedef struct
{
	u8 state;
	t_blink_state blinkState;
	U32 moment;
	U16 onTimeMs;
	U16 onTrsTimeMs;
	U16 offTimeMs;
	U16 offTrsTimeMs;
}t_blink;

typedef enum
{
	ONOFF_EVENT_SET_ONOFF=0,
}t_onoff_event;

typedef struct
{
	OPP_LIGHT_DATA g_light;
	t_blink g_blink;
	pfObserVer observer;
}t_onoff_data;  // 从这里开始，实现数据集成

/******************************************************************************
*                           Interfaces                                       *
******************************************************************************/
U8 Create_I_OPP_LIGHT_ONOFF(I_OPP_LIGHT_ONOFF * pOP_Light_OnOff, U8 * uP8LightIndex);
U8 OppLight_OnOff_Init(const U8 index, const t_bsp_output_cb* sBspOutputCb, const pfunDimAlgorithm fun);
U8 isTimeOn(U32 moment, U32 now, U32 time);
U8 OppLight_OnOff_ProcLightTask(const U8 index, U32 u32SysTick);
U8 OppLight_OnOff_SetOnOff(const U8 index, U8 src, U8 u8OnOff, U16 u16TransitionmMs);

#ifdef __cplusplus
}
#endif

#endif/*__OppLight_OnOff_H__*/


