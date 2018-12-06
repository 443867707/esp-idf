#include "cli-interpreter.h"

#define GI_RSC_MAX 10
static unsigned int GI_RSC[GI_RSC_MAX];

const char* const CommandTableCommandArgumentsRsc[] = {
  "index(0-9)",
  "value",
};

void CommandSetRsc(void);
void CommandGetRsc(void);

CommandEntry CommandTableGeneralInput[] =
{
	CommandEntryActionWithDetails("setRsc", CommandSetRsc, "uu", "set rsc...", CommandTableCommandArgumentsRsc),
	CommandEntryActionWithDetails("getRsc", CommandGetRsc, "u", "get rsc...", CommandTableCommandArgumentsRsc),
	
	CommandEntryTerminator()
};

void CommandSetRsc(void)
{
    if(CliIptArgList[0][0] >= GI_RSC_MAX)
    {
        CLI_PRINTF("Index error(0-9)\r\n");
        return;
    }
    GI_RSC[CliIptArgList[0][0]] = CliIptArgList[1][0];
}

void CommandGetRsc(void)
{
	if(CliIptArgList[0][0] >= GI_RSC_MAX)
    {
        CLI_PRINTF("Index error(0-9)\r\n");
        return;
    }
    CLI_PRINTF("%d\r\n",GI_RSC[CliIptArgList[0][0]]);
}

unsigned int getGIU_RSC0(unsigned char target,unsigned int*res)
{
    *res = GI_RSC[0];
    return 0;
}

unsigned int getGIU_RSC1(unsigned char target,unsigned int*res)
{
    *res = GI_RSC[1];
    return 0;
}
