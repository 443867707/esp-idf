#ifndef __APP_COM_H__
#define __APP_COM_H__

#include "SVS_MsgMmt.h"
#include "SVS_Coap.h"
#include "SVS_Opple.h"


#define MSG_PUSH_OPTION 0           /* 0:non-overwrite queue,1:overwrite queue*/
#define MSG_CHL_MAX     CHL_MAX     /* determined by msgmmt */
#define MSG_HANDLER_MAX 2           /* handlers for handling protocols,defines below*/

/**
  Э���������ע���
  MSG_ITEM_T��type����������±걣��һ��
  typedef struct
  {
		unsigned char type;      // eg. 0:opple ,1:huawei
		unsigned int  srcIp;     // src ip
		unsigned short srcPort;  // src port
		unsigned short len;   // Length of Data(chl,dir,type except)
		unsigned char* data;  // Data
  }MSG_ITEM_T;
*/
#define MSG_HANDLER_LIST {\
	(void*)0,\
    ApsCoapOceanconProcess,\
}

extern void AppComInit(void);
extern void AppComLoop(void);
extern int AppMessageSend(EN_MSG_CHL chl,EN_MSG_QUEUE_TYPE qtype,MSG_ITEM_T *msg,EN_MSG_PRI priority);



#endif


