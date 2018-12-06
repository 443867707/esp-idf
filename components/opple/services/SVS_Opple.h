#ifndef _SVS_OPPLE_H
#define _SVS_OPPLE_H

#pragma pack(1)
#define UPDATE_REQ    0x01
#define UPDATE_ACK    0x02
#define CONTENT       0x10
#define CONTENT_ACK   0x11
#define FIN           0x20
#define FIN_ACK       0x21

typedef struct
{
	unsigned char imei[15];
	unsigned short seqno;
	unsigned char cmd;
	unsigned short len;
}OPP_OTA_HDR;

typedef enum
{
	UPDATING = 0,
	UPDATE_FAIL,
	UPDATE_SUCESS 
}EN_UPDATE;


#pragma pack()

u16 OppCalcCRC16(u8 *buf , u32 len);
void OppOtaUpdateReq(unsigned char cmd, unsigned short seqNo);
void OppOtaSendAck(unsigned char cmd, unsigned short seqNo, unsigned char retCode);
void OppOtaRecvUdpUpdate(U8 *pucVal, U16 nLen);
void OppOtaTask(void);
void OppOtaInit(void);
void OppOtaLoop(void);

#endif
