#include "cli-interpreter.h"
#include "SVS_Lamp.h"

void CommandApsLampOnOff(void);
void CommandApsLampDim(void);
void CommandApsLampMoveToLevel(void);
void CommandApsLampGetState(void);
void CommandApsLampSetDimType(void);
void CommandApsLampGetDimType(void);
void CommandApsLampBlink(void);

#define LAMP_SRC_CLI 88

const char* const argApsLampOnoff[] = {
  "0:off,1:on",
};
const char* const argApsLampDim[] = {
  "level,0-10000",
};
const char* const argApsLampMove[] = {
  "level,0-10000",
  "transaction time in ms",
};
const char* const argApsLampSetdimtype[] = {
  "type(0:0-10V,1:pwm)(ATTENTION!:Won't be saved,but app_lampctrl module do this)",
};
const char* const argApsLampBlink[] = {
  "state","lowLevel","highLevel","lowTime","highTime","lastTime",
};


CommandEntry CommandTableApsLamp[] =
{
	CommandEntryActionWithDetails("onoff", CommandApsLampOnOff, "u", "Lamp onoff", argApsLampOnoff),
	CommandEntryActionWithDetails("dim", CommandApsLampDim, "u", "Lamp dim(0-10000)", argApsLampDim),
	CommandEntryActionWithDetails("move2level", CommandApsLampMoveToLevel, "uu", "Lamp move to level", argApsLampMove),
	CommandEntryActionWithDetails("setDimType", CommandApsLampSetDimType, "u", "Set dim type", argApsLampSetdimtype),
	CommandEntryActionWithDetails("getDimType", CommandApsLampGetDimType, "", "Get dim type", (void*)0),
	CommandEntryActionWithDetails("getstate", CommandApsLampGetState, "", "Lamp get state", (const void*)0),
	CommandEntryActionWithDetails("blink", CommandApsLampBlink, "uuuuuu", "Lamp onoff", argApsLampBlink),
	CommandEntryTerminator()
};

void CommandApsLampBlink(void)
{
	int res;
	res = LampBlink(CliIptArgList[0][0],CliIptArgList[1][0],CliIptArgList[2][0], \
	CliIptArgList[3][0],CliIptArgList[4][0],CliIptArgList[5][0]);
	if(res!=0) CLI_PRINTF("Fail,err=%d\r\n",res);
}

void CommandApsLampSetDimType(void)
{
	int res;

	res = LampOuttypeSet(CliIptArgList[0][0]);
	if(res!=0)
	{
		CLI_PRINTF("Fail,err=%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("Success!\r\nATTENTION:Won't be saved,but app_lampctrl module do this\r\n");
	}
}

void CommandApsLampGetDimType(void)
{
	unsigned int type;

	type = LampOuttypeGet();
	CLI_PRINTF("%d <0:0-10V,1:pwm>\r\n",type);
}

void CommandApsLampOnOff(void)
{
    LampOnOff(LAMP_SRC_CLI,CliIptArgList[0][0]);
}
void CommandApsLampDim(void)
{
    LampDim(LAMP_SRC_CLI,CliIptArgList[0][0]);
}
void CommandApsLampMoveToLevel(void)
{
    LampMoveToLevel(LAMP_SRC_CLI,CliIptArgList[0][0],CliIptArgList[1][0]);
}
void CommandApsLampGetState(void)
{
    int state,level;
    
    LampStateRead(&state,&level);
    CLI_PRINTF("state:%s,level:%d\r\n",state==0?"off":"on",level);
}
