#include "cli-interpreter.h"
#include "APP_Daemon.h"

unsigned char cli_daemon_lrb=1;
unsigned char cli_daemon_lrs=2;
unsigned int cli_test1=0;
unsigned int cli_test2=0;


void Command_SetLogReportBusy(void);
void Command_SetLogReportStatus(void);
void Command_SetTest1(void);
void Command_SetTest2(void);

void Command_ItemSet(void);
void Command_ItemGet(void);
void Command_ItemAdd(void);
void Command_ItemDel(void);
void Command_ItemVal(void);

void Command_ItemSetCustom(void);
void Command_ItemGetCustom(void);
void Command_ItemAddCustom(void);
void Command_ItemDelCustom(void);
void Command_ItemValCustom(void);
void Command_ItemRestoreCustom(void);

void Command_logreport(void);
void Command_logsave(void);


void Command_LogSavaId(void);
void Command_LogQuery(void);
void Command_LogCtlSet(void);
void Command_LogCtlGet(void);
void Command_LogCfgSet(void);
void Command_LogCfgGet(void);
void Command_LogCfgRestore(void);


void Command_Loc_Set(void);
void Command_Loc_Get(void);


extern U32 ApsDaemonLogGen(APP_LOG_T * log);


/*******************************************************************************************/
/*******************************************************************************************/
const char* const CommandArguments_LRB[] = {
  "0:idle,1:busy",
};

const char* const CommandArguments_LRS[] = {
  "0:success,1:fail,2:sending",
};

/*******************************************************************************************/
/*******************************************************************************************/
const char* const CommandArguments_ItemSet[] = {
"index","{resource,targetid,enable,alarmId,alarmIf,\
alarmIfPara1,alarmIfPara2,reportIf,reportIfPara}",
};

const char* const CommandArguments_ItemGet[] = {
"index",
};

const char* const CommandArguments_ItemAdd[] = {
"{resource,targetid,enable,alarmId,alarmIf,\
alarmIfPara1,alarmIfPara2,reportIf,reportIfPara}",
};

const char* const CommandArguments_ItemSetCustom[] = {
"type","index","{resource,targetid,enable,alarmId,alarmIf,\
alarmIfPara1,alarmIfPara2,reportIf,reportIfPara}",
};

const char* const CommandArguments_ItemGetCustom[] = {
"type","index",
};

const char* const CommandArguments_ItemAddCustom[] = {
"type","{resource,targetid,enable,alarmId,alarmIf,\
alarmIfPara1,alarmIfPara2,reportIf,reportIfPara}",
};

const char* const CommandArguments_Logctl[] = {
"dir(0:disable,1:nb,2:WiFi)","period(ms)"
};

const char* const CommandArguments_Logcfg[] = {
"logid","isReportEn","isSaveEn",
};

const char* const CommandArguments_log[]={
"module","level","id","len","log",
};

const char* const CommandArguments_loc[]={
"lat<-90,90>","lng<-180~180>",
};

CommandEntry CommandTableDaemon[] =
{
	CommandEntryActionWithDetails("SetLBBusy", Command_SetLogReportBusy, "u", "SetLogReportBusy", CommandArguments_LRB),
	CommandEntryActionWithDetails("SetLRStatus", Command_SetLogReportStatus, "u", "SetLogReportStatus", CommandArguments_LRS),

    CommandEntryActionWithDetails("set1", Command_SetTest1, "u", "CommandSetTest1", (void*)0),
    CommandEntryActionWithDetails("set2", Command_SetTest2, "u", "CommandSetTest1", (void*)0),

    CommandEntryActionWithDetails("ItemSet", Command_ItemSet, "ua", "", CommandArguments_ItemSet),
    CommandEntryActionWithDetails("ItemGet", Command_ItemGet, "u", "", CommandArguments_ItemGet),
    CommandEntryActionWithDetails("ItemAdd", Command_ItemAdd, "a", "", CommandArguments_ItemAdd),
    CommandEntryActionWithDetails("ItemDel", Command_ItemDel, "u", "", CommandArguments_ItemGet),
    CommandEntryActionWithDetails("ItemVal", Command_ItemVal, "", "", (void*)0),

    CommandEntryActionWithDetails("ItemSetCustom", Command_ItemSetCustom, "uua", "", CommandArguments_ItemSetCustom),
    CommandEntryActionWithDetails("ItemGetCustom", Command_ItemGetCustom, "uu", "", CommandArguments_ItemGetCustom),
    CommandEntryActionWithDetails("ItemAddCustom", Command_ItemAddCustom, "ua", "", CommandArguments_ItemAddCustom),
    CommandEntryActionWithDetails("ItemDelCustom", Command_ItemDelCustom, "uu", "", CommandArguments_ItemGetCustom),
    CommandEntryActionWithDetails("ItemValCustom", Command_ItemValCustom, "u", "", CommandArguments_ItemGetCustom),
    CommandEntryActionWithDetails("ItemRestoreCustom", Command_ItemRestoreCustom, "u", "", CommandArguments_ItemGetCustom),

    
    CommandEntryActionWithDetails("logreport", Command_logreport, "uuuua", "CommandLog", CommandArguments_log),
    CommandEntryActionWithDetails("logsave", Command_logsave, "uuuua", "CommandLog", CommandArguments_log),
    CommandEntryActionWithDetails("logquery", Command_LogQuery, "u", "CommandLog", (void*)0),
    CommandEntryActionWithDetails("logsaveid", Command_LogSavaId, "", "CommandLog", (void*)0),
    CommandEntryActionWithDetails("logctlset", Command_LogCtlSet, "uu", "CommandLog", CommandArguments_Logctl),
    CommandEntryActionWithDetails("logctlget", Command_LogCtlGet, "", "CommandLog", (void*)0),
	CommandEntryActionWithDetails("logcfgset", Command_LogCfgSet, "uuu", "CommandLog", CommandArguments_Logcfg),
    CommandEntryActionWithDetails("logcfgget", Command_LogCfgGet, "u", "CommandLog", (void*)0),
	CommandEntryActionWithDetails("logcfgrestore", Command_LogCfgRestore, "", "CommandLog", (void*)0),

	CommandEntryActionWithDetails("locset", Command_Loc_Set, "uu", "set loction", CommandArguments_loc),
	CommandEntryActionWithDetails("locget", Command_Loc_Get, "", "get loction", (void*)0),

	CommandEntryTerminator()
};

/*******************************************************************************************/
/*******************************************************************************************/
void Command_ItemRestoreCustom(void)
{
	U32 type,res;
	
	type = CliIptArgList[0][0];
	res = ApsDaemonActionItemParaSetDefaultCustom(type);
	if(res != 0)
	{
		CLI_PRINTF("Item restore fail,err=%d\r\n",res);
	}

}

void Command_ItemSetCustom(void)
{
    AD_CONFIG_T cfg;
    U32 type,index,res;

	type = CliIptArgList[0][0];
    index =  CliIptArgList[1][0];
	cfg.resource = CliIptArgList[2][0];
	cfg.targetid = CliIptArgList[2][1];
	cfg.enable = CliIptArgList[2][2];
	cfg.alarmId = CliIptArgList[2][3];
	cfg.alarmIf = CliIptArgList[2][4];
	cfg.alarmIfPara1 = CliIptArgList[2][5];
	cfg.alarmIfPara2 = CliIptArgList[2][6];
	cfg.reportIf = CliIptArgList[2][7];
	cfg.reportIfPara = CliIptArgList[2][8];
	
	res = ApsDaemonActionItemParaSetCustom(type,index,&cfg);
	if(res != 0)
	{
	    CLI_PRINTF("Item set fail,err=%d\r\n",res);
	}
    
}
void Command_ItemGetCustom(void)
{
	AD_CONFIG_T cfg;
    U32 type,index,res;

	type = CliIptArgList[0][0];
	index = CliIptArgList[1][0];
    res = ApsDaemonActionItemParaGetCustom(type,index,&cfg);
    if(res != 0){CLI_PRINTF("Item get fail,err =%d\r\n",res);}
    else{
	CLI_PRINTF("resource:%d\r\ntargetid:%d\r\nenable:%d\r\nalarmId:%d\r\nalarmIf:%d\r\nalarmIfPara1:%d\r\nalarmIfPara2:%d\r\nreportIf:%d\r\nreportIfPara:%d\r\n",\
		cfg.resource,cfg.targetid,cfg.enable,cfg.alarmId,\
		           cfg.alarmIf,cfg.alarmIfPara1,cfg.alarmIfPara2,cfg.reportIf,cfg.reportIfPara);
    }
}
void Command_ItemAddCustom(void)
{
	AD_CONFIG_T cfg;
    U32 type,index,res;

	type = CliIptArgList[0][0];
	cfg.resource = CliIptArgList[1][0];
	cfg.targetid = CliIptArgList[1][1];
	cfg.enable = CliIptArgList[1][2];
	cfg.alarmId = CliIptArgList[1][3];
	cfg.alarmIf = CliIptArgList[1][4];
	cfg.alarmIfPara1 = CliIptArgList[1][5];
	cfg.alarmIfPara2 = CliIptArgList[1][6];
	cfg.reportIf = CliIptArgList[1][7];
	cfg.reportIfPara = CliIptArgList[1][8];
	
	res = ApsDaemonActionItemParaAddCustom(type,&index,&cfg);
	if(res != 0)
	{
	    CLI_PRINTF("Item set fail,err=%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("Item add ok,index=%d\r\n",index);
	}
}
void Command_ItemDelCustom(void)
{
	U32 type,index,res;
	type = CliIptArgList[0][0];
	index = CliIptArgList[1][0];
	res = ApsDaemonActionItemParaDelCustom(type,index);
	if(res != 0) CLI_PRINTF("Item del fail,err =%d\r\n",res);
}
void Command_ItemValCustom(void)
{
	const unsigned char* p;
	U32 type,len;

	type = CliIptArgList[0][0];
	ApsDaemonActionItemIndexValidityCustom(type,&p,&len);
	
	for(int i=0;i<len;i++)
	{
		CLI_PRINTF("id[%d]:%d\r\n",i,*(p+i));
	}
}

/*************************************************************/

void Command_LogCfgRestore(void)
{
	ApsDaemonLogConfigSetDefault();
}


void Command_Loc_Set(void)
{
  //g_stThisLampInfo.stLocation.ldLatitude = CliIptArgList[0][0];
  //g_stThisLampInfo.stLocation.ldLongitude = CliIptArgList[1][0];
  LocSetLat(CliIptArgList[0][0]);
  LocSetLng(CliIptArgList[1][0]);
  
}
void Command_Loc_Get(void)
{
	long double lng, lat;

	LocGetLng(&lng);
	LocGetLat(&lat);
	CLI_PRINTF("CUR:lat:%lf,lng:%lf\r\n",lat, lng);
	LocGetLastLng(&lng);
	LocGetLastLat(&lat);
	CLI_PRINTF("LAST:lat:%lf,lng:%lf\r\n",lat, lng);
}
void Command_SetLogReportBusy(void)
{
	cli_daemon_lrb = CliIptArgList[0][0];
}

void Command_SetLogReportStatus(void)
{
	cli_daemon_lrs = CliIptArgList[0][0];
}

void Command_SetTest1(void)
{
    cli_test1 = CliIptArgList[0][0];
}

void Command_SetTest2(void)
{
    cli_test2 = CliIptArgList[0][0];
}

unsigned int CliGetTest1(unsigned char targetid,unsigned int* res)
{
    *res = cli_test1;
    return 0;
}

unsigned int CliGetTest2(unsigned char targetid,unsigned int* res)
{
    *res = cli_test2;
    return 0;
}

/*******************************************************************************************/
/*******************************************************************************************/
void Command_ItemSet(void)
{
    AD_CONFIG_T cfg;
    U32 index,res;

    index =  CliIptArgList[0][0];
	cfg.resource = CliIptArgList[1][0];
	cfg.targetid = CliIptArgList[1][1];
	cfg.enable = CliIptArgList[1][2];
	cfg.alarmId = CliIptArgList[1][3];
	cfg.alarmIf = CliIptArgList[1][4];
	cfg.alarmIfPara1 = CliIptArgList[1][5];
	cfg.alarmIfPara2 = CliIptArgList[1][6];
	cfg.reportIf = CliIptArgList[1][7];
	cfg.reportIfPara = CliIptArgList[1][8];
	
	res = ApsDaemonActionItemParaSet(index,&cfg);
	if(res != 0)
	{
	    CLI_PRINTF("Item set fail,err=%d\r\n",res);
	}
    
}
void Command_ItemGet(void)
{
	AD_CONFIG_T cfg;
    U32 index,res;

	index = CliIptArgList[0][0];
    res = ApsDaemonActionItemParaGet(index,&cfg);
    if(res != 0){CLI_PRINTF("Item get fail,err =%d\r\n",res);}
    else{
	CLI_PRINTF("resource:%d\r\ntargetid:%d\r\nenable:%d\r\nalarmId:%d\r\nalarmIf:%d\r\nalarmIfPara1:%d\r\nalarmIfPara2:%d\r\nreportIf:%d\r\nreportIfPara:%d\r\n",\
		cfg.resource,cfg.targetid,cfg.enable,cfg.alarmId,\
		           cfg.alarmIf,cfg.alarmIfPara1,cfg.alarmIfPara2,cfg.reportIf,cfg.reportIfPara);
    }
}
void Command_ItemAdd(void)
{
	AD_CONFIG_T cfg;
    U32 index,res;

	cfg.resource = CliIptArgList[0][0];
	cfg.targetid = CliIptArgList[0][1];
	cfg.enable = CliIptArgList[0][2];
	cfg.alarmId = CliIptArgList[0][3];
	cfg.alarmIf = CliIptArgList[0][4];
	cfg.alarmIfPara1 = CliIptArgList[0][5];
	cfg.alarmIfPara2 = CliIptArgList[0][6];
	cfg.reportIf = CliIptArgList[0][7];
	cfg.reportIfPara = CliIptArgList[0][8];
	
	res = ApsDaemonActionItemParaAdd(&index,&cfg);
	if(res != 0)
	{
	    CLI_PRINTF("Item set fail,err=%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("Item add ok,index=%d\r\n",index);
	}
}
void Command_ItemDel(void)
{
	U32 index,res;
	index = CliIptArgList[0][0];
	res = ApsDaemonActionItemParaDel(index);
	if(res != 0) CLI_PRINTF("Item del fail,err =%d\r\n",res);
}
void Command_ItemVal(void)
{
	const unsigned char* p;
	U32 len;

	ApsDaemonActionItemIndexValidity(&p,&len);
	
	for(int i=0;i<len;i++)
	{
		CLI_PRINTF("id[%d]:%d\r\n",i,*(p+i));
	}
}

/*******************************************************************************************/
/*******************************************************************************************/
void Command_logreport(void)
{
  APP_LOG_T log;

  log.module = CliIptArgList[0][0];
  log.level = CliIptArgList[1][0];
  log.id = CliIptArgList[2][0];
  log.len = CliIptArgList[3][0];
  for(int i = 0;i < 12;i++){
  	log.log[i] = CliIptArgList[4][i];
  }
  
  ApsDaemonLogReport(&log);
}

void Command_logsave(void)
{
  APP_LOG_T log;
  U32 saveid;

  log.module = CliIptArgList[0][0];
  log.level = CliIptArgList[1][0];
  log.id = CliIptArgList[2][0];
  log.len = CliIptArgList[3][0];
  for(int i = 0;i < 12;i++){
  	log.log[i] = CliIptArgList[4][i];
  }
  
  if(ApsDaemonLogGen(&log)==0)
  {
  	if(ApsLogWrite(&log, &saveid) == 0)
  	{
  		CLI_PRINTF("Log save success,saveid=%d!\r\n",saveid);
  	}
  	else
  	{
  		CLI_PRINTF("Log save fail!\r\n");
  	}
  }
  else
  {
  	CLI_PRINTF("Log gen fail!\r\n");
  }
}


void Command_LogSavaId(void)
{
	S32 id;

	id =ApsDamemonLogCurrentSaveIndexGet();
	CLI_PRINTF("Current save id:%d\r\n",id);
}
void Command_LogQuery(void)
{
	APP_LOG_T log;
	U32 id,res;

	id = CliIptArgList[0][0];
	res = ApsDaemonLogQuery(id,&log);
	if(res != 0){CLI_PRINTF("Query fail,err=%d\r\n",res);}
	else{
		CLI_PRINTF("seqnum:%d\r\nmodule:%d\r\nlevel:%d\r\n",\
		log.seqnum,log.module,log.level);
		CLI_PRINTF("time:%d,%d,%d,%d,%d,%d\r\n",\
		log.time[0],log.time[1],log.time[2],
		log.time[3],log.time[4],log.time[5]);
		CLI_PRINTF("id:%d\r\nlen:%d\r\n",log.id,log.len);
		CLI_PRINTF("log:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
		log.log[0],log.log[1],log.log[2],
		log.log[3],log.log[4],log.log[5],
		log.log[6],log.log[7],log.log[8],
		log.log[9],log.log[10],log.log[11]);
	}
}
void Command_LogCtlSet(void)
{
	LOG_CONTROL_T ctl;
	
	ctl.dir = CliIptArgList[0][0];
	ctl.period = CliIptArgList[1][0];
	if(ApsDaemonLogControlParaSet(&ctl) != 0)
	{
	    CLI_PRINTF("Log ctl set fail,err=%d\r\n");
	}
}
void Command_LogCtlGet(void)
{
	LOG_CONTROL_T ctl;
	
	ApsDaemonLogControlParaGet(&ctl);
	CLI_PRINTF("Period:%d,Dir:%d\r\n",ctl.period,ctl.dir);
}
void Command_LogCfgSet(void)
{
	LOG_CONFIG_T cfg;
	U32 res;

	cfg.logid = CliIptArgList[0][0];
	cfg.isLogReport = CliIptArgList[1][0];
	cfg.isLogSave = CliIptArgList[2][0];
	res = ApsDaemonLogIdParaSet(&cfg);
	if(res != 0)
	{
		CLI_PRINTF("Log config set fail,err=%d\r\n",res);
	}
}
void Command_LogCfgGet(void)
{
	LOG_CONFIG_T cfg;
	U32 res;

	cfg.logid= CliIptArgList[0][0];
	res = ApsDaemonLogIdParaGet(&cfg);
	if(res != 0) CLI_PRINTF("Log config get fail\r\n");
	else CLI_PRINTF("logid:%d,isLogReport:%d,isLogSave:%d\r\n",cfg.logid,cfg.isLogReport,cfg.isLogSave);
}
