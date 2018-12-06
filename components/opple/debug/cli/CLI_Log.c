#include "cli-interpreter.h"
#include "SVS_Log.h"
#include "SVS_Coap.h"
#include "SVS_MsgMmt.h"
#include "string.h"

void CommandLogReport(void);
void CommandLogBatchAppend(void);
void CommandLogBatchStop(void);



const char* const CommandTableLogCommandArguments[] = {
  "Time {Y,M,D,H,m,S} Year=Y+2000",
  "Length of log",
  "Log {x,x,x}",
};


CommandEntry CommandTableLog[] =
{
	CommandEntryActionWithDetails("report", CommandLogReport, "aua", "log report...", CommandTableLogCommandArguments),
	CommandEntryActionWithDetails("aReport", CommandLogBatchAppend, "aua", "log batch append...", CommandTableLogCommandArguments),
	CommandEntryActionWithDetails("sReport", CommandLogBatchStop, "", "", NULL),
	CommandEntryTerminator()
};

void CommandLogReport(void)
{
	ST_LOG_PARA stLogPara;

	APP_LOG_T log;
	int i;

	stLogPara.chl = CHL_NB;
	stLogPara.leftItems = 0;
	stLogPara.logSaveId = 0;
	//stLogPara.logSeq = 0;
	stLogPara.reqId = 1;
	stLogPara.type = 0;
	stLogPara.log.module = 1;
	stLogPara.log.level = LOG_LEVEL_INFO;
	memset(stLogPara.dstInfo,0,sizeof(stLogPara.dstInfo));
	//stLogPara.log.status = 1;
	stLogPara.log.time[0]=CliIptArgList[0][0];
	stLogPara.log.time[1]=CliIptArgList[0][1];
	stLogPara.log.time[2]=CliIptArgList[0][2];
	stLogPara.log.time[3]=CliIptArgList[0][3];
	stLogPara.log.time[4]=CliIptArgList[0][4];
	stLogPara.log.time[5]=CliIptArgList[0][5];

	log.len = CliIptArgList[1][0];
	if(log.len > APP_LOG_SIZE_MAX){
		CLI_PRINTF("input log length %d greate than %d\r\n", log.len, APP_LOG_SIZE_MAX);
		return;
	}
	for(i=0;i<log.len;i++){
		log.log[i] = CliIptArgList[2][i];
	}
	ApsCoapLogReport(&stLogPara);
}

void CommandLogBatchAppend(void)
{
	//APP_LOG_T log;
	ST_LOG_APPEND_PARA para;
	int i;
	int ret;
	
	para.dstChl = CHL_NB;
	para.logSaveId = 1;
	//para.logSeq = 1;
	para.log.module = 1;
	para.log.level = 1;
	//para.log.status = 1;
	para.log.time[0]=CliIptArgList[0][0];
	para.log.time[1]=CliIptArgList[0][1];
	para.log.time[2]=CliIptArgList[0][2];
	para.log.time[3]=CliIptArgList[0][3];
	para.log.time[4]=CliIptArgList[0][4];
	para.log.time[5]=CliIptArgList[0][5];
	para.log.len = CliIptArgList[1][0];
	if(para.log.len > APP_LOG_SIZE_MAX){
		CLI_PRINTF("input log length %d greate than %d\r\n", para.log.len, APP_LOG_SIZE_MAX);
		return;
	}
	for(i=0;i<para.log.len;i++){
		para.log.log[i] = CliIptArgList[2][i];
	}
	ret = ApsCoapLogBatchAppend(&para);
	CLI_PRINTF("ApsCoapLogBatchAppend ret %d\r\n",ret);
}

void CommandLogBatchStop(void)
{
	ST_LOG_STOP_PARA para;
	para.dstChl = CHL_NB;
	para.leftItems = 0;
	para.type = ACTIVE_REPROT;
	para.reqId = 0;
	ApsCoapLogBatchStop(&para);
}


