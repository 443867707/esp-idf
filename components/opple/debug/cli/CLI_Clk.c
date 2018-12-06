#include "cli-interpreter.h"
#include "SVS_Time.h"

void CommandSetClkSrc(void);
void CommandGetClkSrc(void);
void CommandSetClkSrcToFlash(void);
void CommandGetClkSrcFromFlash(void);
void CommandSetTime(void);
void CommandSetTimeUTC();
void CommandGetAstTime(void);
void CommandGetTime(void);
void CommandGetTimeNbiot(void);
void CommandUpdatePeriodGet(void);
void CommandUpdatePeriodSet(void);
void CommandGetZone(void);
void CommandHasTimeGet(void);
void CommandHasTimeSet(void);


const char* const CommandArguments_ClkSrc[] = {
"<0.ALL_CLK, 1.GPS_CLK, 2.NBIOT_CLK, 3.LOC_CLK>"
};

const char* const CommandArguments_TimeSet[] = {
"year,month,day,hour,min,second,zone"
};
const char* const CommandArguments_AstSet[] = {
"year","month","day","zone","lat","lng"
};
const char* const CommandArguments_ZoneSet[] = {
"lat","lng"
};

char * clkSrcDesc[] = {
"ALL_CLK",
"GPS_CLK",
"NBIOT_CLK",
"LOC_CLK"
};

CommandEntry CommandTablClk[] =
{
	CommandEntryActionWithDetails("srcSet", CommandSetClkSrc, "u", "set time clk src...", CommandArguments_ClkSrc),
	CommandEntryActionWithDetails("srcGet", CommandGetClkSrc, "", "set time clk src...", NULL),
	CommandEntryActionWithDetails("srcSetFlash", CommandSetClkSrcToFlash, "u", "set time clk src...", CommandArguments_ClkSrc),
	CommandEntryActionWithDetails("srcGetFlash", CommandGetClkSrcFromFlash, "", "set time clk src...", NULL),
	CommandEntryActionWithDetails("timeSet", CommandSetTime, "s", "set time...", CommandArguments_TimeSet),
	CommandEntryActionWithDetails("utcSet", CommandSetTimeUTC, "s", "set utctime...", CommandArguments_TimeSet),
	CommandEntryActionWithDetails("astGet", CommandGetAstTime, "uuuuss", "get ast time...", CommandArguments_AstSet),
	CommandEntryActionWithDetails("zoneGet", CommandGetZone, "ss", "get zone...", CommandArguments_ZoneSet),
	CommandEntryActionWithDetails("time", CommandGetTime, "", "get time...", NULL),
	CommandEntryActionWithDetails("nbiot", CommandGetTimeNbiot, "", "get nbiot time...", NULL),
	CommandEntryActionWithDetails("updatePeriodGet", CommandUpdatePeriodGet, "", "get time update period...", NULL),
	CommandEntryActionWithDetails("updatePeriodSet", CommandUpdatePeriodSet, "u", "set time update period...", NULL),
	CommandEntryActionWithDetails("hasTimeGet", CommandHasTimeGet, "", "device has get time...", NULL),
	CommandEntryActionWithDetails("hasTimeSet", CommandHasTimeSet, "u", "set device get time flag...", NULL),
	
	CommandEntryTerminator()
};

void CommandSetClkSrc(void)
{
	int ret;

	if(CliIptArgList[0][0] < ALL_CLK || CliIptArgList[0][0] > LOC_CLK){
		CLI_PRINTF("input error\r\n");
		return;
	}

	TimeSetClockSrc(CliIptArgList[0][0]);
}

void CommandGetClkSrc(void)
{
	CLI_PRINTF("clk src:%s", clkSrcDesc[TimeGetClockSrc()]);
}

void CommandSetClkSrcToFlash(void)
{
	int ret;
	ST_CLKSRC stClkSrc;
	
	if(CliIptArgList[0][0] < ALL_CLK || CliIptArgList[0][0] > LOC_CLK){
		CLI_PRINTF("input error\r\n");
		return;
	}
	stClkSrc.clkSrc = CliIptArgList[0][0];
	ret = TimeSetClkSrcToFlash(&stClkSrc);
	if(0 != ret){
		CLI_PRINTF("TimeSetClkSrcToFlash fail ret %d\r\n", ret);
	}
}

void CommandGetClkSrcFromFlash(void)
{
	int ret;
	ST_CLKSRC stClkSrc;

	ret = TimeGetClkSrcFromFlash(&stClkSrc);
	if(0 != ret){
		CLI_PRINTF("TimeGetClkSrcFromFlash fail ret %d\r\n", ret);
		return;
	}
	CLI_PRINTF("clk src:%s", clkSrcDesc[stClkSrc.clkSrc]);
}

void CommandSetTime(void)
{
	int ret;
	U8 year,month,day,hour,min,second;
	S8 zone;

	/*year = CliIptArgList[0][0];
	month = CliIptArgList[1][0];
	day = CliIptArgList[2][0];
	hour = CliIptArgList[3][0];
	min = CliIptArgList[4][0];
	second = CliIptArgList[5][0];
	zone = CliIptArgList[6][0];*/
	ret = sscanf(&CliIptArgList[0][0],"%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd",&year,&month,&day,&hour,&min,&second,&zone);
	if(7 != ret){
		CLI_PRINTF("date formate should be \"year,month,day,hour,min,second,zone\"\r\n");
		return;
	}
	CLI_PRINTF("y %d m %d d %d h %d m %d s %d z %d\r\n", year, month, day, hour, min, second, zone);
	if(month > 12 || day > 31 || hour > 23 || min > 60 || second > 60 || zone > 12 || zone < -12){
		CLI_PRINTF("time is invalide\r\n");
		return OPP_FAILURE;
	}
	
	ret = TimeSet(year,month,day,hour,min,second,zone);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("TimeSet fail ret %d\r\n", ret);
	}
}

/*zone need muti 4*/
void CommandSetTimeUTC()
{
	int ret;
	U8 year,month,day,hour,min,second;
	S8 zone;

	/*year = CliIptArgList[0][0];
	month = CliIptArgList[1][0];
	day = CliIptArgList[2][0];
	hour = CliIptArgList[3][0];
	min = CliIptArgList[4][0];
	second = CliIptArgList[5][0];
	zone = CliIptArgList[6][0];*/

	ret = sscanf(&CliIptArgList[0][0],"%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd",&year,&month,&day,&hour,&min,&second,&zone);
	if(7 != ret){
		CLI_PRINTF("date formate should be \"year,month,day,hour,min,second,zone\"\r\n");
		return;
	}
	
	CLI_PRINTF("y %d m %d d %d h %d m %d s %d z %d\r\n", year, month, day, hour, min, second, zone);
	if(month > 12 || day > 31 || hour > 23 || min > 60 || second > 60 || zone > 48 || zone < -48){
		CLI_PRINTF("time is invalide\r\n");
		return OPP_FAILURE;
	}
	TimeSetCallback(year,month,day,hour,min,second,zone);

}

void CommandGetAstTime(void)
{
	ST_AST stAstTime;
	int year,month,day;
	char zone;
	long double glat,glong;

	year = CliIptArgList[0][0];
	month = CliIptArgList[1][0];
	day = CliIptArgList[2][0];
	zone = CliIptArgList[3][0];
	glat = atof(CliIptArgList[4]);
	glong = atof(CliIptArgList[5]);

	Opple_Calc_Ast_Time(year,month,day,zone,glat,glong,&stAstTime);
	CLI_PRINTF("set:%02d:%02d:%02d, rise:%02d:%02d:%02d\r\n", stAstTime.setH,stAstTime.setM,stAstTime.setS,stAstTime.riseH,stAstTime.riseM,stAstTime.riseS);
}

void CommandGetZone(void)
{
	long double glat,glong;

	CLI_PRINTF("lat:%s, log:%s\r\n",CliIptArgList[0],CliIptArgList[1]);
	glat = atof(CliIptArgList[0]);
	glong = atof(CliIptArgList[1]);	
	CLI_PRINTF("lat:%Lf, log:%Lf\r\n",glat,glong);
	
	CLI_PRINTF("zone1:%d\r\n",calculateTimezone(glat,glong));
	CLI_PRINTF("zone2:%d\r\n",Zone(glong));
}
void CommandGetTime(void)
{
	ST_TIME time;

	Opple_Get_RTC(&time);
	if(time.Zone >= 0)
		CLI_PRINTF("time:%04d-%02d-%02d %02d:%02d:%02d+%02d\r\n", time.Year, time.Month, time.Day, time.Hour, time.Min, time.Sencond, time.Zone);
	else
		CLI_PRINTF("time:%04d-%02d-%02d %02d:%02d:%02d-%02d\r\n", time.Year, time.Month, time.Day, time.Hour, time.Min, time.Sencond, abs(time.Zone));
}
void CommandGetTimeNbiot(void)
{
	TimeFromNbiotClockEvent();
}

void CommandUpdatePeriodGet(void)
{
	CLI_PRINTF("time update period:%u\r\n",TimeUpdatePeriodGet());

}

void CommandUpdatePeriodSet(void)
{
	TimeUpdatePeriodSet(CliIptArgList[0][0]);

}

void CommandHasTimeGet(void)
{
	CLI_PRINTF("hasTime:%u\r\n",timeRecvTimeGet());

}

void CommandHasTimeSet(void)
{
	TimeRecvTimeSet(CliIptArgList[0][0]);
}


