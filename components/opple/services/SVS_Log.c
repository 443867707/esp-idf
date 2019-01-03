/*****************************************************************
                              How it works
******************************************************************/





/*****************************************************************
                              Includes
******************************************************************/
#include "SVS_Log.h"
#include "DEV_Queue.h"
#include "DEV_LogDb.h"
#include "Opp_ErrorCode.h"
#include "DEV_LogMmt.h"
#include "OPP_Debug.h"
#include "APP_Daemon.h"
#include "DEV_Utility.h"

/*****************************************************************
                              Defines
******************************************************************/
#define BUF_LOG_LENGTH (sizeof(APP_LOG_T))

#define USED_INTERFACE_CONFIG 2   // 0:debug,1: key-value 2:general log mmt

#define APSLOG_P_CHECK(p) \
		if((p)==(void*)0){\
		DEBUG_LOG(DEBUG_MODULE_PARA,DLL_ERROR,"null pointer!\r\n");\
		return OPP_NULL_POINTER;}

/*****************************************************************
                              Datas
******************************************************************/
APSLOG_T_MUTEX log_mutex;

typedef struct{
	unsigned char errorNo;
	char *errDesc;
}ST_ERR_DESC;

const static ST_ERR_DESC errDesc[OPP_UNKNOWN] = {
	{OPP_SUCCESS,"success"},
	{OPP_FAILURE, "failure"},
	{OPP_OVERFLOW, "stack overflow"},
	{OPP_NO_PRIVILEGE, "no privilege"},
	{OPP_EXISTED, "existed"},
	{OPP_UNINIT, "not init"},
	{OPP_NULL_POINTER, "pointer is null"},
	{OPP_RANGE_EXPIRED, "range expired"},
	{OPP_NBLED_INIT_ERR, "nbiot led status init error"},
	{OPP_DIMCTRL_INIT_ERR, "dimctrl init error"},
	{OPP_METER_INIT_ERR, "meter init error"},
	{OPP_GPS_INIT_ERR, "gps init error"},
	{OPP_BC28_INIT_ERR, "bc28 init error"},
	{OPP_LIGHT_SENSOR_INIT_ERR, "light sensor init error"},
	{OPP_BC28_UART_WRITE_ERR, "mcu write cmd to bc28 through uart error"},
	{OPP_BC28_UART_READ_TIMEOUT, "mcu read data from bc28 through uart timeout"},
	{OPP_BC28_BAND_INVALIDE, "bc28 band is invalide"},
	{OPP_BC28_READ_MSG_NO_OK, "mcu read msg from bc28 not include OK"},
	{OPP_BC28_READ_MSG_NO_CGSN, "mcu read msg from bc28 not include CGSN"},
	{OPP_BC28_READ_MSG_NO_CCLK, "mcu read msg from bc28 not include CCLK"},
	{OPP_BC28_READ_MSG_NO_REBOOTING, "mcu read msg from bc28 not include REBOOTING"},
	{OPP_BC28_AT_REBOOT_ERR, "bc28 AT+NRB error"},
	{OPP_BC28_ATE_ERR, "bc28 ATE error"},
	{OPP_BC28_HWDET_ERR, "bc28 hwdet error"},
	{OPP_BC28_SCRAM_ERR, "bc28 set scram error"},
	{OPP_BC28_RESELECT_ERR, "bc28 set reselect error"},
	{OPP_BC28_FREQUENCY_ERR, "bc28 set frequency error"},
	{OPP_BC28_PSM_ERR, "bc28 set psm error"},
	{OPP_BC28_PSM_REPORT_ERR, "bc28 set psm report error"},
	{OPP_BC28_ACTIVE_NETWORK_ERR, "bc2i active nbiot network error"},
	{OPP_COAP_OP_ERR, "coap op error"},
	{OPP_COAP_CREATE_OBJ_ERR, "coap create object error"},
	{OPP_COAP_CREATE_ARRAY_ERR, "coap create array error"},
	{OPP_BC28_QRY_UART, "bc28 query uart para error"},
	{OPP_BC28_SET_UART, "bc28 set uart para error"},
	{OPP_BC28_ABNORMAL_ERR, "bc28 do abnormal error"}
};


/*****************************************************************
                              utilities
******************************************************************/

static unsigned short const wCRC16Table[256] = {      
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,     
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,     
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,      
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,     
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,       
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,     
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,     
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,     
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,     
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,        
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,     
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,     
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,     
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,     
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,        
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,     
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,     
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,     
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,     
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,        
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,     
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,     
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,     
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,     
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,       
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,     
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,     
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,     
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,     
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,       
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,     
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
}; 

unsigned short CRC16(const unsigned char* pDataIn, int iLenIn)     
{     
     unsigned short wResult = 0;     
     unsigned short wTableNo = 0;  
     unsigned short crc;
     
     int i = 0;
    for( i = 0; i < iLenIn; i++)     
    {     
        wTableNo = ((wResult & 0xff) ^ (pDataIn[i] & 0xff));     
        wResult = ((wResult >> 8) & 0xff) ^ wCRC16Table[wTableNo];     
    }     
    
    crc = wResult;  
    return crc;
}  



/*****************************************************************
                              abstract interfaces
******************************************************************/
inline int abstract_ApsLogSaveIdLatest(void)
{
	#if USED_INTERFACE_CONFIG==1
		return LogDbLatestIndex(); 
	#elif USED_INTERFACE_CONFIG==2
		return LogMmtGetLatestId(LOG_MMT_PARTITION_LOG);
	#endif
}

inline int abstract_ApsLogRead(unsigned int saveid,unsigned char* data,unsigned int* len)
{
	#if USED_INTERFACE_CONFIG==1
		return LogDbQuery(saveid,data,len);
	#elif USED_INTERFACE_CONFIG==2
		return LogMmtRead(LOG_MMT_PARTITION_LOG,saveid,data,len);
	#endif
}

inline int abstract_ApsLogWrite(unsigned char* data,unsigned int len,U32* id)
{
	#if USED_INTERFACE_CONFIG==1
		return LogDbWrite(data,len,id);	 
	#elif USED_INTERFACE_CONFIG==2
		return LogMmtWrite(LOG_MMT_PARTITION_LOG,data,len,id);
	#endif
}


/*****************************************************************
                              Implementations
******************************************************************/

/////////////////////////////////////
/*log[0-1]:alarm id
**log[2]:status
**log[3-6]:value*/
int AlarmDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	if(10000 == *((U16 *)(&log->log[0]))){
		logDesc->logDescId = 0;
		sprintf(logDesc->log, "over voltage warning status %d value %d", log->log[2],*((U32 *)(&log->log[3])));
	}
	else if(10001 == *((U16 *)(&log->log[0]))){
		logDesc->logDescId = 1;
		sprintf(logDesc->log, "under voltage warning status %d value %d", log->log[2],*((U32 *)(&log->log[3])));		
	}
	else if(10002 == *((U16 *)(&log->log[0]))){
		logDesc->logDescId = 2;
		sprintf(logDesc->log, "over current status %d value %d", log->log[2],*((U32 *)(&log->log[3])));		
	}
	else if(10003 == *((U16 *)(&log->log[0]))){
		logDesc->logDescId = 3;
		sprintf(logDesc->log, "under current status %d value %d", log->log[2],*((U32 *)(&log->log[3])));		
	}else if(10004 == *((U16 *)(&log->log[0]))){
		logDesc->logDescId = 4;
		sprintf(logDesc->log, "lamp on status %d", log->log[2]);		
	}else if(10005 == *((U16 *)(&log->log[0]))){
		logDesc->logDescId = 5;
		sprintf(logDesc->log, "lamp off status %d", log->log[2]);		
	}else{
		logDesc->logDescId = 0;
		sprintf(logDesc->log, "unknow alarm alarmId %d", *((U16 *)(&log->log[0])));		
	}
	return OPP_SUCCESS;	
}

/*
**log[0]:0,device power on
**log[0]:1,device power off
**log[0]:2,device restore factory configuration
*/
int DevDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	if(0 == log->log[0]){
		logDesc->logDescId = 0;
	}
	else if(1 == log->log[0]){
		logDesc->logDescId = 1;
	}
	else if(2 == log->log[0]){
		logDesc->logDescId = 2;
	}
	logDesc->log[0] = '\0';
	return OPP_SUCCESS;
}
/*
**log[0]:0,nbiot start connect network
**           1,nbiot connect network succ
**           2,nbiot disconnect network
*/
int NbnetDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	if(0 == log->log[0]){
		logDesc->logDescId = 0;
	}
	else if(1 == log->log[0]){
		logDesc->logDescId = 1;
	}
	else if(2 == log->log[0]){
		logDesc->logDescId = 2;
	}
	logDesc->log[0] = '\0';	
	return OPP_SUCCESS;
}

/*
**log[0]:0,lamp switch on
**          :1,lamp switch off
**log[1]:0,nbiot
**          :1,wifi
**          :2,other
*/
int SwitchDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	if(0 == log->log[0]){
		logDesc->logDescId = 0;
	}
	else if(1 == log->log[0]){
		logDesc->logDescId = 1;
	}
	if(0 == log->log[1]){
		sprintf(logDesc->log,"by %s", "nbiot");
	}
	else if(1 == log->log[1]){
		sprintf(logDesc->log,"by %s", "wifi");
	}
	else if(2 == log->log[1]){
		sprintf(logDesc->log,"by %s", "cli");
	}
	else if(3 == log->log[1]){
		sprintf(logDesc->log,"by %s", "workplan");
	}
	else if(4 == log->log[1]){
		sprintf(logDesc->log,"by %s", "boot");
	}
	
	return OPP_SUCCESS;
}


/*
**log[0-1]:,lamp brightness
*/
int BriDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	int len = 0;
	
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	logDesc->logDescId = 0;
	len = sprintf(logDesc->log, "bri=%d", *((U16 *)&log->log[0]));
	if(0 == log->log[2]){
		sprintf(logDesc->log+len,"by %s", "nbiot");
	}
	else if(1 == log->log[2]){
		sprintf(logDesc->log+len,"by %s", "wifi");
	}
	else if(2 == log->log[2]){
		sprintf(logDesc->log+len,"by %s", "cli");
	}
	else if(3 == log->log[2]){
		sprintf(logDesc->log+len,"by %s", "workplan");
	}
	else if(4 == log->log[2]){
		sprintf(logDesc->log+len,"by %s", "boot");
	}

	return OPP_SUCCESS;
}

/*
**log[0]:0,nbiot dev fault
*/
int NbDevDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	logDesc->logDescId = 0;
	logDesc->log[0] = '\0';

	return OPP_SUCCESS;
}

/*
**log[0]:0,elec dev fault
*/
int ElecDevDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	logDesc->logDescId = 0;
	logDesc->log[0] = '\0';

	return OPP_SUCCESS;
}

/*
**log[0]:plan
**log[1]:job
**log[2]:switch
**log[3-4]:bri
*/
int WorkPlanDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	logDesc->logDescId = 0;
	sprintf(logDesc->log, "plan=%d job=%d switch=%d bri=%d", log->log[0], log->log[1], log->log[2], *((U16 *)&log->log[3]));

	return OPP_SUCCESS;
}

/*
**log[0]:
*/
int LocDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	logDesc->logDescId = 0;
	logDesc->log[0] = '\0';	
	//sprintf(logDesc->log, "plan=%d job=%d switch=%d bri=%d", log->log[0]<<8, log->log[1], log->log[2], log->log[3]<<8|log->log[4]);

	return OPP_SUCCESS;
}

/*
**log[0]:1,GPS
**log[0]:2,NBIOT
**log[0]:3,LOC
*/
int TimeDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	logDesc->logDescId = 0;
	if(1 == log->log[0]){
		sprintf(logDesc->log, "%s", "GPS CLK");
	}
	if(2 == log->log[0]){
		sprintf(logDesc->log, "%s", "NBIOT CLK");
	}
	if(3 == log->log[0]){
		sprintf(logDesc->log, "%s", "LOC CLK");
	}
	return OPP_SUCCESS;
}
/*
**log[0]:
*/
int RebootDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	logDesc->logDescId = 0;
	sprintf(logDesc->log, "device is reboot");

	return OPP_SUCCESS;
}


int MakeErrLog(unsigned char module, unsigned char errorno)
{
	APP_LOG_T log;

	log.id = ERROR_LOGID;
	log.level = LOG_LEVEL_ERROR;
	log.module = module;
	log.len = 1;
	log.log[0] = errorno;
	ApsDaemonLogReport(&log);
	
	return OPP_SUCCESS;
}

int ErrorDescIdGet(APP_LOG_T* log, ST_LOG_DESC * logDesc)
{
	unsigned int i=0;
	
	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);

	for(i=0;i<OPP_UNKNOWN;i++){
		if(log->log[0] == errDesc[i].errorNo){
			sprintf(logDesc->log, errDesc[i].errDesc);
		}
	}
	logDesc->logDescId = 0;
	

	return OPP_SUCCESS;
}


const ST_LOGID_ACTION g_aucLogAction[LOG_ID_MAX] = {
	{ALARM_LOGID, AlarmDescIdGet},
	{DEV_LOGID, DevDescIdGet},
	{NBNET_LOGID,NbnetDescIdGet},
	{SWITCH_LOGID,SwitchDescIdGet},
	{BRI_LOGID,BriDescIdGet},
	{NBDEV_LOGID,NbDevDescIdGet},
	{ELECDEV_LOGID,ElecDevDescIdGet},
	{WORKPLAN_LOGID,WorkPlanDescIdGet},
	{LOC_LOGID,LocDescIdGet},
	{TIME_LOGID,TimeDescIdGet},
	{REBOOT_LOGID,RebootDescIdGet},
	{ERROR_LOGID,ErrorDescIdGet},
};

int ApsLogAction(APP_LOG_T * log, ST_LOG_DESC * logDesc)
{
	int ret = 0;

	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(logDesc);
	
	if(log->id >= LOG_ID_MAX || log->id < 0)
		return OPP_FAILURE;
	
	if(g_aucLogAction[log->id].func)
		ret = g_aucLogAction[log->id].func(log, logDesc);

	return ret;
}

int ApsLogRead(unsigned int saveid,APP_LOG_T* log)
{
	int res;
	unsigned int len;

	APSLOG_P_CHECK(log);
	
	APSLOG_MUTEX_LOCK(log_mutex);
	len = BUF_LOG_LENGTH;
	//res = LogDbQuery(saveid,(unsigned char*)log,&len);
	res = abstract_ApsLogRead(saveid,(unsigned char*)log,&len);
  APSLOG_MUTEX_UNLOCK(log_mutex);
  
  return res;
}


int ApsLogWrite(APP_LOG_T* log,unsigned int* saveid)
{
    int res;

	APSLOG_P_CHECK(log);
	APSLOG_P_CHECK(saveid);
	
	APSLOG_MUTEX_LOCK(log_mutex);
    //res = LogDbWrite((unsigned char*)log,BUF_LOG_LENGTH,saveid);
    res = abstract_ApsLogWrite((unsigned char*)log,BUF_LOG_LENGTH,saveid);
	APSLOG_MUTEX_UNLOCK(log_mutex);
	return res;
}

int ApsLogSaveIdLatest(void)
{
	int res;
    //return LogDbLatestIndex();
    APSLOG_MUTEX_LOCK(log_mutex);
    res = abstract_ApsLogSaveIdLatest();
	APSLOG_MUTEX_UNLOCK(log_mutex);
    return res;
}


void OppApsLogInit(void)
{
	APSLOG_MUTEX_CREATE(log_mutex);
}

void OppApsLogLoop(void)
{

}

S32 OppApsRecordIdLatest(void)
{
	S32 id;
	
	APSLOG_MUTEX_LOCK(log_mutex);
	id = LogMmtGetLatestId(LOG_MMT_PARTITION_METER);
	APSLOG_MUTEX_UNLOCK(log_mutex);
	return id;
}

S32 OppApsRecordIdLast(S32 id_now)
{
	S32 id;
	
	APSLOG_MUTEX_LOCK(log_mutex);
	id = LogMmtGetLastId(LOG_MMT_PARTITION_METER,id_now);
	APSLOG_MUTEX_UNLOCK(log_mutex);
	return id;
}

int OppApsRecoderRead(S32 id,U8* data,U32* len)
{
	int res;
	U8 d[16];
	U32 itemLen=16;
	U16 crc;
	
	APSLOG_P_CHECK(data);
	APSLOG_P_CHECK(len);
	
	APSLOG_MUTEX_LOCK(log_mutex);
	//res = LogMmtReadLatestItem(LOG_MMT_PARTITION_METER,id,data,len);
	//res = LogMmtReadLatestItem(LOG_MMT_PARTITION_METER,id,d,itemLen);
	res = LogMmtRead(LOG_MMT_PARTITION_METER,id,d,&itemLen);
	if(res == 0)
	{
		crc = CRC16(d,14);
		//printf("crc=%d\r\n",crc);
		if(crc == *(U16*)(&d[16-2]))
		{
			memcpy(data,d,*len);
		}
		else
		{
			res = 1;
		}
	}
	APSLOG_MUTEX_UNLOCK(log_mutex);
	return res; 
}

int OppApsRecoderWrite(U8* data,U32 len,U32* id)
{
	int res;
	U8 d[16]={0};
	U16 crc;

	APSLOG_P_CHECK(data);
	APSLOG_P_CHECK(id);
	
	APSLOG_MUTEX_LOCK(log_mutex);
	if(len <= 16-2){
		crc = CRC16(data,16-2);
		memcpy(d,data,16-2);
		*(U16*)(&d[16-2]) = crc;
		//printf("crc=%d\r\n",crc);
		//res = LogMmtWrite(LOG_MMT_PARTITION_METER,data,len,id);
		res = LogMmtWrite(LOG_MMT_PARTITION_METER,d,16,id);
	}else{
		res =1;
	}
	APSLOG_MUTEX_UNLOCK(log_mutex);
	return res;
}











