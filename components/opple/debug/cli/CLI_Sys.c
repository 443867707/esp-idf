#include "cli-interpreter.h"
#include <esp_system.h>
#include "esp_spi_flash.h"
#include <APP_LampCtrl.h>
#include <SVS_Config.h>
#include <DRV_Led.h>
#include <DRV_Bc28.h>
#include <APP_Daemon.h>
#include <SVS_Elec.h>
#include <SVS_Coap.h>
#include <SVS_Log.h>
#include <string.h>
#include "SVS_Lamp.h"
#include "opple_main.h"
#include "esp_wifi.h"
#include "SVS_WiFi.h"
#include "SVS_Udp.h"
#include "SVS_Ota.h"

extern esp_err_t print_panic_saved(int core_id, char *outBuf);
extern esp_err_t print_panic_occur_saved(int8_t *occur);
extern esp_err_t erase_panic();

void CommandMemGet(void);
void CommandVersionGet(void);
void CommandDevCapGet(void);
void CommandDevCapGetFlash(void);
void CommandDevCapSet(void);
void CommandLedSet(void);
void CommandSysRestore(void);
void CommandSysPktGet(void);
void CommandSysInfo(void);
void CommandSysTask(void);
void CommandLogGet(void);
void CommandLogStatusGet(void);
void CommandSysModeDescSet(void);
void CommandHardInfo(void);
void CommandLampInfo(void);
void CommandUdpInfo(void);
void CommandSysGetBackTrace(void);
void CommandSysEraseBackTrace(void);
void CommandSysOtaProg(void);

extern ST_BC28_PKT   g_stBc28Pkt;
extern ST_COAP_PKT g_stCoapPkt;
extern char * clkSrcDesc[];
extern char * locSrcDesc[];

//extern int recv_num;
const char* const Arguments_cap[] = {
  "para<0>:nb,para<1>:wifi,para<2>:gps,para<3>:meter,para<4>:sensor,para<5>:rs485,para<6>:light",
};

const char* const Arguments_logDesc[] = {
  "start logSaveId",
  "end logSaveId",
};

char * weekDesc[] = {"monday","tuesday","wednesday","thursday","friday","saturday","sunday"};

CommandEntry CommandTableSys[] =
{
	CommandEntryActionWithDetails("mem", CommandMemGet, "", "get mem status...", NULL),
	CommandEntryActionWithDetails("version", CommandVersionGet, "", "get dev version...", NULL),
	CommandEntryActionWithDetails("capGet", CommandDevCapGet, "", "get dev cap...", NULL),
	CommandEntryActionWithDetails("capFlashGet", CommandDevCapGetFlash, "", "get dev cap from flash...", NULL),
	CommandEntryActionWithDetails("capSet", CommandDevCapSet, "uuuuuuu", "set dev cap...", Arguments_cap),
	CommandEntryActionWithDetails("led", CommandLedSet, "u", "set nb led onoff...", NULL),
	CommandEntryActionWithDetails("restore", CommandSysRestore, "", "sys restore...", NULL),
	CommandEntryActionWithDetails("pkt", CommandSysPktGet, "", "sys pkt info...", NULL),
	CommandEntryActionWithDetails("info", CommandSysInfo, "", "sys info...", NULL),
	CommandEntryActionWithDetails("task", CommandSysTask, "", "sys task...", NULL),
	CommandEntryActionWithDetails("log", CommandLogGet, "uu", "log get...", Arguments_logDesc),
	CommandEntryActionWithDetails("logStatus", CommandLogStatusGet, "", "get latest log saveid...", NULL),
	CommandEntryActionWithDetails("hwInfo", CommandHardInfo, "", "sys hardware info...", NULL),
	CommandEntryActionWithDetails("lampInfo", CommandLampInfo, "", "sys lamp info...", NULL),
	CommandEntryActionWithDetails("sysModeCntDescSet", CommandSysModeDescSet, "", "sys mode cnt desc set...", NULL),
	CommandEntryActionWithDetails("udpInfo", CommandUdpInfo, "", "sys udp info...", NULL),
	CommandEntryActionWithDetails("btGet", CommandSysGetBackTrace, "", "sys panic backtrace...", NULL),
	CommandEntryActionWithDetails("btErase", CommandSysEraseBackTrace, "", "sys panic backtrace...", NULL),
	CommandEntryActionWithDetails("otaProg", CommandSysOtaProg, "", "ota progress...", NULL),

	CommandEntryTerminator()
};

void CommandSysModeDescSet(void)
{
	int res;
	res = sysModeDecreaseSet();
	if(res!=0) CLI_PRINTF("FAIL,res=%d\r\n",res);
}

void CommandMemGet(void)
{
	CLI_PRINTF("%d\n", esp_get_free_heap_size());

}

void CommandVersionGet(void)
{
    unsigned int Temp_ProductClass = PRODUCTCLASS;
    unsigned int Temp_ProductSku = PRODUCTSKU;
    unsigned int Temp_Ver = OPP_LAMP_CTRL_CFG_DATA_VER;
    
    CLI_PRINTF("\r\n  ***** class%08x sku%08x ver:%d %s %s****** \r\n", Temp_ProductClass,Temp_ProductSku, Temp_Ver, __DATE__, __TIME__);

}

void CommandDevCapGet(void)
{
	//char buffer[100] = {'\0'};
	ST_DEV_CONFIG stDevConfig;
	
	ApsDevConfigRead(&stDevConfig);
	CLI_PRINTF("nb:%d,wifi:%d,gps:%d,meter:%d,sensor:%d,rs485:%d,light:%d\r\n", 
		stDevConfig.nb, stDevConfig.wifi, stDevConfig.gps,
		stDevConfig.meter, stDevConfig.sensor, stDevConfig.rs485, stDevConfig.light);
}

void CommandDevCapGetFlash(void)
{
	int ret;
	//char buffer[100] = {'\0'};
	ST_DEV_CONFIG stDevConfig;
	
	ret = ApsDevConfigReadFromFlash(&stDevConfig);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("ApsDevConfigReadFromFlash ret %d\r\n", ret);
		return;
	}
	
	CLI_PRINTF("nb:%d,wifi:%d,gps:%d,meter:%d,sensor:%d,rs485:%d,light:%d\r\n", 
		stDevConfig.nb, stDevConfig.wifi, stDevConfig.gps,
		stDevConfig.meter, stDevConfig.sensor, stDevConfig.rs485, stDevConfig.light);
}

void CommandDevCapSet(void)
{
	int ret;
	ST_DEV_CONFIG stDevConfig;
	stDevConfig.nb = CliIptArgList[0][0];
	stDevConfig.wifi = CliIptArgList[1][0]; 
	stDevConfig.gps = CliIptArgList[2][0];
	stDevConfig.meter = CliIptArgList[3][0];
	stDevConfig.sensor = CliIptArgList[4][0]; 
	stDevConfig.rs485 = CliIptArgList[5][0]; 
	stDevConfig.light = CliIptArgList[6][0];
	ret = ApsDevConfigWriteToFlash(&stDevConfig);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("ApsDevConfigWriteToFlash err ret %d\r\n",ret);
	}

	ApsDevConfigWrite(&stDevConfig);
}
void CommandLedSet(void)
{
	CLI_PRINTF("CommandLedSet %d\r\n",CliIptArgList[0][0]);

	if(0 == CliIptArgList[0][0]){
		NB_Status_Led_Off();
	}
	else if(1 == CliIptArgList[0][0]){
		NB_Status_Led_On();
	}
}

void CommandSysRestore(void)
{
	int ret = 0;
	//ST_RUNTIME_CONFIG stRuntime;
	ST_ELEC_CONFIG stElecConfig;
	
	//restore dimmer
	ret = OppLampCtrlRestoreFactory();
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("lampctrl restore factory fail ret %d\r\n", ret);
	}
	//restore runtime
	/*stRuntime.hisTime = 0;
	ret = OppLampCtrlSetRuntimeParaToFlash(&stRuntime);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("runtime restore factory fail ret %d\r\n", ret);
	}*/
	//restore runtime and consumption
	stElecConfig.hisTime = 0;
	stElecConfig.hisConsumption = 0;
	stElecConfig.hisLightTime = 0;
	ret = ElecWriteFlash(&stElecConfig);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("elec restore factory fail ret %d\r\n", ret);
	}
	ElecHisConsumptionSet(0);
	OppLampCtrlSetRtime(0,0);
	OppLampCtrlSetHtime(0,0);	
	OppLampCtrlSetLtime(0,0);
	OppLampCtrlSetHLtime(0,0);	
	//restore nbiot
	ret = NeulBc28RestoreFactory();
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("nbiot restore factory fail ret %d\r\n", ret);
	}
	//NeulBc28SetDevState(BC28_INIT);
	ret = NbiotHardReset(5000);
	if(1 == ret){
		CLI_PRINTF("nbiot reboot succ\r\n");
	}else{
		CLI_PRINTF("nbiot reboot fail\r\n");
	}
	//restore iot
	ret = OppCoapIOTRestoreFactory();
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("iot restore factory fail ret %d\r\n", ret);
	}
	//restore log config
	ret = ApsDaemonLogConfigSetDefault();
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("log restore factory fail ret %d\r\n", ret);
	}
	//restore alarm config
	ret = ApsDaemonActionItemParaSetDefaultCustom(CUSTOM_ALARM);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("alarm restore factory fail ret %d\r\n", ret);
	}	
	//restore report config
	ret = ApsDaemonActionItemParaSetDefaultCustom(CUSTOM_REPORT);
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("update restore factory fail ret %d\r\n", ret);
	}
	ret = ApsCoapRestoreFactory();
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("coap restore factory fail ret %d\r\n", ret);
	}
	ret = ApsWifiRestore();
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("wifi restore factory fail ret %d\r\n", ret);
	}
	ret = LocRestore();
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("location restore factory fail ret %d\r\n", ret);
	}
	/*ret = ApsExepRestore();
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("exep restore factory fail ret %d\r\n", ret);
	}*/
	ret = TimeRestore();
	if(OPP_SUCCESS != ret){
		CLI_PRINTF("time restore factory fail ret %d\r\n", ret);
	}
}

void CommandSysPktGet(void)
{
	int ret;
	char *sArgv[MAX_ARGC];
	char *rArgv[MAX_ARGC];
	int sArgc = 0, rArgc = 0;

	MUTEX_LOCK(g_stCoapPkt.mutex, MUTEX_WAIT_ALWAYS);
	CLI_PRINTF("nb coap tx succ num: %d\r\n", g_stCoapPkt.g_iCoapNbTxSuc);
	CLI_PRINTF("nb coap tx fail num: %d\r\n", g_stCoapPkt.g_iCoapNbTxFail);
	CLI_PRINTF("nb coap tx retry num: %d\r\n", g_stCoapPkt.g_iCoapNbTxRetry);
	CLI_PRINTF("nb coap tx retry req num: %d\r\n", g_stCoapPkt.g_iCoapNbTxReqRetry);
	CLI_PRINTF("nb coap tx retry rsp num: %d\r\n", g_stCoapPkt.g_iCoapNbTxRspRetry);
	CLI_PRINTF("nb coap rx pkt num: %d\r\n", g_stCoapPkt.g_iCoapNbRx);
	CLI_PRINTF("nb coap rx rsp num: %d\r\n", g_stCoapPkt.g_iCoapNbRxRsp);
	CLI_PRINTF("nb coap rx req num: %d\r\n", g_stCoapPkt.g_iCoapNbRxReq);
	CLI_PRINTF("nb coap rx dev req num: %d\r\n", g_stCoapPkt.g_iCoapNbDevReq);
	CLI_PRINTF("nb coap rx dev rsp num: %d\r\n", g_stCoapPkt.g_iCoapNbDevRsp);
	CLI_PRINTF("nb coap rx pkt unknow num: %d\r\n", g_stCoapPkt.g_iCoapNbUnknow);
	CLI_PRINTF("nb ota rx pkt num: %d\r\n", g_stCoapPkt.g_iCoapNbOtaRx);
	CLI_PRINTF("nb ota tx pkt num: %d\r\n", g_stCoapPkt.g_iCoapNbOtaTx);
	CLI_PRINTF("nb ota tx pkt succ num: %d\r\n", g_stCoapPkt.g_iCoapNbOtaTxSucc);
	CLI_PRINTF("nb ota tx pkt fail num: %d\r\n", g_stCoapPkt.g_iCoapNbOtaTxFail);
	MUTEX_UNLOCK(g_stCoapPkt.mutex);

	MUTEX_LOCK(g_stBc28Pkt.mutex, MUTEX_WAIT_ALWAYS);
	CLI_PRINTF("bc28 rx pkt from msgqueue num:%d\r\n", g_stBc28Pkt.iPktRxFromQue);
	CLI_PRINTF("bc28 tx pkt num:%d\r\n", g_stBc28Pkt.iPktTx);
	CLI_PRINTF("bc28 tx pkt faild num:%d\r\n", g_stBc28Pkt.iPktTxFail);
	CLI_PRINTF("bc28 tx pkt expire num:%d\r\n", g_stBc28Pkt.iPktTxExpire);
	CLI_PRINTF("bc28 tx pkt succ num:%d\r\n", g_stBc28Pkt.iPktTxSucc);
	CLI_PRINTF("bc28 rx NNMI pkt num:%d\r\n", g_stBc28Pkt.iNnmiRx);	
	CLI_PRINTF("bc28 pkt push suc num:%d\r\n", g_stBc28Pkt.iPktPushSuc);
	CLI_PRINTF("bc28 pkt push fail num:%d\r\n", g_stBc28Pkt.iPktPushFail);
	//CLI_PRINTF("driver recv num:%d\r\n", recv_num);
	MUTEX_UNLOCK(g_stBc28Pkt.mutex);
	
	//NeulBc28GetUnrmsgCount(buffer);
	ret = sendEvent(RMSG_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(RMSG_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent RMSG_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	CLI_PRINTF("bc28 read cache:%s", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
	
	//NeulBc28GetUnsmsgCount(buffer);
	ret = sendEvent(SMSG_EVENT,RISE_STATE,sArgc,sArgv);
	ret = recvEvent(SMSG_EVENT,&rArgc,rArgv,EVENT_WAIT_DEFAULT);
	if(0 != ret){
		CLI_PRINTF("recvEvent SMSG_EVENT ret fail%d\r\n", ret);
		ARGC_FREE(rArgc,rArgv);
		return;
	}
	CLI_PRINTF("bc28 write cache:%s", rArgv[0]);
	ARGC_FREE(rArgc,rArgv);
}

void CommandSysInfo(void)
{
	char buffer[512] = {0};
	int len = 0;
	unsigned int Temp_ProductClass = PRODUCTCLASS;
	unsigned int Temp_ProductSku = PRODUCTSKU;
	unsigned int Temp_Ver = OPP_LAMP_CTRL_CFG_DATA_VER;
	U32 hTime, rTime, lTime, hlTime;
	ST_OPP_LAMP_CURR_ELECTRIC_INFO stElecInfo;
    esp_chip_info_t chip_info;
	AD_CONFIG_T config;
	int i;
	ST_FASTPROP_PARA *propPtr;
	ST_FASTALARM_PARA *alarmPtr;
	LOG_CONFIG_T logConfig;
	ST_TIME time;
	long double lat,lng;
	wifi_config_t conf;
	ST_IOT_CONFIG stIotConfig;	
	int ret;
	int state;
	int level;
	ST_AST stAstTime;
	LOG_CONTROL_T logControl;
	extern uint16_t g_DacLevel;
	extern uint16_t g_PwmLevel;
	extern uint8_t  g_RelayGpioStatus;
	extern uint8_t  g_DimTypeStatus;
	
	//esp32 idf version
	len = 0;
	len += snprintf(buffer+len, 512, "\r\n********esp32 info*********\r\n");
	len += snprintf(buffer+len,512,"IDF_VER:%s\r\n",esp_get_idf_version());
    esp_chip_info(&chip_info);
    len += snprintf(buffer+len, 512,"This is ESP32 chip with %d CPU cores, WiFi%s%s, \r\n",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    len += snprintf(buffer+len, 512,"silicon revision %d, \r\n", chip_info.revision);
    len += snprintf(buffer+len, 512,"%dMB %s flash\r\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
	
	//version
	len += snprintf(buffer+len, 512, "\r\n********sys version info*********\r\n");
	len += snprintf(buffer+len, 512, "class:%04X\r\n", Temp_ProductClass);			
	len += snprintf(buffer+len, 512, "sku:%04X\r\n", Temp_ProductSku);			
	len += snprintf(buffer+len, 512, "verion:%u\r\n", Temp_Ver);
	len += snprintf(buffer+len, 512, "data:%s\r\n", __DATE__);
	len += snprintf(buffer+len, 512, "time:%s\r\n", __TIME__);
	//hard info
	len += snprintf(buffer+len, 512, "\r\n********hard info*********\r\n");
	len += snprintf(buffer+len, 512, "dacLevel :%u, pwmLevel:%u, relaySatus: %s , DimType : %s \r\n", g_DacLevel, g_PwmLevel, g_RelayGpioStatus == 0 ? "OFF" : "ON", g_DimTypeStatus == 0 ? "0-10V" : "PWM");
	//lamp info
	len += snprintf(buffer+len, 512, "\r\n********lamp info*********\r\n");
	len += snprintf(buffer+len, 512, "dimType:%d\r\n", LampOuttypeGet());
	LampStateRead(&state,&level);
	len += snprintf(buffer+len, 512, "switch:%d,level:%d\r\n",state,level/10);
	//runtime
	len += snprintf(buffer+len, 512, "\r\n********runtime info*********\r\n");	
	OppLampCtrlGetHtime(0,&hTime);
	OppLampCtrlGetRtime(0,&rTime);
	OppLampCtrlGetHLtime(0,&hlTime);
	OppLampCtrlGetLtime(0,&lTime);
	len += snprintf(buffer+len, 512, "hTime:%u\r\n", hTime);
	len += snprintf(buffer+len, 512, "rTime:%u\r\n", rTime);
	len += snprintf(buffer+len, 512, "hlTime:%u\r\n", hlTime);
	len += snprintf(buffer+len, 512, "lTime:%u\r\n", lTime);
	len += snprintf(buffer+len, 512, "sysTick:%u\r\n", OppTickCountGet());
	//elec
	len += snprintf(buffer+len, 512, "\r\n********elec info*********\r\n");
	ElecGetElectricInfo(&stElecInfo);
	len += snprintf(buffer+len, 512, "current:%u\r\n", stElecInfo.current);
	len += snprintf(buffer+len, 512, "voltage:%u\r\n", stElecInfo.voltage);
	len += snprintf(buffer+len, 512, "power:%u\r\n", stElecInfo.factor);
	len += snprintf(buffer+len, 512, "factor:%u\r\n", stElecInfo.power);	
	//EC
	len += snprintf(buffer+len, 512, "EC:%u\r\n", stElecInfo.consumption);	
	len += snprintf(buffer+len, 512, "HEC:%u\r\n", stElecInfo.hisConsumption);
	CLI_PRINTF("%s", buffer);

	len = 0;
	//clk src
	len += snprintf(buffer+len, 512, "\r\n********time info*********\r\n");
	len += snprintf(buffer+len, 512, "clk src:%s\r\n", clkSrcDesc[TimeGetClockSrc()]);

	Opple_Get_RTC(&time);
	if(time.Zone >= 0)
		len += snprintf(buffer+len, 512, "time:%04d-%02d-%02d %02d:%02d:%02d+%02d\r\n", time.Year, time.Month, time.Day, time.Hour, time.Min, time.Sencond, time.Zone);
	else
		len += snprintf(buffer+len, 512, "time:%04d-%02d-%02d %02d:%02d:%02d-%02d\r\n", time.Year, time.Month, time.Day, time.Hour, time.Min, time.Sencond, abs(time.Zone));
	len += snprintf(buffer+len, 512, "week:%s\r\n",weekDesc[TimeCacWeek(time.Year, time.Month, time.Day)]);	
	LocGetLat(&lat);
	LocGetLng(&lng);
	Opple_Calc_Ast_Time(time.Year, time.Month, time.Day,time.Zone, lat,lng,&stAstTime);
	len += snprintf(buffer+len, 512, "astSet:%02d:%02d:%02d, rise:%02d:%02d:%02d\r\n", stAstTime.setH,stAstTime.setM,stAstTime.setS,stAstTime.riseH,stAstTime.riseM,stAstTime.riseS);
	//loc src
	len += snprintf(buffer+len, 512, "\r\n********loc info*********\r\n");
	len += snprintf(buffer+len, 512, "loc src:%s\r\n", locSrcDesc[LocGetSrc()]);
	LocGetLat(&lat);
	LocGetLng(&lng);
	len += snprintf(buffer+len, 512, "loc:%Lf,lng:%Lf\r\n", lat, lng);
	//config loc
	LocGetCfgLat(&lat);
	LocGetCfgLng(&lng);
	len += snprintf(buffer+len, 512, "config loc:%Lf,lng:%Lf\r\n", lat, lng);
	//wifi info
	len += snprintf(buffer+len, 512, "\r\n********wifi info*********\r\n");
	esp_wifi_get_config(WIFI_IF_STA, &conf);
    len += snprintf(buffer+len,512,"wifi ssid:%s, passwd:%s\r\n",conf.sta.ssid,conf.sta.password);
	//iot address
	len += snprintf(buffer+len, 512, "\r\n********iot info*********\r\n");
	OppCoapIOTGetConfig(&stIotConfig);
	len += snprintf(buffer+len,512,"addr:%s,port:%d\r\n", stIotConfig.ocIp, stIotConfig.ocPort);
	CLI_PRINTF("%s\r\n", buffer);
	
	len = 0;
	len += snprintf(buffer+len, 512, "\r\n********report config para********\r\n");
	propPtr = ApsCoapFastConfigPtrGet();
	for(i=0;i<ApsCoapFastConfigSizeGet();i++){
		ApsDaemonActionItemParaGetCustom(CUSTOM_REPORT,(propPtr+i)->defaultIdx,&config);
		len+=snprintf(buffer+len, 512, "resName:%20s resId:%d enable:%x reportIf:%d reportIfPara:%03d\r\n", (propPtr+i)->resName, config.resource, config.enable, config.reportIf, config.reportIfPara);
	}
	CLI_PRINTF("%s", buffer);
	len = 0;
	len += snprintf(buffer+len, 512, "\r\n********alarm config para********\r\n");
	alarmPtr = ApsCoapFastAlarmPtrGet();
	for(i=0;i<ApsCoapFastAlarmSizeGet();i++){
		ApsDaemonActionItemParaGetCustom(CUSTOM_ALARM, (alarmPtr+i)->defaultIdx,&config);
		len+=snprintf(buffer+len, 512, "alarmDesc:%20s alarmId:%d resId:%d enable:%x alarmIf:%d alarmIfPara1:%04d alarmIfPara2:%04d\r\n", 
			(alarmPtr+i)->alarmDesc, (alarmPtr+i)->alarmId, config.resource, config.enable, config.alarmIf, config.alarmIfPara1, config.alarmIfPara2);
	}
	CLI_PRINTF("%s", buffer);
	len = 0;
	len += snprintf(buffer+len, 512, "\r\n********log config para********\r\n");
	for(i=0;i<LOG_ID_MAX;i++){
		logConfig.logid=i;
		ApsDaemonLogIdParaGet(&logConfig);
		len+=snprintf(buffer+len, 512, "logId:%02d isLogReport:%d isLogEnable:%d\r\n",
			logConfig.logid,logConfig.isLogReport,logConfig.isLogSave);
	}
	len+=snprintf(buffer+len, 512, "latest saveLogId:%d\r\n", ApsDamemonLogCurrentSaveIndexGet());
	ApsDaemonLogControlParaGet(&logControl);
	len+=snprintf(buffer+len, 512, "log report period:%d\r\n", logControl.period);
	CLI_PRINTF("%s", buffer);
}

void CommandSysTask(void)
{
	uint8_t pcWriteBuffer[500];

	vTaskList((char *)&pcWriteBuffer);
	CLI_PRINTF("name		status pri   stack no\r\n");
	CLI_PRINTF("%s\r\n", pcWriteBuffer);	

	vTaskGetRunTimeStats((char *)&pcWriteBuffer);
	CLI_PRINTF("\r\nname		 count		  rata\r\n");
	CLI_PRINTF("%s\r\n", pcWriteBuffer);		

}

void CommandLogGet(void)
{
	int i;
	APP_LOG_T log;
	int ret;
	char buffer[512] = {0};
	char tBuf[64] ={0};
	char mBuf[64] ={0};
	char lBuf[64] ={0};
	int len = 0;
	ST_LOG_DESC logDesc;
	
	if(CliIptArgList[0][0] > CliIptArgList[1][0]){
		CLI_PRINTF("start logSaveId must > end logSaveId");
		return;
	}
	for(i=CliIptArgList[0][0];i<=CliIptArgList[1][0];i++){
		ret = ApsDaemonLogQuery(i,&log);
		if(OPP_SUCCESS == ret){
			sprintf(tBuf,"%02d-%02d-%02d %02d:%02d:%02d", 
				log.time[0], log.time[1], log.time[2], log.time[3], log.time[4], log.time[5]);
			ApsDaemonLogModuleNameGet(log.module, mBuf);
			ApsDaemonLogLevelGet(log.level,lBuf);
			ret = ApsLogAction(&log,&logDesc);
			if(OPP_SUCCESS != ret){
				continue;
			}

			snprintf(buffer,512,"logSeq:%05d logSaveId:%05d logId:%02d time:%18s module:%10s level:%10s logDescId:%02d log:%s\r\n",
				log.seqnum,i,log.id,tBuf,mBuf,lBuf,logDesc.logDescId,logDesc.log);
			CLI_PRINTF("%s",buffer);
		}
	}
}

void CommandLogStatusGet(void)
{
	CLI_PRINTF("current logSaveId:%d",ApsLogSaveIdLatest());
}

void CommandHardInfo(void)
{
	extern uint16_t g_DacLevel;
	extern uint16_t g_PwmLevel;
	extern uint8_t  g_RelayGpioStatus;
	extern uint8_t  g_DimTypeStatus;

	CLI_PRINTF("dacLevel :%u, pwmLevel:%u, relaySatus: %s , DimType : %s \r\n", g_DacLevel, g_PwmLevel, g_RelayGpioStatus == 0 ? "OFF" : "ON", g_DimTypeStatus == 0 ? "0-10V" : "PWM");
}

void CommandLampInfo(void)
{
	int state;
	int level;
	U32 hlTime,lTime;
	char actTime[ACTTIME_LEN] = {0};
	U8 srcChl, sw;
	
	CLI_PRINTF("dimType:%d\r\n", LampOuttypeGet());
	LampStateRead(&state,&level);
	CLI_PRINTF("switch:%d,level:%d\r\n",state,level/10);
	OppLampCtrlGetHLtime(0,&hlTime);
	OppLampCtrlGetLtime(0,&lTime);
	CLI_PRINTF("history light time:%d,light time:%d\r\n",hlTime, lTime);
	OppLampActTimeGet(actTime);
	CLI_PRINTF("actTime %s\r\n", actTime);
	OppLampDelayInfo(&srcChl,&sw);
	CLI_PRINTF("delay info:switch %d chl %d\r\n", sw, srcChl);
}

void CommandUdpInfo(void)
{
	int state;
	int level;
	
	UdpClientListShow(CliPrintf);
}

void CommandSysGetBackTrace(void)
{
	int8_t occur;
	char buf[200] = {0};
	int ret;
	
	ret = print_panic_occur_saved(&occur);
	if(OPP_SUCCESS == ret){
		CLI_PRINTF("occur:%d\r\n",occur);
		ret = print_panic_saved(0,buf);
		if(OPP_SUCCESS == ret)
			CLI_PRINTF("%s\r\n",buf);
		ret = print_panic_saved(1,buf);
		if(OPP_SUCCESS == ret)
			CLI_PRINTF("%s\r\n",buf);
	}
}

void CommandSysEraseBackTrace(void)
{
	erase_panic();
}

void CommandSysOtaProg(void)
{
	CLI_PRINTF("state:%d progres:%d\r\n",OtaState(),OtaProg());
}

