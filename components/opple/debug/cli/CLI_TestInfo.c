#include "cli-interpreter.h"
#include <string.h>
#include "SVS_Test.h"

const char* const arg_testinfo_set[]={
	"type(0:cur,1:fct,2:c1,3:age,4:c4,5-8:remain)",
    "info(0:unkown,1:success,2:fail)",
};

const char* const arg_testinfo_multi_get[]={
	"len(1-8)"
};


static void Command_set(void);
static void Command_get(void);
static void Command_info(void);
static void Command_multi_get(void);
static void Command_setCur(void);
static void Command_getCur(void);


CommandEntry CommandTablTestInfo[] =
{
	CommandEntryActionWithDetails("set", Command_set, "uu", "", arg_testinfo_set),
    CommandEntryActionWithDetails("get", Command_get, "u", "", arg_testinfo_set),
    CommandEntryActionWithDetails("multiGet", Command_multi_get, "u", "",arg_testinfo_multi_get),
    CommandEntryActionWithDetails("setCur", Command_setCur, "u", "", arg_testinfo_set),
    CommandEntryActionWithDetails("getCur", Command_getCur, "", "", arg_testinfo_set),
    CommandEntryActionWithDetails("info", Command_info, "", "", (void*)0),
	
	CommandEntryTerminator()
};

static void Command_set(void)
{
    int res;

    res = SvsTestInfoSet(CliIptArgList[0][0],CliIptArgList[1][0]);
    if(res!=0)
    {
    	CLI_PRINTF("Fail,res=%d\r\n");
    }
}

static void Command_get(void)
{
    unsigned int result;
    int res;

    res = SvsTestInfoGet(CliIptArgList[0][0],&result);
    if(res!=0)
    {
    	CLI_PRINTF("Fail,res=0x%x\r\n");
    }
    else
    {
    	CLI_PRINTF("%u\r\n",result);
    }
}

static void Command_multi_get(void)
{
	EN_TEST_RESULT result[TEST_TYPE_MAX];
	int res,i;

	res = SvsTestInfoMultiGet(result,CliIptArgList[0][0]);
	if(res!=0)
	{
		CLI_PRINTF("Fail,err=%d\r\n",res);
	}
	else
	{
		for(i = 0;i < CliIptArgList[0][0];i++)
		{
			CLI_PRINTF("type %u : %u\r\n",i,result[i]);
		}
	}
}

static void Command_info(void)
{
    EN_TEST_RESULT result;
    EN_TEST_TYPE type;
    int i,res;

	res = SvsTestCurGet(&type);
	CLI_PRINTF("Current test:%d\r\n",type);
	
    for(i = TEST_FCT;i < TEST_TYPE_MAX;i++)
    {
    	res = SvsTestInfoGet(i,&result);
    	if(res!=0)
    	{
    		CLI_PRINTF("type %d : fail,err=0x%x\r\n",res);
    	}
    	else
    	{
    		CLI_PRINTF("type %u : %u\r\n",i,result);
    	}
    }
}

static void Command_setCur(void)
{
	int res;

	res = SvsTestCurSet(CliIptArgList[0][0]);
	if(res!=0)
	{
		CLI_PRINTF("Fail,err=%d\r\n",res);
	}
}

static void Command_getCur(void)
{
	EN_TEST_TYPE type;

	SvsTestCurGet(&type);
	CLI_PRINTF("Current test:%d\r\n",type);
}


