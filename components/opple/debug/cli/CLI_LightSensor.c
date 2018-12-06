#include "cli-interpreter.h"
#include "DRV_LightSensor.h"

void Command_GetVolt(void);
void Command_GetLx(void);

CommandEntry CommandTab_LightSensor[] =
{
	CommandEntryActionWithDetails("volt", Command_GetVolt, "", "Get volt", (void*)0),
  CommandEntryActionWithDetails("lx", Command_GetLx, "", "Get lx", (void*)0),
	CommandEntryTerminator()
};


void Command_GetVolt(void)
{
    unsigned res,data;

    res = BspLightSensorVoltageGet(&data);
    if(res != 0)
    {
        CLI_PRINTF("Fail,err=%d\r\n",res);
    }
    else
    {
        CLI_PRINTF("Volt: %d \r\n",data);
    }
}

void Command_GetLx(void)
{
    unsigned res,data;

    res = BspLightSensorLuxGet(&data);
    if(res != 0)
    {
        CLI_PRINTF("Fail,err=%d\r\n",res);
    }
    else
    {
        CLI_PRINTF("Lx: %d \r\n",data);
    }
}
