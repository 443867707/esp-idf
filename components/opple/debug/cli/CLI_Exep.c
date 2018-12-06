#include "cli-interpreter.h"
#include "Includes.h"
#include "Opp_ErrorCode.h"
#include "SVS_Exep.h"
#include "stdlib.h"
#include "string.h"

void CommandExepNormalRstGet(void);
void CommandExepNormalRstSet(void);
void CommandExepGet(void);
void CommandExepSet(void);
void CommandExepErase(void);
void CommandMakeErr(void);

extern const char* const rstDesc[];
extern const char* const exepDesc[];
char *normalRstDesc[] = {
	"NORMAL_REBOOT",
	"ABNORMAL_REBOOT"
};

CommandEntry CommandTableExep[] =
{
	CommandEntryActionWithDetails("normalRstGet", CommandExepNormalRstGet, "", "normal reboot set...", NULL),
	CommandEntryActionWithDetails("normalRstSet", CommandExepNormalRstSet, "u", "normal reboo get...", NULL),
	CommandEntryActionWithDetails("exepGet", CommandExepGet, "u", "get exep...", exepDesc),
	CommandEntryActionWithDetails("exepSet", CommandExepSet, "u", "set exep...", rstDesc),
	CommandEntryActionWithDetails("exepErase", CommandExepErase, "", "erase exep...", NULL),
	CommandEntryActionWithDetails("makeErr", CommandMakeErr, "", "make error...", NULL),
	CommandEntryTerminator()
};

void CommandExepNormalRstGet(void)
{
	int ret;
	U32 normalRst;
	int rst;
		
	ret = ApsExepReadNormalReboot(&normalRst);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("ApsExepReadNormalReboot fail ret %d\r\n", ret);
		return;
	}
	if(NORMAL_REBOOT == normalRst)
		rst = 0;
	else
		rst = 1;
	//CLI_PRINTF("normalRst:%x tmp:%s\r\n", normalRst, tmp==0?"NORMAL_REBOOT":"ABNORMAL_REBOOT");
	CLI_PRINTF("normalRst:%x rst:%s\r\n", normalRst, normalRstDesc[rst]);	
}


void CommandExepNormalRstSet(void)
{
	int ret;
	U32 normalRst;
	
	if(0 == CliIptArgList[0][0]){
		normalRst = NORMAL_REBOOT;
	}else{
		normalRst =ABNORMAL_REBOOT;
	}
	ret = ApsExepWriteNormalReboot(normalRst);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("ApsExepWriteNormalReboot fail ret %x\r\n", ret);
	}
}

void CommandExepErase(void)
{
	ST_EXEP_INFO stExepInfo;

	memset(&stExepInfo, 0, sizeof(ST_EXEP_INFO));
	ApsExepWriteCurExep(&stExepInfo);
	ApsExepWritePreExep(&stExepInfo);
}

void CommandMakeErr(void)
{
	char * ptr= NULL;
	memset(ptr,0,100);
}

