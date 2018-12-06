/******************************************************************************
* Version     :									                              *
* File        : LightBri.h                                                    *
* Description :                                                               *
*               OPPLE 具备Bright功能的灯抽象结构定义头文件                    *
* Note        : (none)                                                        *
* Author      : 智能控制部                                                    *
* Date:       : 2018-03-13                                                    *
* Mod History : (none)                                                        *
******************************************************************************/
#ifndef __OppLight_Bri_H__
#define __OppLight_Bri_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "LightOnOff.h"



#define DIM_STEP_TIME_MS  ((U32)20)   // 20ms步进一次
#define DIM_STEP_TIME_MS_DELT ((DIM_STEP_TIME_MS)>>1)
#define DIM_DEV_LEVEL_MAX 10000

/******************************************************************************
*                             OppLight_Bri                                    *
******************************************************************************/
typedef enum
{
	DIM_STATE_IDLE = 0,
	DIM_STAE_START,
	DIM_STATE_DIM,
	DIM_STATE_END,
}t_dim_state;

typedef S32 dimSetp[DEV_CHANNEL_SIZE];
typedef struct
{
	U8 state;
	OPP_LIGHT_DEV_LVL setValue;
	OPP_LIGHT_DEV_LVL curValue;
	OPP_LIGHT_DEV_LVL outValue;
	U8 isWithOnOff;
	dimSetp           step;
	U32 tick;
	U32 moment;
	U32 trsMs;
}t_dim;

typedef struct
{
	t_dim dim;
	t_level_limit limit;
	t_blink g_blink;
	U8 u8CmdSourceIndex;
}t_bri;

typedef struct _I_OPP_LIGHT_BRI  I_OPP_LIGHT_BRI;  

/******************************************************************************
*                            Interfaces                                       *
******************************************************************************/
U8 Create_I_OPP_LIGHT_BRI(I_OPP_LIGHT_BRI * pOL_Bri, U8 * uP8LightIndex);
U16 OppLight_SetDevLvlOutput(const U8 index, U8 u8CmdSourceIndex, OPP_LIGHT_DEV_LVL * pLvlOutput, U16 u16TransitionMs,U8 u8IsWithOnOff);
U16 OppLight_LocalSetDevLvlOutput(const U8 index, U8 u8CmdSourceIndex, OPP_LIGHT_DEV_LVL * pLvlOutput, U16 u16TransitionMs, U8 u8IsWithOnOff);
U8 OppLight_Bri_Init(const U8 index, const t_bsp_output_cb* sBspOutputCb, const pfunDimAlgorithm fun);
U8 OppLight_Bri_SetOnOff(const U8 index, U8 u8CmdResourceIndex, U8 u8OnOff, U16 u16TransitionmMs);


#ifdef __cplusplus
}
#endif

#endif/*__OppLight_Bri_H__*/

