/*****************************************************************
                              How it works
******************************************************************/




/*****************************************************************
                              Includes
******************************************************************/
#include "Includes.h"
#include "SVS_Para.h"
#include "DEV_ItemMmt.h"
#include "OPP_Debug.h"
/*****************************************************************
                              Defines
******************************************************************/



/*****************************************************************
                              Typedefs
******************************************************************/



/*****************************************************************
                              Data
******************************************************************/
T_MUTEX mutex_para;



/*****************************************************************
                              Funs
******************************************************************/




int AppParaRead(int module,int num,unsigned char* data,unsigned int* lenMax)
{
	int res;
	MUTEX_LOCK(mutex_para, MUTEX_WAIT_ALWAYS);
    res = DataItemRead(module,num,data,lenMax);
    MUTEX_UNLOCK(mutex_para);
    if(res != 0)
    DEBUG_LOG(DEBUG_MODULE_PARA, DLL_ERROR, "DataItemRead module %d num %d ret %x\r\n", module, num, res);
    return res;
}

int AppParaWrite(int module,int num,unsigned char* data,unsigned int len)
{
	int res;
	MUTEX_LOCK(mutex_para, MUTEX_WAIT_ALWAYS);
    res =  DataItemWrite(module,num,data,len);
    MUTEX_UNLOCK(mutex_para);
    if(res != 0)
    DEBUG_LOG(DEBUG_MODULE_PARA, DLL_ERROR, "DataItemWrite module %d num %d ret %x\r\n", module, num, res);	
    return res;
}

int AppParaReadU32(int module,int num,unsigned int* data)
{
	int res;
	MUTEX_LOCK(mutex_para, MUTEX_WAIT_ALWAYS);
    res =  DataItemReadU32(module,num,data);
    if(res != 0)
	DEBUG_LOG(DEBUG_MODULE_PARA, DLL_ERROR, "DataItemReadU32 module %d num %d ret %x\r\n", module, num, res);	
    MUTEX_UNLOCK(mutex_para);
    return res;
}
int AppParaWriteU32(int module,int num,unsigned int data)
{
	int res;
	MUTEX_LOCK(mutex_para, MUTEX_WAIT_ALWAYS);
    res =  DataItemWriteU32(module,num,data);	
    MUTEX_UNLOCK(mutex_para);
    if(res != 0)
    DEBUG_LOG(DEBUG_MODULE_PARA, DLL_ERROR, "DataItemWriteU32 module %d num %d ret %x\r\n", module, num, res);	
    return res;
}
int AppParaReadU16(int module,int num,unsigned short* data)
{
	int res;
	MUTEX_LOCK(mutex_para, MUTEX_WAIT_ALWAYS);
	res =  DataItemReadU16(module,num,data);	
	MUTEX_UNLOCK(mutex_para);
	if(res != 0)
	DEBUG_LOG(DEBUG_MODULE_PARA, DLL_ERROR, "DataItemReadU16 module %d num %d ret %x\r\n", module, num, res);
    return res;
}
int AppParaWriteU16(int module,int num,unsigned short data)
{
	int res;
	MUTEX_LOCK(mutex_para, MUTEX_WAIT_ALWAYS);
	res =  DataItemWriteU16(module,num,data);
	MUTEX_UNLOCK(mutex_para);
	if(res != 0)
	DEBUG_LOG(DEBUG_MODULE_PARA, DLL_ERROR, "DataItemWriteU16 module %d num %d ret %x\r\n", module, num, res);				
    return res;
}
int AppParaReadU8(int module,int num,unsigned char* data)
{
	int res;
	MUTEX_LOCK(mutex_para, MUTEX_WAIT_ALWAYS);
	res =  DataItemReadU8(module,num,data);
	MUTEX_UNLOCK(mutex_para);
	if(res != 0)
	DEBUG_LOG(DEBUG_MODULE_PARA, DLL_ERROR, "DataItemReadU8 module %d num %d ret %x\r\n", module, num, res);				
    return res;
}
int AppParaWriteU8(int module,int num,unsigned char data)
{
	int res;
	MUTEX_LOCK(mutex_para, MUTEX_WAIT_ALWAYS);
	res =  DataItemWriteU8(module,num,data);
	MUTEX_UNLOCK(mutex_para);
	if(res != 0)
	DEBUG_LOG(DEBUG_MODULE_PARA, DLL_ERROR, "DataItemWriteU8 module %d num %d ret %x\r\n", module, num, res);					
    return res;
}

void AppParaInit(void)
{
    MUTEX_CREATE(mutex_para);
}

void AppParaLoop(void)
{

}
