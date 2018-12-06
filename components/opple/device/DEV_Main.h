#ifndef __DEVICE_MAIN_H__
#define __DEVICE_MAIN_H__

#include "Includes.h"
#include "DEV_ItemMmt.h"
#include "DEV_LogDb.h"
#include "DEV_LogMmt.h"


#define DEV_MAIN_INIT() \
	ItemMmtInit();\
	OppDevTimerInit();\
	LogMmtInit();




#define DEV_MAIN_LOOP() \
	/*OppDevTimerLoop();*/



#endif
