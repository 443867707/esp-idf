#ifndef __SVS_TEST_H__
#define __SVS_TEST_H__

typedef enum
{
    TEST_IDLE=0,   // 未测试(空闲中)
    TEST_FCT=1,    // FCT测试(Saved in flash)
    TEST_C1,       // 整机测试1(Saved in flash)
    TEST_AG,       // 老化测试(Saved in flash)
    TEST_C2,       // 整机测试2(Saved in flash)
    TEST_REMAIN0,  // Remain(Saved in flash)
    TEST_REMAIN1,  // Remain(Saved in flash)
    TEST_REMAIN2,  // Remain(Saved in flash)
    TEST_REMAIN3,  // Remain(Saved in flash)
/***********************/
    TEST_TYPE_MAX,
}EN_TEST_TYPE;

typedef enum
{
    TEST_RES_UNKNOW=0,
    TEST_RES_SUCCESS,
    TEST_RES_FAIL,
    TEST_RES_NO_RECORD,
}EN_TEST_RESULT;

extern void SvsTestInit(void);
extern int SvsTestInfoGet(EN_TEST_TYPE type,EN_TEST_RESULT* result);
extern int SvsTestInfoSet(EN_TEST_TYPE type,EN_TEST_RESULT result);
extern int SvsTestInfoMultiGet(EN_TEST_RESULT* res,int inLen); // 获取多个TestInfo
extern int SvsTestCurGet(EN_TEST_TYPE* type); // 获取当前测试类型
extern int SvsTestCurSet(EN_TEST_TYPE type);  // 设置当前测试类型



#endif
