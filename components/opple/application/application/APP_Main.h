#ifndef _APP_MAIN_H
#define _APP_MAIN_H

#pragma pack(1)

#pragma pack()
#include <APP_LampCtrl.h>
#include <APP_Communication.h>
#include "APP_Daemon.h"

#define APP_MAIN_INIT_FIRST() \
	AppDaemonInit(); \

#define APP_MAIN_INIT_SECOND() \
	OppLampCtrlInit(); \
	AppComInit();\
	
#define APP_MAIN_INIT() \
	OppLampCtrlInit(); \
	AppComInit();\
  AppDaemonInit();

#define APP_MAIN_LOOP() \
	OppLampCtrlLoop(); \
	AppComLoop();\
  AppDaemonLoop();

void NbiotNpingtask(void);
void NbiotWatchdogTask(void);
void MainThread(void *pvParameter);
void MainInit();

#endif

