/******************************************************************************
* Version     :									                              *
* File        : LightCct.h                                                    *
* Description :                                                               *
*               OPPLE �߱�CCT���ܵĵƳ���ṹ����ͷ�ļ�                       *
* Note        : (none)                                                        *
* Author      : ���ܿ��Ʋ�                                                    *
* Date:       : 2018-03-13                                                    *
* Mod History : (none)                                                        *
******************************************************************************/
#ifndef __OppLight_Cct_H__
#define __OppLight_Cct_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "LightBri.h"

/******************************************************************************
*                             OppLight_CCT                                    *
******************************************************************************/
typedef struct _I_OPP_LIGHT_CCT I_OPP_LIGHT_CCT;

/******************************************************************************
*                            Create & Delete                                  *
******************************************************************************/
U8 Create_I_OPP_LIGHT_CCT(I_OPP_LIGHT_CCT * pOL_CCT, U8 * uP8LightIndex);
U8 OppLight_CCT_Init(const U8 index, const t_bsp_output_cb* sBspOutputCb, const pfunDimAlgorithm fun);

#ifdef __cplusplus
}
#endif

#endif/*__OppLight_Cct_H__*/
