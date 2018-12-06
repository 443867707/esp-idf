#include "cli-interpreter.h"
#include "SVS_Log.h"

void CommandApslogRead(void);
void CommandApslogWrite(void);
void CommandApslogSaveIdLatest(void);

void CommandApslogRidLatest(void);
void CommandApslogRidLast(void);
void CommandApslogRRead(void);
void CommandApslogRWrite(void);



const char* const argApsLogRead[] = {
"Module","Id","Data array","Len"
};

//	typedef struct
//	{
//		unsigned int  seqnum;
//	    unsigned char module;
//	    unsigned char level;
//	    unsigned char time[6]; // Y-M-D H-M-S(Year=Y+2000)
//	    unsigned char id;  // log id
//	    unsigned char len; // length of log(except module and level)
//	    unsigned char log[APP_LOG_SIZE_MAX];
//	}APP_LOG_T;

const char* const argApsLogWrite[] = {
"seqnum",
"Module",
"Level",
"time",
"Logid",
"Len",
"Log",
};

const char* const argApsRIDLast[] = {
"id_now",
};
const char* const argApsRRead[] = {
"id",
};
const char* const argApsRWrite[] = {
"data",
"len",
};



CommandEntry CommandTableApsLog[] =
{
	CommandEntryActionWithDetails("Read", CommandApslogRead, "u", "Read log", argApsLogRead),
    //CommandEntryActionWithDetails("ReadFinish", CommandApslogReadFinish, "u", "Read finish", (void*)0),
	CommandEntryActionWithDetails("Write", CommandApslogWrite, "uuuauua", "Write log", argApsLogWrite),
	CommandEntryActionWithDetails("SaveIdLatest", CommandApslogSaveIdLatest, "", "Latese save id", argApsLogWrite),
	CommandEntryActionWithDetails("RecordIdLatest", CommandApslogRidLatest, "", "", argApsLogWrite),
	CommandEntryActionWithDetails("RecordIdLast", CommandApslogRidLast, "u", "", argApsRIDLast),
	CommandEntryActionWithDetails("RecordRead", CommandApslogRRead, "u", "", argApsRRead),
	CommandEntryActionWithDetails("RecordWrite", CommandApslogRWrite, "au", "", argApsRWrite),

	CommandEntryTerminator()
};

void CommandApslogRidLatest(void)
{
	int id = OppApsRecordIdLatest();
	CLI_PRINTF("%d\r\n",id);
}
void CommandApslogRidLast(void)
{
	int id = OppApsRecordIdLast(CliIptArgList[0][0]);
	CLI_PRINTF("%d\r\n",id);
}
void CommandApslogRRead(void)
{
	int res ;
	unsigned char buf[16];
	unsigned int len=8;
	
	res = OppApsRecoderRead(CliIptArgList[0][0],buf,&len);
	if(res!=0)
	{
		CLI_PRINTF("Fail,res=%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("U32_0:0x%08X,U32_1:0x%08X\r\n",*(unsigned int*)(&buf[0]),*(unsigned int*)(&buf[4]));
	}
}
void CommandApslogRWrite(void)
{
	int res;
	unsigned char buf[16];
	unsigned int len=8;
	unsigned int id;

	len = CliIptArgList[1][0];
	if(len > 14) len = 14;
	for(int i=0;i<len;i++)
	{
		buf[i] = CliIptArgList[0][i];
	}
	
	res = OppApsRecoderWrite(buf,len,&id);
	if(res!=0) CLI_PRINTF("FAIL,res=%d\r\n",res);
	else CLI_PRINTF("Success,id=%d\r\n",id);
}


void CommandApslogSaveIdLatest(void)
{
	int id = ApsLogSaveIdLatest();
    CLI_PRINTF("Latest save id :%d\r\n",id);
}

void CommandApslogRead(void)
{
    int err;
    APP_LOG_T log;
    
    err = ApsLogRead(CliIptArgList[0][0],&log);
    if(err != 0){
        CLI_PRINTF("Read fail,err=%d\r\n",err);
        return;
    }
    CLI_PRINTF("seq:%d,module:%d,level:%d,time:",log.seqnum,log.module,log.level);
	//CLI_PRINTF("time:");
    for(unsigned int i=0;i < 6;i++)
    {
        CLI_PRINTF("%d ",log.time[i]);
    }
    CLI_PRINTF(",id:%d,len:%d,log:",log.id,log.len);
    for(unsigned int j=0;j < log.len;j++)
    {
        CLI_PRINTF("%d ",log.log[j]);
    }
    CLI_PRINTF("\r\n");
}

void CommandApslogWrite(void)
{
    int err;
    APP_LOG_T log;
	unsigned int id;
    unsigned int len=CliIptArgList[5][0];

    if(len > 20)
    {
        CLI_PRINTF("Fail,len over 20!\r\n");
        return;
    }
    log.seqnum = CliIptArgList[0][0];
    log.module = CliIptArgList[1][0];
    log.level = CliIptArgList[2][0];
    for(unsigned int i =0;i<6;i++)
    {
    	log.time[i] = CliIptArgList[3][i];
    }
    log.id = CliIptArgList[4][0];
    log.len = CliIptArgList[5][0];
    for(unsigned int i=0;i < len;i++)
    {
        log.log[i] = CliIptArgList[6][i];
    }
    err = ApsLogWrite(&log,&id);
    if(err != 0)
    {
        CLI_PRINTF("Write fail,err=%d\r\n",err);
    }
	else
	{
	    CLI_PRINTF("OK,saveid=%d\r\n",id);
	}
}
