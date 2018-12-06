#include "cli-interpreter.h"
#include "SVS_Ota.h"
#include "string.h"

/***********************************************************************************************
Fun
***********************************************************************************************/
void CommandOta_set(void);
void CommandOta_show(void);
void CommandOta_start(void);


/***********************************************************************************************
Args
***********************************************************************************************/
const char* const Arguments_set[] = {
  "ip,192.168.10.222",
  "port,80",
  "file,/opple.bin",
};

/***********************************************************************************************
Table
***********************************************************************************************/
CommandEntry CommandTabl_ota[] =
{
	CommandEntryActionWithDetails("set", CommandOta_set, "sss", "set ip/port/file...", Arguments_set),
  CommandEntryActionWithDetails("show", CommandOta_show, "", "Show settings...", (void*)0),
	CommandEntryActionWithDetails("start", CommandOta_start, "", "start ota...", (void*)0),
	CommandEntryTerminator()
};


/***********************************************************************************************
Fun implementation
***********************************************************************************************/
void CommandOta_set(void)
{
    strcpy(g_aucOtaSvrIp,&CliIptArgList[0][0]);
    strcpy(g_aucOtaSvrPort,&CliIptArgList[1][0]);
    strcpy(g_aucOtaFile,&CliIptArgList[2][0]);
}

void CommandOta_show(void)
{
    CLI_PRINTF("ip:%s,port:%s,file:%s\r\n",g_aucOtaSvrIp,g_aucOtaSvrPort,g_aucOtaFile);
}

void CommandOta_start(void)
{
    g_aucOtaStart = 1;
}




