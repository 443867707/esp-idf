/******************************************************************************
* Version     : OPP_PLATFORM V100R001C01B001D001                              *
* File        : Opp_ErrorCode.h                                               *
* Description :                                                               *
*               OPPLE平台错误码定义头文件                                     *
* Note        : (none)                                                        *
* Author      : 智能控制研发部                                                *
* Date:       : 2013-08-09                                                    *
* Mod History : (none)                                                        *
******************************************************************************/



#ifndef __OPP_ERRORCODE_H__
#define __OPP_ERRORCODE_H__



#ifdef __cplusplus
extern "C" {
#endif



/******************************************************************************
*                                Includes                                     *
******************************************************************************/




/******************************************************************************
*                                Defines                                      *
******************************************************************************/




/******************************************************************************
*                                Typedefs                                     *
******************************************************************************/

#pragma pack(1)


/*OPPLE错误码(失败码/原因码)定义*/
typedef enum
{
    OPP_SUCCESS = 0, 
    OPP_FAILURE = 1, 
    OPP_OVERFLOW = 2, 
    OPP_NO_PRIVILEGE = 3, 
    OPP_EXISTED = 4, 
    OPP_UNINIT = 5,
    OPP_NULL_POINTER = 6,
    OPP_RANGE_EXPIRED = 7,
    OPP_NBLED_INIT_ERR = 8,
    OPP_DIMCTRL_INIT_ERR = 9,
	OPP_METER_INIT_ERR = 10,
	OPP_GPS_INIT_ERR = 11,
	OPP_BC28_INIT_ERR = 12,
	OPP_LIGHT_SENSOR_INIT_ERR = 13,	
	OPP_BC28_UART_WRITE_ERR = 14,
	OPP_BC28_UART_READ_TIMEOUT = 15,
	OPP_BC28_BAND_INVALIDE = 16,
	OPP_BC28_READ_MSG_NO_OK = 17,
	OPP_BC28_READ_MSG_NO_CGSN = 18,	
	OPP_BC28_READ_MSG_NO_CCLK = 19,
	OPP_BC28_READ_MSG_NO_REBOOTING = 20,
	OPP_BC28_AT_REBOOT_ERR = 21,
	OPP_BC28_ATE_ERR = 22,
	OPP_BC28_HWDET_ERR = 23,
	OPP_BC28_AUTO_CON_ERR = 24,
	OPP_BC28_SCRAM_ERR = 25,
	OPP_BC28_RESELECT_ERR = 26,
	OPP_BC28_FREQUENCY_ERR =27,
	OPP_BC28_PSM_ERR = 28,
	OPP_BC28_PSM_REPORT_ERR = 29,
	OPP_BC28_ACTIVE_NETWORK_ERR = 30,
	OPP_COAP_CREATE_OBJ_ERR = 31,
	OPP_COAP_CREATE_ARRAY_ERR = 32,
	OPP_COAP_OP_ERR = 32,
	OPP_BC28_QRY_UART = 33,
	OPP_BC28_SET_UART = 34,
	OPP_BC28_ABNORMAL_ERR = 35,
    /*后续继续扩展*/
    OPP_UNKNOWN
}OPP_ERROR_CODE;


#pragma pack()



/******************************************************************************
*                           Extern Declarations                               *
******************************************************************************/



/******************************************************************************
*                              Declarations                                   *
******************************************************************************/




#ifdef __cplusplus
}
#endif



#endif /*__OPP_ERRORCODE_H__*/


