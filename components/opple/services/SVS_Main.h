#ifndef __SERVICE_MAIN_H__
#define __SERVICE_MAIN_H__

#include "SVS_Lamp.h"
#include "SVS_Log.h"
#include "SVS_Coap.h"
#include "SVS_Config.h"
#include "SVS_MsgMmt.h"
#include "SVS_Time.h"
#include "SVS_Location.h"
#include "SVS_Elec.h"
#include "SVS_Para.h"
#include "telnet.h"
#include "SVS_Ota.h"
#include "SVS_Config.h"
#include "SVS_WiFi.h"
#include "SVS_Exep.h"
#include "SVS_Test.h"

#define SVS_MAIN_INIT_FIRST() \
	AppParaInit(); \
	ApsWifiInit(); \
	ApsExepInit(); \
	SvsTestInit(); \
	TimeInit(); \
	ApsDevConfigInit(); \
    OppApsLogInit();\

#define SVS_MAIN_INIT_SECOND() \
    ApsLampInit();\
    ApsCoapInit();\
    OppCoapConfigInit();\
    MsgMmtInit();\
    aps_telnet_init(); \
	LocationInit(); \
	ElecInit(); \
	NbOtaInit(); \

#define SVS_MAIN_INIT() \
	AppParaInit();\
    ApsLampInit();\
    OppApsLogInit();\
    ApsCoapInit();\
    OppCoapConfigInit();\
    MsgMmtInit();\
    TimeInit(); \
    aps_telnet_init(); \
	LocationInit(); \
	ApsDevConfigInit(); \
	ElecInit(); \
	ApsWifiInit();

#define SVS_MAIN_LOOP() \
    ApsLampLoop();\
    OppApsLogLoop();\
    ApsCoapLoop();\
    MsgMmtLoop();\
    /*LocationLoop();*/\
    TimeLoop(); \
	OppCoapLoop(); \



#endif
