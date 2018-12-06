#include "cli-interpreter.h"
#include "SVS_Time.h"
#include "SVS_Location.h"
#include "DEV_Timer.h"

void CommandShowLocation(void);
void CommandShowTime(void);


CommandEntry CommandTablGps[] =
{
	CommandEntryActionWithDetails("location", CommandShowLocation, "", "Show Gps...", (void*)0),
	CommandEntryActionWithDetails("time", CommandShowTime, "", "Show Gps...", (void*)0),
	CommandEntryTerminator()
};

void CommandShowLocation(void)
{
	int ret;
	ST_OPP_LAMP_LOCATION locPara;
	
	//ret = OppLocationFromGps();
	//CLI_PRINTF("CommandShowLocation ret %d\r\n",ret);
	
	LocationRead(GPS_LOC, &locPara);
	CLI_PRINTF("Lat:%Lf, Log:%Lf\r\n", locPara.ldLatitude, locPara.ldLongitude);  //¾­¶È

}

void CommandShowTime(void)
{
	int ret;
	int zone;
	ST_OPPLE_TIME tPara;
	
	/*ret = TimeFromGpsClock();
	CLI_PRINTF("CommandShowTime ret %d\r\n",ret);*/
	
	TimeRead(GPS_CLK, &zone, &tPara);
	CLI_PRINTF("zone:%d Y%04d:M%02d:D%02d-H%02d-M%02d-S%02d\r\n", zone, tPara.uwYear, tPara.ucMon, tPara.ucDay, tPara.ucHour, tPara.ucMin, tPara.ucSec);
	CLI_PRINTF("week:%d\r\n", tPara.ucWDay);

}

