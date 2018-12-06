#include "cli-interpreter.h"
#include <string.h>
#include "APP_LampCtrl.h"
#include "DRV_Bc28.h"
#include "SVS_WiFi.h"
#include "SVS_Lamp.h"
#include "SVS_Elec.h"
#include "DRV_LightSensor.h"
#include "DRV_Gps.h"
#include "DRV_Nb.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "DRV_DimCtrl.h"
#include "DRV_SpiMeter.h"
#include "SVS_Test.h"


void Command_mcuinit(void);
void Command_version(void);
void Command_sdkversion(void);
void Command_bc28Version(void);
void Command_bc28IMEI(void);
void Command_wifiMac(void);
void Command_UsimICCID(void);
void Command_Relay(void);
void Command_pwm(void);
void Command_ana(void);
void Command_Ptt(void);
void Command_bc28Reset(void);
void Command_bc28RegInfo(void);
void Command_L80Reset(void);
void Command_meter(void);
void Command_ps(void);
void Command_anaLevel(void);
void Command_anaCali(void);
void Command_anaCaliRestore(void);
void Command_bc28ResetStart(void);
void Command_bc28ResetResult(void);
void Command_anaVolt(void);
void Command_nbPid(void);
void Command_PttVolt(void);
void Command_bc28regset(void);
void Command_bc28ResetReadImd(void);
void Command_FCTInfoWrite(void);
void Command_nbset(void);
void Command_UsimICCIDImd(void);

#define BC28_EVENT_WAIT_MS  500  // 500,EVENT_WAIT_DEFAULT

const char* const CommandArguments_pwm[]={
	"level 0-1000(0:0%,1000:100%)"
};
const char* const CommandArguments_bc28regset[]={
	"chl(2505,2509...)"
};
const char* const CommandArguments_nbset[]={
	"chl(2505,2509...)",
	"band(5,28,20,8,3,1)"
};


static int gTestInit=1;

CommandEntry CommandTablTest[] =
{
	CommandEntryActionWithDetails("mcuInit", Command_mcuinit, "", "", (void*)0),
	CommandEntryActionWithDetails("version", Command_version, "", "", (void*)0),
	CommandEntryActionWithDetails("sdkversion", Command_sdkversion, "", "", (void*)0),
	CommandEntryActionWithDetails("bc28Version", Command_bc28Version, "", "", (void*)0),
	CommandEntryActionWithDetails("nbPid", Command_nbPid, "", "", (void*)0),
	CommandEntryActionWithDetails("bc28IMEI", Command_bc28IMEI, "", "", (void*)0),
	CommandEntryActionWithDetails("wifiMac", Command_wifiMac, "", "", (void*)0),
	CommandEntryActionWithDetails("UsimICCID", Command_UsimICCID, "", "", (void*)0),
	CommandEntryActionWithDetails("UsimICCIDImd", Command_UsimICCIDImd, "", "", (void*)0),
	CommandEntryActionWithDetails("Relay", Command_Relay, "u", "", (void*)0),
	CommandEntryActionWithDetails("pwm", Command_pwm, "u", "", CommandArguments_pwm),
	CommandEntryActionWithDetails("ana", Command_ana, "u", "", CommandArguments_pwm),
	CommandEntryActionWithDetails("Ptt", Command_Ptt, "", "", (void*)0),
	CommandEntryActionWithDetails("PttVolt", Command_PttVolt, "", "", (void*)0),
	CommandEntryActionWithDetails("bc28Reset", Command_bc28Reset, "", "", (void*)0),
	CommandEntryActionWithDetails("bc28ResetReadImd", Command_bc28ResetReadImd, "", "", (void*)0),
	CommandEntryActionWithDetails("bc28RegSet", Command_bc28regset, "u", "", CommandArguments_bc28regset),
	CommandEntryActionWithDetails("nbSet", Command_nbset, "uu", "", CommandArguments_nbset),
	CommandEntryActionWithDetails("bc28ResetStart", Command_bc28ResetStart, "", "", (void*)0),
	CommandEntryActionWithDetails("bc28ResetResult", Command_bc28ResetResult, "", "", (void*)0),
	CommandEntryActionWithDetails("bc28RegInfo", Command_bc28RegInfo, "", "", (void*)0),
	CommandEntryActionWithDetails("L80Reset", Command_L80Reset, "", "u", (void*)0),
	CommandEntryActionWithDetails("meter", Command_meter, "", "", (void*)0),
	CommandEntryActionWithDetails("ps", Command_ps, "", "", (void*)0),
	CommandEntryActionWithDetails("anaLevel", Command_anaLevel, "u", "", (void*)0),
	CommandEntryActionWithDetails("anaVolt", Command_anaVolt, "u", "", (void*)0),
	CommandEntryActionWithDetails("anaCali", Command_anaCali, "uu", "", (void*)0),
	CommandEntryActionWithDetails("anaCaliRestore", Command_anaCaliRestore, "", "", (void*)0),
	CommandEntryActionWithDetails("FCTinfoWrite", Command_FCTInfoWrite, "u", "", (void*)0),
	
	CommandEntryTerminator()
};

void Command_mcuinit(void) // OK
{
	int res1,res2,init=0;

	if(gTestInit == 0)
	{
		CLI_PRINTF("[Test mcuInit]FAIL,0x100\r\n");
		return;
	}
	
	res1 = SvsTestCurSet(TEST_FCT);
	res2 = SvsTestInfoSet(TEST_FCT,TEST_RES_UNKNOW);
	if(res1 == 0 && res2 == 0)
	{
		CLI_PRINTF("[Test mcuInit]OK\r\n");
	}
	else
	{
		CLI_PRINTF("[Test mcuInit]FAIL,0x%0x\r\n",res1*16+res2);
	}
}

void Command_version(void) // OK
{
	CLI_PRINTF("[Test version]SKU:0x%04X,CLASS:0x%04X,VERSION:%d\r\n",\
	PRODUCTSKU,PRODUCTCLASS,OPP_LAMP_CTRL_CFG_DATA_VER);
}

void Command_sdkversion(void)
{
	CLI_PRINTF("[Test sdkversion]%s\r\n",esp_get_idf_version());
}

void Command_bc28Version(void) // OK
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(VER_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(VER_EVENT,&rArgc,rArgv,BC28_EVENT_WAIT_MS);
	if(0 != ret){
		CLI_PRINTF("[Test bc28Version]FAIL,%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}

	CLI_PRINTF("[Test bc28Version]%s\r\n",rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void Command_nbPid(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	char id[100]={0};
	U8 len=0;
	
	ret = sendEvent(PID_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(PID_EVENT,&rArgc,rArgv,BC28_EVENT_WAIT_MS);
	if(0 != ret){
		CLI_PRINTF("[Test nbPid]FAIL,%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	for(int i=0;i<strlen(rArgv[0]);i++)
	{
		if(rArgv[0][i] == '\r' || rArgv[0][i] == '\n' )
		{
			id[len++] = '.';
		}
		else
		{
			id[len++] = (rArgv[0][i]);
		}
		if(len >= 90) {
			break;
		}
	}
	id[len] = '\0';
	CLI_PRINTF("[Test nbPid]%s\r\n",id);
	ARGC_FREE(rArgc,rArgv);
}

void Command_wifiMac(void) // OK
{
	unsigned char mac[10];
	char s[100];
	
	int res;

	res = ApsGetWifiMac(mac);
	if(res != 0)
	{
		CLI_PRINTF("[Test wifiMac]FAIL,%d\r\n",res);
	}
	else
	{
		snprintf(s,100,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		CLI_PRINTF("[Test wifiMac]%s\r\n",s);
	}
	
}

void Command_bc28IMEI(void) // OK
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(IMEI_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(IMEI_EVENT,&rArgc,rArgv,BC28_EVENT_WAIT_MS);
	if(0 != ret){
		CLI_PRINTF("[Test bc28IMEI]FAIL,%d\r\n",ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	CLI_PRINTF("[Test bc28IMEI]%s\r\n",rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void Command_UsimICCID(void) // OK
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(ICCID_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(ICCID_EVENT,&rArgc,rArgv,BC28_EVENT_WAIT_MS);
	if(0 != ret){
		CLI_PRINTF("[Test UsimICCID]FAIL,%d\r\n",ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	CLI_PRINTF("[Test UsimICCID]%s\r\n",rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void Command_UsimICCIDImd(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(ICCIDIMM_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(ICCIDIMM_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("[Test UsimICCIDImd]FAIL,%d\r\n",ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}

	CLI_PRINTF("[Test UsimICCIDImd]%s\r\n", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void Command_Relay(void) // OK
{
	LampOnOff(TEST_SRC,CliIptArgList[0][0]);
	CLI_PRINTF("[Test Relay][%d]OK\r\n",CliIptArgList[0][0]);
}

void Command_pwm(void) // OK
{
	LampOuttypeSet(1);
	LampDim(TEST_SRC,CliIptArgList[0][0]*10);
	CLI_PRINTF("[Test pwm][%d]OK\r\n",CliIptArgList[0][0]);
}

void Command_ana(void) // OK
{
	LampOuttypeSet(0);
	LampDim(TEST_SRC,CliIptArgList[0][0]*10);
	CLI_PRINTF("[Test ana][%d]OK\r\n",CliIptArgList[0][0]);
}

void Command_Ptt(void) // OK
{
	U32 res,lx;

	res = BspLightSensorLuxGet(&lx);
	if(res != 0){
		CLI_PRINTF("[Test Ptt]FAIL,%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("[Test Ptt]%u\r\n",lx);
	}
}

void Command_PttVolt(void)
{
	U32 res,volt;

	res = BspLightSensorVoltageGet(&volt);
	if(res != 0){
		CLI_PRINTF("[Test PttVolt]FAIL,%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("[Test PttVolt]%u\r\n",volt);
	}
}

void Command_bc28Reset(void) // OK
{
	int res;

	res = NbiotHardReset(5000);
	//res = NbiotHardReset(CliIptArgList[0][0]);
	CLI_PRINTF("[Test bc28Reset]%s\r\n",res==0?"FAIL":"OK");
}

void Command_bc28ResetStart(void)
{
	int res;
	res = NbiotHardResetStart();
	if(res!=0)
	{
		CLI_PRINTF("[Test bc28ResetStart]FAIL,%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("[Test bc28ResetStart]OK\r\n");
	}
	//CLI_PRINTF("[Test bc28ResetStart]%s\r\n",res==0?"OK":"FAIL");
}

void Command_bc28ResetResult(void)
{
	int result;
	result = NbiotHardResetResult();
	CLI_PRINTF("[Test bc28ResetResult]%d\r\n",result);
}

void Command_bc28ResetReadImd(void)
{
	int res;
	res = NeulBc28HardRestResultGet();
	if(res == 0) CLI_PRINTF("[Test bc28ResetReadImd]%d\r\n",1);
	else if(res == 1) CLI_PRINTF("[Test bc28ResetReadImd]%d\r\n",0);
	else CLI_PRINTF("[Test bc28ResetReadImd]%d\r\n",res);
}

void Command_bc28regset(void)
{
	int res;
	res = NeulBc28SetEarfcnToRam(1,CliIptArgList[0][0]);
	CLI_PRINTF("[Test bc28RegSet][%d]%s\r\n",CliIptArgList[0][0],res==0?"OK":"FAIL");
}

// Test nbSet 2505 5
void Command_nbset(void)
{
	int res;
	res = NeulBc28SetParaToRam(1,CliIptArgList[0][0],CliIptArgList[1][0]);
	CLI_PRINTF("[Test nbSet][%d %d]%s\r\n",CliIptArgList[0][0],CliIptArgList[1][0],res==0?"OK":"FAIL");
}

void Command_bc28RegInfo(void) // OK
{
	ST_NEUL_DEV *pstNeulDev = NULL;
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;
	
	ret = sendEvent(UESTATE_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(UESTATE_EVENT,&rArgc,rArgv,BC28_EVENT_WAIT_MS);
	if(0 != ret){
		CLI_PRINTF("[Test bc28RegInfo]FAIL,%d\r\n",ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	pstNeulDev = (ST_NEUL_DEV *)rArgv[0];
	CLI_PRINTF("[Test bc28RegInfo]snr:%d,rsrp:%d,txpower:%d,did:%d,chl:%d\r\n",\
	pstNeulDev->snr,pstNeulDev->rsrp,pstNeulDev->txPower,pstNeulDev->cellId,pstNeulDev->earfcn);
	ARGC_FREE(rArgc,rArgv);
}

void Command_L80Reset(void) // OK
{
	int res;

	res = L80Reset();
	CLI_PRINTF("[Test L80Reset]%s\r\n",res==0?"OK":"FAIL");
}

void Command_meter(void)  // OK
{
	int res;
	//ST_OPP_LAMP_CURR_ELECTRIC_INFO m;
	meter_info_t m;
	
	//res = ElecGetElectricInfo(&m);
	res = MeterInfoGet(&m);
	if(res!=0)
	{
		CLI_PRINTF("[Test meter]FAIL,%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("[Test meter]cur:%u,volt:%u,pow:%u,pf:%u\r\n",\
		m.u32Current,m.u32Voltage,m.u32Power,m.u32PowerFactor);
	}
}

void Command_ps(void)
{
	CLI_PRINTF("[Test ps]OK\r\n");
	gTestInit = 0;
	NbSetRstGpioLevel(1);
	GpsSetRstGpioLevel(1);
	OS_DELAY_MS(1000);
	
    //rtc_gpio_hold_en(GPIO_NUM_33);
    //gpio_hold_en(GPIO_NUM_21);
    
	esp_wifi_stop();
	//esp_deep_sleep((U64)1000000*60*60);
}
void Command_anaVolt(void)
{
	LampOuttypeSet(0);
	LampDim(TEST_SRC,CliIptArgList[0][0]);
	CLI_PRINTF("[Test anaVolt][%d]OK\r\n",CliIptArgList[0][0]);
}
void Command_anaLevel(void)
{
	int res;
    
	LampOuttypeSet(0);
	if(CliIptArgList[0][0] == 1)
	{
		res = DimDacRawLevelSet(64);
	}
	else if (CliIptArgList[0][0] == 2)
	{
		res = DimDacRawLevelSet(192);
	}
	else
	{
		res = 1;
	}

	if(res!=0)
	{
		CLI_PRINTF("[Test anaLevel][%d]FAIL,%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("[Test anaLevel][%d]OK\r\n",CliIptArgList[0][0]);
	}
	
	//CLI_PRINTF("[Test anaLevel][%d]%s\r\n",CliIptArgList[0][0],res==0?"OK":"FAIL");
}
void Command_anaCali(void)
{
	int res1,res2;

	res1 = DacLevel64ParamSet(CliIptArgList[0][0]);
	res2 = DacLevel192ParamSet(CliIptArgList[1][0]);

	if(res1 ==0 && res2 == 0)
	{
		CLI_PRINTF("[Test anaCali][%d %d]OK\r\n",\
		CliIptArgList[0][0],CliIptArgList[1][0]);
	}
	else
	{
		CLI_PRINTF("[Test anaCali][%d %d]FAIL,%d\r\n",\
		CliIptArgList[0][0],CliIptArgList[1][0],res1*100+res2);
	}
}
void Command_anaCaliRestore(void)
{
	int res;

	res = delDacCalParam();
	if(res!=0)
	{
		CLI_PRINTF("[Test anaCaliRestore]FAIL,%d\r\n",res);
	}
	else
	{
		CLI_PRINTF("[Test anaCaliRestore]OK\r\n");
	}
	//CLI_PRINTF("[Test anaCaliRestore]%s\r\n",res==0?"OK":"FAIL");
}

void Command_FCTInfoWrite(void)
{
	int res;

	res = SvsTestInfoSet(TEST_FCT,CliIptArgList[0][0]);
	if(res!=0)
	{
		CLI_PRINTF("[Test FCTinfoWrite][%d]FAIL,%d\r\n",CliIptArgList[0][0],res);
	}
	else
	{
		CLI_PRINTF("[Test FCTinfoWrite][%d]OK\r\n",CliIptArgList[0][0]);
	}
}




