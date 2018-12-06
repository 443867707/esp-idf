/******************************************************************************
* Version     :									                              *
* File        : LightRGB.h                                                    *
* Description :                                                               *
*               OPPLE �߱�RGB���ܵĵƳ���ṹ����ͷ�ļ�                       *
* Note        : (none)                                                        *
* Author      : ���ܿ��Ʋ�                                                    *
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
