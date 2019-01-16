
#include <Includes.h>
#include <DRV_Gps.h>
#include "DRV_Nb.h"
#include <Opp_ErrorCode.h>
#include <APP_LampCtrl.h>
#include <APP_Main.h>
#include <SVS_Time.h>
#include <SVS_Config.h>
#include <DEV_Utility.h>
#include <SVS_Location.h>
#include <SVS_Para.h>
#include <DEV_Timer.h>
#include "OPP_Debug.h"
#include "DRV_Bc28.h"
#include "stdio.h"
#include "math.h"
#include "stdlib.h"

static ST_TIME g_stTime;
static U32 g_ulTimeHookTick;
static T_MUTEX g_stClkSrcMutex;
static T_MUTEX g_ulTimeSem;
TimerHandle_t xTimer;

static U8 g_ucClkSrc = NBIOT_CLK;
static ST_UPDATE_PERIOD g_stUpdatePeriod = {
	.updatePeriod = NBIOT_ATCLCK_NOTIME_TO
};
static ST_RECV_TIME g_stRecvTime = {
	.recvTime = NO_RECV_TIME,
};
/////////////////////////////////
#define M_PI       3.14159265358979323846

static double RAD = 180.0 * 3600 / M_PI;
static double midDayTime;
static double dawnTime;

/*************************
* 儒略日的计算
*
* @param y 年
*
* @param M 月
*
* @param d 日
*
* @param h 小时
*
* @param m 分
*
* @param s秒
*
* @return int
***************************/
static double timeToDouble(int y, int tM, double d)
{
	//        double A=0;
	double B = 0;
	double jd = 0;

	//设Y为给定年份，M为月份，D为该月日期（可以带小数）。
	//若M > 2，Y和M不变，若 M =1或2，以Y–1代Y，以M+12代M，换句话说，如果日期在1月或2月，则被看作是在前一年的13月或14月。
	//对格里高利历有 ：A = INT（Y/100）   B = 2 - A + INT(A/4)
	//对儒略历，取 B = 0
	//JD = INT(365.25(Y+4716))+INT(30.6001(M+1))+D+B-1524.5 （7.1）
	B = -13;
	jd = floor(365.25 * (y + 4716)) + floor(30.60001 * (tM + 1)) + B + d - 1524.5;
	return jd;
}

static void doubleToStr(double time, char *str)
{
	double t;
	int h, m, s;

	t = time + 0.5;
	t = (t - (int)t) * 24;
	h = (int)t;
	t = (t - h) * 60;
	m = (int)t;
	t = (t - m) * 60;
	s = (int)t;

	sprintf(str, "%02d:%02d:%02d", h, m, s);
}

/****************************
* @param t 儒略世纪数
*
* @return 太阳黄经
*****************************/
static double sunHJ(double t)
{
	double j;
	t = t + (32.0 * (t + 1.8) * (t + 1.8) - 20) / 86400.0 / 36525.0;
	// 儒略世纪年数,力学时
	j = 48950621.66 + 6283319653.318 * t + 53 * t * t - 994 + 334166 * cos(4.669257 + 628.307585 * t) + 3489 * cos(4.6261 + 1256.61517 * t) + 2060.6 * cos(2.67823 + 628.307585 * t) * t;
	return (j / 10000000);
}

static double mod(double num1, double num2)
{
	num2 = fabs(num2);
	// 只是取决于Num1的符号
	return num1 >= 0 ? (num1 - (floor(num1 / num2)) * num2) : ((floor(fabs(num1) / num2)) * num2 - fabs(num1));
}
/********************************
* 保证角度∈(-π,π)
*
* @param ag
* @return ag
***********************************/
static double degree(double ag)
{
	ag = mod(ag, 2 * M_PI);
	if (ag <= -M_PI){
		ag = ag + 2 * M_PI;
	}
	else if (ag>M_PI){
		ag = ag - 2 * M_PI;
	}
	return ag;
}

/***********************************
*
* @param date  儒略日平午
*
* @param lo    地理经度
*
* @param la    地理纬度
*
* @param tz    时区
*
* @return 太阳升起时间
*************************************/
double sunRiseTime(double date, double lo, double la, double tz)
{
	double t, j, sinJ, cosJ, gst, E, a, tD, cosH0, cosH1, H0, H1, H;
	date = date - tz;
	// 太阳黄经以及它的正余弦值
	t = date / 36525;
	j = sunHJ(t);
	// 太阳黄经以及它的正余弦值
	sinJ = sin(j);
	cosJ = cos(j);
	// 其中2*M_PI*(0.7790572732640 + 1.00273781191135448*jd)恒星时（子午圈位置）
	gst = 2 * M_PI * (0.779057273264 + 1.00273781191135 * date) + (0.014506 + 4612.15739966 * t + 1.39667721 * t * t) / RAD;
	E = (84381.406 - 46.836769 * t) / RAD; // 黄赤交角
	a = atan2(sinJ * cos(E), cosJ);// '太阳赤经
	tD = asin(sin(E) * sinJ); // 太阳赤纬
	cosH0 = (sin(-50 * 60 / RAD) - sin(la) * sin(tD)) / (cos(la) * cos(tD)); // 日出的时角计算，地平线下50分
	cosH1 = (sin(-6 * 3600 / RAD) - sin(la) * sin(tD)) / (cos(la) * cos(tD)); // 天亮的时角计算，地平线下6度，若为航海请改为地平线下12度
	// 严格应当区分极昼极夜，本程序不算
	if (cosH0 >= 1 || cosH0 <= -1){
		return -0.5;// 极昼
	}
	H0 = -acos(cosH0); // 升点时角（日出）若去掉负号 就是降点时角，也可以利用中天和升点计算
	H1 = -acos(cosH1);
	H = gst - lo - a; // 太阳时角
	midDayTime = date - degree(H) / M_PI / 2 + tz; // 中天时间
	dawnTime = date - degree(H - H1) / M_PI / 2 + tz;// 天亮时间
	return date - degree(H - H0) / M_PI / 2 + tz; // 日出时间，函数返回值
}
///////////////////////////////////////////////
#define PI 3.1415926

int days_of_month_1[]={31,28,31,30,31,30,31,31,30,31,30,31}; 
int days_of_month_2[]={31,29,31,30,31,30,31,31,30,31,30,31}; 
long double h=-0.833; 

//判断是否为闰年:若为闰年，返回1；若非闰年，返回0 
int leap_year(int year)
{ 
	if((year%400==0) || ((year%100!=0) && (year%4==0))) return 1; 
	else return 0; 
 } 
 
 
 
 //求从格林威治时间公元2000年1月1日到计算日天数days 
int days(int year, int month, int date)
{ 
 
     int i,a=0; 
 
     for(i=2000;i<year;i++){ 
 
         if(leap_year(i)) a=a+366; 
 
         else a=a+365; 
 
     } 
 
     if(leap_year(year)){ 
 
         for(i=0;i<month-1;i++){ 
 
             a=a+days_of_month_2[i]; 
 
         } 
 
     } 
 
     else { 
 
         for(i=0;i<month-1;i++){ 
 
             a=a+days_of_month_1[i]; 
 
         } 
 
     } 
 
     a=a+date; 
 
     return a; 
 
 } 
 
 //求格林威治时间公元2000年1月1日到计算日的世纪数t 
 long double t_century(int days, long double UTo)
 { 
 
     return ((long double)days+UTo/360)/36525; 
 
 } 
 
 
 //求太阳的平黄径  
 long double L_sun(long double t_century)
 { 
 
     return (280.460+36000.770*t_century); 
 
 } 
 
 
 //求太阳的平近点角  
long double G_sun(long double t_century)
{ 
	return (357.528+35999.050*t_century);  
} 
 
 
 //求黄道经度  
long double ecliptic_longitude(long double L_sun,long double G_sun)
{
	return (L_sun+1.915*sin(G_sun*PI/180)+0.02*sin(2*G_sun*PI/180));
} 
 
 
 //求地球倾角  
long double earth_tilt(long double t_century)
{ 
	return (23.4393-0.0130*t_century);  
} 
 
 
 //求太阳偏差  
long double sun_deviation(long double earth_tilt, long double ecliptic_longitude)
{
	return (180/PI*asin(sin(PI/180*earth_tilt)*sin(PI/180*ecliptic_longitude)));  
} 
 
 
 //求格林威治时间的太阳时间角GHA  
long double GHA(long double UTo, long double G_sun, long double ecliptic_longitude)
{ 
     return (UTo-180-1.915*sin(G_sun*PI/180)-0.02*sin(2*G_sun*PI/180)+2.466*sin(2*ecliptic_longitude*PI/180)-0.053*sin(4*ecliptic_longitude*PI/180));  
} 
 
 
 //求修正值e  
long double e(long double h, long double glat, long double sun_deviation)
{
	return 180/PI*acos((sin(h*PI/180)-sin(glat*PI/180)*sin(sun_deviation*PI/180))/(cos(glat*PI/180)*cos(sun_deviation*PI/180))); 
} 
 
 
 //求日出时间  
long double UT_rise(long double UTo, long double GHA, long double glong, long double e)
{ 
	return (UTo-(GHA+glong+e)); 
} 
 
 
 //求日落时间  
long double UT_set(long double UTo, long double GHA, long double glong, long double e)
{ 
 
     return (UTo-(GHA+glong-e)); 

} 
 
 
 
 //判断并返回结果（日出） 
long double result_rise(long double UT, long double UTo, long double glong, long double glat, int year, int month, int date){ 
 
     long double d; 
 
     if(UT>=UTo) d=UT-UTo; 
 
     else d=UTo-UT; 
 
     if(d>=0.1)
	 { 
         UTo=UT; 
         UT=UT_rise(UTo,GHA(UTo,G_sun(t_century(days(year,month,date),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,date),UTo)),G_sun(t_century(days(year,month,date),UTo)))),glong,e(h,glat,sun_deviation(earth_tilt(t_century(days(year,month,date),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,date),UTo)),G_sun(t_century(days(year,month,date),UTo)))))); 
         result_rise(UT,UTo,glong,glat,year,month,date); 
     } 
 
     return UT; 
 } 
 
 
 
 //判断并返回结果（日落） 
long double result_set(long double UT, long double UTo, long double glong, long double glat, int year, int month, int date){ 
 
     long double d; 
 
     if(UT>=UTo) d=UT-UTo; 
 
     else d=UTo-UT; 
 
     if(d>=0.1)
	 { 
         UTo=UT;
		 UT=UT_set(UTo,GHA(UTo,G_sun(t_century(days(year,month,date),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,date),UTo)),G_sun(t_century(days(year,month,date),UTo)))),glong,e(h,glat,sun_deviation(earth_tilt(t_century(days(year,month,date),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,date),UTo)),G_sun(t_century(days(year,month,date),UTo)))))); 
		 result_set(UT,UTo,glong,glat,year,month,date); 
     } 
 
     return UT;
} 
 
 
 //求时区 
int Zone(long double glong)
{ 
 
     if(glong>=0)
	 {
	 	return (int)((int)(glong/15.0)+1);
     }
     else 
	 {
	 	return (int)((int)(glong/15.0)-1);
     }
} 

 
//打印结果 
void output(long double rise, long double set, long double glong, int * riseH, int * riseM, int *setH, int * setM)
{ 
	if((int)(60*(rise/15+Zone(glong)-(int)(rise/15+Zone(glong))))<10) 
	{
		*riseH = (int)(rise/15+Zone(glong));
		*riseM = (int)(60*(rise/15+Zone(glong)-(int)(rise/15+Zone(glong))));
	}
	else
	{
		*riseH = (int)(rise/15+Zone(glong));
		*riseM = (int)(60*(rise/15+Zone(glong)-(int)(rise/15+Zone(glong))));
	}
	//DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"The time at which the sun rises is %d, %d\r\n", *riseH, *riseM); 
	
	if((int)(60*(set/15+Zone(glong)-(int)(set/15+Zone(glong))))<10) 
	{
		*setH = (int)(set/15+Zone(glong));
		*setM = (int)(60*(set/15+Zone(glong)-(int)(set/15+Zone(glong))));
	}
	else
	{
		*setH = (int)(set/15+Zone(glong));
		*setM = (int)(60*(set/15+Zone(glong)-(int)(set/15+Zone(glong))));
	}
	//DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"The time at which the sun sets is %d, %d\r\n", *setH, *setM); 
	
} 

int Opple_Calc_Ast_Time(int year, int month, int day, char zone, long double glat, long double glong, ST_AST * pstAstTime)
{
#if 0
	long double UTo=180.0;
	long double rise,set; 
	rise=result_rise(UT_rise(UTo,GHA(UTo,G_sun(t_century(days(year,month,day),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,day),UTo)),G_sun(t_century(days(year,month,day),UTo)))),glong,e(h,glat,sun_deviation(earth_tilt(t_century(days(year,month,day),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,day),UTo)),G_sun(t_century(days(year,month,day),UTo)))))),UTo,glong,glat,year,month,day); 
	set=result_set(UT_set(UTo,GHA(UTo,G_sun(t_century(days(year,month,day),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,day),UTo)),G_sun(t_century(days(year,month,day),UTo)))),glong,e(h,glat,sun_deviation(earth_tilt(t_century(days(year,month,day),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,day),UTo)),G_sun(t_century(days(year,month,day),UTo)))))),UTo,glong,glat,year,month,day); 

	//DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"rise %Lf set %Lf\r\n", rise, set);
	output(rise, set, glong, &pstAstTime->riseH, &pstAstTime->riseM, &pstAstTime->setH, &pstAstTime->setM);
#else
    int tz = 0;
	int i = 0;
    char timestr[20] = {0};
	int hour,min,second;
	static double richu;
	static double jd_degrees;
	static double jd_seconds;
	static double wd_degrees;
	static double wd_seconds;
	static double jd;
	static double wd;
	
    jd_degrees = (int)glong;
    jd_seconds =(glong-jd_degrees)*60;
    wd_degrees = (int)glat;
    wd_seconds =(glat-wd_degrees)*60;
    tz=zone;
	//step 1
	jd=-(jd_degrees+jd_seconds/60)/180*M_PI;
	wd=(wd_degrees+wd_seconds/60)/180*M_PI;

	//step 2
	if(month<=2){
		year -= 1;
		month += 12;
	}
	richu = timeToDouble(year,month,day) - 2451544.5;
	for (i = 0; i < 10; i++){
		richu = sunRiseTime(richu, jd, wd, tz/24.0);// 逐步逼近法算10次
	}

	doubleToStr(richu,timestr);
	sscanf(timestr, "%02d:%02d:%02d", &hour, &min, &second);
	pstAstTime->riseH = hour;
	pstAstTime->riseM = min;
	pstAstTime->riseS = second;

	doubleToStr(midDayTime + midDayTime - richu, timestr);
	sscanf(timestr, "%02d:%02d:%02d", &hour, &min, &second);	
	pstAstTime->setH = hour;
	pstAstTime->setM = min;
	pstAstTime->setS = second;
#endif
	return 0;
}

#if 0
int test()
{
	long double UTo=180.0;
	int year,month,date;
	long double glat,glong;
	long double rise,set; 
	int c[3];
	int riseH, riseM, setH, setM;

	//input_date(c); 
	//year=c[0]; 
	//month=c[1]; 
	//date=c[2]; 

	year=2009; 
	month=03; 
	date=0; 
	//year=t[0]; 
	//month=t[1]; 
	//date=t[2]; 

	//input_glat(c); 

	//40°40′40″
	c[0]=40;
	c[1]=40;
	c[2]=40;
	glat=c[0]+c[1]/60+c[2]/3600;
	//glat=glat[0]+glat[1]/60+glat[2]/3600;

	
	//input_glong(c); 
	//40°40′40″
	c[0]=40;
	c[1]=40;
	c[2]=40;
	glong=c[0]+c[1]/60+c[2]/3600;
	//glong=glong[0]+glong[1]/60+glong[2]/3600;
	
	rise=result_rise(UT_rise(UTo,GHA(UTo,G_sun(t_century(days(year,month,date),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,date),UTo)),G_sun(t_century(days(year,month,date),UTo)))),glong,e(h,glat,sun_deviation(earth_tilt(t_century(days(year,month,date),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,date),UTo)),G_sun(t_century(days(year,month,date),UTo)))))),UTo,glong,glat,year,month,date); 
	set=result_set(UT_set(UTo,GHA(UTo,G_sun(t_century(days(year,month,date),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,date),UTo)),G_sun(t_century(days(year,month,date),UTo)))),glong,e(h,glat,sun_deviation(earth_tilt(t_century(days(year,month,date),UTo)),ecliptic_longitude(L_sun(t_century(days(year,month,date),UTo)),G_sun(t_century(days(year,month,date),UTo)))))),UTo,glong,glat,year,month,date); 

	//printf("rise %ld set %ld\r\n", rise, set);
	output(rise, set, glong, &riseH, &riseM, &setH, &setM);
/*
	pstAstTime->riseH = riseH;
	pstAstTime->riseM = riseM;
	pstAstTime->setH = setH;
	pstAstTime->setM = setM;
*/	
}
#endif
//////////////////////////////////////////////
struct tm {
	int tm_sec;	 /* seconds after the minute, 0 to 60(0 - 60 allows for the occasional leap second) */
	int tm_min;	 /* minutes after the hour, 0 to 59 */
	int tm_hour;  /* hours since midnight, 0 to 23 */
	int tm_mday;  /* day of the month, 1 to 31 */
	int tm_mon;	 /* months since January, 0 to 11 */
	int tm_year;  /* years since 1900 */
	//int tm_wday;  /* days since Sunday, 0 to 6 */
	//int tm_yday;  /* days since January 1, 0 to 365 */
	//int tm_isdst; /* Daylight Savings Time flag */
};
time_t mk_time(int year,int mon,int day,int hour,int min,int sec)
{

    if (0 >= (int) (mon -= 2)) {    /* 1..12 -> 11,12,1..10 */
        mon += 12;      /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return (((
        (unsigned long) (year/4 - year/100 + year/400 + 367*mon/12 + day) +
        year*365 - 719499
        )*24 + hour - 8 /* now have hours */
        )*60 + min /* now have minutes */
        )*60 + sec; /* finally seconds */

}
void local_time(time_t time,struct tm *tm_time)
{
    const char Days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    unsigned  int n32_Pass4year;
    int n32_hpery;

    //计算时差
    int timezone = 8*3600;
    time=time + timezone;

    if(time < 0)
    {
       time = 0;
    }
    //取秒时间
    tm_time->tm_sec=(int)(time % 60);
    time /= 60;
    //取分钟时间
    tm_time->tm_min=(int)(time % 60);
    time /= 60;
    //取过去多少个四年，每四年有 1461*24 小时
    n32_Pass4year=((unsigned int)time / (1461L * 24L));
    //计算年份
    tm_time->tm_year=(n32_Pass4year << 2)+70;
    //四年中剩下的小时数
    time %= 1461L * 24L;
    //校正闰年影响的年份，计算一年中剩下的小时数
    for (;;)
    {
        //一年的小时数
        n32_hpery = 365 * 24;
        //判断闰年
        if ((tm_time->tm_year & 3) == 0)
		{
            //是闰年，一年则多24小时，即一天
            n32_hpery += 24;
		}
        if (time < n32_hpery)
		{
            break;
		}
        tm_time->tm_year++;
        time -= n32_hpery;
    }
    //小时数
    tm_time->tm_hour=(int)(time % 24);
    //一年中剩下的天数
    time /= 24;
    //假定为闰年
    time++;
    //校正润年的误差，计算月份，日期
    if ((tm_time->tm_year & 3) == 0)
    {
		if (time > 60)
		{
			time--;
		}
		else
		{
			if (time == 60)
			{
				tm_time->tm_mon = 1;
				tm_time->tm_mday = 29;
				return ;
			}
		}
    }
    //计算月日
    for (tm_time->tm_mon = 0; Days[tm_time->tm_mon] < time;tm_time->tm_mon++)
    {
          time -= Days[tm_time->tm_mon];
    }

    tm_time->tm_mday = (int)(time);

    return;

}

static int days_of_month(ST_TIME * time)
{
    int days = 0;

    switch (time->Month)
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            days = 31;
            break;

        case 4:
        case 6:
        case 9:
        case 11:
            days = 30;
            break;

        case 2:
            if (leap_year(time->Year))
            { days = 29; }
            else
            { days = 28; }

            break;
    }

    return days;
}

void  time_zone_convert(ST_TIME *time, U8 src_tz, U8 des_tz)
{
    U8 hour, delta = 0;

    /* Check time zone */
    if (time == NULL || des_tz > 12 || des_tz < -12)
        return;

    if (des_tz == src_tz)
        return;

    /* Ready to convert time */
    hour = (time->Hour + (des_tz - src_tz)) % 24;
    //printf("time->hour=%d hour = %d des_tz=%d src_tz=%d\r\n", time->hour, hour, des_tz, src_tz);

    if ((hour >= 0) &&( hour < time->Hour) && (des_tz > src_tz)) {
        delta = 1;
    } else if (hour < 0) {
        hour += 24;
        delta = -1;
    }

    //printf("delta = %d time->day=%d ,hour :%d \r\n", delta, time->day,hour);
    time->Hour = hour;
    time->Day = time->Day + delta;

    if (time->Day == 0 || time->Day > days_of_month(time)) {
        time->Day = 1;
        time->Month = time->Month + delta;

        if (time->Month == 0 || time->Month > 12) {
            time->Month = 1;
            time->Year = time->Year + delta;
        }
    }

}

int TimeInit(void)
{
	int ret;
	ST_CLKSRC stClkSrc;

	/*2000/1/1 0:0:0*/
	g_stTime.Year = 2000;
	g_stTime.Month= 01;
	g_stTime.Day= 01;
	g_stTime.Hour= 00;
	g_stTime.Min= 00;
	g_stTime.Sencond= 00;
	g_stTime.Zone= DEFAULT_ZONE;
	g_ulTimeHookTick = OppTickCountGet();

	MUTEX_CREATE(g_stClkSrcMutex);
	MUTEX_CREATE(g_ulTimeSem);
	MUTEX_CREATE(g_stUpdatePeriod.mutex);
	MUTEX_CREATE(g_stRecvTime.mutex);
	//if(NULL == g_ulTimeSem)
	//g_ulTimeSem=xSemaphoreCreateMutex();
	
	ret = TimeGetClkSrcFromFlash(&stClkSrc);
	if(OPP_SUCCESS == ret){
		TimeSetClockSrc(stClkSrc.clkSrc);
	}

	if(ALL_CLK == g_ucClkSrc || GPS_CLK == g_ucClkSrc){
		TimeFromGpsClock();
	}
	
    // 申请定时器， 配置
    /*xTimer = xTimerCreate
                   ("One Sec TImer",
                   1000,
                   pdTRUE,
                  ( void * ) 0,
                  TimerCallback);

     if( xTimer != NULL ) {
        xTimerStart( xTimer, 0 );
    }*/

	
	return OPP_SUCCESS;
}

void Opple_Get_RTC(ST_TIME* time)
{
	U32 elapseTick;
	struct tm T;
	uint32_t cur_tim = 0;
	time_t t;
	struct tm time_field;
	
	if(time == NULL)
		return;
	
	// lock
	MUTEX_LOCK(g_ulTimeSem,MUTEX_WAIT_ALWAYS);
	
	elapseTick = OppTickCountGet() - g_ulTimeHookTick;

/*	T.tm_year = g_stTime.Year;
	T.tm_mon = g_stTime.Month;
	T.tm_mday = g_stTime.Day;
	T.tm_hour = g_stTime.Hour;
	T.tm_min = g_stTime.Min;
	T.tm_sec = g_stTime.Sencond;
	cur_tim = fun_mktime(&T);
	cur_tim += elapseTick;
	memset(&T,0,sizeof(struct tm));
	fun_localtime(&T,cur_tim);
	g_stTime.Year = T.tm_year;
	g_stTime.Month = T.tm_mon;
	g_stTime.Day = T.tm_mday;
	g_stTime.Hour = T.tm_hour;
	g_stTime.Min= T.tm_min;
	g_stTime.Sencond = T.tm_sec;
*/
	t = mk_time(g_stTime.Year,g_stTime.Month,g_stTime.Day,g_stTime.Hour,g_stTime.Min,g_stTime.Sencond);/*2017-01-01 00:00:00*/
	t  += elapseTick/1000;
	local_time(t,&time_field);
	time_field.tm_year += 1900;
	time_field.tm_mon += 1;
	time->Year = time_field.tm_year;
	time->Month = time_field.tm_mon;
	time->Day = time_field.tm_mday;
	time->Hour = time_field.tm_hour;
	time->Min= time_field.tm_min;
	time->Sencond = time_field.tm_sec;
	time->Zone = g_stTime.Zone;
	//memcpy(time, &g_stTime, sizeof(g_stTime));
	// unlock
	MUTEX_UNLOCK(g_ulTimeSem);

	return;
}

int TimeUpdatePeriodSet(U32 period)
{
	MUTEX_LOCK(g_stUpdatePeriod.mutex,MUTEX_WAIT_ALWAYS);
	g_stUpdatePeriod.updatePeriod = period;
	MUTEX_UNLOCK(g_stUpdatePeriod.mutex);
	return 0;
}

U32 TimeUpdatePeriodGet(void)
{
	U32 period;
	
	MUTEX_LOCK(g_stUpdatePeriod.mutex,MUTEX_WAIT_ALWAYS);
	period = g_stUpdatePeriod.updatePeriod;
	MUTEX_UNLOCK(g_stUpdatePeriod.mutex);
	return period;
}

int TimeRecvTimeSet(U8 hasRecv)
{
	MUTEX_LOCK(g_stRecvTime.mutex,MUTEX_WAIT_ALWAYS);
	if(NO_RECV_TIME == hasRecv)
		g_stRecvTime.recvTime = NO_RECV_TIME;
	else
		g_stRecvTime.recvTime = RECV_TIME;
	MUTEX_UNLOCK(g_stRecvTime.mutex);
	return 0;
}

U8 timeRecvTimeGet(void)
{
	U8 hasRecv;
	
	MUTEX_LOCK(g_stRecvTime.mutex,MUTEX_WAIT_ALWAYS);
	if(NO_RECV_TIME == g_stRecvTime.recvTime)
		hasRecv = NO_RECV_TIME;
	else
		hasRecv = RECV_TIME;
	MUTEX_UNLOCK(g_stRecvTime.mutex);
	
	return hasRecv;
}

int TimeCacWeek(int y,int m, int d)
{
    int iWeek=(d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)%7;

    if(m==1||m==2) {
        m+=12;
        y--;
    }
    switch(iWeek)
    {
    case 0: DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"monday\n"); break;
    case 1: DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"tuesday\n"); break;
    case 2: DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"wednesday\n"); break;
    case 3: DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"thursday\n"); break;
    case 4: DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"friday\n"); break;
    case 5: DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"saturday\n"); break;
    case 6: DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"sunday\n"); break;
    }

	return iWeek;
} 

int TimeLeapYear(int year)
{
	int leap;
	
	if (year%4==0&&year%100!=0)
	{
		leap = 1;
	}
	else
	{
		leap = 0;
	}

	return leap;
}

int TimeSetClockSrc(U8 ucClkSrc)
{
	g_ucClkSrc = ucClkSrc;

	return 0;
}

int TimeGetClockSrc(void)
{
	return g_ucClkSrc;
}

U32 TimeGetClkSrcFromFlash(ST_CLKSRC *pstClkSrc)
{
	int ret = OPP_SUCCESS;
	unsigned int len = sizeof(ST_CLKSRC);
	ST_CLKSRC stClkSrc;

	MUTEX_LOCK(g_stClkSrcMutex,100);
	ret = AppParaRead(APS_PARA_MODULE_APS_TIME, CLKSRC_ID, (unsigned char *)&stClkSrc, &len);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO, "TimeGetClkSrcFromFlash fail ret %d\r\n",ret);
		MUTEX_UNLOCK(g_stClkSrcMutex);
		return OPP_FAILURE;
	}
	
	memcpy(pstClkSrc,&stClkSrc,sizeof(ST_CLKSRC));
	MUTEX_UNLOCK(g_stClkSrcMutex);
	return OPP_SUCCESS;
}

U32 TimeSetClkSrcToFlash(ST_CLKSRC *pstClkSrc)
{
	int ret = OPP_SUCCESS;
	
	MUTEX_LOCK(g_stClkSrcMutex,MUTEX_WAIT_ALWAYS);
	ret = AppParaWrite(APS_PARA_MODULE_APS_TIME, CLKSRC_ID, (unsigned char *)pstClkSrc, sizeof(ST_CLKSRC));
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO, "TimeSetClkSrcToFlash fail ret %d\r\n",ret);
		MUTEX_UNLOCK(g_stClkSrcMutex);
		return OPP_FAILURE;
	}
	MUTEX_UNLOCK(g_stClkSrcMutex);
	return OPP_SUCCESS;
}

/*计算行政时区*/
int calculateTimezone(double lat,double lon){
    int a,b,c,timezone;
    a = (int)(fabs(lon)+0.5);// 对经度进行四舍五入，且取正整数
    b = a/15; // 商
    c = a%15; // 余数
    if((lat >=17.9 && lat <=53 && lon>=75 && lon<=125) || (lat>=40 && lat <=53 && lon>=125 && lon<= 135)){// 如果经纬度处于中国版图内，则都划为东8区，为什么要这样划分详见第三节
    timezone = 8;
    }
    else{
 
        if(c > 7.5)
            timezone = b+1;
        else
            timezone = b;
        if(lon > 0.0f)
            timezone = timezone;
        else
            timezone = (-1)*timezone;
    }
    return timezone;
}

/*
*reture: 0成功，1获取时间失败
*/
int TimeFromGpsClock(void)
{
	int len = 0;
	char buf[50] = {0};
	gps_time_info_t info;
	long double lat;
	long double lng;
	int ret;

#if 0
#ifdef DEBUG
	strcpy(info.UTC_Date, "180420");
	strcpy(info.UTC_Time, "020101.111");
#else
	ret = GpsTimeInfoGet(&info,100);
#endif
	if(!IS_ZERO((char *)info.UTC_Date, sizeof(info.UTC_Date)) && !IS_ZERO((char *)info.UTC_Time, sizeof(info.UTC_Time)))
#endif
	ret = GpsTimeInfoGet(&info,100);
	DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"GpsTimeInfoGet ret %d\r\n", ret);

	if(0 == ret)
	{
		DEBUG_LOG(DEBUG_MODULE_LOC, DLL_INFO, "GPS time\r\n");
		//wangtao
		DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"TimeFromGpsClock Lock\r\n");
		MUTEX_LOCK(g_ulTimeSem,MUTEX_WAIT_ALWAYS);
		
		//day
		memset(buf, 0, sizeof(buf));
		strncpy(buf, info.UTC_Date, 2);
		len += 2;
		buf[2] = '\0';
		g_stTime.Day = atoi(buf);
		//month
		memset(buf, 0, sizeof(buf));
		strncpy(buf, &info.UTC_Date[len], 2);
		buf[2] = '\0';
		len += 2;
		g_stTime.Month = atoi(buf);
		//year
		memset(buf, 0, sizeof(buf));
		strncpy(buf, &info.UTC_Date[len], 2);
		buf[2] = '\0';
		len += 2;
		g_stTime.Year = atoi(buf);
		g_stTime.Year += 2000;
		//hh
		len = 0;
		memset(buf, 0, sizeof(buf));
		strncpy(buf, &info.UTC_Time[len], 2);
		buf[2] = '\0';
		len += 2;
		g_stTime.Hour = atoi(buf);
		//mm
		memset(buf, 0, sizeof(buf));
		strncpy(buf, &info.UTC_Time[len], 2);
		buf[2] = '\0';
		len += 2;
		g_stTime.Min= atoi(buf);
		//ss.sss->ss
		memset(buf, 0, sizeof(buf));
		strncpy(buf, &info.UTC_Time[len], 2);
		buf[2] = '\0';
		len += 2;
		g_stTime.Sencond = atoi(buf);
		//zone
		//OppLocationGet(GPS_LOC);
		LocGetLat(&lat);
		LocGetLng(&lng);
		g_stTime.Zone = calculateTimezone(lat,lng);
		time_zone_convert(&g_stTime,0,g_stTime.Zone);
		g_ulTimeHookTick = OppTickCountGet();
		//wangtao
		MUTEX_UNLOCK(g_ulTimeSem);
		TimeRecvTimeSet(RECV_TIME);
		DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"y %d, m %d, d %d, h %d, m %d, s %d, z %d\r\n", g_stTime.Year, g_stTime.Month, g_stTime.Day,g_stTime.Hour, g_stTime.Min, g_stTime.Sencond, g_stTime.Zone);
		return OPP_SUCCESS;
	}

	return OPP_FAILURE;
}

/*FOR NBIOT*/
int TimeSetCallback(U8 year, U8 month, U8 day, U8 hour, U8 min, U8 second, S8 zone)
{
	MUTEX_LOCK(g_ulTimeSem, MUTEX_WAIT_ALWAYS);

	g_stTime.Year = year + 2000;
	g_stTime.Month = month;
	g_stTime.Day = day;
	g_stTime.Hour = hour;   //格林尼治时间
	g_stTime.Min = min;
	g_stTime.Sencond = second;
	g_stTime.Zone = zone/4;
	
	time_zone_convert(&g_stTime,0,g_stTime.Zone);
	g_ulTimeHookTick = OppTickCountGet();

	MUTEX_UNLOCK(g_ulTimeSem);
	
	return OPP_SUCCESS;
}

/*手动直接设置当前时间*/
int TimeSet(U8 year, U8 month, U8 day, U8 hour, U8 min, U8 second, S8 zone)
{
	MUTEX_LOCK(g_ulTimeSem, MUTEX_WAIT_ALWAYS);

	if(month > 12 || day > 31 || hour > 23 || min > 60 || second > 60 || zone > 12 || zone < -12){
		MUTEX_UNLOCK(g_ulTimeSem);		
		return OPP_FAILURE;
	}
	
	g_stTime.Year = year + 2000;
	g_stTime.Month = month;
	g_stTime.Day = day;
	g_stTime.Hour = hour;
	g_stTime.Min = min;
	g_stTime.Sencond = second;
	g_stTime.Zone = zone;
	g_ulTimeHookTick = OppTickCountGet();

	MUTEX_UNLOCK(g_ulTimeSem);
	
	return OPP_SUCCESS;
}

int TimeFromNbiotClockEvent(void)
{
	//char buf[36] = {0};
	int ret = OPP_SUCCESS;
	int argc;
	char *argv[MAX_ARGC]={NULL};
	U8 year, month, day, hour, min, second;
	S8 zone;

	/*
	if(BC28_READY != NeulBc28GetDevState()){
		return OPP_SUCCESS;
	}*/
	
	DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"NBIOT time\r\n");	
	/*+CCLK:18/01/30,09:17:55+32*/
	/*ret = NeulBc28QueryClock(buf, 36);
	if(OPP_SUCCESS != ret)
	{
		DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"TimeFromNbiotClock err %s\r\n", buf);	
		return ret;
	}*/
	//printf("TimeFromNbiotClockEvent\r\n");
	sendEvent(TIME_EVENT,RISE_STATE,0,argv);
	ret = recvEvent(TIME_EVENT,&argc,argv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		//printf("recvEvent TIME_EVENT error\r\n");
		ARGC_FREE(argc,argv);
		return ret;
	}
	/*printf("argc %d\r\n",argc);
	for(int i=0;i<argc;i++){
		printf("[%d] %s\r\n", i,argv[i]);
	}*/

	if(strrchr(argv[0],'+'))
		ret = sscanf(argv[0], "\r\n+CCLK:%hhu/%hhu/%hhu,%hhu:%hhu:%hhu+%hhu\r\n",
			&year, &month, &day,&hour, &min, &second, &zone);
	else{
		ret = sscanf(argv[0], "\r\n+CCLK:%hhu/%hhu/%hhu,%hhu:%hhu:%hhu-%hhu\r\n",
			&year, &month, &day,&hour, &min, &second, &zone);
		zone = 0-zone;
	}
	if(ret == 7){
		//printf("%d-%d-%d %d:%d:%d %d\r\n", year, month, day, hour, min, second, zone);
		TimeSetCallback(year,month,day,hour,min,second,zone);
		TimeUpdatePeriodSet(NBIOT_ATCLCK_TO);
		TimeRecvTimeSet(RECV_TIME);
	}
	ARGC_FREE(argc,argv);

	return ret;
}

int TimeFromNbiotClock(void)
{
	char buf[36] = {0};
	int ret = OPP_SUCCESS;
	U8 year, month, day, hour, min, second, zone;

	if(BC28_READY != NeulBc28GetDevState()){
		return OPP_SUCCESS;
	}
	
	DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"NBIOT time\r\n");	
	/*+CCLK:18/01/30,09:17:55+32*/
	ret = NeulBc28QueryClock(buf);
	if(OPP_SUCCESS != ret)
	{
		DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"TimeFromNbiotClock err %s\r\n", buf);	
		return ret;
	}
	if(strrchr(buf,'+'))
		ret = sscanf(buf, "\r\n+CCLK:%hhu/%hhu/%hhu,%hhu:%hhu:%hhu+%hhu\r\n",
			&year, &month, &day,&hour, &min, &second, &zone);
	else{
		ret = sscanf(buf, "\r\n+CCLK:%hhu/%hhu/%hhu,%hhu:%hhu:%hhu-%hhu\r\n",
			&year, &month, &day,&hour, &min, &second, &zone);
		zone = 0-zone;
	}
	if(ret == 7){
		//printf("%d-%d-%d %d:%d:%d %d\r\n", year, month, day, hour, min, second, zone);
		TimeSetCallback(year,month,day,hour,min,second,zone);
		TimeUpdatePeriodSet(NBIOT_ATCLCK_TO);
		TimeRecvTimeSet(RECV_TIME);
	}
	//printf("%s", buf);
	//OppMutexLock(g_ulTimeSem, LOS_WAIT_FOREVER);
	//wangtao todo
	//OppMutexLock(g_ulTimeSem);
	/*sscanf(buf, "\r\n+CCLK:%d/%d/%d,%d:%d:%d+%d\r\n",
		&g_stTime.Year, &g_stTime.Month, &g_stTime.Day,
		&g_stTime.Hour, &g_stTime.Min, &g_stTime.Sencond, &g_stTime.Zone);*/
	//异步获取
	/*sscanf(buf, "\r\n+CCLK:%hu/%hhu/%hhu,%hhu:%hhu:%hhu+%hhu\r\n",
		&g_stTime.Year, &g_stTime.Month, &g_stTime.Day,
		&g_stTime.Hour, &g_stTime.Min, &g_stTime.Sencond, &g_stTime.Zone);
		
	g_stTime.Year += 2000;
	g_stTime.Hour += 8;*/
	//OppMutexUnLock(g_ulTimeSem);

	return ret;
}

U32 TimeRead(unsigned char ucClkSrc, int*zone, ST_OPPLE_TIME * pstTimePara)
{
	U32 ret = OPP_SUCCESS;
	time_t t;
	struct tm time_field;
	U32 elapseTick;
	
	if(ALL_CLK == ucClkSrc)
	{
		ret = TimeFromGpsClock();
		if(OPP_SUCCESS != ret)
		{
			if(BC28_READY == NeulBc28GetDevState()){
				ret = TimeFromNbiotClockEvent();
			}
		}
	}
	else if(GPS_CLK == ucClkSrc)
	{
		ret = TimeFromGpsClock();
	}
	else if(NBIOT_CLK == ucClkSrc)
	{
		if(BC28_READY == NeulBc28GetDevState()){
			ret = TimeFromNbiotClockEvent();
		}
	}
	else
	{
		//DEBUG_LOG(DEBUG_MODULE_TIME, DLL_DEBUG, "y %d, m %d, d %d, h %d, m %d, s %d, z %d\r\n", g_stTime.Year, g_stTime.Month, g_stTime.Day,g_stTime.Hour, g_stTime.Min, g_stTime.Sencond, g_stTime.Zone);
		//printf("y %d, m %d, d %d, h %d, m %d, s %d, z %d\r\n", g_stTime.Year, g_stTime.Month, g_stTime.Day,g_stTime.Hour, g_stTime.Min, g_stTime.Sencond, g_stTime.Zone);
		return OPP_FAILURE;
	}
	
	MUTEX_LOCK(g_ulTimeSem,MUTEX_WAIT_ALWAYS);
	elapseTick = OppTickCountGet() - g_ulTimeHookTick;
	t = mk_time(g_stTime.Year,g_stTime.Month,g_stTime.Day,g_stTime.Hour,g_stTime.Min,g_stTime.Sencond);/*2017-01-01 00:00:00*/
	t  += elapseTick/1000;
	local_time(t,&time_field);
	time_field.tm_year += 1900;
	time_field.tm_mon += 1;
	
	*zone = g_stTime.Zone;
	MUTEX_UNLOCK(g_ulTimeSem);

	pstTimePara->uwYear = time_field.tm_year;
	pstTimePara->ucMon = time_field.tm_mon;
	pstTimePara->ucDay = time_field.tm_mday;
	pstTimePara->ucHour = time_field.tm_hour;
	pstTimePara->ucMin = time_field.tm_min;
	pstTimePara->ucSec = time_field.tm_sec;
	pstTimePara->usMs = 0;
	pstTimePara->ucWDay = TimeCacWeek(time_field.tm_year, time_field.tm_mon, time_field.tm_mday);
	return ret;
}
void TimerCallback(TimerHandle_t timer)
{
	int leap = 0;
	int day = 0;

	//printf("per second plus one\r\n");
	MUTEX_LOCK(g_ulTimeSem, MUTEX_WAIT_ALWAYS);
	g_stTime.Sencond++;
	if(g_stTime.Sencond == 60)
	{
		g_stTime.Min++;
		g_stTime.Sencond = 0;
	}

	if(g_stTime.Min == 60)
	{
		g_stTime.Hour++;
		g_stTime.Min = 0;
	}

	if(g_stTime.Hour == 24)
	{
		g_stTime.Day++;
		g_stTime.Hour = 0;
	}
	if(g_stTime.Month == 1 && g_stTime.Month == 3 && g_stTime.Month == 5 && g_stTime.Month == 7 && g_stTime.Month == 8 && g_stTime.Month == 10 && g_stTime.Month == 12)
	{
		if(g_stTime.Day > 31)
		{
			g_stTime.Month++;
			g_stTime.Day = 1;
		}
	}
	else if(g_stTime.Month == 4 && g_stTime.Month == 6 && g_stTime.Month == 5 && g_stTime.Month == 9 && g_stTime.Month == 8 && g_stTime.Month == 11)
	{
		if(g_stTime.Day > 30)
		{
			g_stTime.Month++;
			g_stTime.Day = 1;
		}

	}
	else if(g_stTime.Month == 2)
	{
		leap = TimeLeapYear((g_stTime.Year));
		if(leap == 1)
		{
			day = 29;
		}
		else
		{
			day = 28;
		}
		if(g_stTime.Day > day)
		{
			g_stTime.Month++;
			g_stTime.Day = 1;
		}
	}

	if(g_stTime.Month > 12)
	{
		g_stTime.Year++;
		g_stTime.Month = 1;
	}
	
	//DEBUG_LOG(DEBUG_MODULE_TIME, DLL_DEBUG, "y %d, m %d, d %d, h %d, m %d, s %d, z %d\r\n", g_stTime.Year, g_stTime.Month, g_stTime.Day,g_stTime.Hour, g_stTime.Min, g_stTime.Sencond, g_stTime.Zone);
	//DEBUG_LOG(DEBUG_MODULE_TIME, DLL_DEBUG, "aaaaaaaaa");
  MUTEX_UNLOCK(g_ulTimeSem);
  
	//DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"y %d, m %d, d %d, h %d, m %d, s %d, z %d\r\n", g_stTime.Year, g_stTime.Month, g_stTime.Day,g_stTime.Hour, g_stTime.Min, g_stTime.Sencond, g_stTime.Zone);
	
	//tick_start2 = OppTickCountGet();
}

void TimeMakeLog(U8 src)
{
	APP_LOG_T log;

	log.id = TIME_LOGID;
	log.level = LOG_LEVEL_INFO;
	log.module = MC_MODULE_LAMP;
	log.len = 5;
	log.log[0] = src;
	ApsDaemonLogReport(&log);
}
void TimeLoop(void)
{
	static U32 tick_start1 = 0, tick_start2 = 0;
	static U8 init = 0, init1 = 0;
	int leap = 0;
	int day = 0;
	//int len = 0;
	int ret = OPP_SUCCESS;
	int support;
	//TimeInit();
	//tick_start2 = (UINT32)OppTickCountGet();
	
	//while(1)
	//printf("TimeLoop\r\n");
#if 0
	if(!init1)
	{
		if(ALL_CLK == g_ucClkSrc || GPS_CLK == g_ucClkSrc){
			TimeFromGpsClock();
		}
		tick_start2 = OppTickCountGet();	
		init1 = 1;
	}
	if ((OppTickCountGet() - tick_start2) >= 1000)
	{
		//printf("per second plus one\r\n");
		MUTEX_LOCK(g_ulTimeSem, MUTEX_WAIT_ALWAYS);
		g_stTime.Sencond++;
		if(g_stTime.Sencond == 60)
		{
			g_stTime.Min++;
			g_stTime.Sencond = 0;
		}

		if(g_stTime.Min == 60)
		{
			g_stTime.Hour++;
			g_stTime.Min = 0;
		}

		if(g_stTime.Hour == 24)
		{
			g_stTime.Day++;
			g_stTime.Hour = 0;
		}
		if(g_stTime.Month == 1 && g_stTime.Month == 3 && g_stTime.Month == 5 && g_stTime.Month == 7 && g_stTime.Month == 8 && g_stTime.Month == 10 && g_stTime.Month == 12)
		{
			if(g_stTime.Day > 31)
			{
				g_stTime.Month++;
				g_stTime.Day = 1;
			}
		}
		else if(g_stTime.Month == 4 && g_stTime.Month == 6 && g_stTime.Month == 5 && g_stTime.Month == 9 && g_stTime.Month == 8 && g_stTime.Month == 11)
		{
			if(g_stTime.Day > 30)
			{
				g_stTime.Month++;
				g_stTime.Day = 1;
			}

		}
		else if(g_stTime.Month == 2)
		{
			leap = TimeLeapYear((g_stTime.Year));
			if(leap == 1)
			{
				day = 29;
			}
			else
			{
				day = 28;
			}
			if(g_stTime.Day > day)
			{
				g_stTime.Month++;
				g_stTime.Day = 1;
			}
		}

		if(g_stTime.Month > 12)
		{
			g_stTime.Year++;
			g_stTime.Month = 1;
		}
		
		//DEBUG_LOG(DEBUG_MODULE_TIME, DLL_DEBUG, "y %d, m %d, d %d, h %d, m %d, s %d, z %d\r\n", g_stTime.Year, g_stTime.Month, g_stTime.Day,g_stTime.Hour, g_stTime.Min, g_stTime.Sencond, g_stTime.Zone);
		//DEBUG_LOG(DEBUG_MODULE_TIME, DLL_DEBUG, "aaaaaaaaa");

		DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"y %d, m %d, d %d, h %d, m %d, s %d, z %d\r\n", g_stTime.Year, g_stTime.Month, g_stTime.Day,g_stTime.Hour, g_stTime.Min, g_stTime.Sencond, g_stTime.Zone);
		MUTEX_UNLOCK(g_ulTimeSem);
		tick_start2 = OppTickCountGet();
	}
#endif
	
	if(NBIOT_CLK == g_ucClkSrc || ALL_CLK == g_ucClkSrc){
		if(BC28_READY != NeulBc28GetDevState()){
			//DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"~~~~~DevState %d~~~~~~~~~\r\n", NeulBc28GetDevState());
			return;
		}
	}
	
	if(!init)
	{
		init = 1;
		tick_start1 = OppTickCountGet();
		DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"~~~~~tick_start1 %lld init %d~~~~~~~~~\r\n", tick_start1, init);
	}
	if ((OppTickCountGet() - tick_start1) >= TimeUpdatePeriodGet()) //two hour
	{
		DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"~~~~~Opple_Nbiot_Time_Task~~~~~~~~~\r\n");
		if(ALL_CLK == g_ucClkSrc)
		{
			ret = ApsDevFeaturesGet(GPS_FEATURE,&support);
			if(OPP_SUCCESS == ret && 1 == support){
				ret = TimeFromGpsClock();
				if(OPP_SUCCESS != ret)
				{
					if(BC28_READY == NeulBc28GetDevState()){
						TimeFromNbiotClockEvent();
						//TimeMakeLog(NBIOT_CLK);
					}
				}else{
					//TimeMakeLog(GPS_CLK);
				}
			}else{
				if(BC28_READY == NeulBc28GetDevState()){
					TimeFromNbiotClockEvent();
				}
			}
		}
		else if(GPS_CLK == g_ucClkSrc)
		{
			ret = ApsDevFeaturesGet(GPS_FEATURE,&support);
			if(OPP_SUCCESS == ret && 1 == support){
				TimeFromGpsClock();
			}
			//TimeMakeLog(GPS_CLK);
		}
		else if(NBIOT_CLK == g_ucClkSrc)
		{
			if(BC28_READY == NeulBc28GetDevState()){
				TimeFromNbiotClockEvent();
				//TimeMakeLog(NBIOT_CLK);
			}
		}
		else
		{
			//DEBUG_LOG(DEBUG_MODULE_TIME, DLL_DEBUG, "y %d, m %d, d %d, h %d, m %d, s %d, z %d\r\n", g_stTime.Year, g_stTime.Month, g_stTime.Day,g_stTime.Hour, g_stTime.Min, g_stTime.Sencond, g_stTime.Zone);
			//printf("y %d, m %d, d %d, h %d, m %d, s %d, z %d\r\n", g_stTime.Year, g_stTime.Month, g_stTime.Day,g_stTime.Hour, g_stTime.Min, g_stTime.Sencond, g_stTime.Zone);
		}
		tick_start1 = OppTickCountGet();
	}
}

void TimeThread(void *pvParameter)
{
	while(1){
		TimeLoop();
	}
}

U32 TimeRestore()
{
	int ret;
	ST_CLKSRC stClkSrc;
	
	TimeSetClockSrc(NBIOT_CLK);
	stClkSrc.clkSrc = NBIOT_CLK;
	ret = TimeSetClkSrcToFlash(&stClkSrc);
	if(OPP_SUCCESS != ret){
		DEBUG_LOG(DEBUG_MODULE_TIME, DLL_INFO,"time restore set clk src to flash fail ret %d\r\n", ret);
		return OPP_FAILURE;
	}

	return OPP_SUCCESS;
}
