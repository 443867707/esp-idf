#include "cli-interpreter.h"
#include "SVS_Location.h"
#include "stdlib.h"

void CommandSetLocSrc(void);
void CommandGetLocSrc(void);
void CommandSetLocSrcToFlash(void);
void CommandGetLocSrcFromFlash(void);
void CommandGetLocCfgFromFlash(void);
void CommandSetLocCfgToFlash(void);
void CommandGetLocCfg(void);
void CommandSetLocCfg(void);
void CommandGetLoc(void);
void CommandGetLocDir(void);


const char* const CommandArguments_LocSrc[] = {
"<0.ALL_LOC, 1.GPS_LOC, 2.NBIOT_LOC, 3.CFG_LOC>"
};
const char* const CommandArguments_LocCfg[] = {
"lat, dd.ddddd","lng, ddd.dddddd"
};

char * locSrcDesc[]={
"ALL_LOC",
"GPS_LOC",
"NBIOT_LOC",
"CFG_LOC"
};

CommandEntry CommandTablLoc[] =
{
	CommandEntryActionWithDetails("srcSet", CommandSetLocSrc, "u", "set loc src...", CommandArguments_LocSrc),
	CommandEntryActionWithDetails("srcGet", CommandGetLocSrc, "", "get loc src...", NULL),
	CommandEntryActionWithDetails("srcSetFlash", CommandSetLocSrcToFlash, "u", "set loc src to flash...", CommandArguments_LocSrc),
	CommandEntryActionWithDetails("srcGetFlash", CommandGetLocSrcFromFlash, "", "get loc src from flash...", NULL),
	CommandEntryActionWithDetails("setCfgLoc", CommandSetLocCfg, "ss", "set loc cfg to flash...", CommandArguments_LocCfg),
	CommandEntryActionWithDetails("getCfgLoc", CommandGetLocCfg, "", "get loc cfg from flash...", NULL),
	CommandEntryActionWithDetails("setCfgLocFlash", CommandSetLocCfgToFlash, "ss", "set loc cfg to flash...", CommandArguments_LocCfg),
	CommandEntryActionWithDetails("getCfgLocFlash", CommandGetLocCfgFromFlash, "", "get loc cfg from flash...", NULL),
	CommandEntryActionWithDetails("get", CommandGetLoc, "u", "get loc...", CommandArguments_LocSrc),
	CommandEntryActionWithDetails("getLoc", CommandGetLocDir, "", "get loc directly...", NULL),

	CommandEntryTerminator()
};

void CommandSetLocSrc(void)
{	
	if(CliIptArgList[0][0] < ALL_LOC || CliIptArgList[0][0] > CFG_LOC){
		CLI_PRINTF("input error\r\n");
		return;
	}
	LocSetSrc(CliIptArgList[0][0]);
}

void CommandGetLocSrc(void)
{
	CLI_PRINTF("%s\r\n", locSrcDesc[LocGetSrc()]);
}

void CommandSetLocSrcToFlash(void)
{
	int ret;
	ST_LOCSRC stLocSrc;
	
	if(CliIptArgList[0][0] < ALL_LOC || CliIptArgList[0][0] > CFG_LOC){
		CLI_PRINTF("input error\r\n");
		return;
	}
	stLocSrc.locSrc = CliIptArgList[0][0];

	ret = LocSetSrcToFlash(&stLocSrc);
	if(0 != ret){
		CLI_PRINTF("LocSetSrcToFlash fail ret %d\r\n", ret); 
	}
}
void CommandGetLocSrcFromFlash(void)
{
	int ret;
	ST_LOCSRC stLocSrc;

	ret = LocSrcGetFromFlash(&stLocSrc);
	if(0 != ret){
		CLI_PRINTF("LocSrcGetFromFlash fail ret %d\r\n", ret);
		return;
	}
	CLI_PRINTF("locSrc %s\r\n", locSrcDesc[stLocSrc.locSrc]);
}

void CommandGetLocCfgFromFlash(void)
{
	int ret;
	ST_OPP_LAMP_LOCATION stLoc;

	ret = LocCfgGetFromFlash(&stLoc);
	if(0 != ret){
		CLI_PRINTF("CommandGetLocSrcFromFlash fail ret %d\r\n", ret);
		return;
	}
	CLI_PRINTF("lat:%lf, lng:%lf\r\n", stLoc.ldLatitude, stLoc.ldLongitude);
}
void CommandSetLocCfgToFlash(void)
{
	int ret;
	ST_OPP_LAMP_LOCATION stLoc;

	stLoc.ldLatitude = atof(&CliIptArgList[0][0]);
	stLoc.ldLongitude = atof(&CliIptArgList[1][0]);
	//stLoc.ldLatitude = CliIptArgList[0][0];
	//stLoc.ldLongitude = CliIptArgList[1][0];
	if(stLoc.ldLatitude == INVALIDE_LAT && stLoc.ldLongitude == INVALIDE_LNG){
		CLI_PRINTF("loc is invalide\r\n");
		return;
	}
	
	ret = LocCfgSetToFlash(&stLoc);
	if(0 != ret){
		CLI_PRINTF("LocCfgSetToFlash fail ret %d\r\n", ret);
	}
}

void CommandGetLocCfg(void)
{
	long double lat, lng;

	LocGetCfgLat(&lat);
	LocGetCfgLng(&lng);
	CLI_PRINTF("lat:%lf, lng:%lf\r\n", lat, lng);
}

void CommandSetLocCfg(void)
{
	long double lng;
	long double lat;
	
	lat = atof(&CliIptArgList[0][0]);
	lng = atof(&CliIptArgList[1][0]);
	if(lat == INVALIDE_LAT && lng == INVALIDE_LNG){
		CLI_PRINTF("loc is invalide\r\n");
		return;
	}
	LocSetCfgLat(lat);
	LocSetCfgLng(lng);
}


void CommandGetLocDir(void)
{
	int ret;
	long double lat,lng;

	ret = LocGetLat(&lat);
	if(0 != ret){
		CLI_PRINTF("LocGetLat fail ret %d\r\n", ret);
		return;
	}
	ret = LocGetLng(&lng);
	if(0 != ret){
		CLI_PRINTF("LocGetLng fail ret %d\r\n", ret);
		return;
	}
	CLI_PRINTF("lat:%lf, lng:%lf\r\n", lat, lng);
}

void CommandGetLoc(void)
{
	int ret;
	ST_OPP_LAMP_LOCATION stLocationPara;

	ret = LocationRead(CliIptArgList[0][0], &stLocationPara);
	if(0 != ret){
		//CLI_PRINTF("LocationRead fail ret %d\r\n", ret);
		//return;
	}
	CLI_PRINTF("lat:%lf, lng:%lf\r\n", stLocationPara.ldLatitude, stLocationPara.ldLongitude);
}


