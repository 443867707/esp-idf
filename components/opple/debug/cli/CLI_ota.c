#include "cli-interpreter.h"
#include "SVS_Ota.h"
#include "string.h"

/***********************************************************************************************
Fun
***********************************************************************************************/
void CommandOta_set(void);
void CommandOta_show(void);
void CommandOta_start(void);

void CommandOra_ov_set(void);
void CommandOra_ov_get(void);
void CommandOra_hv_push(void);
void CommandOra_hv_get(void);




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
	CommandEntryActionWithDetails("ovget", CommandOra_ov_get, "", "get original version", (void*)0),
	CommandEntryActionWithDetails("ovset", CommandOra_ov_set, "u", "set original version", (void*)0),
	CommandEntryActionWithDetails("hvpush", CommandOra_hv_push, "u", "history version push", (void*)0),
	CommandEntryActionWithDetails("hvget", CommandOra_hv_get, "", "history version get", (void*)0),
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

void CommandOra_ov_set(void)
{
	int err;

	err = OriginalVersionSet(CliIptArgList[0][0]);
	if(err != 0) CLI_PRINTF("Fail,err=%d\r\n",err);
}

void CommandOra_ov_get(void)
{
	int err;
	unsigned int v;

	err = OriginalVersinGet(&v);
	if(err != 0) CLI_PRINTF("Fail,err=%d\r\n",err);
	else CLI_PRINTF("Original version : %d\r\n",v);
}

void CommandOra_hv_push(void)
{
    int err;

    err = HistoryVersionPush(CliIptArgList[0][0]);
    if(err != 0) CLI_PRINTF("Fail,err=%d\r\n",err);
}

void CommandOra_hv_get(void)
{
	unsigned int hv[15],len=15;
	int err;

	err = HistoryVersionGet(hv,&len);
	if(err != 0){
		CLI_PRINTF("Fail,err=%d\r\n",err);
	}else {
		CLI_PRINTF("History Version(from the Original Version to the latest Version):\r\n");
		for(int i = 0;i < len;i++)
		{
			CLI_PRINTF("%d ",hv[i]);
		}
		CLI_PRINTF("\r\n");
	}
}




