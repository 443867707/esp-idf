#ifndef __SVS_OTA_H__
#define __SVS_OTA_H__

#define IP_LEN      18
#define PORT_LEN    6
#define FILE_LEN    64

extern char g_aucOtaSvrIp[];
extern char g_aucOtaSvrPort[];
extern char g_aucOtaFile[];
extern int g_aucOtaStart;

extern void OtaStart(void);

extern void OtaThread(void *pvParameter);
/*******nb ota*********/
#define OTA_HDR                 0xFFFE
#define VER_LEN                 16
#define PRO_VER                 0x01
#define PKT_RETRY_TIMES         3
#define NB_OTA_ID               0

#define QRY_VER_MSG					19
#define NEW_VER_NTY_MSG				20
#define PKT_REQ_MSG					21
#define DL_ST_REPORT_MSG       		22
#define EXC_UPGRADE_MSG          	23
#define UPGRADE_RESULT_REPORT_MSG   24

#define NBOTA_START()    do{ \
	g_ucOtaPsm = 1; \
}while(0)

#define NBOTA_STOP()    do{ \
	g_ucOtaPsm = 0; \
}while(0)

#pragma pack(1)
typedef struct{
	unsigned short hdr;
	unsigned char protVersion;
	unsigned char msg;
	unsigned short crc;
	unsigned short length;
}ST_NB_OTA;

typedef struct{
	char dstVersion[VER_LEN];
	unsigned short fragSize;
	unsigned short fragNum;
	unsigned short checkCode;
}ST_VER_NOTY;

typedef struct{
	unsigned char dstVersion[VER_LEN];
	unsigned short fragNum;
}ST_PKT_REQ;

typedef struct{
	unsigned char retCode;
	unsigned short fragNum;
}ST_PKT_RSP;

typedef struct{
	ST_VER_NOTY stVerNoty;
	unsigned short curFragNum;
}ST_NB_OTA_STATE;

typedef enum{
	RETCODE_SUCC=0X00,	//????3¨¦1|
	RETCODE_DEV_UESED=0X01,	//¨¦¨¨¡À?¨º1¨®??D
	RETCODE_SIGNAL_POOR=0X02,	//D?o??¨º¨¢?2?
	RETCODE_NEWEST_VER=0X03,     //¨°??-¨º?¡Á?D?¡ã?¡À?
	RETCODE_NOT_ENOUGH_POWER=0X04,	//¦Ì?¨¢?2?¡Á?
	RETCODE_NOT_ENOUGH_ROM=0X05, //¨º¡ê¨®¨¤????2?¡Á?
	RETCODE_DL_TIMEOUT=0X06,	//????3?¨º¡À
	RETCODE_DL_CKERR=0X07,	//¨¦y??¡ã¨¹D¡ê?¨¦¨º¡ì¡ã¨¹
	RETCODE_DL_PKT_NOTSUPPORT=0X08,	//¨¦y??¡ã¨¹¨¤¨¤D¨ª2??¡ì3?
	RETCODE_NOT_ENOUGH_RAM=0X09, //?¨²¡ä?2?¡Á?
	RETCODE_UPDATE_FAIL=0X0A,	//¡ã2¡Á¡ã¨¦y??¡ã¨¹¨º¡ì¡ã¨¹	
	RETCODE_INTER_EXP=0X7F,	//?¨²2?¨°¨¬3¡ê
	RETCODE_NOT_UPDATE_TASK=0X80,	//¨¦y??¨¨???2?¡ä??¨²
	RETCODE_NOT_FRAGNUM=0X81,	//???¡§¦Ì?¡¤???2?¡ä??¨²
}EN_RETCODE;

typedef enum{
	JUDGE = 0,
	UPDATE,
	EXCEPTION,
	FINISH,
}EN_NBOTA_PSM;
#pragma pack()

int NbOtaInit();
int NbOtaMsgProcess(unsigned char * data, unsigned short len);
void NbOtaThread(void *pvParameter);
int NbOtaGetState();
void NbOtaSetState(unsigned char state);

#endif
