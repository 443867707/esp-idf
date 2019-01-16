/**
******************************************************************************
* @file    SVS_MsgMmt.c
* @author  Liqi
* @version V1.0.0
* @date    2018-6-22
* @brief   
******************************************************************************

******************************************************************************
*/ 

#include "Includes.h"
#include "SVS_MsgMmt.h"
#include <string.h>
#include "OPP_Debug.h"
#include "Opp_ErrorCode.h"

#define IMD_ELE_DEPTH   ((NB_BUF_TX_IMD_SIZE)/(NB_ELE_LEN))
#define NB_ELE_DEPTH    ((NB_BUF_SIZE)/(NB_ELE_LEN))
#define WIFI_ELE_DEPTH  ((WIFI_BUF_SIZE)/(WIFI_ELE_LEN))
#define CHL_ID_MAX      2 

#define NB_TXMUTEX    0
#define NB_RXMUTEX    1
#define WIFI_TXMUTEX  2
#define WIFI_RXMUTEX  3
#define IMD_TXMUTEX   4
#define MAX_MUTEX     5

#define MSGMMT_P_CHECK(p) \
		if((p)==(void*)0){\
		DEBUG_LOG(DEBUG_MODULE_MSGMMT,DLL_ERROR,"null pointer!\r\n");\
		return OPP_NULL_POINTER;}


#pragma pack(1)
typedef struct{
		unsigned char type;     // 0:opple,1:huawei
		unsigned char info[MSG_INFO_SIZE];   // info(ip,port)
		unsigned short len;     // Length of Data(chl,dir,type except)
		unsigned char data[1];  // Data
}MSG_FORMAT_T;
#pragma pack()

static t_queue queue_nb_tx,queue_nb_rx,queue_wifi_tx,queue_wifi_rx,queue_nb_tx_imd;
static unsigned char buf_nb_tx[NB_BUF_SIZE],buf_nb_rx[NB_BUF_SIZE],buf_nb_tx_imd[NB_BUF_TX_IMD_SIZE];
static unsigned char buf_wifi_tx[WIFI_BUF_SIZE],buf_wifi_rx[WIFI_BUF_SIZE];
static T_MUTEX m_astMsgMutex[MAX_MUTEX];
static p_fun_sender p_fun_sender_chl[CHL_ID_MAX];
static p_fun_isbusy p_fun_isbusy_chl[CHL_ID_MAX];

#define MSG_CREATE(index) MUTEX_CREATE(m_astMsgMutex[index])
#define MSG_LOCK(index) MUTEX_LOCK(m_astMsgMutex[index], MUTEX_WAIT_ALWAYS)
#define MSG_UNLOCK(index) MUTEX_UNLOCK(m_astMsgMutex[index])


inline void* MSG_MEMCPY_S(unsigned char* dst,unsigned char* src,int dst_len,int src_len)
{
	if(src_len > dst_len)
	{
		DEBUG_LOG(DEBUG_MODULE_MSGMMT, DLL_ERROR, "Memcpy len error!\r\n");
		src_len = dst_len;
	}
	return memcpy(dst,src,src_len);
}

/**
	@brief init
	@attention none
	@param none
	@retval none
*/
void MsgMmtInit(void)
{
	int i;
	
	initQueue(&queue_nb_tx,buf_nb_tx,NB_BUF_SIZE,NB_ELE_DEPTH,NB_ELE_LEN);
	initQueue(&queue_nb_rx,buf_nb_rx,NB_BUF_SIZE,NB_ELE_DEPTH,NB_ELE_LEN);
	initQueue(&queue_wifi_tx,buf_wifi_tx,WIFI_BUF_SIZE,WIFI_ELE_DEPTH,WIFI_ELE_LEN);
	initQueue(&queue_wifi_rx,buf_wifi_rx,WIFI_BUF_SIZE,WIFI_ELE_DEPTH,WIFI_ELE_LEN);
  	initQueue(&queue_nb_tx_imd,buf_nb_tx_imd,NB_BUF_TX_IMD_SIZE,IMD_ELE_DEPTH,NB_ELE_LEN);

	for(i=0;i<MAX_MUTEX;i++){
		MSG_CREATE(i);
		if(m_astMsgMutex[i]==NULL)
		{
			DEBUG_LOG(DEBUG_MODULE_MSGMMT, DLL_ERROR,"Mutex %d MSG_CREATE ERROR\r\n",i);
		}
	}
}

/**
	@brief pull message from queue
	@attention none
	@param chl 通道号参照EN_MSG_CHL
	@param qtype 队列类型参照EN_MSG_QUEUE_TYPE
	@param item the message 
	       -Input : item->len
           -Output: item->type,item->data,item->len
	@retval 0:success,1:fail
*/
int MsgPull(EN_MSG_CHL chl,EN_MSG_QUEUE_TYPE qtype,MSG_ITEM_T *item)
{
	t_queue* queue;
	unsigned char mutexIdx;
	t_queu_bool qbool;
	unsigned char buf[BUF_LEN_MAX];
	MSG_FORMAT_T *fmt;
	unsigned int err;

	MSGMMT_P_CHECK(item);
	
	if(chl == CHL_NB){
		if(qtype == MSG_QUEUE_TYPE_TX){
		mutexIdx = IMD_TXMUTEX;
		MSG_LOCK(mutexIdx);
		qbool = isQueueEmpty(&queue_nb_tx_imd);
		MSG_UNLOCK(mutexIdx);
		
      if(qbool==QB_FALSE ){
	  	  mutexIdx = IMD_TXMUTEX;
          queue = &queue_nb_tx_imd;
      }else{
	  	  mutexIdx = NB_TXMUTEX;
		  queue = &queue_nb_tx;
      }
		}else{
			mutexIdx = NB_RXMUTEX;
			queue = &queue_nb_rx;
		}
	}
	else if(chl == CHL_WIFI){
		if(qtype == MSG_QUEUE_TYPE_TX){
			mutexIdx = WIFI_TXMUTEX;
			queue = &queue_wifi_tx;
		}else{
			mutexIdx = WIFI_RXMUTEX;
			queue = &queue_wifi_rx;
		}
	}else{
		return 1;
	}
	
	MSG_LOCK(mutexIdx);
	err = pullQueue(queue,buf,BUF_LEN_MAX);
	if(err != 0)
	{
		MSG_UNLOCK(mutexIdx);
		return 10+err;
	}
	MSG_UNLOCK(mutexIdx);

	fmt = (MSG_FORMAT_T*)buf;
	if(item->len < fmt->len)
	{
		DEBUG_LOG(DEBUG_MODULE_MSGMMT, DLL_ERROR,"Msg pull len error!\r\n");
		return 3;
	}
	MSG_MEMCPY_S(item->info,fmt->info,MSG_INFO_SIZE,MSG_INFO_SIZE);
	// memcpy(item->data,fmt->data,fmt->len);
	MSG_MEMCPY_S(item->data,fmt->data,item->len,fmt->len);
	item->type = fmt->type;
	item->len = fmt->len;
	
	return 0;
	
}

/**
	@brief push message from queue
	@attention may push message to another rx queue
	@param chl 通道号参照EN_MSG_CHL
	@param qtype 队列类型参照EN_MSG_QUEUE_TYPE
	@param item the message
		   -Input : item->type,item->data,item->len
		   -Output: None
	@retval 0:success,1:fail
*/
int MsgPush(EN_MSG_CHL chl,EN_MSG_QUEUE_TYPE qtype,MSG_ITEM_T *item,unsigned char option,EN_MSG_PRI priority)
{
	t_queue* queue;
	unsigned char mutexIdx;
	unsigned int len_max;
	unsigned char buf[BUF_LEN_MAX];
	MSG_FORMAT_T *fmt;
	unsigned int err;

	MSGMMT_P_CHECK(item);
	
	if(chl == CHL_NB){
		len_max = NB_ELE_LEN;
		if(qtype == MSG_QUEUE_TYPE_TX){
      if(priority == PRIORITY_LOW){
	  	mutexIdx = NB_TXMUTEX;
		queue = &queue_nb_tx;
      }else{
		  mutexIdx = IMD_TXMUTEX;
         queue = &queue_nb_tx_imd;
      }
		}else{
			mutexIdx = NB_RXMUTEX;
			queue = &queue_nb_rx;
		}
	}else if(chl == CHL_WIFI){
		len_max = WIFI_ELE_LEN;
		if(qtype == MSG_QUEUE_TYPE_TX){
			mutexIdx = WIFI_TXMUTEX;
			queue = &queue_wifi_tx;
		}else{
			mutexIdx = NB_RXMUTEX;
			queue = &queue_wifi_rx;
		}
	}else{
		return 1;
	}

	if(item->len > (len_max-MSG_HEAD_SIZE)) return 2;

	fmt = (MSG_FORMAT_T*)buf;
	fmt->type = item->type;
	fmt->len = item->len;
	MSG_MEMCPY_S(fmt->info,item->info,MSG_INFO_SIZE,MSG_INFO_SIZE);
	//memcpy(fmt->data,item->data,item->len);
	MSG_MEMCPY_S(fmt->data,item->data,BUF_LEN_MAX-MSG_HEAD_SIZE,item->len);
	
	MSG_LOCK(mutexIdx);
	err = pushQueue(queue,buf,option);
	if(err != 0){
		MSG_UNLOCK(mutexIdx);
		return 10+err;
	}
	MSG_UNLOCK(mutexIdx);
	return 0;
	
}

/**
	@brief 将发送队列中需要发送的消息发送出去
	@attention none
	@param none
	@retval none
*/
void MsgMmtLoop(void)
{
	MSG_ITEM_T item;
	unsigned char buf[BUF_LEN_MAX];

	U32 tmp_t0,tmp_t1;
	
	// Read NB -> NB_RX_QUEUE ;Triggled by NB-DEV module
	// Read WIFI -> WIFI_RX_QUEUE;Triggled by WIFI-DEV module
	
	// TX_QUEUE -> SEND OUT
	for(int i=0;i < CHL_ID_MAX;i++)
	{
		if(p_fun_sender_chl[i]==NULL || p_fun_isbusy_chl[i]==NULL)
		{
			DEBUG_LOG(DEBUG_MODULE_MSGMMT, DLL_ERROR, "PFUN error!\r\n");
			continue;
		}
		
		if(p_fun_isbusy_chl[i]() == 0) // idle
		{
	    	item.data = buf;
	    	item.len = BUF_LEN_MAX;
	    	if(MsgPull(i,MSG_QUEUE_TYPE_TX,&item) == 0) // success
		    {
				tmp_t0 = OppTickCountGet();
		        //(p_fun_sender_chl[i])(item.type,item.data,item.len);
		        (p_fun_sender_chl[i])(&item);
		        tmp_t1 = OppTickCountGet();
		        DEBUG_LOG(DEBUG_MODULE_MSGMMT, DLL_INFO, "p_fun_sender_chl use time %u ms!\r\n",tmp_t1-tmp_t0);
		    }
		}
	}
}

/**
	@brief 为发送通道注册发送函数和发送繁忙查询函数
	@attention none
	@param chl 通道号
	@param sender 发送接口
	@param fun 发送繁忙查询接口
	@retval 0:success,1:fail
*/
int MsgMmtSenderReg(unsigned char chl,p_fun_sender sender,p_fun_isbusy fun)
{
	MSGMMT_P_CHECK(fun);
	
	if((chl+1) > CHL_ID_MAX)
	{
		DEBUG_LOG(DEBUG_MODULE_MSGMMT, DLL_ERROR, "Chl error!\r\n");
		return 1;
	}
		
	p_fun_sender_chl[chl] = sender;
	p_fun_isbusy_chl[chl] = fun;
	return 0;
}


int MsgMmtIsBusy(int chl,int* isbusy)
{
	MSGMMT_P_CHECK(isbusy);
	if((chl+1) > CHL_ID_MAX) return 1;

	if(p_fun_isbusy_chl[chl] == 0) return 2;

	*isbusy = p_fun_isbusy_chl[chl]();
	return 0;
}

int MsgMmtGetQueue(int chl,t_queue** queue)
{
	if(chl == 0){*queue = &queue_nb_tx;return 0;}
	else if(chl == 1){*queue = &queue_nb_rx;return 0;}
	else if(chl == 2){*queue = &queue_wifi_tx;return 0;}
	else if(chl == 3){*queue = &queue_wifi_rx;return 0;}
	else if(chl == 4){*queue = &queue_nb_tx_imd;return 0;}
	else {return 1;}
}











