#include "cli-interpreter.h"
#include "DEV_ItemMmt.h"

#define ITEM_CACHE_MAX 300
static unsigned char tmp[ITEM_CACHE_MAX];

void CommandItmRead(void);
void CommandItmWrite(void);
void CommandItmReadU32(void);
void CommandItmWriteU32(void);


const char* const argitmRead[] = {
"Module","Id","Data array","Len"
};


CommandEntry CommandTableItemMmt[] =
{
	CommandEntryActionWithDetails("Read", CommandItmRead, "uu", "Read item array", argitmRead),
	CommandEntryActionWithDetails("Write", CommandItmWrite, "uuau", "Write item array", argitmRead),
    CommandEntryActionWithDetails("ReadU32", CommandItmReadU32, "uu", "Read item u32", argitmRead),
	CommandEntryActionWithDetails("WriteU32", CommandItmWriteU32, "uuu", "Write item u32", argitmRead),
	CommandEntryTerminator()
};


void CommandItmRead(void)
{
    int err;
    unsigned int len = ITEM_CACHE_MAX;
    err = DataItemRead(CliIptArgList[0][0],CliIptArgList[1][0],tmp,&len);
    if(err != 0){
        CLI_PRINTF("Read fail,err=%d\r\n",err);
        return;
    }
    for(unsigned int i=0;i < len;i++)
    {
        CLI_PRINTF("%d ",tmp[i]);
    }
    CLI_PRINTF("\r\n");
}

void CommandItmWrite(void)
{
    unsigned int len = CliIptArgList[3][0];
    int err;
    
    if(len > ITEM_CACHE_MAX)
    {
        CLI_PRINTF("Fail,Length of array over 20!\r\n");
        return;
    }
    
    for(unsigned int i = 0; i< len ;i++)
    {
        tmp[i] = CliIptArgList[2][i];
    }
    err =DataItemWrite(CliIptArgList[0][0],CliIptArgList[1][0],tmp,len);
    if(err != 0)
    {
        CLI_PRINTF("Write fail,err=%d\r\n",err);
    }
}

void CommandItmReadU32(void)
{
    int err;
    unsigned int data;
    err = DataItemReadU32(CliIptArgList[0][0],CliIptArgList[1][0],&data);
    if(err != 0){
        CLI_PRINTF("Read fail,err=%d\r\n",err);
        return;
    }
    CLI_PRINTF("%d\r\n",data);
}

void CommandItmWriteU32(void)
{
    int err;
    
    err =DataItemWriteU32(CliIptArgList[0][0],CliIptArgList[1][0],CliIptArgList[2][0]);
    if(err != 0)
    {
        CLI_PRINTF("Write fail,err=%d\r\n",err);
    }
}
