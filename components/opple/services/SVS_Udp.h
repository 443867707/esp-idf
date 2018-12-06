#ifndef __SVS_UDP_H__
#define __SVS_UDP_H__
#include <sys/socket.h>

#define UDP_TO    60000

#pragma pack(1)
typedef struct _list_udp_client_entry
{
	struct sockaddr_in addr;
	U32 tick;
	U32 rx;
	U32 pushFail;
	U32 txTick;
	U32 tx;
	U32 txFail;
	LIST_ENTRY(_list_udp_client_entry) elements;	
}ST_UDP_CLIENT_ENTRY;
#pragma pack()

typedef void (*myprint)(char* msg,...);

void UdpSend(const MSG_ITEM_T *item);
unsigned char UdpIsBusy(void);
void UdpThread(void *pvParameter);
void UdpClientListShow(myprint print);
void UdpClientListTxUpdate(struct sockaddr_in *addr, int sendFail);

#endif
