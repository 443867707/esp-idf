#include "Includes.h"
#include "OPP_Debug.h"
#include "SVS_Para.h"
#include "SVS_Test.h"

#define TEST_PARA_ID_START 0  ///< Test参数模块起始ID

static EN_TEST_RESULT testInfo[TEST_TYPE_MAX]={0}; ///< 各站测试记录(index=0无效)
static EN_TEST_TYPE testTypeCur=TEST_IDLE; ///< 当前测试站

/**
	@brief 模块初始化
	@attention none
	@param none
	@retval none
*/
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
    		testInfo[i] = TEST_RES_NO_RECORD;
    	}
    }
}

/**
	@brief 模块loop接口
	@attention none
	@param none
	@retval none
*/
void SvsTestLoop(void)
{
    
}

/**
	@brief 获取测试站点记录结果
	@attention none
	@param type 站点，@see EN_TEST_TYPE
	@param result[out] 测试结果记录，@see EN_TEST_RESULT
	@retval 0:成功，1:失败
*/
int SvsTestInfoGet(EN_TEST_TYPE type,EN_TEST_RESULT* result)
{
    if(type >= TEST_TYPE_MAX || type <= TEST_IDLE){ 
        return 1;
    }else{
        *result = testInfo[type];
        return 0;
    }
}

/**
	@brief 设置测试站点记录结果
	@attention none
	@param type 站点，@see EN_TEST_TYPE
	@param result[in] 测试结果记录，@see EN_TEST_RESULT
	@retval 0:成功，else:失败
*/
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

/**
	@brief 获取所有测试站点记录结果
	@attention none
	@param res[out] 测试结果记录，@see EN_TEST_RESULT
	@param inLen[in]，接收测试结果的长度
	@retval 0:成功，else:失败
*/
int SvsTestInfoMultiGet(EN_TEST_RESULT* res,int inLen)
{
	if(inLen <=0 || inLen > TEST_TYPE_MAX-1) return 1;

	for(int i=0 ;i< inLen;i++)
	{
		res[i] = testInfo[i+1];
	}
	return 0;
}

/**
	@brief 获取当前测试站点
	@attention none
	@param type[out] 站点，@see EN_TEST_TYPE
	@retval 0:成功，else:失败
*/
int SvsTestCurGet(EN_TEST_TYPE* type)
{
	*type = testTypeCur;
	return 0;
}

/**
	@brief 设置当前测试站点
	@attention none
	@param type[in] 站点，@see EN_TEST_TYPE
	@retval 0:成功，else:失败
*/
int SvsTestCurSet(EN_TEST_TYPE type)
{
	if(type >= TEST_TYPE_MAX) return 1;

	testTypeCur = type;
	return 0;
}



