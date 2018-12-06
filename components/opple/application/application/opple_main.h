#ifndef __OPPLE_MAIN_H__
#define __OPPLE_MAIN_H__

#define SYS_MODE_CNT_MAX 15   // 15
#define SYS_MODE_NORMAL_RUN_TIME (10*60*1000) //(10*60*1000)

#define SYS_MODE_NORMAL 0
#define SYS_MODE_SAFE   1


typedef struct
{
	unsigned int period; /* 看门狗周期（S）（>=300S） */
	//unsigned char wdtEn; /* 看门狗是否生效 */
	unsigned char resetEn; /* 看门狗超时后是否复位 */
}TaskWdtConfig_EN;

extern int taskWdtConfigSet(TaskWdtConfig_EN* config);
extern int taskWdtConfigGet(TaskWdtConfig_EN* config);
extern int taskWdtReset(void);

extern int sysModeSet(unsigned short mode,unsigned short  cnt);
extern int sysModeGet(unsigned short  *mode,unsigned short * cnt);
extern int sysModeDecreaseSet(void);




#endif


