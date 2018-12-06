#include "cli-interpreter.h"
#include "OPP_Debug.h"

extern t_debug_info module_list[];
extern t_debug_info_level level_list[];

void CommandMainEnable(void);
void CommandTimeEnable(void);
void CommandNbBc28Enable(void);
void CommandNbEnable(void);
void CommandCoapLampCtrlEnable(void);
void CommandCoapDebugEnable(void);
void CommandOtaDebugEnable(void);
void CommandGpsDebugEnable(void);
void CommandLocDebugEnable(void);
void CommandMeterDebugEnable(void);
void CommandAllDebugEnable(void);
void CommandShowDebugInfo(void);
void CommandApslampDebugEnable(void);
void CommandDebugEnableAus(void);
void CommandDebugEnableDaemon(void);
void CommandElecDebugEnable(void);
void CommandParaDebugEnable(void);
void CommandWifiDebugEnable(void);
void CommandMsgmmtDebugEnable(void);
void Command_LevelSet(void);
void Command_LevelFilt(void);
void Command_LevelShow(void);
void CommandLightSensorDebugEnable(void);
void CommandExepDebugEnable(void);

const char* const Command_Arg_level_set[]={
	"Level(1:fetal,2:error,3:warn,4:info,5:debug)",
	"Enable(0:disable,1:enable)"
};

const char* const Command_Arg_level_filt[]={
	"Level filter(1:fetal,2:<=error,3:<=warn,4:<=info,5:<=debug)"
};


CommandEntry CommandTablDebug[] =
{
	CommandEntryActionWithDetails("MAIN", CommandMainEnable, "u", "MAIN debug enable...", (void*)0),
	CommandEntryActionWithDetails("TIME", CommandTimeEnable, "u", "TIME debug enable...", (void*)0),
	CommandEntryActionWithDetails("BC28", CommandNbBc28Enable, "u", "BC28 debug enable...", (void*)0),
	CommandEntryActionWithDetails("NB", CommandNbEnable, "u", "NB debug enable...", (void*)0),
	CommandEntryActionWithDetails("LAMPCTRL", CommandCoapLampCtrlEnable, "u", "LAMPCTRL debug enable...", (void*)0),
	CommandEntryActionWithDetails("COAP", CommandCoapDebugEnable, "u", "COAP debug enable...", (void*)0),
	CommandEntryActionWithDetails("OTA", CommandOtaDebugEnable, "u", "OTA debug enable...", (void*)0),
	CommandEntryActionWithDetails("GPS", CommandGpsDebugEnable, "u", "GPS debug enable...", (void*)0),
	CommandEntryActionWithDetails("LOC", CommandLocDebugEnable, "u", "LOC debug enable...", (void*)0),
	CommandEntryActionWithDetails("METER", CommandMeterDebugEnable, "u", "METER debug enable...", (void*)0),
  	CommandEntryActionWithDetails("APSLAMP", CommandApslampDebugEnable, "u", "APSLAMP debug enable...", (void*)0),
  	CommandEntryActionWithDetails("AUS", CommandDebugEnableAus, "u", "AUS debug enable...", (void*)0),
  	CommandEntryActionWithDetails("DAEMON", CommandDebugEnableDaemon, "u", "DAEMON debug enable...", (void*)0),
	CommandEntryActionWithDetails("ELEC", CommandElecDebugEnable, "u", "ELEC debug enable...", (void*)0),
	CommandEntryActionWithDetails("PARA", CommandParaDebugEnable, "u", "PARA debug enable...", (void*)0),
	CommandEntryActionWithDetails("WIFI", CommandWifiDebugEnable, "u", "WIFI debug enable...", (void*)0),
	CommandEntryActionWithDetails("MSGMMT", CommandMsgmmtDebugEnable, "u", "MSGMMT(appcom,msgmmt,queue)", (void*)0),
	CommandEntryActionWithDetails("LIGHTSENSOR", CommandLightSensorDebugEnable, "u", "Light Sensor enable", (void*)0),
	CommandEntryActionWithDetails("EXEP", CommandExepDebugEnable, "u", "Exep enable", (void*)0),
	CommandEntryActionWithDetails("ALL", CommandAllDebugEnable, "u", "ALL debug enable...", (void*)0),
	CommandEntryActionWithDetails("SHOW", CommandShowDebugInfo, "", "SHOW debug info...", (void*)0),

	CommandEntryActionWithDetails("LevelSet", Command_LevelSet, "uu", "Level set", Command_Arg_level_set),
	CommandEntryActionWithDetails("LevelFilt", Command_LevelFilt, "u", "Filt level <= setting", Command_Arg_level_filt),
	CommandEntryActionWithDetails("LevelShow", Command_LevelShow, "", "Show level setting", Command_Arg_level_filt),

	CommandEntryTerminator()
};

void Command_LevelShow(void)
{
	int i;
    for(i = 0;i < 5;i++)
    {
        CLI_PRINTF("%-16s : %d\r\n",level_list[i].name,level_list[i].enable);
    }
}

void Command_LevelSet(void)
{
	int res;

	res = debug_level_set(CliIptArgList[0][0],CliIptArgList[1][0]);
	if(res != 0)
	{
		CLI_PRINTF("Fail,err=%d\r\n",res);
	}
}

void Command_LevelFilt(void)
{
	int res;

	res = debug_level_filt(CliIptArgList[0][0]);
	if(res != 0)
	{
		CLI_PRINTF("Fail,err=%d\r\n",res);
	}
}

void CommandLightSensorDebugEnable(void)
{
	module_list[DEBUG_MODULE_LIGHTSENSOR].enable = CliIptArgList[0][0];
}

void CommandExepDebugEnable(void)
{
	module_list[DEBUG_MODULE_EXEP].enable = CliIptArgList[0][0];
}

void CommandMsgmmtDebugEnable(void)
{
	module_list[DEBUG_MODULE_MSGMMT].enable = CliIptArgList[0][0];
}

void CommandDebugEnableAus(void)
{
    module_list[DEBUG_MODULE_AUS].enable = CliIptArgList[0][0];
}

void CommandDebugEnableDaemon(void)
{
    module_list[DEBUG_MODULE_DAEMON].enable = CliIptArgList[0][0];
}

void CommandApslampDebugEnable(void)
{
		module_list[DEBUG_MODULE_APSLAMP].enable = CliIptArgList[0][0];
}

void CommandMainEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_MAIN].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_MAIN].enable = DISABLE;
	}
}

void CommandTimeEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_TIME].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_TIME].enable = DISABLE;
	}
}

void CommandNbBc28Enable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_BC28].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_BC28].enable = DISABLE;
	}
}
void CommandNbEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_NB].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_NB].enable = DISABLE;
	}
}

void CommandNbLampCtrlEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_NB].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_NB].enable = DISABLE;
	}
}

void CommandCoapLampCtrlEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_LAMPCTRL].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_LAMPCTRL].enable = DISABLE;
	}
}

void CommandCoapDebugEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_COAP].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_COAP].enable = DISABLE;
	}
}


void CommandOtaDebugEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_OTA].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_OTA].enable = DISABLE;
	}
}


void CommandGpsDebugEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_GPS].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_GPS].enable = DISABLE;
	}
}

void CommandLocDebugEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_LOC].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_LOC].enable = DISABLE;
	}
}

void CommandMeterDebugEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_METER].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_METER].enable = DISABLE;
	}
}

void CommandElecDebugEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_ELEC].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_ELEC].enable = DISABLE;
	}
}

void CommandParaDebugEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_PARA].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_PARA].enable = DISABLE;
	}
}

void CommandWifiDebugEnable(void)
{
	if(1 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_WIFI].enable = ENABLE;
	}
	else if(0 == CliIptArgList[0][0]){
		module_list[DEBUG_MODULE_WIFI].enable = DISABLE;
	}
}

void CommandAllDebugEnable(void)
{
	  int i;
    for(i = 0;i < DEBUG_MODULE_MAX;i++)
    {
        module_list[i].enable = CliIptArgList[0][0];
    }
}
void CommandShowDebugInfo(void)
{
    int i;
    for(i = 0;i < DEBUG_MODULE_MAX;i++)
    {
        CLI_PRINTF("%-16s : %d\r\n",module_list[i].name,module_list[i].enable);
    }
}

