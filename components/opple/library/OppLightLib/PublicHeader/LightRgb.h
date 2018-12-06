/******************************************************************************
* Version     :									                              *
* File        : LightRGB.h                                                    *
* Description :                                                               *
*               OPPLE 具备RGB功能的灯抽象结构定义头文件                       *
* Note        : (none)                                                        *
* Author      : 智能控制部                                                    *
* Date:       : 2018-03-13                                                    *
* Mod History : (none)                                                        *
******************************************************************************/
#ifndef __OppLight_RGB_H__
#define __OppLight_RGB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "LightCct.h"

/******************************************************************************
*                             OppLight_RGB                                    *
******************************************************************************/
typedef struct _I_OPP_LIGHT_RGB  I_OPP_LIGHT_RGB;  

/******************************************************************************
*                            Create & Delete                                  *
******************************************************************************/
U8 Create_I_OPP_LIGHT_RGB(I_OPP_LIGHT_RGB * pOL_RGB, U8 * uP8LightIndex);

#ifdef __cplusplus
}
#endif

#endif/*__OppLight_RGB_H__*/
