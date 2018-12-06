#include "cli-interpreter.h"
#include "SVS_Log.h"
#include "SVS_Coap.h"
#include "SVS_MsgMmt.h"
#include "string.h"

void CommandPropReport(void);
void CommandPropAlarm(void);

const char* const report[] = {
	"propId","valude"
};

const char* const alarm[] = {
	"alarmId","type","valude"
};


CommandEntry CommandTableProp[] =
{
	CommandEntryActionWithDetails("report", CommandPropReport, "uu", "g_pstReProp report...", report),
	CommandEntryActionWithDetails("alartm", CommandPropAlarm, "uu", "g_pstReProp alarm...", alarm),

	CommandEntryTerminator()
};

void CommandPropReport(void)
{
	ST_REPORT_PARA para;
	
	CLI_PRINTF("propId %d  value %d\r\n", CliIptArgList[0][0],CliIptArgList[1][0]);

	para.chl = CHL_NB;
	para.resId = CliIptArgList[0][0];
	para.value = CliIptArgList[1][0];
	ApsCoapStateReport(&para);
}

void CommandPropAlarm(void)
{
	ST_ALARM_PARA para;

	CLI_PRINTF("alarmid %d  status %d value %d\r\n", CliIptArgList[0][0],CliIptArgList[1][0], CliIptArgList[2][0]);

	para.dstChl = CHL_NB;
	para.alarmId = CliIptArgList[0][0];
	para.status = CliIptArgList[1][0];
	para.value = CliIptArgList[2][0];	
	memset(para.dstInfo,0,sizeof(para.dstInfo));
	ApsCoapAlarmReport(&para);

}

