/******************************************************************************
* Version     :								      *
* File        : LightPrivateData.h                                            *
* Description :                                                               *
*               OPPLE 灯状态私有数据结构定义头文件                            *
* Note        : (none)                                                        *
* Author      : 智能控制部                                                    *
* Date:       : 2018-03-13                                                    *
* Mod History : (none)                                                        *
******************************************************************************/

#ifndef __LightPrivateData_H__
#define __LightPrivateData_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../PublicHeader/Opp_Std.h" 
#include "../PublicHeader/LightDef.h"

#define MAX_LIGHT_NUM 10

#define CHECK_LIGHT_INDEX(index) if(index>MAX_LIGHT_NUM)\
                                 {return 1;}

	/******************************************************************************
	*                            OnOff_Data_Body                                  *
	******************************************************************************/
#define OnOff_Data_Body						 \
											 \
	void *					pOppLight;		 \
	t_bsp_output_cb     	sBspOutputCb;	 \
	pfunDimAlgorithm 	    pDimAlog;		 \
											 \
	U16						u16TickCircle;	 \
											 \
	OPP_LIGHT_STATE *		sOLStateCur;	 \
	OPP_LIGHT_STATE *		sOLStateSet;

	typedef struct 
	{
		void *					pOppLight;
	  t_bsp_output_cb 	    sBspOutputCb;		
		pfunDimAlgorithm 	    pDimAlog;
		OPP_LIGHT_STATE 		sOLStateCur;	 
		OPP_LIGHT_STATE 		sOLStateSet;
	}OPP_LIGHT_DATA;	

	/******************************************************************************
	*                             Bri_Data_Body                                   *
	******************************************************************************/
#define Bri_Data_Body						 \
	OnOff_Data_Body							 \
											 \
	OPP_LIGHT_DEV_LVL *		pOLDevLvl;

	struct OPP_LIGHT_BRI_DATA
	{
		Bri_Data_Body
	};

	/******************************************************************************
	*                           CCTRGB_Data_Body                                  *
	******************************************************************************/
#define CCTRGB_Data_Body					 \
	Bri_Data_Body							 \
											 \
	CCTRGB_STRUCT *			pCCTRGB;

	struct OPP_LIGHT_CCTRGB_DATA
	{
		CCTRGB_Data_Body
	};

#ifdef __cplusplus
}
#endif

#endif/*__LightPrivateData_H__*/
