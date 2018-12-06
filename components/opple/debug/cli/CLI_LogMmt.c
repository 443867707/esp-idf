#include "cli-interpreter.h"
#include "DEV_LogMmt.h"
#include <stdlib.h>

static unsigned char tmp[40];

void Command_LogMmt_Write(void);
void Command_LogMmt_Read(void);
void Command_LogMmt_GetLatestId(void);
void Command_LogMmt_init(void);
void Command_LogMmt_readbitmtp(void);
void Command_LogMmt_erase_partition(void);
void Command_LogMmt_erase_sector(void);
void Command_LogMmt_partition_info(void);
void Command_LogMmt_ReadLatestItem(void);
void Command_LogMmt_partition_total(void);
void Command_LogMmt_partition_lastItem(void);
void Command_LogMmt_partition_lastId(void);

const char* const arg_LogMmt_Write[] = {
  "The partation [0,1]",
  "The array to push",
  "The length of data to push"
};

const char* const arg_LogMmt_Read[] = {
  "The partation [0,1]",
  "The save id",
};

const char* const arg_LogMmt_Readbitmap[] = {
  "The partation [0,1]",
  "The sector [0-]",
};

const char* const arg_LogMmt_LastItem[] = {
  "The partation [0,1]",
  "id now",
};


CommandEntry CommandTable_LogMmt[] =
{
	CommandEntryActionWithDetails("write", Command_LogMmt_Write, "uau", "Push log", arg_LogMmt_Write),
	CommandEntryActionWithDetails("read", Command_LogMmt_Read, "uu", "Query log", arg_LogMmt_Read),
	CommandEntryActionWithDetails("getLatestSaveId", Command_LogMmt_GetLatestId, "u", "Latest save id", arg_LogMmt_Read),
	CommandEntryActionWithDetails("readLatestItem", Command_LogMmt_ReadLatestItem, "u", "Read the latest item", arg_LogMmt_Read),
	CommandEntryActionWithDetails("init", Command_LogMmt_init, "", "Iinit bitmap", arg_LogMmt_Read),
	CommandEntryActionWithDetails("readbitmap", Command_LogMmt_readbitmtp, "uu", "read bitmap", arg_LogMmt_Readbitmap),
	CommandEntryActionWithDetails("erasepartition", Command_LogMmt_erase_partition, "u", "erase partition", arg_LogMmt_Readbitmap),
	CommandEntryActionWithDetails("erasesector", Command_LogMmt_erase_sector, "uu", "erase sector", arg_LogMmt_Readbitmap),
	CommandEntryActionWithDetails("info", Command_LogMmt_partition_info, "", "", arg_LogMmt_Readbitmap),
	CommandEntryActionWithDetails("total", Command_LogMmt_partition_total, "u", "Total num", arg_LogMmt_LastItem),
	CommandEntryActionWithDetails("lastItem", Command_LogMmt_partition_lastItem, "uu", "last item", arg_LogMmt_LastItem),
	CommandEntryActionWithDetails("lastId", Command_LogMmt_partition_lastId, "uu", "last id", arg_LogMmt_LastItem),
	
	CommandEntryTerminator()
};

void Command_LogMmt_partition_lastId(void)
{
	S32 id;
	id = LogMmtGetLastId(CliIptArgList[0][0],CliIptArgList[1][0]);
	CLI_PRINTF("%d\r\n",id);
}

void Command_LogMmt_partition_total(void)
{
	int res;
	U32 num;
	res = LogMmtToalItems(CliIptArgList[0][0],&num);
	if(res!=0) CLI_PRINTF("Fail,res=%d\r\n",res);
	else CLI_PRINTF("%d\r\n",num);
}

void Command_LogMmt_partition_lastItem(void)
{
	int res;
	int id;
	unsigned int len=40;
	res =  LogMmtReadLastItem(CliIptArgList[0][0],CliIptArgList[1][0],&id,tmp,&len);
	if(res != 0)
	{
		CLI_PRINTF("Read fail,err=%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("id=%d,len=%d\r\n",id,len);
		for(unsigned int i = 0;i < len;i++){
	    CLI_PRINTF("%d ",tmp[i]);
	    }
	    CLI_PRINTF("\r\n");
	}
}

void Command_LogMmt_ReadLatestItem(void)
{
	int res;
	int id;
	unsigned int len=40;
	res =  LogMmtReadLatestItem(CliIptArgList[0][0],&id,tmp,&len);
	if(res != 0)
	{
		CLI_PRINTF("Read fail,err=%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("id=%d,len=%d\r\n",id,len);
		for(unsigned int i = 0;i < len;i++){
	    CLI_PRINTF("%d ",tmp[i]);
	    }
	    CLI_PRINTF("\r\n");
	}
}


void Command_LogMmt_partition_info(void)
{
    U8* init;
    U32 *ws,*wi;
    log_mmt_t* pt;
    LogMmtInfo(&init,&wi,&ws,&pt);
    
    /*
    CLI_PRINTF("Logmmt partition 0 info:\r\n");
    CLI_PRINTF("init:%u,working sector:%d,working item:%d\r\n",init[0],ws[0],wi[0]);
    CLI_PRINTF("item_size:%d,sectors_per_partation:%d,addr_start:%d,partation_size:%d\r\n",\
    pt[0].item_size,pt[0].sectors_per_partation,pt[0].addr_start,pt[0].partation_size);

    CLI_PRINTF("Logmmt partition 1 info:\r\n");
    CLI_PRINTF("init:%u,working sector:%d,working item:%d\r\n",init[1],ws[1],wi[1]);
    CLI_PRINTF("item_size:%d,sectors_per_partation:%d,addr_start:%d,partation_size:%d\r\n",\
    pt[1].item_size,pt[1].sectors_per_partation,pt[1].addr_start,pt[1].partation_size);
    */

    CLI_PRINTF("partation addr      sectors  partation_size  item_size  init working_sector  working_item  \r\n");
    for(U8 i = 0;i < 2;i++)
    {
        CLI_PRINTF("%-10d0x%-8x%-9d%-16d%-11d%-5d%-16d%-14d\r\n",
        i,pt[i].addr_start,pt[i].sectors_per_partation,pt[i].partation_size,pt[i].item_size,init[i],ws[i],wi[i]);
    }
    
}

void Command_LogMmt_erase_partition(void)
{
    U32 err;
    
    err = LogMmtPartitionErase(CliIptArgList[0][0]);
    if(err != 0)
    {
        CLI_PRINTF("Erase error,err=%d\r\n",err);
    }
}

void Command_LogMmt_erase_sector(void)
{
    U32 err;
    
    err = LogMmtSectorErase(CliIptArgList[0][0],CliIptArgList[1][0]);
    if(err != 0)
    {
        CLI_PRINTF("Erase error,err=%d\r\n",err);
    }
}

void Command_LogMmt_readbitmtp(void)
{
   U32 err;
   char s[10];

    err = LogMmtReadBitMap(CliIptArgList[0][0],CliIptArgList[1][0],tmp);
    if(err != 0)
    {
        CLI_PRINTF("Read bitmap error,err=%d\r\n",err);
    }
    for(unsigned int i = 0;i < LOG_MMT_BITMAP_BYTES;i++){
        itoa(tmp[i], s, 2);
        CLI_PRINTF("%s ",s);
    }
    CLI_PRINTF("\r\n");
}

void Command_LogMmt_init(void)
{
    LogMmtInit();
}

void Command_LogMmt_GetLatestId(void)
{
    unsigned int id;

    id = LogMmtGetLatestId(CliIptArgList[0][0]);

    CLI_PRINTF("Latest save id:%d\r\n",id);
}

void Command_LogMmt_Write(void)
{
    unsigned int len = CliIptArgList[2][0];
    unsigned int part = CliIptArgList[0][0];
    int err;
    unsigned short saveid;
    
    if(len > 20)
    {
        CLI_PRINTF("Fail,Length of array over 20!\r\n");
        return;
    }
    
    for(unsigned int i = 0; i< len ;i++)
    {
        tmp[i] = CliIptArgList[1][i];
    }
    err =LogMmtWrite(part,tmp,len,&saveid);
    if(err != 0)
    {
        CLI_PRINTF("Write fail,err=%d\r\n",err);
    }
    else
    {
        CLI_PRINTF("Write success,saveid=%d\r\n",saveid);
    }
}

void Command_LogMmt_Read(void)
{
    int err;
    unsigned int len=40;

    err = LogMmtRead(CliIptArgList[0][0],CliIptArgList[1][0],tmp,&len);
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

