#ifndef _SVS_COAP_H
#define _SVS_COAP_H

#include <SVS_Time.h>
#include <SVS_Log.h>
#include <SVS_Exep.h>
#include <APP_Daemon.h>
#include <LIB_List.h>
#include <cJSON.h>

#pragma pack(1)
#define CODEC_HDR     0xFBFB

#define DEVICE_REQ_MSG    0x00
#define DEVICE_RSP_MSG    0x01
#define CLOUD_REQ_MSG     0x02
#define CLOUD_RSP_MSG     0x03

///////////////////////////
#define PROP_SERVICE        0x20
#define FUNC_SERVICE        0x21
#define LOG_SERVICE         0x22

//alarm default index
#define OVER_VOLALARMID       10000
#define OVER_VOL_ALARM_IDX    0
#define UNDER_VOLALARMID       10001
#define UNDER_VOL_ALARM_IDX   1
#define OVER_CURALARMID       10002
#define OVER_CUR_ALARM_IDX    2
#define UNDER_CURALARMID       10003
#define UNDER_CUR_ALARM_IDX   3
#define EXEP_ONALARMID       10004
#define EXEP_ON_ALARM_IDX   4
#define EXEP_OFFALARMID       10005
#define EXEP_OFF_ALARM_IDX   5

//change report
#define VOL_PROPCFG_IDX    0
#define CUR_PROPCFG_IDX   1
#define NET_PROPCFG_IDX    3
#define SWITCH_PROPCFG_IDX    4
#define BRI_PROPCFG_IDX    5
//period report
#define HEARTBEAT_PERIOD_IDX    6

#define COAP_RETRY_TO        180000
#define COAP_RX_HIGH_LIMIT_TO     (90*60*1000)    /*90min->5400000ms*/
#define COAP_RX_LOW_LIMIT_TO     (15*60*1000)
#define COAP_RX_DELTA             (60000)     /*heartbeat response delta time*/
#define COAP_RX_DELTA1            (300000)    /*attach delta and online response time*/
#define INT_MAX                   (2147483647)
#define SHORT_MAX                 (65535)
#define EC_MAX                    (178956970)
#define ONLINE_TO                 (60000)


#define DIS_HEART_ID    0

#define APS_COAPS_JSON_FORMAT_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,reqId,service,json,node,mid) do{ \
	cJSON *obj = NULL; \
	obj = cJSON_GetObjectItem(json, node); \
	if(NULL == obj){ \
		ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,service,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); \
		cJSON_Delete(json); \
		return; \
	} \
}while(0) \

#define APS_COAPS_JSON_TYPE_ERROR_WITH_FREE(dstChl,dstInfo,isNeedRsp,reqId,service,json,type,mid) do{ \
					int ret; \
					if(type == cJSON_Number) \
						ret = cJSON_IsNumber(json); \
					else if(type == cJSON_String) \
						ret = cJSON_IsString(json); \
					else if(type == cJSON_Array) \
						ret = cJSON_IsArray(json); \
					else if(type == cJSON_Object) \
						ret = cJSON_IsObject(json); \
					else if(type == cJSON_NULL) \
						ret = cJSON_IsNull(json); \
					else if(type == cJSON_Raw) \
						ret = cJSON_IsRaw(json); \
					else if(type == cJSON_False) \
						ret = cJSON_IsFalse(json); \
					else if(type == cJSON_True) \
						ret = cJSON_IsFalse(json); \
					else \
						ret = 0; \
					if(!ret){ \
						DEBUG_LOG(DEBUG_MODULE_COAP,DLL_ERROR,"node name:%s type error\r\n",json->string); \
						ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,service,0,JSON_TYPE_ERR,JSON_TYPE_ERR_DESC,NULL,mid); \
						cJSON_Delete(json); \
						return; \
					} \
				}while(0) \ 	

#define APS_COAPS_JSON_FORMAT_ERROR_WITH_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,service,json,node,mid) do{ \
		cJSON *obj = NULL; \
		obj = cJSON_GetObjectItem(json, node); \
		if(NULL == obj){ \
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,service,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); \
			cJSON_Delete(json); \
			return OPP_FAILURE; \
		} \
	}while(0) \

#define APS_COAPS_JSON_TYPE_ERROR_WITH_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,service,json,type,mid) do{ \
							int ret; \
							if(type == cJSON_Number) \
								ret = cJSON_IsNumber(json); \
							else if(type == cJSON_String) \
								ret = cJSON_IsString(json); \
							else if(type == cJSON_Array) \
								ret = cJSON_IsArray(json); \
							else if(type == cJSON_Object) \
								ret = cJSON_IsObject(json); \
							else if(type == cJSON_NULL) \
								ret = cJSON_IsNull(json); \
							else if(type == cJSON_Raw) \
								ret = cJSON_IsRaw(json); \
							else if(type == cJSON_False) \
								ret = cJSON_IsFalse(json); \
							else if(type == cJSON_True) \
								ret = cJSON_IsFalse(json); \
							else \
								ret = 0; \
							if(!ret){ \
								DEBUG_LOG(DEBUG_MODULE_COAP,DLL_ERROR,"node name:%s type error\r\n",json->string); \
								ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,service,0,JSON_TYPE_ERR,JSON_TYPE_ERR_DESC,NULL,mid); \
								cJSON_Delete(json); \
								return OPP_FAILURE; \
							} \
						}while(0) \ 	

#define APS_COAPS_JSON_FORMAT_ERROR_WITHOUT_FREE(dstChl,dstInfo,isNeedRsp,reqId,service,json,node,mid) do{ \
		cJSON *obj = NULL; \
		obj = cJSON_GetObjectItem(json, node); \
		if(NULL == obj){ \
			ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,service,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); \
			return; \
		} \
	}while(0) \

#define APS_COAPS_JSON_TYPE_ERROR_WITHOUT_FREE(dstChl,dstInfo,isNeedRsp,reqId,service,json,type,mid) do{ \
									int ret; \
									if(type == cJSON_Number) \
										ret = cJSON_IsNumber(json); \
									else if(type == cJSON_String) \
										ret = cJSON_IsString(json); \
									else if(type == cJSON_Array) \
										ret = cJSON_IsArray(json); \
									else if(type == cJSON_Object) \
										ret = cJSON_IsObject(json); \
									else if(type == cJSON_NULL) \
										ret = cJSON_IsNull(json); \
									else if(type == cJSON_Raw) \
										ret = cJSON_IsRaw(json); \
									else if(type == cJSON_False) \
										ret = cJSON_IsFalse(json); \
									else if(type == cJSON_True) \
										ret = cJSON_IsFalse(json); \
									else \
										ret = 0; \
									if(!ret){ \
										DEBUG_LOG(DEBUG_MODULE_COAP,DLL_ERROR,"node name:%s type error\r\n",json->string); \
										ApsCoapRsp(dstChl,dstInfo,CMD_MSG,isNeedRsp,reqId,service,0,JSON_TYPE_ERR,JSON_TYPE_ERR_DESC,NULL,mid); \
										return; \
									} \
								}while(0) \ 	


#define APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE(dstChl,dstInfo,isNeedRsp,reqId,service,cmdId,json,node,mid) do{ \
	cJSON *obj = NULL; \
	obj = cJSON_GetObjectItem(json, node); \
	if(NULL == obj){ \
		ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,service,cmdId,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); \
		return; \
	} \
}while(0) \		

#define APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE(dstChl,dstInfo,isNeedRsp,reqId,service,cmdId,json,type,mid) do{ \
					int ret; \
					if(type == cJSON_Number) \
						ret = cJSON_IsNumber(json); \
					else if(type == cJSON_String) \
						ret = cJSON_IsString(json); \
					else if(type == cJSON_Array) \
						ret = cJSON_IsArray(json); \
					else if(type == cJSON_Object) \
						ret = cJSON_IsObject(json); \
					else if(type == cJSON_NULL) \
						ret = cJSON_IsNull(json); \
					else if(type == cJSON_Raw) \
						ret = cJSON_IsRaw(json); \
					else if(type == cJSON_False) \
						ret = cJSON_IsFalse(json); \
					else if(type == cJSON_True) \
						ret = cJSON_IsFalse(json); \
					else \
						ret = 0; \
					if(!ret){ \
						DEBUG_LOG(DEBUG_MODULE_COAP,DLL_ERROR,"node name:%s type error\r\n",json->string); \
						ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,service,cmdId,reqId,0,JSON_TYPE_ERR,JSON_TYPE_ERR_DESC,NULL,mid); \
						return; \
					} \
				}while(0) \ 	


#define APS_COAPS_FUNC_JSON_FORMAT_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,service,cmdId,json,node,mid) do{ \
		cJSON *obj = NULL; \
		obj = cJSON_GetObjectItem(json, node); \
		if(NULL == obj){ \
			ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,service,cmdId,reqId,0,JSON_NODE_ERR,JSON_NODE_ERR_DESC,NULL,mid); \
			return OPP_FAILURE; \
		} \
	}while(0) \ 	

#define APS_COAPS_FUNC_JSON_TYPE_ERROR_WITHOUT_FREE_WITH_RET(dstChl,dstInfo,isNeedRsp,reqId,service,cmdId,json,type,mid) do{ \
				int ret; \
				if(type == cJSON_Number) \
					ret = cJSON_IsNumber(json); \
				else if(type == cJSON_String) \
					ret = cJSON_IsString(json); \
				else if(type == cJSON_Array) \
					ret = cJSON_IsArray(json); \
				else if(type == cJSON_Object) \
					ret = cJSON_IsObject(json); \
				else if(type == cJSON_NULL) \
					ret = cJSON_IsNull(json); \
				else if(type == cJSON_Raw) \
					ret = cJSON_IsRaw(json); \
				else if(type == cJSON_False) \
					ret = cJSON_IsFalse(json); \
				else if(type == cJSON_True) \
					ret = cJSON_IsFalse(json); \
				else \
					ret = 0; \
				if(!ret){ \
					DEBUG_LOG(DEBUG_MODULE_COAP,DLL_ERROR,"node name:%s type error\r\n",json->string); \
					ApsCoapFuncRsp(dstChl,dstInfo,isNeedRsp,service,cmdId,reqId,0,JSON_TYPE_ERR,JSON_TYPE_ERR_DESC,NULL,mid); \
					return OPP_FAILURE; \
				} \
			}while(0) \ 	

#define APS_COAP_NULL_POINT_CHECK(ptr) do{ \
		if(NULL == ptr){ \
			MakeErrLog(DEBUG_MODULE_COAP,OPP_NULL_POINTER); \
			DEBUG_LOG(DEBUG_MODULE_COAP, DLL_ERROR,"coap have null ptr\r\n"); \	
			return OPP_NULL_POINTER; \
		} \
	}while(0) \

#define HEARTBEAT_CMDID      "Heartbeat"
#define HEARTBEATACK_CMDID   "HeartbeatAck"
#define GETTIME_CMDID      "GetTime"
#define WORKPLAN_CMDID      "WorkPlan"
#define WORKPLANSYN_CMDID      "WorkPlanSyn"
#define THRESHOLD_CMDID      "Threshold"
#define THRESHOLDSYNC_CMDID   "ThresholdSyn"
#define OTAUDP_CMDID          "OTAUDP"
#define TIME_CMDID            "Time"
#define RESTOR_CMDID            "RestoreFactory"
#define ALARM_CMDID            "Alarm"
#define ALARMSYN_CMDID          "AlarmSyn"
#define ALARMFASTCFG_CMDID            "AlarmFastCfg"
#define ALARMFASTCFGALL_CMDID            "AlarmFastCfgAll"
#define ALARMFASTREAD_CMDID            "AlarmFastRead"
#define PROPCONFIG_CMDID            "PropConfig"
#define PROPCONFIGSYN_CMDID            "PropConfigSyn"
#define PROPCONFIGFASTCFG_CMDID        "FastPropConfig"
#define PROPCONFIGFASTREAD_CMDID        "FastPropRead"
#define REBOOT_CMDID            "Reboot"
#define GPSLOC_CMDID            "GpsLoc"
#define GPSTIME_CMDID           "GpsTime"
#define PATROL_CMDID            "Patrol"
#define NBCMD_CMDID            "NbCmd"
#define WORKPLANREPORT_CMDID   "WorkPlanReport"
#define WORKPLANREPORT_CMDID   "WorkPlanReport"
#define LSVOL_CMDID            "LightSensorVol"
#define LSPARA_CMDID            "LightSensorPara"
#define FCT_CMDID            "FCT"
#define EXEP_CMDID            "Exep"
#define ATTACH_CMDID            "AttachTime"
#define PANIC_CMDID            "Panic"
#define ONLINE_CMDID      "Online"
#define ONLINEACK_CMDID   "OnlineAck"

/** LOG SERVICE **/
#define LOGSTATUS_CMDID       "LogStatus"
#define LOG_CMDID       	  "Log"
#define LOGCONFIG_CMDID       "LogConfig"
#define LOGPERIOD_CMDID       "LogPeriod"

#define RETRY_TIMES_MAX     0x01
#define R_MAX_ITEMS         0x01
#define JSON_R_MAX_LEN      512
#define JSON_S_MAX_LEN      512
#define BRI_MIN             0
#define BRI_MAX             1000
#define DEFAULT_RHB         3
#define RETRY_ALWAYS        0xFF
#define SUCCESS           0x00

//ÏµÍ³´íÎó1xx
#define MEMORY_ERR      		100
#define MEMORY_ERR_DESC 		"device memory is not enough"
#define NBIOT_RECV_BIG_ERR      101
#define NBIOT_RECV_BIG_ERR_DESC "NBIOT recv packet is too big"
#define NBIOT_SEND_BIG_ERR      102
#define NBIOT_SEND_BIG_ERR_DESC "NBIOT send packet is too big"
#define WRITE_FLASH_ERR         103
#define WRITE_FLASH_ERR_DESC    "para write flash error"
#define JSON_FORMAT_ERR         104
#define JSON_FORMAT_ERR_DESC    "json format error"
#define UNKNOW_CMDID_ERR         105
#define UNKNOW_CMDID_ERR_DESC    "unknow cmdid error"
#define NBCMD_EXEC_ERR         106
#define NBCMD_EXEC_ERR_DESC    "nbiot cmd execute error"
#define INNER_ERR        		107
#define INNER_ERR_DESC   		"inner error"
#define FUNC_NO_CMDID_ERR         108
#define FUNC_NO_CMDID_ERR_DESC    "func service should include [cmdid]"
#define NOT_SUPPORT_CFG_BY_NB_ERR      109
#define NOT_SUPPORT_CFG_BY_NB_ERR_DESC  "not support config through nb"
#define NBIOT_DEV_IS_ERR      110
#define NBIOT_DEV_IS_ERR_DESC "NBIOT device is error"

//json½âÎö´íÎó2xx
#define JSON_NODE_ERR      200
#define JSON_NODE_ERR_DESC  "json node is not exit"
#define PROP_OP_ERR         201
#define PROP_OP_ERR_DESC    "json element [op] val should in [R,W]"
#define PROP_R_E_ERR        202
#define PROP_R_E_ERR_DESC   "property [op:R] request parameter [prop] should not be empty" 
#define PROP_R_ELE_ERR      203
#define PROP_R_ELE_ERR_DESC  "property [op:R] json format is not valid"
#define PROP_W_ERR         	204
#define PROP_W_ERR_DESC     "[prop] attibute not support write operation"
#define PROP_R_ERR         	205
#define PROP_R_ERR_DESC     "[prop] attibute not support read  operation"
#define PROP_ELE_ERR        206
#define PROP_ELE_ERR_DESC    "[prop] attibute is not in attibute list"
#define PROP_R_ONE_ERR      207
#define PROP_R_ONE_ERR_DESC  "[prop] attibute read one element one time"
#define JSON_ID_ZERO_ERR         208
#define JSON_ID_ZERO_ERR_DESC    "json id should be greate than 0"
#define JSON_TYPE_STRING_ERR         209
#define JSON_TYPE_STRING_ERR_DESC    "json data type should be string"
#define JSON_TYPE_NUMBER_ERR         210
#define JSON_TYPE_NUMBER_ERR_DESC    "json data type should be number"
#define JSON_STRING_EMPTY_ERR         211
#define JSON_STRING_EMPTY_ERR_DESC    "json string should not be empty"
#define PROP_UNKONW_OP_ERR         212
#define PROP_UNKONW_OP_ERR_DESC    "prop unknow op"
#define PROP_RE_OP_ERR         213
#define PROP_RE_OP_ERR_DESC    "prop RE not allowed op"
#define JSON_TYPE_FLOAT_ERR         214
#define JSON_TYPE_FLOAT_ERR_DESC    "json data should not be float"
#define JSON_TYPE_ERR         215
#define JSON_TYPE_ERR_DESC    "json data type error"
#define PROP_W_ELE_ERR      216
#define PROP_W_ELE_ERR_DESC  "property [op:W] json format is not valid"

//ÊôÐÔ·þÎñ´íÎó3xx
#define SWITCH_VAL_ERR      300
#define SWITCH_VAL_ERR_DESC  "[switch] val in [0,1]"
#define BRI_VAL_ERR      	301
#define BRI_VAL_ERR_DESC  	 "[bri] val between 0 and 1000"
#define HTIME_VAL_ERR      	302
#define HTIME_VAL_ERR_DESC   "[hTime] val great 0"
#define RTIME_VAL_ERR      	303
#define RTIME_VAL_ERR_DESC   "[rTime] val great 0"
#define OCPORT_VAL_ERR      304
#define OCPORT_VAL_ERR_DESC  "[ocPort] val between 0 and 65535"
#define BAND_VAL_ERR      	305
#define BAND_VAL_ERR_DESC  	 "[band] val [1,3,5,8,20]"
#define DIMT_VAL_ERR      	306
#define DIMT_VAL_ERR_DESC  	 "[dimType] val [0,1]"
#define PERIOD_VAL_ERR      307
#define PERIOD_VAL_ERR_DESC  "[period] val should great 0"
#define EC_VAL_ERR      	308
#define EC_VAL_ERR_DESC   "[EC] val great 0"
#define HEC_VAL_ERR      	309
#define HEC_VAL_ERR_DESC   "[HEC] val great 0"
#define ADJ_VOL_ERR      	310
#define ADJ_VOL_ERR_DESC   "adjust voltage error"
#define ADJ_CUR_ERR      	311
#define ADJ_CUR_ERR_DESC   "adjust current error"
#define ADJ_PWR_ERR      	312
#define ADJ_PWR_ERR_DESC   "adjust power error"
#define ADJ_PHASE_ERR      	313
#define ADJ_PHASE_ERR_DESC   "adjust phase error"
#define ADJ_ADC_64_ERR      	314
#define ADJ_ADC_64_ERR_DESC   "adjust adc 64 error"
#define ADJ_ADC_192_ERR      	315
#define ADJ_ADC_192_ERR_DESC   "adjust adc 192 error"
#define SET_WATCHDOG_ERR      	316
#define SET_WATCHDOG_ERR_DESC   "set watchdog error"
#define BLINK_ERR      	317
#define BLINK_ERR_DESC   "blink error"
#define NBIOT_EARFCN_SET_ERR      	318
#define NBIOT_EARFCN_SET_ERR_DESC   "nbiot set earfcn error"
#define LOC_VAL_ERR      	319
#define LOC_VAL_ERR_DESC   "loc set value error"
#define CLK_VAL_ERR      	320
#define CLK_VAL_ERR_DESC   "clk set value error"
#define VAL_L_ERR      	321
#define VAL_L_ERR_DESC   "value shoule be greate than 0"
#define VAL_G_ERR      	322
#define VAL_G_ERR_DESC   "value shoule be less than 2147483647"
#define LAT_VAL_ERR      	323
#define LAT_VAL_ERR_DESC   "latiude value shoule be in [-90,90]"
#define LNG_VAL_ERR      	324
#define LNG_VAL_ERR_DESC   "longitude value shoule be in [-180,180]"
#define EC_VAL_G_ERR      	325
#define EC_VAL_G_ERR_DESC   "value shoule be less than 178956970"
#define ELEC_CUR_PARA_SET_ERR      	326
#define ELEC_CUR_PARA_SET_ERR_DESC   "elec current para set error"
#define ELEC_VOL_PARA_SET_ERR      	327
#define ELEC_VOL_PARA_SET_ERR_DESC   "elec voltage para set error"
#define ELEC_PWR_PARA_SET_ERR      	328
#define ELEC_PWR_PARA_SET_ERR_DESC   "elec power para set error"
#define ELEC_PHASE_PARA_SET_ERR      	329
#define ELEC_PHASE_PARA_SET_ERR_DESC   "elec phse para set error"
#define ATTACH_DISCRETE_WIN_SET_ERR      	330
#define ATTACH_DISCRETE_WIN_SET_ERR_DESC   "attach discrete window para is invalide"
#define ATTACH_DISCRETE_INTRVAL_SET_ERR      	331
#define ATTACH_DISCRETE_INTRVAL_SET_ERR_DESC   "attach discrete intrval para is invalide"
#define ATTACH_DISCRETE_PARA_SET_ERR      	332
#define ATTACH_DISCRETE_PARA_SET_ERR_DESC   "attach discrete para set error"
#define HEART_DISCRETE_WIN_SET_ERR      	333
#define HEART_DISCRETE_WIN_SET_ERR_DESC   "heart discrete window para is invalide"
#define HEART_DISCRETE_INTRVAL_SET_ERR      	334
#define HEART_DISCRETE_INTRVAL_SET_ERR_DESC   "heart discrete intrval para is invalide"
#define HEART_DISCRETE_PARA_SET_ERR      	335
#define HEART_DISCRETE_PARA_SET_ERR_DESC   "heart discrete para set error"
#define TEST_VAL_ERR      					336
#define TEST_VAL_ERR_DESC  	 				"[test] val [1,2]"
#define TEST_SET_ERR      					337
#define TEST_SET_ERR_DESC  	 				"test set para to flash error"
#define ACTTIME_LEN_ERR      				338
#define ACTTIME_LEN_ERR_DESC  	 			"[actTime] length error"
#define ACTTIME_FORMAT_ERR      			339
#define ACTTIME_FORMAT_ERR_DESC  	 		"[actTime] format must be XX-XX-XX XX:XX:XX"
#define ACTTIME_GT_CURTIME_ERR      			340
#define ACTTIME_GT_CURTIME_ERR_DESC  	 		"[actTime] must great than current time"
#define OV_SET_ERR      					341
#define OV_SET_ERR_DESC  	 				"set ota original version to flash error"
#define LT_G_ERR      						342
#define LT_G_ERR_DESC   					"light time should less than 16777215"


//º¯Êý·þÎñ´íÎó4xx
#define PLAN_ID_OF_ERR      400
#define PLAN_ID_OF_ERR_DESC "[plan.id] should not be over [plansNum]"
#define JOB_ID_OF_ERR       401
#define JOB_ID_OF_ERR_DESC  "jobs items is over items limit"
#define JOB_EMPTY_ERR      	402
#define JOB_EMPTY_DESC       "jobs is empty"
#define IDS_R_EMPTY_ERR      403
#define IDS_R_EMPTY_DESC     "read [ids] should not be empty"
#define IDS_D_EMPTY_ERR      404
#define IDS_D_EMPTY_DESC     "del [ids] should not be empty"
#define IDS_MORE_ERR      	405
#define IDS_MORE_DESC       "[ids] must not great than 1"
#define IDS_R_BIG_ERR      	406
#define IDS_R_BIG_DESC       "read number of ids must not great than [plansNum]"
#define IDS_R_SMALL_ERR      	407
#define IDS_R_SMALL_ERR_DESC   "read number of ids must not small than 0"
#define IDS_D_BIG_ERR      	408
#define IDS_D_BIG_DESC       "del number of ids must not great than plansNum"
#define IDS_D_SMALL_ERR      	409
#define IDS_D_SMALL_ERR_DESC  "del number of ids must not small than 0"
#define IDS_D_NUM_ERR      	410
#define IDS_D_NUM_DESC       "del ids number of element not great than plansNum"
#define PLAN_DATE_ERR      	411
#define PLAN_DATE_ERR_DESC   "plan sDate is great than eDate"
#define JOB_TIME_ERR      	412
#define JOB_TIME_ERR_DESC    "job time error"
#define PLAN_TYPE_ERR      	413
#define PLAN_TYPE_ERR_DESC   "plan type should be [0,1,2]"
#define PLAN_VALID_ERR       414
#define PLAN_VALID_ERR_DESC   "plan valide should be [0,1]"
#define PLAN_LEVEL_ERR       415
#define PLAN_LEVEL_ERR_DESC  "plan level should be [0~255]"
#define PLAN_WEEK_ERR      	 416
#define PLAN_WEEK_ERR_DESC   "plan week length is not 7"
#define THRESHOLD_ID_ERR      	 417
#define THRESHOLD_ID_ERR_DESC   "threshold id should not empty"
#define THRESHOLD_PARA_SET_ERR   418
#define THRESHOLD_PARA_SET_ERR_DESC   "threshold para set error"
#define THRESHOLD_IDS_R_EMPTY_ERR      419
#define THRESHOLD_IDS_R_EMPTY_ERR_DESC     "threshold read [ids] should not be empty"
#define THRESHOLD_IDS_D_EMPTY_ERR      420
#define THRESHOLD_IDS_D_EMPTY_ERR_DESC     "threshold del [ids] should not be empty"
#define THRESHOLD_IDS_D_SET_ERR      421
#define THRESHOLD_IDS_D_SET_ERR_DESC     "threshold del [ids] set para error"
#define THRESHOLD_IDS_W_PARA_ERR      422
#define THRESHOLD_IDS_W_SET_PARA_DESC     "threshold write [ids] should not be empty"
#define THRESHOLD_PROPNAME_W_ERR      423
#define THRESHOLD_PROPNAME_W_DESC     "threshold w propName is not invalide"
#define THRESHOLD_IDS_R_CONFIG_ERR      424
#define THRESHOLD_IDS_R_CONFIG_ERR_DESC     "threshold read [ids] not exist"
#define THRESHOLD_IDS_D_ERR      425
#define THRESHOLD_IDS_D_ERR_DESC     "threshold del [ids] not exist"
#define THRESHOLD_ALARM_PARA_ERR      426
#define THRESHOLD_ALARM_PARA_ERR_DESC     "threshold alarm para error"
#define PLANID_R_NOT_EXIST_ERR      427
#define PLANID_R_NOT_EXIST_ERR_DESC     "read plan [ids] not exist"
#define LOG_PERIOD_WRITE_ERR      428
#define LOG_PERIOD_WRITE_ERR_DESC     "log period write error"
#define LOG_READ_ERR     			429
#define LOG_READ_ERR_DESC     		"log read error"
#define ALARM_PARA_SET_ERR   430
#define ALARM_PARA_SET_ERR_DESC   "alarm para set error"
#define ALARM_PARA_ERR      431
#define ALARM_PARA_ERR_DESC     "alarm para error"
#define ALARM_IDS_R_EMPTY_ERR      432
#define ALARM_IDS_R_EMPTY_ERR_DESC     "alarm read [ids] should not be empty"
#define ALARM_IDS_R_CONFIG_ERR      433
#define ALARM_IDS_R_CONFIG_ERR_DESC     "alarm read not [ids] config error"
#define ALARM_IDS_D_ERR      434
#define ALARM_IDS_D_ERR_DESC     "alarm del [ids] not exist"
#define ALARM_IDS_D_EMPTY_ERR      435
#define ALARM_IDS_D_EMPTY_ERR_DESC     "alarm del [ids] should not be empty"
#define ALARM_IDS_D_SET_ERR      436
#define ALARM_IDS_D_SET_ERR_DESC     "alarm del [ids] should not be empty"
#define PROPCONFIG_PARA_SET_ERR   437
#define PROPCONFIG_PARA_SET_ERR_DESC   "propconfig para set error"
#define PROPCONFIG_PARA_ERR      438
#define PROPCONFIG_PARA_ERR_DESC     "propconfig para error"
#define PROPCONFIG_IDS_R_EMPTY_ERR      439
#define PROPCONFIG_IDS_R_EMPTY_ERR_DESC     "propconfig read [ids] should not be empty"
#define PROPCONFIG_IDS_R_CONFIG_ERR      440
#define PROPCONFIG_IDS_R_CONFIG_ERR_DESC     "propconfig read not [ids] config error"
#define PROPCONFIG_IDS_D_ERR      441
#define PROPCONFIG_IDS_D_ERR_DESC     "propconfig del [ids] not exist"
#define PROPCONFIG_IDS_D_EMPTY_ERR      442
#define PROPCONFIG_IDS_D_EMPTY_ERR_DESC     "propconfig del [ids] should not be empty"
#define PROPCONFIG_IDS_D_SET_ERR      443
#define PROPCONFIG_IDS_D_SET_ERR_DESC     "propconfig del [ids] error"
#define PROPCONFIG_R_IS_NOT_REPORT      444
#define PROPCONFIG_R_IS_NOT_REPORT_DESC     "propconfig read is not report"
#define ALARM_R_IS_NOT_REPORT      445
#define ALARM_R_IS_NOT_REPORT_DESC     "alarm read is not report"
#define RESID_ERR   446
#define RESID_ERR_DESC   "alarm resource id set error"
#define RESID_W_ERR   447
#define RESID_W_ERR_DESC   "resource id not support write"
#define RESID_M_ERR   448
#define RESID_M_ERR_DESC   "resource id not support modify"
#define RESID_D_ERR   449
#define RESID_D_ERR_DESC   "resource id not support delete"
#define RESID_R_ERR   450
#define RESID_R_ERR_DESC   "resource id not support read"
#define RESTORE_DIMMER_ERR   451
#define RESTORE_DIMMER_ERR_DESC   "restore factory dimmer error"
#define RESTORE_TIME_ERR   452
#define RESTORE_TIME_ERR_DESC   "restore factory time error"
#define RESTORE_ELEC_ERR   453
#define RESTORE_ELEC_ERR_DESC   "restore factory elec error"
#define RESTORE_NBIOT_ERR   454
#define RESTORE_NBIOT_ERR_DESC   "restore factory nbiot error"
#define RESTORE_IOT_ERR   455
#define RESTORE_IOT_ERR_DESC   "restore factory iot error"
#define RESTORE_LOG_ERR   456
#define RESTORE_LOG_ERR_DESC   "restore factory log config error"
#define RESTORE_ALARM_ERR   457
#define RESTORE_ALARM_ERR_DESC   "restore factory alarm config error"
#define RESTORE_UPDATE_ERR   458
#define RESTORE_UPDATE_ERR_DESC   "restore factory update config error"
#define WORKPLANSYN_NO_PLAN_ERR   459
#define WORKPLANSYN_NO_PLAN_ERR_DESC   "workplan syn no plan"
#define ALARM_FAST_CFG_NO_ALARM_ERR   460
#define ALARM_FAST_CFG_NO_ALARM_ERR_DESC   "alarm fast config no alarm config"
#define ALARM_FAST_CFG_INVALID_ALARMID_ERR   461
#define ALARM_FAST_CFG_INVALID_ALARMID_ERR_DESC   "alarm fast config invalid alarmid"
#define ALARM_CFG_OP_ERR   462
#define ALARM_CFG_OP_ERR_DESC   "alarm config op is invalide"
#define PROP_CFG_OP_ERR   463
#define PROP_CFG_OP_ERR_DESC   "prop config op is invalide"
#define GPS_NOT_SUPPORT_ERR   464
#define GPS_NOT_SUPPORT_ERR_DESC   "not support gps"
#define GPS_GET_LOC_ERR   465
#define GPS_GET_LOC_ERR_DESC   "get loc from gps error"
#define GPS_GET_TIME_ERR   466
#define GPS_GET_TIME_ERR_DESC   "get time from gps error"
#define LIGHT_SENSOR_VOL_GET_ERR   467
#define LIGHT_SENSOR_VOL_GET_ERR_DESC   "get light sensor voltage error"
#define LIGHT_SENSOR_PARA_WRITE_ERR   468
#define LIGHT_SENSOR_PARA_WRITE_ERR_DESC   "light sensor para write error"
#define LIGHT_SENSOR_PARA_READ_ERR   469
#define LIGHT_SENSOR_PARA_READ_ERR_DESC   "light sensor para read error"
#define LIGHT_SENSOR_PARA_DEL_ERR   470
#define LIGHT_SENSOR_PARA_DEL_ERR_DESC   "light sensor para del error"
#define LIGHT_SENSOR_OP_ERR   471
#define LIGHT_SENSOR_OP_ERR_DESC   "light sensor op error"
#define TIME_OP_W_ERR   472
#define TIME_OP_W_ERR_DESC   "time write data format error"
#define TIME_VAL_ERR   473
#define TIME_VAL_ERR_DESC   "time val error"
#define PLAN_OP_ERR   474
#define PLAN_OP_ERR_DESC   "work plan only support [W|R|D]"
#define EXEP_OP_ERR   475
#define EXEP_OP_ERR_DESC   "exep op error"
#define EXEP_NUM_ERR   476
#define EXEP_NUM_ERR_DESC   "exep num should be [0,1]"
#define EXEP_GET_ERR   477
#define EXEP_GET_ERR_DESC   "get exep info error"
#define ATTACH_DEV_NOT_READY_ERR   478
#define ATTACH_DEV_NOT_READY_ERR_DESC   "dev not in ready state"
#define TIME_SET_ERR   479
#define TIME_SET_ERR_DESC   "set time error"
#define RESTORE_COAP_ERR   480
#define RESTORE_COAP_ERR_DESC   "restore coap config error"
#define RESTORE_WIFI_ERR   481
#define RESTORE_WIFI_ERR_DESC   "restore wifi config error"
#define IPV4_ADDR_FORMAT_ERROR   482
#define IPV4_ADDR_FORMAT_ERROR_DESC   "ipv4 address format error"
#define TIME_OP_ERROR            483
#define TIME_OP_ERROR_DESC   "time only support [W|R]"
#define NBPARA_ENABLE_PARA_ERROR            484
#define NBPARA_ENABLE_PARA_ERROR_DESC   "nbPara enable should be [0,1]"
#define NBPARA_EARFCN_PARA_ERROR            485
#define NBPARA_EARFCN_PARA_ERROR_DESC   "nbPara earfcn should be less than 65535"
#define NBPARA_BAND_PARA_ERROR            486
#define NBPARA_BAND_PARA_ERROR_DESC   "nbPara band should be [28,5,20,8,3,1]"
#define LOGSTATUS_OP_ERR     			487
#define LOGSTATUS_OP_ERR_DESC     		"logstatus op only support R"
#define LOG_OP_ERR     			488
#define LOG_OP_ERR_DESC     		"log op only support R"
#define LOGPERIOD_OP_ERR     			489
#define LOGPERIOD_OP_ERR_DESC     		"logperiod op only support R|W"
#define RESTORE_LOC_ERR   490
#define RESTORE_LOC_ERR_DESC   "restore loc config error"
#define RESTORE_EXEP_ERR   491
#define RESTORE_EXEP_ERR_DESC   "restore exep config error"
#define PROPFASTCFG_NOT_SUPPORT_ERR   492
#define PROPFASTCFG_NOT_SUPPORT_ERR_DESC   "prop fast config not support"
#define PROPFASTCFG_EMPTY_ERR   493
#define PROPFASTCFG_EMPTY_ERR_DESC   "prop fast config no config data"
#define PANIC_EMPTY_ERR   494
#define PANIC_EMPTY_ERR_DESC   "no panic"
#define OTA_UPGRADING_ERR   495
#define OTA_UPGRADING_ERR_DESC   "OTA upgrading"
#define HV_ELE_NUM_ERR   496
#define HV_ELE_NUM_ERR_DESC   "history version array element should be 1"
#define HV_SET_ERR   497
#define HV_SET_ERR_DESC   "history version set error"

//ÈÕÖ¾·þÎñ´íÎó
typedef int (*cmdIdFunc)(unsigned char dstChl);
typedef int (*propFunc)(cJSON *prop, void * data);
typedef int (*rebootFunc)(void);

typedef enum
{
	/********************************/
	PROP_SWITCH=0,
	PROP_BRI,
	PROP_RUNTIME,
	PROP_ELEC,
	PROP_ECINFO,
	PROP_LOC,
	PROP_NBBASE,	
	PROP_NBSIGNAL,
	PROP_NBCON,
	PROP_PRODUCT,
	PROP_SN,
	PROP_PROTID,	
	PROP_MODEL,
  	PROP_MANU,
  	PROP_SKU,
  	PROP_CLAS,
	PROP_FWV,
	PROP_OCIP,
	PROP_OCPORT,
	PROP_BAND,
	PROP_DIMTYPE,
	PROP_APNADDRESS,
	PROP_PERIOD,
	PROP_SCRAM,
	PROP_PLANSNUM,
	PROP_HBPERIOD,
	PROP_WIFI,
	PROP_AP,	
	PROP_WIFICFG,
	PROP_LOCCFG,
	PROP_LOCSRC,
	PROP_CLKSRC,
	PROP_DEVCAP,
	PROP_NBVER,
	PROP_NBINFO,
	PROP_SYSINFO,
	PROP_NBPID,
	PROP_LUX,
	PROP_EARFCN,
	PROP_ADJELEC,
	PROP_ADJPHASE,
	PROP_ADJADC64,
	PROP_ADJADC192,
	PROP_WDT,
	PROP_SDKVER,
	PROP_BLINK,
	PROP_TDIMTYPE,
	PROP_HARDINFO,
	PROP_RXPROTECT,
	PROP_TELEC,
	PROP_ELECPARA,
	PROP_ATTACHDISPARA,
	PROP_ATTACHDISOFFSET,
	PROP_ATTACHDISENABLE,
	PROP_HEARTDISPARA,
	PROP_HEARTDISOFFSET,
	PROP_TCUR,
	PROP_T1,
	PROP_T2,
	PROP_T3,
	PROP_T4,
	PROP_T5,
	PROP_T6,
	PROP_T7,
	PROP_T8,
	PROP_NBDEVSTATETO,
	PROP_NBREGSTATETO,
	PROP_NBIOTSTATETO,
	PROP_NBHARDRSTTO,
	PROP_NBOTATO,
	PROP_TIME0,  /*no time up period*/
	PROP_TIME1,  /*has time up period*/
	PROP_SAVEHTIMETO,
	PROP_SCANECTO,
	PROP_SAVEHECTO,
	PROP_ONLINETO,
	PROP_BC28PARA,
	PROP_ACTTIME,
	PROP_OV,
	PROP_HV,
	/********************************/
	PROP_MAX,
}ST_PROP_ID;

typedef enum{
	LOG_SEND_ERR=0,
	LOG_SEND_SUCC,
	LOG_SEND_ING,
	LOG_SEND_INIT,
	LOG_SEND_UNKNOW,
}EN_LOG_SEND_STATUS;

typedef enum{
	LOG_MSG = 0,
	ALARM_MSG,
	REPORT_MSG,
	CMD_MSG
}EN_COAP_MSG_TYPE;

typedef enum
{
    REBOOT_MODULE_TEST=0, // MODULE 0  FOR TEST
    REBOOT_MODULE_ELEC,    
    REBOOT_MODULE_MAX,
}EN_REBOOT_MODULE;

typedef struct{
	U8 resId;
	char * name;
	U8 type;
	U32  auth;
	//cJSON *jsonCom;
	propFunc func;
}ST_PROP;

typedef struct{
	EN_MC_RESOURCE id;
	U8 type;
	char *propName;
	char *comPropName;
}ST_PROP_MAP;

typedef struct{
	EN_MC_RESOURCE id;
	char *cmdId;
	cmdIdFunc func;
}ST_CMDID_MAP;

typedef struct{
	EN_MC_RESOURCE id;
	U8 type;
	U32  auth;	
}ST_RES_MAP;

typedef enum{
	STR_T = 0,
	INT_T,
	OBJ_T,
	ERR_T
}EN_TYPE;

typedef enum{
	U8_T = 2,
	U16_T = 3,
	U32_T = 4,	
	MAX_T,
}PARA_TYPE;

#define R    0x01
#define W    0x02
#define RE   0x04
#define A    0x08
#define M    0x10
#define D    0x20


typedef struct
{
	U16 hdr;
	U8  deviceCmd;
	U8  hasMore;
	U8  opCmd;
}ST_DEV_REQ_HDR;

typedef struct
{
	U16 hdr;
	U8  deviceCmd;
	U8  errorCode;
	U8  hasMid;
	U16 mid;
}ST_DEV_RSP_HDR;

typedef struct
{
	U16 hdr;
	U8  deviceCmd;
	U8  hasMore;
	U8  opCmd;
	U8  cmdStatus;
}ST_CLOUD_REQ_HDR;

typedef struct
{
	U16 hdr;
	U8  deviceCmd;
	U16 hasMore;
	U8  opCmd;
	U8  errorCode;
}ST_CLOUD_RSP_HDR;


typedef struct
{
	U8 urlLength;
	U8 domain[100];
	U32 version;
	U32 size;
}ST_SET_UDP_OTA_PARA;

typedef struct
{
	U8 version[4];
	U32 size;
	U32 crc;
}ST_SET_OTA_PARA;

typedef struct
{
	U32 frameNo;
	U8 fin;
	char content[500];
}ST_SET_OTA_CONTENT;

typedef struct
{
	U8 type;
	U32 minValue;
	U32 maxValue;
}ST_SET_TH_PARA;


typedef struct
{
	ST_TIME startTime;
	ST_TIME endTime;
	U8 logType;
}ST_QUERY_LOG_PARA;


typedef struct
{
	U8 lightSwitch;
	U8 dim;
	U16 voltage;
	U16 current;
	U16 power;
	U16 factor;
	U32 consumption;
	U32 runTime;
	U32 version;
	U32 rsrp;
	U32 rsrq;
	U32 signalEcl;
	U32 cellid;
	U32 signalPCI;
	U32 snr;
}ST_RI_PARA;

typedef struct{
	U32 rTime;
	U32 hTime;
	U32 lTime;
	U32 hlTime;
}ST_RUNTIME_PROP;

typedef struct{
	U8 imei[NEUL_IMEI_LEN];
	U8 imsi[NEUL_IMSI_LEN];
	U8 nccid[NEUL_NCCID_LEN];
	U8 addr[NEUL_IPV6_LEN];
}ST_NBBASE_PROP;

typedef struct{
	S32 rsrp;
	S32 rsrq;
	U32 signalEcl;
	U32 cellId;
	U32 signalPCI;
	S32 snr;
	U32 earfcn;
	U32 txPower;	
}ST_NBSIGNAL_PROP;

typedef struct{
	U8 regStatus;
	U8 conStatus;
}ST_NBCON_PROP;

typedef struct{
	U32 current;
	U32 voltage;
	U32 factor;
	U32 power;	
}ST_ELEC_PROP;

#define RSP_T    0
#define REPORT_T 1
typedef struct _list_entry
{
	U8 type; /*0:rsp, 1:report*/
	U8 chl;
	U16 mid;
	U32 tick;
	U16 length;
	U8 *data;
	U8 times;
	U8 msgType;
	U8 dstInfo[MSG_INFO_SIZE];
	LIST_ENTRY(_list_entry) elements;
}ST_COAP_ENTRY;

typedef struct _list_report_entry
{
	U8 chl;
	char propName[20];
	//t_mc_resource resId;
	void *data;
	LIST_ENTRY(_list_report_entry) elements;	
}ST_REPORT_ENTRY;

typedef struct _list_retry_hb_entry
{
	U8 chl;
	U8 info[MSG_INFO_SIZE];
	int reqId;
	U8 times;
	U16 mid;
	U32 tick;
	LIST_ENTRY(_list_retry_hb_entry) elements;	
}ST_RHB_ENTRY;

typedef struct{
	U8 chl;
	U8 dstInfo[MSG_INFO_SIZE];
	U32 logSaveId;
	U32 reqId;
}ST_LOG_QUERY_REPORT;
////////////////////////////////////////
typedef struct
{
	U16 hdr;
	U8  comDir;
	U8  hasMore;
	U8  serviceId;
	U16 length;
}ST_COMDIR_REQ_HDR;

typedef struct
{
	U16 hdr;
	U8  comDir;
	U8  hasMore;
	U8  serviceId;
	U8  error;
	U16 length;
}ST_COMDIR_RSQ_HDR;


typedef struct
{
	union{
		ST_COMDIR_REQ_HDR req;
		ST_COMDIR_RSQ_HDR rsp;
	}u;
}ST_DISP_PARA;

typedef struct
{
	U32 hdr;
	U8 protId;
	U8 protVer;
	U16 dataLen;
	U16 crc;
	U32 seqNum;
	U8 isRsp;
	U8 conType;
}ST_LOC_HDR;

typedef struct{
	U8 chl;
	EN_MC_RESOURCE resId;
	unsigned int value;
	char * data;
}ST_REPORT_PARA;

typedef struct{
	U8 id;
	U8 dstChl;
	U16 alarmId;
	U8 status;
	U32 value;
	unsigned char dstInfo[MSG_INFO_SIZE];	
}ST_ALARM_PARA;

#define ACTIVE_REPROT 0
#define QUERY_REPORT 1

typedef struct{
	int chl;
	unsigned char type;   /*type=0 主动上报，type=1查询上报type=0可不指定reqId and leftItems*/
	unsigned int reqId;
	unsigned int leftItems;
	unsigned int err;	
	unsigned int logSaveId;
	APP_LOG_T log;
	unsigned char dstInfo[MSG_INFO_SIZE];	
}ST_LOG_PARA;

typedef struct{
	unsigned char dstChl;
	//unsigned int logSeq;
	unsigned int logSaveId;
	APP_LOG_T log;
}ST_LOG_APPEND_PARA;

typedef struct{
	unsigned char dstChl;
	unsigned char type; /*type=0 主动上报，type=1查询上报type=0可不指定reqId and leftItems*/
	unsigned int reqId;
	unsigned int leftItems;
	unsigned char dstInfo[MSG_INFO_SIZE];
}ST_LOG_STOP_PARA;

#define OTA_RSP    0
#define OTA_REPORT 1
#define OTA_VER_LEN    20

#define OTA_WAIT         0
#define OTA_DOWNLOADING  1
#define OTA_DOWNLOADED   2
#define OTA_UPGRADING    3
#define OTA_SUCCESS      4
#define OTA_FAIL         5

typedef struct
{
	U8 type;    /*0:rsp,1:report*/
	U32 reqId;
	U8 state;
	U8 process;
	U16 mid;
	U8 dstChl;
	U8 error;
	char version[OTA_VER_LEN];
	U8 dstInfo[MSG_INFO_SIZE];	
}ST_OPP_OTA_PROC;

#define REBOOT_ID    0

typedef struct{
	U8 dstChl;
	U32 reqId;
	U8 doReboot;
	U16 mid;
}ST_REBOOT;

typedef struct{
	int moduleId;
	char * moduleName;
    rebootFunc reboot;
}ST_REBOOT_FUNC;

typedef struct{
	U8 planId;
	U8 type;
	U8 jobId;
	U8 sw;
	U16 bri;
}ST_WORKPLAN_REPORT;

typedef struct{
	T_MUTEX mutex;
	int g_iCoapNbTxSuc;
	int g_iCoapNbTxFail;
	int g_iCoapNbTxRetry;	
	int g_iCoapNbTxReqRetry;	
	int g_iCoapNbTxRspRetry;
	int g_iCoapNbRx;
	int g_iCoapNbRxRsp;
	int g_iCoapNbRxReq;
	int g_iCoapNbDevReq;
	int g_iCoapNbDevRsp;
	int g_iCoapNbUnknow;
	int g_iCoapNbOtaRx;
	int g_iCoapNbOtaTx;
	int g_iCoapNbOtaTxSucc;
	int g_iCoapNbOtaTxFail;
	int g_iCoapBusy;
}ST_COAP_PKT;

typedef struct{
	T_MUTEX mutex;
	U32 udwHeartTick;
	U32 udwRandHeartPeriod;
}ST_HEART_PARA;

typedef struct{
	int resId;
	int defaultIdx;
	char *resName;
}ST_FASTPROP_PARA;

typedef struct{
	int alarmId;
	int resId;
	int defaultIdx;
	char *alarmDesc;
}ST_FASTALARM_PARA;

typedef struct{
	T_MUTEX mutex;
	U16 mid;
	EN_LOG_SEND_STATUS status;
}ST_LOG_STATUS;

#define RX_PROTECT_ENABLE      0x1
#define RX_PROTECT_DISABLE     0x0
#define RX_PROTECT_ENABLE_H    0x0
#define RX_PROTECT_ENABLE_L    0x0
#define RX_PROTECT_DISABLE_H   0x11223344
#define RX_PROTECT_DISABLE_L   0xAABBCCDD
typedef struct{
	T_MUTEX mutex;
	U32 enableH;
	U32 enableL;
}ST_RX_PROTECT;

#define HEART_DIS_WIN_DEFAULT    1800
#define HEART_DIS_INTERVAL_DEFAULT    10

typedef struct{
	T_MUTEX mutex;
	U32 udwDiscreteWin;
	U32 udwDiscreteInterval;
}ST_DIS_HEART_PARA;

typedef struct{
	U32 udwDiscreteWin;
	U32 udwDiscreteInterval;
}ST_DIS_HEART_SAVE_PARA;

#pragma pack()

extern U8 m_ucCoapStart;
extern ST_OPP_OTA_PROC g_stOtaProcess;;

int ApsCoapInit(void);
void ApsCoapOceanconProcess(EN_MSG_CHL chl,const MSG_ITEM_T *item);
unsigned int ApsCoapStartHeartbeat();
int ApsCoapOceanconHeart(U8 dstChl);
int ApsCoapOceanconHeartOnline(U8 dstChl, unsigned char *dstInfo);
int ApsCoapOceanconOnline(U8 dstChl, unsigned char *dstInfo);
int ApsCoapWorkPlanReport(U8 dstChl, unsigned char *dstInfo, ST_WORKPLAN_REPORT *stWorkPlanR);
int ApsCoapGetTime(U8 dstChl, unsigned char * dstInfo);
int ApsCoapOceanconRecv(char *outbuf, int maxrlen);
void ApsCoapLoop(void);
int ApsCoapReboot(void);
void ApsCoapPropProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, char *text, U32 mid);
void ApsCoapFuncProc(unsigned char dstChl, unsigned char *dstInfo, unsigned char isNeedRsp, char * text, U32 mid);
int ApsCoapLogProc(U8 dstChl, unsigned char *dstInfo, U8 isNeedRsp, char * text, U32 mid);
int ApsCoapLogReport(ST_LOG_PARA *pstLogPara);
int ApsCoapLogBatchAppend(ST_LOG_APPEND_PARA * para); // 0:success,1:full,2:fail
/*
*type=0 主动上报，type=1查询上报
*/


int ApsCoapLogBatchStop(ST_LOG_STOP_PARA * para);
int ApsCopaLogReporterIsBusy(void); // 0:idle,1:busy
int ApsCoapAlarmReport(ST_ALARM_PARA *pstAlarmPara);
int ApsCoapStateReport(ST_REPORT_PARA *pstReportPara);
int ApsCoapReport(U8 dstChl, unsigned char *dstInfo);
int OppCoapPackRuntimeState(cJSON *prop, void * data);
int OppCoapPackSwitchState(cJSON *prop, void * data);
int OppCoapPackBriState(cJSON *prop, void * data);
int OppCoapPackLoctionState(cJSON *prop, void * data);
int OppCoapPackElecState(cJSON *prop, void * data);
int OppCoapPackNbConState(cJSON *prop, void * data);
int OppCoapPackNbSignalState(cJSON *prop, void * data);
int OppCoapPackNbBaseState(cJSON *prop, void * data);
int ApsCoapOtaProcessRsp(ST_OPP_OTA_PROC * pstOtaProcess);
int ApsCoapFuncRsp(unsigned char dstChl, unsigned char * dstInfo, unsigned char isNeedRsp, unsigned char serviceId, char * cmdId, U32 reqId, U8 errHdr, int error, char * errDesc, cJSON *cmdData, U32 mid);
int ApsCoapGetLogReportStatus(U16 *status, U16 * mid);
int ApsCoapGetLogReportMid(void);
int ApsCoapSetLogReportStatus(U16 status, U16 mid);
int ApsCoapGetRetrySpec(void);
int ApsCoapSetRetrySpec(U8 flag);
ST_FASTPROP_PARA * ApsCoapFastConfigPtrGet(void);
int ApsCoapFastConfigSizeGet(void);
ST_FASTALARM_PARA * ApsCoapFastAlarmPtrGet(void);
int ApsCoapFastAlarmSizeGet(void);
int TestReboot();
int ApsCoapRxProtectSet(int enable);
int ApsCoapRxProtectGet(void);
int ApsCoapWriteExepInfo(U8 rstType);
int ApsCoapExepReport(U8 dstChl, unsigned char * dstInfo, int exepNum, ST_EXEP_INFO * pstExepInfo);
int ApsCoapExep(void);
int ApsCoapHeartDisParaGet(ST_DIS_HEART_SAVE_PARA *pstDisHeartPara);
int ApsCoapHeartDisParaSet(ST_DIS_HEART_SAVE_PARA *pstDisHeartPara);
int ApsCoapHeartDisOffsetSecondGet();
U32 ApsCoapHeartDisTick(void);
int ApsCoapHeartDisParaGetFromFlash(ST_DIS_HEART_SAVE_PARA *pstDisHeartPara);
int ApsCoapHeartDisParaSetToFlash(ST_DIS_HEART_SAVE_PARA *pstDisHeartPara);
int ApsCoapRestoreFactory(void);
int ApsCoapDoReboot(void);
int ApsCoapPanicReport(U8 dstChl,unsigned char *dstInfo);

#endif
