#include "cli-interpreter.h"
#include "APP_LampCtrl.h"
#include "SVS_Elec.h"

void CommandVoltageGet(void);
void CommandCurrentGet(void);
void CommandVoltageSet(void);
void CommandCurrentSet(void);
void CommandConsumptionGet(void);
void CommandElecInfo(void);
void CommandElecGetFlash(void);
void CommandElecSetFlash(void);
void CommandElecGetPower(void);
void CommandElecSetPower(void);

CommandEntry CommandTableElec[] =
{
	CommandEntryActionWithDetails("gVoltage",     CommandVoltageGet,    "", "get elec voltage...", NULL),
	CommandEntryActionWithDetails("gCurrent",     CommandCurrentGet,    "", "get elec current...", NULL),
	CommandEntryActionWithDetails("sCurrent",     CommandCurrentSet,    "u", "get elec current...", NULL),
	CommandEntryActionWithDetails("sVoltage",     CommandVoltageSet,    "u", "set elec voltage...", NULL),
	CommandEntryActionWithDetails("gConsumption", CommandConsumptionGet, "", "get elec consumption...", NULL),
	CommandEntryActionWithDetails("info",         CommandElecInfo,       "", "get elec info...", NULL),
	CommandEntryActionWithDetails("gFlash",		  CommandElecGetFlash,	 "", "get elec flash...", NULL),
	CommandEntryActionWithDetails("sFlash", 	  CommandElecSetFlash,	 "u", "set elec flash...", NULL),
	CommandEntryActionWithDetails("sPower", 	  CommandElecSetPower,	 "u", "set elec power...", NULL),
	CommandEntryActionWithDetails("gPower", 	  CommandElecGetPower,	 "", "get elec power...", NULL),

	CommandEntryTerminator()
};

void CommandVoltageGet(void)
{
	unsigned int voltage;
	ElecVoltageGet(0, &voltage);
	CLI_PRINTF("voltage %d\r\n", voltage);
}

void CommandVoltageSet(void)
{
	//g_stThisLampInfo.stCurrElecric.fVoltage = CliIptArgList[0][0];
	ElecVoltageSet(0, CliIptArgList[0][0]);
	CLI_PRINTF("voltage %d\r\n", CliIptArgList[0][0]);
}

void CommandCurrentGet(void)
{
	unsigned int current;
	ElecCurrentGet(0, &current);
	CLI_PRINTF("current %d\r\n", current);
}

void CommandCurrentSet(void)
{
	//g_stThisLampInfo.stCurrElecric.fCurrent= CliIptArgList[0][0];
	ElecCurrentSet(0, CliIptArgList[0][0]);
	CLI_PRINTF("current %d\r\n", CliIptArgList[0][0]);
}

void CommandConsumptionGet(void)
{
	ST_OPP_LAMP_CURR_ELECTRIC_INFO stElecInfo;
		
	ElecGetElectricInfo(&stElecInfo);
	CLI_PRINTF("Consumption %d\r\n", stElecInfo.consumption);
	CLI_PRINTF("History Consumption %d\r\n", stElecInfo.hisConsumption);

}
void CommandElecInfo(void)
{
	ST_OPP_LAMP_CURR_ELECTRIC_INFO stElecInfo;
		
	ElecGetElectricInfo(&stElecInfo);
	
	CLI_PRINTF("current %d\r\n", stElecInfo.current);
	CLI_PRINTF("voltage %d\r\n", stElecInfo.voltage);
	CLI_PRINTF("power %d\r\n", stElecInfo.power);
	CLI_PRINTF("consumption %d\r\n", stElecInfo.consumption);
	CLI_PRINTF("hisConsumption %d\r\n", stElecInfo.hisConsumption);
	CLI_PRINTF("fDelta %d\r\n", stElecInfo.delta);
	CLI_PRINTF("fFactor %d\r\n", stElecInfo.factor);
}

void CommandElecGetFlash(void)
{
	ST_ELEC_CONFIG stElecConfig;
	int ret;
	unsigned int ltime;
	
	ret = ElecReadFlash(&stElecConfig);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("ElecReadFlash ret %d\r\n", ret);
		return;
	}
	LightTimeCrc8ToLightTime(stElecConfig.hisLightTime,&ltime);
	CLI_PRINTF("hisConsumption %u hisTime %u hisLightTime %u\r\n", stElecConfig.hisConsumption, stElecConfig.hisTime, ltime);
}
void CommandElecSetFlash(void)
{
	ST_ELEC_CONFIG stElecConfig;
	int ret;

	if(CliIptArgList[0][0] > MAX_EC){
		CLI_PRINTF("hisConsumption should not great than %d\r\n", MAX_EC);
		return;
	}
	OppLampCtrlGetHtime(0,&stElecConfig.hisTime);
	OppLampCtrlGetHLtimeWithCrc8(0,&stElecConfig.hisLightTime);
	ret = ElecWriteFlash(&stElecConfig);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("ElecWriteFlash ret %d\r\n", ret);
		return;
	}

	ElecHisConsumptionSet(CliIptArgList[0][0]);
}
void CommandElecGetPower(void)
{
	U32 power;
	int ret;
	
	ret = ElecPowerGet(0,&power);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("ElecReadFlash ret %d\r\n", ret);
		return;
	}
	
	CLI_PRINTF("power %d\r\n", power);
}
void CommandElecSetPower(void)
{
	ElecPowerSet(0,CliIptArgList[0][0]);
}

