#include "cli-interpreter.h"
#include "DEV_LogDb.h"

static unsigned char tmp[20];

void CommandLogDbWrite(void);
void CommandLogDbRead(void);
void CommandLogDbGetLatestIndex(void);



const char* const argLogDbWrite[] = {
  "The array to push",
  "The length of data to push"
};

const char* const argLogDbRead[] = {
  "The save id",
};

CommandEntry CommandTableLogDb[] =
{
	CommandEntryActionWithDetails("write", CommandLogDbWrite, "au", "Push log", argLogDbWrite),
	CommandEntryActionWithDetails("query", CommandLogDbRead, "u", "Query log", argLogDbRead),
	CommandEntryActionWithDetails("getLatestIndex", CommandLogDbGetLatestIndex, "", "Latest save id", argLogDbRead),
	CommandEntryTerminator()
};

void CommandLogDbGetLatestIndex(void)
{
    unsigned int id;

    id = LogDbLatestIndex();

    CLI_PRINTF("Latest save id:%d\r\n",id);
}

void CommandLogDbWrite(void)
{
    unsigned int len = CliIptArgList[1][0];
    int err;
    unsigned int saveid;
    
    if(len > 20)
    {
        CLI_PRINTF("Fail,Length of array over 20!\r\n");
        return;
    }
    
    for(unsigned int i = 0; i< len ;i++)
    {
        tmp[i] = CliIptArgList[0][i];
    }
    err =LogDbWrite(tmp,len,&saveid);
    if(err != 0)
    {
        CLI_PRINTF("Write fail,err=%d\r\n",err);
    }
    else
    {
        CLI_PRINTF("Write success,saveid=%d\r\n",saveid);
    }
}

void CommandLogDbRead(void)
{
    int err;
    unsigned int len=20;

    err = LogDbQuery(CliIptArgList[0][0],tmp,&len);
    if(err != 0)
    {
        if(err == 2)
            CLI_PRINTF("LogDb empty!\r\n");
        else
            CLI_PRINTF("Query fail,err=%d\r\n",err);
        return;
    }
    
    for(unsigned int i = 0;i < len;i++){
    CLI_PRINTF("%d ",tmp[i]);
    }
    CLI_PRINTF("\r\n");
}

