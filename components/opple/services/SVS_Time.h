#ifndef _SVS_TIME_H
#define _SVS_TIME_H


#include <Includes.h>
#include <DEV_Timer.h>

#pragma pack(1)

#define DEFAULT_ZONE    8

typedef struct 
{
	U16 Year;
	U8 Month;
	U8 Day;
	U8 Hour;
	U8 Min;
	U8 Sencond;
	S8 Zone;
}ST_TIME;

/*
typedef struct
{
	U32 year;
	U32 month;
	U32 day;
	U32 week; // 0:invalid,1-7:��һ-����
	U32 hour;
	U32 min;
	U32 seconds;
	U32 ms;
} ST_TIME_PARA;
*/
typedef struct
{
    U16 sunrise;  // �ճ�ʱ�䣬ByteH:ʱ��ByteL����
    U16 sunset;  //����ʱ�䣬 ByteH:ʱ��Byte����
} ST_SUN_TIME_PARA;

typedef struct
{
	int riseH; /*�ճ�h*/
	int riseM; /*�ճ�m*/
	int riseS; /*�ճ�s*/	
	int setH; /*����h*/
	int setM; /*����m*/
	int setS; /*����s*/
}ST_AST;

typedef struct
{
	U8 clkSrc;
}ST_CLKSRC;

typedef struct{
	T_MUTEX mutex;
	U32 updatePeriod;
}ST_UPDATE_PERIOD;

#define NO_RECV_TIME    0
#define RECV_TIME       1

typedef struct{
	T_MUTEX mutex;
	U8 recvTime;
}ST_RECV_TIME;

#pragma pack()

#define CLKSRC_ID    0
/*
#define LOCAL_CLK    0x01
#define GPS_CLK    	 0x02
#define NBIOT_CLK    0x04
*/
#define ALL_CLK      0x00
#define GPS_CLK    	 0x01
#define NBIOT_CLK    0x02
#define LOC_CLK      0x03
#define UNKNOW_CLK   0x04

#define NBIOT_ATCLCK_NOTIME_TO    120000    /*two seconds*/
//#define NBIOT_ATCLCK_TO    7200000    /*two hour*/
#define NBIOT_ATCLCK_TO    0xFFFFFFFF

//extern static ST_TIME g_stTime;
extern U8 g_ucInitTime;

void Opple_Get_RTC(ST_TIME* time);
int TimeCacWeek(int y,int m, int d);
int Opple_Calc_Ast_Time(int year, int month, int day, char zone, long double glat, long double glong, ST_AST * pstAstTime);
int TimeSetClockSrc(U8 ucClkSrc);
int TimeGetClockSrc(void);
U32 TimeGetClkSrcFromFlash(ST_CLKSRC *pstClkSrc);
U32 TimeSetClkSrcToFlash(ST_CLKSRC *pstClkSrc);
U32 TimeRead(unsigned char ucClkSrc, int*zone, ST_OPPLE_TIME * pstTimePara);
void TimeTask(void);
void TimeLoop(void);
int TimeInit(void);
int TimeFromNbiotClockEvent(void);
int TimeFromNbiotClock(void);
int TimeFromGpsClock(void);
int TimeSetCallback(U8 year, U8 month, U8 day, U8 hour, U8 min, U8 second, S8 zone);
int TimeSet(U8 year, U8 month, U8 day, U8 hour, U8 min, U8 second, S8 zone);
void TimeThread(void *pvParameter);
void TimerCallback(TimerHandle_t timer);
U32 TimeUpdatePeriodGet(void);
int TimeUpdatePeriodSet(U32 period);
void  time_zone_convert(ST_TIME *time, U8 src_tz, U8 des_tz);
int calculateTimezone(double lat,double lon);
int Zone(long double glong);
int TimeRecvTimeSet(U8 hasRecv);
U8 timeRecvTimeGet(void);
U32 TimeRestore();
time_t mk_time(int year,int mon,int day,int hour,int min,int sec);


#endif
