#ifndef __OPP_LOG_DB_H__
#define __OPP_LOG_DB_H__


#define LOG_DB_NUM_MAX 30000


extern int LogDbWrite(unsigned char* data,unsigned int len,unsigned int* saveid);
extern int LogDbQuery(unsigned int saveid,unsigned char* data,unsigned int* len);
extern int LogDbLatestIndex(void);
extern void LogDbInit(void);
extern void LogDbLoop(void);







#endif
