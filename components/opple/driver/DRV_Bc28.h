#ifndef _NEUL_BC28_H
#define _NEUL_BC28_H

#include "Includes.h"
#include "SVS_MsgMmt.h"

//#define NEUL_MAX_BUF_SIZE 1064
//#define NEUL_MAX_BUF_SIZE 400
#define LOS_DEV_UART_DUALBUF_SIZE /*1024*/(4*1024)
#define NEUL_MAX_BUF_SIZE 1064

#define NEUL_MAX_SOCKETS 1 // the max udp socket links
#define NEUL_IP_LEN 16
#define NEUL_IPV6_LEN 46
#define NEUL_MANUFACT_LEN 20
#define NEUL_IMEI_LEN    16
#define NEUL_IMSI_LEN    16
#define NEUL_NCCID_LEN   21
#define APN_NAME_LEN     128
#define NEUL_VER_LEN	 256	

#define NBCONFIG_ID      0
#define DISCRETEWIN_ID   1

#define EVENT_WAIT_ALWAYS    0xFFFF
#define EVENT_WAIT_DEFAULT   60000
#define MAX_MTU              512
#define DEVSTATE_DEFAULT_TO  600000
#define REGSTATE_DEFAULT_TO  120000
#define TXDATA_DEFAULT_TO    120000
#define FIRMWARE_DEFAULT_TO  1800000
#define HARD_RESET_TO        6000
#define DEVINIT_RETRY_TIMES  10
#define UART_BAUDRATE_DEFAULT    9600
#define UART_SYNC_DEFAULT    2
#define UART_STOPBIT_DEFAULT    1

#define URC_FILTER(inBuffer,inLen,str,startMsg,endMsg) do{ \
	char *str1 = NULL, *p = NULL; \
	int len = 0, len1 = 0, len2 = 0, len3 = 0; \
	char tmpbuf[1064] = {0}; \
	/*printf("URC_FILTER len %d %s",inLen, inBuffer);*/ \
	p = str+strlen(startMsg); \
	str1 = strstr(p, endMsg); \
	/*printf("startMsg %s, endMsg %s p %s",startMsg,endMsg,p);*/ \
	/*printf("0 str:%p inBuffer:%p p:%p str1:%p\r\n", str, inBuffer,p, str1);*/ \
	if(str1){ \
		p = str1+strlen(endMsg); \
		/*only urc msg*/ \
		if((str == inBuffer) && (p == (inBuffer + inLen))){ \
			/*printf("1\r\n");*/ \
			memset(inBuffer,0,inLen); \
			inLen = 0; \
		} \
		/*urc msg at head*/ \
		else if((str == inBuffer) && (p < (inBuffer + inLen))){ \
			len1 = (int)(p - str); \
			len = inLen - len1; \
			/*printf("2 inLen %d len1 %d len %d\r\n", inLen, len1,len);*/ \
			memcpy(tmpbuf, p, len); \
			memset(inBuffer, 0, inLen); \
			memcpy(inBuffer, tmpbuf, len); \
			inLen = len; \
		} \
		/*urc msg at middle*/ \
		else if((str > inBuffer) && (p < (inBuffer + inLen))){ \
			len1 = (int)(str - inBuffer); \
			memcpy(tmpbuf, inBuffer, len1); \
			len = len1; \
			len2 = (int)(p-str); \
			len3 = inLen-len1-len2; \
			memcpy(tmpbuf + len, p, len3); \
			len += len3; \
			/*printf("3 len1 %d len2 %d len3 %d len %d\r\n",len1,len2,len3,len);*/ \
			memset(inBuffer,0,inLen); \
			memcpy(inBuffer, tmpbuf, len); \
			inLen = len; \
		} \
		/*urc msg at tail*/ \
		else if((str > inBuffer) && (p == (inBuffer + inLen))){ \
			len = (int)(str - inBuffer); \
			/*printf("4 inLen %d len %d\r\n", inLen,len);*/ \
			memcpy(tmpbuf, inBuffer, len); \
			memset(inBuffer, 0, inLen); \
			memcpy(inBuffer, tmpbuf, len); \
			inLen = len; \
		}else{ \
			/*printf("5\r\n");*/ \
		} \
	}else{ \
		/*cannot find endmsg*/	\
		memset(inBuffer, 0, inLen);	\
		inLen = 0;	\
	} \
}while(0)

#define MAX_ARGC    6		
#define ARGC_FREE(sArgc, sArgv) do{ \
				int i = 0; \
				for(i=0;i<sArgc;i++){ \
					if(sArgv[i]){ \
						free(sArgv[i]); \
						sArgv[i] = NULL; \
					} \
				} \
		}while(0)

typedef int (*neul_read)(char *buf, int maxrlen, int mode, unsigned int timeout);
typedef int (*neul_write)(char *buf, int writelen, int mode);
typedef int (*neul_state_chg)(void);
typedef int (*neul_time_set)(unsigned char year, unsigned char month, unsigned char day, unsigned char hour, unsigned char min, unsigned char second, unsigned char zone);

#pragma pack(1)
typedef enum
{
	UDP_PRO = 0,
	COAP_PRO,
	UNKNOW_PRO
}EN_PROTOCOL_TYPE;

typedef enum{
	IOT_NOWORK = -4,
	IOT_INIT = -3,
	IOT_WAIT_MSG = -2,
	IOT_START_REG = -1,
	IOT_REG = 0,
	IOT_DEREG = 1,
	IOT_UPREG = 2,
	IOT_1900_OBSERVE = 3,
	IOT_BOOTSTRAP = 4,
	IOT_503_OBSERVE = 5,
	IOT_UPDATE_URL = 6,
	IOT_UPDATE_MSG = 7,
	IOT_CANCEL_1900 = 9
}EN_IOT_STATE;

typedef enum{
	NOT_REG = 0,
	REG = 1,
	TRY_REG = 2,
	DENY_REG = 3,
	UNKNOW_REG = 4,
	ROAMING_REG = 5,
	INVALID_REG
}EN_REG_STATE;

typedef struct _neul_dev_operation_t 
{
    neul_read dev_read;
    neul_write dev_write;
	neul_state_chg state_chg;
	neul_state_chg iot_chg;
	neul_state_chg dev_chg;
	neul_time_set time_set;
} neul_dev_operation_t;

typedef struct _remote_info_t
{
    unsigned short port;
    int socket;
    char ip[16];
}remote_info;

typedef struct _neul_dev_info_t 
{
	U8 imei[NEUL_IMEI_LEN];
	U8 imsi[NEUL_IMSI_LEN];
	U8 nccid[NEUL_NCCID_LEN];
	U8 pdpAddr0[NEUL_IPV6_LEN];
	U8 pdpAddr1[NEUL_IPV6_LEN];
    char *rbuf;
    char *wbuf;
    int remotecount;
    remote_info *addrinfo;
    neul_dev_operation_t *ops;
    char cdpip[16];
	char revision[NEUL_VER_LEN];
	/*U8 regStatus;*/ /*0 Not registered, UE is not currently searching an operator to register to##1 Registered, home network##2 Not registered, but UE is currently trying to attach or searching an operator to register to##3 Registration denied##4 Unknown (e.g. out of E-UTRAN coverage)##5 Registered, roaming*/
	/*U8 conStatus;*/ /*0 Idle, 1 Connected*/
} neul_dev_info_t;

typedef struct
{
	U8 imei[NEUL_IMEI_LEN];
	U8 imsi[NEUL_IMSI_LEN];
	U8 nccid[NEUL_NCCID_LEN];
	U8 pdpAddr0[NEUL_IPV6_LEN];
	U8 pdpAddr1[NEUL_IPV6_LEN];
	S32 rsrp;
	S32 totalPower;
	S32 txPower;
	U32 txTime;
	U32 rxTime;
	U32 cellId;
	U32 signalEcl;
	S32 snr;
	U32 earfcn;
	U32 signalPci;
	S32 rsrq;
	U8  opmode;	
	/*U8 regStatus;*/ /*0 Not registered, UE is not currently searching an operator to register to##1 Registered, home network##2 Not registered, but UE is currently trying to attach or searching an operator to register to##3 Registration denied##4 Unknown (e.g. out of E-UTRAN coverage)##5 Registered, roaming*/
	/*U8 conStatus;*/ /*0 Idle, 1 Connected*/	
}ST_NEUL_DEV;

typedef enum
{
	BC28_INIT = 0,
	BC28_CGA,
	BC28_GIP,
	BC28_READY,
	BC28_ERR,
	BC28_TEST,
	BC28_UPDATE,
	BC28_RESET,
	BC28_WAITCGA,
	BC28_UNKNOW
}EN_BC28_STATE;

//config parameter
typedef struct
{
	U8 band;
	U8 scram; /*ÈÅÂë*/	
	char apn[APN_NAME_LEN]; /*ÓòÃû*/
	U32 earfcn;
}ST_NB_CONFIG;

typedef enum{
	NONE = 0,
	RISE_STATE = 1,
	ING_STATE = 2,
	OVER_STATE = 3,
}EN_STATE;

typedef struct{
	T_MUTEX mutex;
	OS_SEMAPHORE_T sem;
	int event;
	int state; //0. nothing, 1.gen event, 2.proc ing, 3.proc over
	int reqArgc;
	char *reqArgv[MAX_ARGC];
	int retCode;
	int rspArgc;
	char *rspArgv[MAX_ARGC];
}ST_EVENT;

typedef struct{
	T_MUTEX mutex;
	int haveData; //0.no data, 1.have data
	U32 tick;
	char data[MAX_MTU];
	int len;
}ST_TX_DATA;

typedef enum{
	SETCDP_EVENT = 0,
	SETREGMOD_EVENT,
	IOTREG_EVENT,
	TIME_EVENT,
	UESTATE_EVENT,
	NBCMD_EVENT,
	CSQ_EVENT,
	VER_EVENT,
	RMSG_EVENT,
	SMSG_EVENT,
	IMEI_EVENT,
	ICCID_EVENT,
	IMSI_EVENT,	
	PID_EVENT,
	CSEARFCN_EVENT,
	NCONFIG_EVENT,
	CGDCONTSET_EVENT,
	CGDCONTGET_EVENT,
	CGATTSET_EVENT,
	CFUNSET_EVENT,
	ICCIDIMM_EVENT,
	BANDSET_EVENT,
	BANDGET_EVENT,
	MAX_EVENT,
}EN_EVENT;

typedef struct{
	T_MUTEX mutex;
	int iPktRxFromQue;
	int iPktTx;
	int iPktTxSucc;	
	int iPktTxFail;
	int iPktTxExpire;	
	int iNnmiRx;
	int iPktPushSuc;
	int iPktPushFail;
	int iTxEnable;
}ST_BC28_PKT;

#define REBOOTING    0
#define REBOOTED     1

typedef enum{
	REBOOT_OK = 0,
	REBOOT_EXPIRE,
	REBOOT_UNKNOW
};

typedef struct{
	T_MUTEX	mutex;
	int iHardRebooted;    /*0, rebooting 1,rebooted*/
	int iResult;
}ST_HARD_RESET;

#define ABNORMAL_PROTECT_ENABLE    0x01
#define ABNORMAL_PROTECT_ENABLE_H    0xAABBCCDD
#define ABNORMAL_PROTECT_ENABLE_L    0x11223344
#define ABNORMAL_PROTECT_DISABLE    0x0
#define ABNORMAL_PROTECT_DISABLE_H   0x0
#define ABNORMAL_PROTECT_DISABLE_L   0x0

typedef struct{
	T_MUTEX mutex;
	U32 udwAbnormalH;
	U32 udwAbnormalL;
}ST_SUPPORT_ABNORMAL_PARA;

#define EARFCN_ENABLE      0x01
#define EARFCN_ENABLE_H    0xAABBCCDD
#define EARFCN_ENABLE_L    0x11223344
#define EARFCN_DISABLE     0x00
#define EARFCN_DISABLE_H   0x00
#define EARFCN_DISABLE_L   0x00

typedef struct{
	T_MUTEX mutex;
	U32 enableH;
	U32 enableL;
	unsigned short earfcn;
	unsigned short band;
}ST_BC28_RAM_PARA;

#define FIRMWARE_NONE               	0
#define FIRMWARE_DOWNLOADING        	1
#define FIRMWARE_DOWNLOADED         	2
#define FIRMWARE_UPDATING           	3
#define FIRMWARE_FOTA_UPGRADE_REBOOT    4
#define FIRMWARE_UPDATE_SUCCESS     	5
#define FIRMWARE_UPDATE_OVER        	6
#define FIRMWARE_UPDATE_FAILED      	7

typedef struct{
	T_MUTEX mutex;
	U8 ucFwState;
	U8 ucFwLastState;
}ST_FW_ST;

typedef struct{
	T_MUTEX mutex;
	U32 startTick;
	U32 endTick;
	U32 eplaseTick;
}ST_ATTACH_TIME;

#define DIS_DEFAULT_WINDOWS    1800
#define DIS_DEFAULT_INTERVAL      10
#define DIS_WINDOW_MAX_SECOND       (3600)
#define DIS_INTRVL_SECOND           (2)

typedef struct{
	T_MUTEX mutex;
	U32 udwDiscreteWin;
	U32 udwDiscreteInterval;
}ST_DISCRETE_PARA;

typedef struct{
	U32 udwDiscreteWin;
	U32 udwDiscreteInterval;
}ST_DISCRETE_SAVE_PARA;

#define DIS_ENABLE      0x01
#define DIS_ENABLE_H    0x00
#define DIS_ENABLE_L    0x00
#define DIS_DISABLE     0x00
#define DIS_DISABLE_H   0xAABBCCDD
#define DIS_DISABLE_L   0x11223344

typedef struct{
	T_MUTEX mutex;
	U32 enableH;
	U32 enableL;
}ST_DISCRETE_ENABLE;

#pragma pack()

extern ST_NB_CONFIG g_stNbConfigDefault;

U8 is_ipv4_address(char* host);
int sendEvent(int event, int state, int argc, char *argv[]);
int recvEvent(int event, int *argc, char *argv[], int timeout);
int GetDataStatus();
int GetDataLength();
int SetDataStatus(int haveData);
int NbiotDataRead(char *buf, int maxrlen, int mode, unsigned int timeout);
int NbiotHardResetStart(void);
int NbiotHardResetResult(void);
int NbiotHardReset(int timeout);
int NbiotHardResetInner(void);
int NeulBc28SetConfig(ST_NB_CONFIG *pstConfigPara);
int NeulBc28GetConfig(ST_NB_CONFIG *pstConfigPara);
int NeulBc28GetConfigFromFlash(ST_NB_CONFIG *pstConfigPara);
int NeulBc28SetConfigToFlash(ST_NB_CONFIG *pstConfigPara);
int NeulBc28RestoreFactory(void);
int NeulBc28GetVersion(char * version,int len);
int NeulBc28SetUpdParam(const char *rip, const unsigned short port, const int socket);
int NeulBc28GetCscon(void);
int NeulBc28GetCereg(void);
int NeulBc28QueryUestats(ST_NEUL_DEV * pstNeulSignal);
U32 NeulBc28QueryRsrp(U8 targetId, U32 * rsrp);
U32 NeulBc28QueryRegCon(U8 targetId,U32 * regCon);
int NeulBc28QueryNetstats(U8 * regStatus, U8 * conStatus);
int NeulBc28QueryIOTstats(signed char * iotStatus);
int NeulBc28QueryLastIOTstats(signed char * iotStatus);
int NeulBc28SetRegisterNotify(int regNty);
int NeulBc28RegisterNotify(void);
int NeulBc28QueryCsq(int * rsrp, int * ber);
int NeulBc28SetCdpServer(const char *ipaddr);
int NeulBc28HuaweiDataEncryption(void);
int NeulBc28HuaweIotSetRegmod(int flag);
int NeulBc28HuaweiIotQueryReg(int * type);
int NeulBc28HuaweiIotRegistration(const int flag);
int NeulBc28QueryNatSpeed(int * baud_rate, int * sync_mode, int * stopbits, int * parity);
int NeulBc28SetNatSpeed(void);
int NeulBc28SendCoapPaylaod(const char *buf, int sendlen);
int NeulBc28GetUnrmsgCount(char *buf, int len);
int NeulBc28GetUnsmsgCount(char *buf, int len);
int NeulBc28GetNnmi(char *buf, int len);
int NeulBc28SendCoapPaylaodByNmgs(const char *buf, int sendlen);
int NeulBc28ReadCoapMsgByNmgr(char *outbuf, int maxrlen);
int NeulBc28LogLevelSet(const char *core, const char *level);
int NeulBc28LogLevelGet(char * buffer, int len);
int NeulBc28Command(const char *cmd, char *buffer, int len);
int NeulBc28CreateUdpSocket(unsigned short localport);
int NeulBc28CloseUdpSocket(int socket);
int NeulBc28SocketConfigRemoteInfo(int socket, const char *remoteip, unsigned short port);
int NeulBc28UdpSend(int socket, const char *buf, int sendlen);
int NeulBc28UdpRead(int socket,char *buf, int maxrlen, int mode);
int NeulBc28UdpNewRead(int socket,char *buf, int maxrlen, int mode, int * rleft);
int NeulBc28HwInit(void);
int NeulBc28DfotaSet(int updateMode);
int NeulBc28Sleep(int ticks);
int NeulBc28QueryClock(char * time);
void NeulBc28Thread(void *pvParameter);
int uart_data_flush(void);
int NeulBc28DataSend(const MSG_ITEM_T *item);
U8 NeulBc28IsBusy(void);
int NeulBc28Init(void);
int NeulBc28ProtoInit(EN_PROTOCOL_TYPE protocolType);
int NeulBc28StateChgReg(neul_state_chg func);
int NeulBc28DevChgReg(neul_state_chg func);
int NeulBc28IotChgReg(neul_state_chg func);
int NeulBc28TimeReg(neul_time_set func);
int NeulBc28SetDevState(int state);
int NeulBc28GetDevState(void);
int NeulBc28SetLastDevState(int state);
int NeulBc28GetLastDevState(void);
int NeulBc28SetConState(int con);
int NeulBc28GetConState(void);
int NeulBc28SetLastConState(int con);
int NeulBc28GetLastConState(void);
int NeulBc28SetRegState(int reg);
int NeulBc28GetRegState(void);
int NeulBc28SetLastRegState(int reg);
int NeulBc28GetLastRegState(void);
int NeulBc28SetPsmState(int psm);
int NeulBc28GetPsmState(void);
int NeulBc28SetLastPsmState(int psm);
int NeulBc28GetLastPsmState(void);
int NeulBc28SetIotState(int iot);
int NeulBc28GetIotState(void);
int NeulBc28SetLastIotState(int iot);
int NeulBc28GetLastIotState(void);
int NeulBc28SetSupportAbnormal(int abnormal);
int NeulBc28GetSupportAbnormal(void);
int NeulBc28SetEarfcnToRam(U8 enable, U16 earfcn);
int NeulBc28GetEarfcnFromRam(U8 *enable, U16 *earfcn);
int NeulBc28GetBandFromRam(U8 *enable, U16 *band);
int NeulBc28SetParaToRam(U8 enable, U16 earfcn, U16 band);
int NeulBc28GetParaFromRam(U8 *enable, U16 *earfcn, U16 *band);
int NeulBc28HardRestResultGet();
int NeulBc28HardRestStateGet();
int NeulBc28GetFwState(U8 * pucCurSt, U8 * pucLastSt);
int NeulBc28SetAttachStartTime();
int NeulBc28SetAttachEndTime();
U32 NeulBc28GetAttachEplaseTime();
U32 NeulBc28SetDiscreteParaToFlash(ST_DISCRETE_SAVE_PARA *pstDisSavePara);
U32 NeulBc28GetDiscreteParaFromFlash(ST_DISCRETE_SAVE_PARA *pstDisSavePara);
U8 OppGetDisOffsetSecond(U32 disPeriodWnd, U32 disIntvl, U8 mac1,U8 mac0,U32* outDisOffset);
U32 NeulBc28DisOffsetSecondGet();
U32 NeulBc28AttachDisTick(void);
U32 NeulBc28SetDiscretePara(ST_DISCRETE_SAVE_PARA *pstDisSavePara);
U32 NeulBc28GetDiscretePara(ST_DISCRETE_SAVE_PARA *pstDisSavePara);
U32 NeulBc28SetDiscreteWindow(U32 disWin);
U32 NeulBc28GetDiscreteWindow(void);
U32 NeulBc28GetDiscreteScale(void);
U32 NeulBc28SetDisEnable(U32 enable);
U32 NeulBc28SetDisEnableWithoutMutex(U32 enable);
U32 NeulBc28GetDisEnable(void);

#endif /* HELLO_GIGADEVICE_H */
