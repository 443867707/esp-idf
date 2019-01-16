#include "cli-interpreter.h"
#include <string.h>
#include "DRV_LightSensor.h"
#include "DRV_Gps.h"
#include "DRV_SpiMeter.h"
#include "DRV_DimCtrl.h"
#include "DRV_Nb.h"
#include "DRV_Led.h"

const char* const CommandArguments_RelayLevel[]={
	"level 0-1 (0:off, 1:on)"
};

const char* const CommandArguments_DimType[]={
	"type 0-1 (0:dac, 1:pwm)"
};

const char* const CommandArguments_PwmLevel[]={
	"level 0-255(0:0%, 255:100%)"
};
const char* const CommandArguments_DacLevel[]={
	"level 0-255(0:0v, 255:10v)"
};

const char* const CommandArguments_GpsRstLevel[]={
	"level 0-1 (0:no reset, 1:reset)"
};

const char* const CommandArguments_NbRstLevel[]={
	"level 0-1 (0:no reset, 1:reset)"
};

const char* const CommandArguments_LedLevel[]={
	"level 0-1 (0: off, 1: on)"
};

void Command_GetMeterInfo(void)
{
    uint8_t u8Ret;

    meter_info_t info;

    memset(&info, 0, sizeof(meter_info_t));

    u8Ret = MeterInfoGet(&info);
    if (u8Ret == 0) {
	    CLI_PRINTF("Energy = %u, Power = %u, Voltage = %u, Current = %u, Factor = %u \r\n", info.u16Energy, info.u32Power, info.u32Voltage, info.u32Current, info.u32PowerFactor);
	    CLI_PRINTF("Meter info get success \r\n");
    } else {
	    CLI_PRINTF("Meter info get fail \r\n");
    }
}

void Command_SetRelayLevel(void)
{
    uint8_t u8Ret;

    if (CliIptArgList[0][0] == 0) {
        u8Ret = DimRelayOff();
        if (u8Ret == 0) {
	        CLI_PRINTF("set relay level %d success \r\n", CliIptArgList[0][0]);
        } else {
	        CLI_PRINTF("set relay level %d fial \r\n", CliIptArgList[0][0]);
        }
    } else if (CliIptArgList[0][0] == 1) {
        u8Ret = DimRelayOn();
        if (u8Ret == 0) {
	        CLI_PRINTF("set relay level %d success \r\n", CliIptArgList[0][0]);
        } else {
	        CLI_PRINTF("set relay level %d fial \r\n", CliIptArgList[0][0]);
        }

    } else {
	    CLI_PRINTF("unkown relay level %d !!!! \r\n", CliIptArgList[0][0]);
   }
}

void Command_SetDimType(void)
{
    uint8_t u8Ret;

    if (CliIptArgList[0][0] == 0) {
        u8Ret = DimCtrlSwitchToDac();
        if (u8Ret == 0) {
	        CLI_PRINTF("set dim type dac success \r\n");
        } else {
	        CLI_PRINTF("set dim type dac fail \r\n");
        }
    } else if (CliIptArgList[0][0] == 1) {
        u8Ret = DimCtrlSwitchToPwm();
        if (u8Ret == 0) {
	        CLI_PRINTF("set dim type pwm success \r\n");
        } else {
	        CLI_PRINTF("set dim type pwm fail \r\n");
        }

    } else {
	    CLI_PRINTF("unkown dim type %d !!!! \r\n", CliIptArgList[0][0]);
    }
}

void Command_SetPwmLevel(void)
{
    uint8_t u8Ret;

    u8Ret = DimPwmDutyCycleSet(CliIptArgList[0][0]);
    if (u8Ret == 0) {
	    CLI_PRINTF("set pwm level %d success \r\n", CliIptArgList[0][0]);
    } else {
	    CLI_PRINTF("set pwm level %d fail \r\n", CliIptArgList[0][0]);
    }
}

void Command_SetDacLevel(void)
{
    uint8_t u8Ret;

    u8Ret = DimDacRawLevelSet(CliIptArgList[0][0]);
    if (u8Ret == 0) {
	    CLI_PRINTF("set dac level %d success \r\n", CliIptArgList[0][0]);
    } else {
	    CLI_PRINTF("set dac level %d fail \r\n", CliIptArgList[0][0]);
    }
}

void Command_RecvGpsInfo(void)
{
    uint8_t u8Recv;

    u8Recv = GpsTest();
    if (u8Recv == 0) {
	    CLI_PRINTF("gps is running \r\n");
    } else if (u8Recv == 1) {
	    CLI_PRINTF("test err, try again \r\n");
    } else {
	    CLI_PRINTF("gps is not work \r\n");
    }
}

void Command_SetGpsRstPinLevel(void)
{
    uint8_t u8Ret;

    u8Ret = GpsSetRstGpioLevel(CliIptArgList[0][0]);
    if (u8Ret == 0) {
	    CLI_PRINTF("set gps rst pin level %d success \r\n", CliIptArgList[0][0]);
    } else {
	    CLI_PRINTF("set gps rst pin level %d fail \r\n", CliIptArgList[0][0]);
    }
}

void Command_SendNbCmd(void)
{
    char *cmd = "AT+NUESTATS\r";
    char *cmd1 = "AT+CGATT?\r";
    char *cmd2 = "AT+CGPADDR\r";

    uint8_t *data = (uint8_t *) malloc(256);

	CLI_PRINTF("send cmd %s \r\n", cmd);
    NbUartWriteBytes(cmd, strlen(cmd));
    memset(data, 0, 256);
    NbUartReadBytes(data, 256, 300);
	CLI_PRINTF("%s \r\n", data);

	CLI_PRINTF("send cmd %s \r\n", cmd1);
    NbUartWriteBytes(cmd1, strlen(cmd1));
    memset(data, 0, 256);
    NbUartReadBytes(data, 256, 300);
	CLI_PRINTF("%s \r\n", data);

	CLI_PRINTF("send cmd %s \r\n", cmd2);
    NbUartWriteBytes(cmd2, strlen(cmd2));
    memset(data, 0, 256);
    NbUartReadBytes(data, 256, 300);
	CLI_PRINTF("%s \r\n", data);
}

void Command_SetNbRstPinLevel(void)
{
    uint8_t u8Ret;

    u8Ret = NbSetRstGpioLevel(CliIptArgList[0][0]);
    if (u8Ret == 0) {
	    CLI_PRINTF("set nb rst pin level %d success \r\n", CliIptArgList[0][0]);
    } else {
	    CLI_PRINTF("set nb rst pin level %d fail \r\n", CliIptArgList[0][0]);
    }
}

void Command_GetLightSensorVol(void)
{
    uint8_t u8Ret;
    uint32_t u32Voltage;

    u8Ret = BspLightSensorVoltageGet(&u32Voltage);
    if (u8Ret == 0) {
	    CLI_PRINTF("get Light Sensor Voltage %u \r\n", u32Voltage);
	    CLI_PRINTF("sucess\r\n");
    } else {
	    CLI_PRINTF("get LIght Sensor Voltage fail \r\n");
    }
}



void Command_SetLedLevel(void)
{
    uint8_t u8Ret;

    if (CliIptArgList[0][0] == 0) {
        u8Ret = NB_Status_Led_Off();
        if (u8Ret == 0) {
	        CLI_PRINTF("set led on success \r\n");
        } else {
	        CLI_PRINTF("set led off fail \r\n");
        }
    } else if (CliIptArgList[0][0] == 1) {
        u8Ret = NB_Status_Led_On();
        if (u8Ret == 0) {
	        CLI_PRINTF("set led on success \r\n");
        } else {
	        CLI_PRINTF("set led off fail \r\n");
        }
    } else {
	   CLI_PRINTF("unkwon led level %d \r\n", CliIptArgList[0][0]);
    }
}

void Command_resetMeter()
{
    uint8_t ret;
    ret = MeterReset();
    if (ret) {
	    CLI_PRINTF("reset fail \r\n");
    } else {
	    CLI_PRINTF("reset success \r\n");
    }
}

void Command_dumpMeterReg()
{
    uint8_t ret;
    ret = MeterDumpReg();
    if (ret) {
	    CLI_PRINTF("dump fail \r\n");
    } else {
	    CLI_PRINTF("dump success \r\n");
    }
}

void Command_reInitMeter()
{
    uint8_t ret;
    ret = MeterReinit();
    if (ret) {
	    CLI_PRINTF("reInit fail \r\n");
    } else {
	    CLI_PRINTF("reInit success \r\n");
    }
}

void Command_ShowHardStatus()
{
    extern uint16_t g_DacLevel;
    extern uint16_t g_PwmLevel;
    extern uint8_t  g_RelayGpioStatus;
    extern uint8_t  g_DimTypeStatus;
    
	CLI_PRINTF("dacLevel :%u, pwmLevel:%u, relaySatus: %s , DimType : %s \r\n", g_DacLevel, g_PwmLevel, g_RelayGpioStatus == 0 ? "OFF" : "ON", g_DimTypeStatus == 0 ? "0-10V" : "PWM");
}

CommandEntry CommandTablBspTest[] =
{
	CommandEntryActionWithDetails("gMeter", Command_GetMeterInfo, "", "", (void*)0),
	CommandEntryActionWithDetails("sRelay", Command_SetRelayLevel, "u", "", CommandArguments_RelayLevel),
	CommandEntryActionWithDetails("sDimType", Command_SetDimType, "u", "", CommandArguments_DimType),
	CommandEntryActionWithDetails("sPwmLevel", Command_SetPwmLevel, "u", "", CommandArguments_PwmLevel),
	CommandEntryActionWithDetails("sDacLevel", Command_SetDacLevel, "u", "", CommandArguments_DacLevel),
	CommandEntryActionWithDetails("GpsTest", Command_RecvGpsInfo, "", "", (void*)0),
	CommandEntryActionWithDetails("sGpsRst", Command_SetGpsRstPinLevel, "u", "", CommandArguments_GpsRstLevel),
	CommandEntryActionWithDetails("NbTest", Command_SendNbCmd, "", "", (void*)0),

	CommandEntryActionWithDetails("sNbRst", Command_SetNbRstPinLevel, "u", "", CommandArguments_NbRstLevel),
	CommandEntryActionWithDetails("gLighSensor", Command_GetLightSensorVol, "", "", (void*)0),
	CommandEntryActionWithDetails("sLed", Command_SetLedLevel, "u", "", CommandArguments_LedLevel),
	CommandEntryActionWithDetails("rstMeter", Command_resetMeter, "", "", (void *)0),
	CommandEntryActionWithDetails("reInitMeter", Command_reInitMeter, "", "", (void *)0),
	CommandEntryActionWithDetails("DumpMeterReg", Command_dumpMeterReg, "", "", (void *)0),
	CommandEntryActionWithDetails("showHardStatus", Command_ShowHardStatus, "", "", (void*)0),

	CommandEntryTerminator()
};
