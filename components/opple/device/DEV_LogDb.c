/*****************************************************************
                              How it works
******************************************************************/
/*
Key-Value模块中，M0存储为Log内容，其中M0N0存储Log的index，M0N(10-N)
存储Log内容，index记录压入的Log数量，每次Log压入，index增加，读出时
index减少。

*/
/*****************************************************************
                              Includes
******************************************************************/
#include "Includes.h"
#include "DEV_LogDb.h"
#include "DEV_ItemMmt.h"


/*****************************************************************
                              Defines
******************************************************************/
#define LOG_DB_MODULE 0
#define LOG_DB_ID_INDEX 0
#define LOG_DB_ID_INDEX_SYNC 1
#define LOG_DB_ID_CONTEXT_START 10




// 对ID进行管理
// ID=0   ：index
// ID=1-N : log
static struct
{
    unsigned int init;
	unsigned int indexLatest;
    unsigned int index;
    int workingState;
}sLogDb;

int LogDbWriteIndex(unsigned int index)
{
    return DataItemWriteU32(LOG_DB_MODULE,LOG_DB_ID_INDEX,index);
}

int LogDbWriteIndexSync(unsigned int index)
{
    return DataItemWriteU32(LOG_DB_MODULE,LOG_DB_ID_INDEX_SYNC,index);
}

int LogDbWrite(unsigned char* data,unsigned int len,unsigned int* saveid)
{
    int index;
    if(sLogDb.init == 0) return 1;
    if(DataItemWrite(LOG_DB_MODULE,sLogDb.index+LOG_DB_ID_CONTEXT_START,data,len) != 0) return 2;

	*saveid = sLogDb.index;
	sLogDb.indexLatest = sLogDb.index;
    index = sLogDb.index;
    index++;
    if(index >= LOG_DB_NUM_MAX)
    {
        index = 0;
    }
    
    if(LogDbWriteIndex(index) == 0)
    {
        sLogDb.index = index;
        sLogDb.workingState = 1;
		return 0;
    }
    else
    {
        return 3;
    }
}

int LogDbQuery(unsigned int saveid,unsigned char* data,unsigned int* len)
{
    if(DataItemRead(LOG_DB_MODULE,saveid+LOG_DB_ID_CONTEXT_START,data,len)!= 0) return 1;
	else return 0;
}

int LogDbLatestIndex(void)
{
	if(sLogDb.workingState == 0) return -1;	
    else return sLogDb.indexLatest;
}

void LogDbInit(void)
{
    sLogDb.init = 0;
}

void LogDbLoop(void)
{
    if(sLogDb.init == 0)
    {
        sLogDb.init = 1;
        if(DataItemReadU32(LOG_DB_MODULE,LOG_DB_ID_INDEX,&sLogDb.index) != 0)
        {
            sLogDb.index = 0;
            sLogDb.workingState = 0;
        }
        else
        {
        	sLogDb.workingState = 1;;
        }
        
        //if(DataItemReadU32(LOG_DB_MODULE,LOG_DB_ID_INDEX_SYNC,&sLogDb.index_sync) != 0)
        //{
        //    sLogDb.index_sync = 0;
        //}
    }
}



