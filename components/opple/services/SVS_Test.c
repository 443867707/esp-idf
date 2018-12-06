#include "Includes.h"
#include "OPP_Debug.h"
#include "SVS_Para.h"
#include "SVS_Test.h"

#define TEST_PARA_ID_START 0

static EN_TEST_RESULT testInfo[TEST_TYPE_MAX]={0};
static EN_TEST_TYPE testTypeCur=TEST_IDLE;

void SvsTestInit(void)
{
    int i,res;
    unsigned int result;

	testTypeCur=TEST_IDLE;
	testInfo[0] = 0;
    for(i = 1;i < TEST_TYPE_MAX;i++)
    {
    	res = AppParaReadU32(APS_PARA_MODULE_APS_TEST,i+TEST_PARA_ID_START,&result);
    	if(res==0)
    	{
    		testInfo[i] = result;
    	}
    	else
    	{
    		testInfo[i] = 0;
    	}
    }
}

void SvsTestLoop(void)
{
    
}

int SvsTestInfoGet(EN_TEST_TYPE type,EN_TEST_RESULT* result)
{
    if(type >= TEST_TYPE_MAX || type <= TEST_IDLE){ 
        return 1;
    }else{
        *result = testInfo[type];
        return 0;
    }
}

int SvsTestInfoSet(EN_TEST_TYPE type,EN_TEST_RESULT result)
{
    int res;

	if(type >= TEST_TYPE_MAX || type <= TEST_IDLE){ 
        return 1;
    }
    if(result > TEST_RES_FAIL){
		return 2;
    }
    
    res = AppParaWriteU32(APS_PARA_MODULE_APS_TEST,type+TEST_PARA_ID_START,result);
    if(res!=0)
    {
    	return 3;
    }
    else
    {
    	testInfo[type] = result;
    	return 0;
    }
}

int SvsTestInfoMultiGet(EN_TEST_RESULT* res,int inLen)
{
	if(inLen <=0 || inLen > TEST_TYPE_MAX-1) return 1;

	for(int i=0 ;i< inLen;i++)
	{
		res[i] = testInfo[i+1];
	}
	return 0;
}

int SvsTestCurGet(EN_TEST_TYPE* type)
{
	*type = testTypeCur;
	return 0;
}

int SvsTestCurSet(EN_TEST_TYPE type)
{
	if(type >= TEST_TYPE_MAX) return 1;

	testTypeCur = type;
	return 0;
}



