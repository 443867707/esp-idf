#ifndef __GENERAL_CLI_H__
#define __GENERAL_CLI_H__

#include "cli-interpreter.h"

/*************************************************************************************************
                                          Components Configuration
*************************************************************************************************/
/* Function Reset define */
//#define CLI_RESET() HAL_NVIC_SystemReset()
extern void esp_restart();
#define CLI_RESET() esp_restart()

/* Extern Funcs */
extern void lightlib_cli_init(void);
extern void lightlib_cli_loop(void);

/* Extern Tables */
extern CommandEntry CommandTablClk[];
extern CommandEntry CommandTablDebug[];
extern CommandEntry CommandTablGps[];
extern CommandEntry CommandTableItemMmt[];
extern CommandEntry CommandTableLed[];
extern CommandEntry CommandTableLogDb[];
extern CommandEntry CommandTableApsLamp[];
extern CommandEntry CommandTableApsLog[];
extern CommandEntry CommandTablWs[];
extern CommandEntry CommandTableLog[];
extern CommandEntry CommandTableProp[];
extern CommandEntry CommandTableBc28[];
extern CommandEntry CommandTableElec[];
extern CommandEntry CommandTableDaemon[];
extern CommandEntry CommandTabl_ota[];
extern CommandEntry CommandTableWiFi[];
extern CommandEntry CommandTableSys[];
extern CommandEntry CommandTablTest[];
extern CommandEntry CommandTable_LogMmt[];
extern CommandEntry CommandTablLoc[];
extern CommandEntry CommandTable_MsgMmt[];
extern CommandEntry CommandTableAdjust[];
extern CommandEntry CommandTab_LightSensor[];
extern CommandEntry CommandTablBspTest[];
extern CommandEntry CommandTableWdt[];
extern CommandEntry CommandTableExep[];
extern CommandEntry CommandTablTestInfo[];
extern CommandEntry CommandTableGeneralInput[];

/* Sub-Menue Table */
#define CLI_COMPONENTS_LIST \
CommandEntrySubMenu("led", CommandTableLed, "Led on/off"),\
CommandEntrySubMenu("item", CommandTableItemMmt, "ItemMmt(cache 20)"),\
CommandEntrySubMenu("apslog", CommandTableApsLog, "apslog(cache 20)"),\
CommandEntrySubMenu("apslamp", CommandTableApsLamp, "apslamp"),\
CommandEntrySubMenu("gps", CommandTablGps, "GPS"), \
CommandEntrySubMenu("clk", CommandTablClk, "CLK"), \
CommandEntrySubMenu("ws", CommandTablWs, "WS"), \
CommandEntrySubMenu("debug", CommandTablDebug, "DEBUG"), \
CommandEntrySubMenu("log", CommandTableLog, "LOG"), \
CommandEntrySubMenu("prop", CommandTableProp, "PROP"), \
CommandEntrySubMenu("nb", CommandTableBc28, "nb"), \
CommandEntrySubMenu("elec", CommandTableElec, "ELEC"), \
CommandEntrySubMenu("daemon", CommandTableDaemon, "Daemon"), \
CommandEntrySubMenu("ota", CommandTabl_ota, "OTA"), \
CommandEntrySubMenu("wifi", CommandTableWiFi, "WiFi info"), \
CommandEntrySubMenu("sys", CommandTableSys, "mem info"), \
CommandEntrySubMenu("test", CommandTablTest, "test"), \
CommandEntrySubMenu("logmmt", CommandTable_LogMmt, "Log db 2.0"), \
CommandEntrySubMenu("loc", CommandTablLoc, "loc"), \
CommandEntrySubMenu("msgmmt", CommandTable_MsgMmt, "msgmmt"), \
CommandEntrySubMenu("adjust", CommandTableAdjust, "Adjust"), \
CommandEntrySubMenu("lightsensor", CommandTab_LightSensor, "lightsensor"), \
CommandEntrySubMenu("bspTest", CommandTablBspTest, "bspTest"), \
CommandEntrySubMenu("wdt", CommandTableWdt, "wdt"), \
CommandEntrySubMenu("exep", CommandTableExep, "exep"), \
CommandEntrySubMenu("testinfo", CommandTablTestInfo, "test info"), \
CommandEntrySubMenu("GI", CommandTableGeneralInput, "General input"), \




// CommandEntrySubMenu("queue", CommandQueueTable, "Queue(cache 60)"),
// CommandEntrySubMenu("lightlib", CommandTableLightlib, "Lightlib"),


/* Init Interfaces */
#define CLI_COMPONENTS_INIT \
lightlib_cli_init();

/* Loop Interfaces */
#define CLI_COMONENTS_LOOP \
lightlib_cli_loop();


/*************************************************************************************************
                                          Cli
*************************************************************************************************/
extern CommandEntry MainTable[];

extern void cliInit(void);
extern void cliLoop(void);


#endif


