#include "cli-interpreter.h"
#include "opple_main.h"


void CommandWdt_set(void);
void CommandWdt_get(void);

// CliIptArgList[0][0]
const char* const CommandTableWdtCommandArguments[] = {
  "Period(S)",
  "Reset Enable",
};


CommandEntry CommandTableWdt[] =
{
	CommandEntryActionWithDetails("set", CommandWdt_set, "uu", "Task wdt set", CommandTableWdtCommandArguments),
	CommandEntryActionWithDetails("get", CommandWdt_get, "", "Task wdt get", (void*)0),
	CommandEntryTerminator()
};


void CommandWdt_set(void)
{
    int res;
    TaskWdtConfig_EN config;
    
    config.period = CliIptArgList[0][0];
    config.resetEn = CliIptArgList[1][0];
    res = taskWdtConfigSet(&config);
    if(res!=0)
    {
        CLI_PRINTF("Fail,err=%d\r\n",res);
    }
}

void CommandWdt_get(void)
{
    int res;
    TaskWdtConfig_EN config;
    
    res = taskWdtConfigGet(&config);
    if(res != 0)
    {
        CLI_PRINTF("Fail,err=%d\r\n",res);
    }
    else
    {
        CLI_PRINTF("Period:%d,ResetEn:%d\r\n",config.period,config.resetEn);
    }
}


