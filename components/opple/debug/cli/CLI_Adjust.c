#include "cli-interpreter.h"
#include "DRV_SpiMeter.h"
#include "DRV_DimCtrl.h"
#include "DRV_LightSensor.h"

void CommandVoltageAdjust(void);
void CommandCurrentAdjust(void);
void CommandPowerAdjust(void);
void CommandBPhCalAdjust(void);

void CommandVoltageParamGet(void);
void CommandCurrentParamGet(void);
void CommandPowerParamGet(void);
void CommandBPhCalParamGet(void);
void CommandDacLevel64ParamGet(void);
void CommandDacLevel192ParamGet(void);
void CommandLiSenParamGet(void);

void CommandDacParamDel(void);
void CommandElecParamDel(void);
void CommandLiSenParamDel(void);

void CommandVoltageParamSet(void);
void CommandCurrentParamSet(void);
void CommandPowerParamSet(void);
void CommandBPhCalParamSet(void);
void CommandDacLevel64ParamSet(void);
void CommandDacLevel192ParamSet(void);
void CommandLiSenParamSet(void);


CommandEntry CommandTableAdjust[] =
{
	CommandEntryActionWithDetails("Current", CommandCurrentAdjust, "u", "start ajust current...", NULL),
	CommandEntryActionWithDetails("Voltage", CommandVoltageAdjust, "u", "start adjust voltage...", NULL),
	CommandEntryActionWithDetails("Power", CommandPowerAdjust, "u", "start adjust power...", NULL),
	CommandEntryActionWithDetails("BPhCal", CommandBPhCalAdjust, "u", "start adjust BPh...", NULL),

	CommandEntryActionWithDetails("gCur", CommandCurrentParamGet, "", "get curent ajust param...", NULL),
	CommandEntryActionWithDetails("gVol", CommandVoltageParamGet, "", "get voltage adjust param...", NULL),
	CommandEntryActionWithDetails("gPower", CommandPowerParamGet, "", "get power adjust...", NULL),
	CommandEntryActionWithDetails("gBPhCal", CommandBPhCalParamGet, "", "get power adjust...", NULL),
	CommandEntryActionWithDetails("gDacL64", CommandDacLevel64ParamGet, "", "get voltage of dac level 64...", NULL),
	CommandEntryActionWithDetails("gDacL192", CommandDacLevel192ParamGet, "", "get voltage of dac level 192...", NULL),
	CommandEntryActionWithDetails("gLiSenVol", CommandLiSenParamGet, "", "get voltage of light sensor cal...", NULL),

	CommandEntryActionWithDetails("delDacParam", CommandDacParamDel, "", "del dac param...", NULL),
	CommandEntryActionWithDetails("delElecParam", CommandElecParamDel, "", "del elec param...", NULL),
	CommandEntryActionWithDetails("delLiSenParam", CommandLiSenParamDel, "", "del light sensor param...", NULL),


	CommandEntryActionWithDetails("sCur", CommandCurrentParamSet, "u", "set curent ajust param...", NULL),
	CommandEntryActionWithDetails("sVol", CommandVoltageParamSet, "u", "set voltage adjust param...", NULL),
	CommandEntryActionWithDetails("sPower", CommandPowerParamSet, "u", "set power adjustparam...", NULL),
	CommandEntryActionWithDetails("sBPhCal", CommandBPhCalParamSet, "u", "set bphcal adust param...", NULL),
	CommandEntryActionWithDetails("sDacLevel64", CommandDacLevel64ParamSet, "u", "set the voltage of dac level 64 ...", NULL),
	CommandEntryActionWithDetails("sDacLevel192", CommandDacLevel192ParamSet, "u", "set the voltage of dac level 192 ...", NULL),
	CommandEntryActionWithDetails("sLiSenVol", CommandLiSenParamSet, "u", "set the voltage ofligt sensor cal voltage  ...", NULL),

	CommandEntryTerminator()
};

void CommandVoltageAdjust()
{
	VoltageAdjust(CliIptArgList[0][0]);
    CLI_PRINTF("voltage %d\r\n", CliIptArgList[0][0]);
}

void CommandCurrentAdjust()
{
	CurrentAdjust(CliIptArgList[0][0]);
    CLI_PRINTF("current %d\r\n", CliIptArgList[0][0]);
}

void CommandPowerAdjust()
{
	PowerAdjust(CliIptArgList[0][0]);
    CLI_PRINTF("power %d\r\n", CliIptArgList[0][0]);
}

void CommandBPhCalAdjust()
{
	BPhCalAdjust(CliIptArgList[0][0]);
    CLI_PRINTF("BphCal %d\r\n", CliIptArgList[0][0]);
}

void CommandVoltageParamGet()
{
    uint8_t u8Ret;
    uint16_t Ugain = 0;

    u8Ret = VoltageParamGet(&Ugain);

    if (u8Ret == 0) {
        CLI_PRINTF("Ugain %x \r\n", Ugain);
    } else {
        CLI_PRINTF("no voltage param or get err \r\n");
    }
}

void CommandCurrentParamGet()
{
    uint8_t u8Ret;
    uint16_t IbGain = 0;

    u8Ret = CurrentParamGet(&IbGain);
    if (u8Ret == 0) {
        CLI_PRINTF("IbGain %x \r\n", IbGain);
    } else {
        CLI_PRINTF("no current param or get err \r\n");
    }
}

void CommandPowerParamGet()
{
    uint8_t u8Ret;
    uint16_t PbGain = 0;

    u8Ret = PowerParamGet(&PbGain);
    if (u8Ret == 0) {
        CLI_PRINTF("PbGain %x\r\n", PbGain);
    } else {
        CLI_PRINTF("no power param or get err \r\n");
    }
}

void CommandBPhCalParamGet()
{
    uint8_t u8Ret, u8BPhCal = 0;

    u8Ret = BPhCalParamGet(&u8BPhCal);
    if (u8Ret == 0) {
        CLI_PRINTF("BPhCal %x\r\n", u8BPhCal);
    } else {
        CLI_PRINTF("no BPhCal param or get err \r\n");
    }
}

void CommandDacLevel64ParamGet()
{
    uint8_t u8Ret;
    uint16_t u16DacOutput = 0;
    u8Ret = DacLevel64ParamGet(&u16DacOutput);
    if (u8Ret == 0) {
        CLI_PRINTF("DacLevel64 %u\r\n", u16DacOutput);
    } else {
        CLI_PRINTF("no DacLevel 64 param or get err \r\n");
    }
}

void CommandDacLevel192ParamGet()
{
    uint8_t u8Ret;
    uint16_t u16DacOutput = 0;

    u8Ret = DacLevel192ParamGet(&u16DacOutput);
    if (u8Ret == 0) {
        CLI_PRINTF("DacLevel192 %u\r\n", u16DacOutput);
    } else {
        CLI_PRINTF("no DacLevel 192 param or get err \r\n");
    }
}

void CommandLiSenParamGet()
{
    uint8_t u8Ret;
    uint16_t u16LiSenVol = 0;

    u8Ret = LightSensorParamGet(&u16LiSenVol);
    if (u8Ret == 0) {
        CLI_PRINTF("light sensor cal vol %u\r\n", u16LiSenVol);
    } else {
        CLI_PRINTF("no light sensor param \r\n");
    }
}

void CommandDacParamDel()
{
    delDacCalParam();
    CLI_PRINTF("Dac Param Del Done \r\n");
}

void CommandElecParamDel()
{
    delAdjustParam();
    CLI_PRINTF("Elec Param Del Done \r\n");
}

void CommandLiSenParamDel()
{
    DelLightSensorParam();
    CLI_PRINTF("Light Senor  Param Del Done \r\n");
}

void CommandVoltageParamSet()
{
    VoltageParamSet((uint16_t)CliIptArgList[0][0]);
    CLI_PRINTF("Ugain %x \r\n", CliIptArgList[0][0]);
}

void CommandCurrentParamSet()
{
	CurrentParamSet((uint16_t)CliIptArgList[0][0]);
    CLI_PRINTF("IbGain %x \r\n", CliIptArgList[0][0]);
}

void CommandPowerParamSet()
{
	PowerParamSet(CliIptArgList[0][0]);
    CLI_PRINTF("PbGain %x \r\n", CliIptArgList[0][0]);
}

void CommandBPhCalParamSet()
{
	BPhCalParamSet(CliIptArgList[0][0]);
    CLI_PRINTF("BPhCal %x \r\n", CliIptArgList[0][0]);
}

void CommandDacLevel64ParamSet()
{
	DacLevel64ParamSet(CliIptArgList[0][0]);
    CLI_PRINTF("dac level 64 %u \r\n", CliIptArgList[0][0]);
}

void CommandDacLevel192ParamSet()
{
	DacLevel192ParamSet(CliIptArgList[0][0]);
    CLI_PRINTF("dac level 192 %u \r\n", CliIptArgList[0][0]);
}

void CommandLiSenParamSet()
{
	LightSensorParamSet(CliIptArgList[0][0]);
    CLI_PRINTF("light sensor cal voltage  %u \r\n", CliIptArgList[0][0]);
}
