#ifndef __SVS_MSGMMT_H__
#define __SVS_MSGMMT_H__

#include "DEV_Queue.h"

#define MSG_INFO_SIZE       6
#define MSG_HEAD_SIZE       (MSG_INFO_SIZE+3)
#define NB_ELE_LEN          (512+MSG_HEAD_SIZE)
#define WIFI_ELE_LEN        (512+MSG_HEAD_SIZE)
#define NB_BUF_SIZE         ((NB_ELE_LEN)*12)
#define WIFI_BUF_SIZE       ((WIFI_ELE_LEN)*12)
#define NB_BUF_TX_IMD_SIZE  ((NB_ELE_LEN)*4)


#define BUF_LEN_MAX ((NB_ELE_LEN)>(WIFI_ELE_LEN)?(NB_ELE_LEN):(WIFI_ELE_LEN))

typedef enum
{
	CHL_NB=(unsigned char)0,
	CHL_WIFI,
	CHL_BLE,
	/*------------*/
	CHL_MAX,
}EN_MSG_CHL;

typedef enum
{
	MSG_QUEUE_TYPE_TX=0,
	MSG_QUEUE_TYPE_RX,
}EN_MSG_QUEUE_TYPE;

typedef enum
{
	MSG_TYPE_OPPLE=(unsigned char)0,
	MSG_TYPE_HUAWEI,
}EN_MSG_TYPE;

typedef enum
{
    PRIORITY_LOW=0,
    PRIORITY_HIGH,
}EN_MSG_PRI;

typedef struct{
		unsigned char type;      // 0:opple,1:huawei
		unsigned char info[MSG_INFO_SIZE];   // info(ip,port)
		unsigned short len;      // Length of Data
		unsigned char* data;     // Data
}MSG_ITEM_T;

typedef void (*p_fun_sender)(MSG_ITEM_T *item);
typedef unsigned char (*p_fun_isbusy)(void);

extern void MsgMmtInit(void);
extern void MsgMmtLoop(void);
extern int MsgPull(EN_MSG_CHL chl,EN_MSG_QUEUE_TYPE qtype,MSG_ITEM_T *item);
extern int MsgPush(EN_MSG_CHL chl,EN_MSG_QUEUE_TYPE qtype,MSG_ITEM_T *item,unsigned char option,EN_MSG_PRI priority);
extern int MsgMmtSenderReg(unsigned char chl,p_fun_sender sender,p_fun_isbusy fun);

extern int MsgMmtIsBusy(int chl,int* isbusy);
extern int MsgMmtGetQueue(int chl,t_queue** queue);

#endif


