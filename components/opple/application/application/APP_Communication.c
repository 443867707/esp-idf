/**
******************************************************************************
* @file    APP_Communication.c
* @author  Liqi
* @version V1.0.0
* @date    2018-6-22
* @brief   
******************************************************************************

******************************************************************************
*/ 

#include "Includes.h"
#include "APP_Communication.h"

/**
	@brief 消息处理接口
	@attention none
	@param chl 源通道号
	@param data 数据
	@param len 数据长度
	@retval none
*/
typedef void (*p_fun_msg_handler)(EN_MSG_CHL chl, const MSG_ITEM_T *item);
const p_fun_msg_handler MsgHandler[MSG_HANDLER_MAX]= \
MSG_HANDLER_LIST;

/**
	@brief 初始化
	@attention none
	@param none
	@retval none
*/
void AppComInit(void)
{
	// Do nothing
}

/**
	@brief 从不同的通信通道接收队列提取数据分发到各处理模块
	@attention none
	@param none
	@retval none
*/
void AppComLoop(void)
{
	MSG_ITEM_T item;
	static unsigned char buf[BUF_LEN_MAX];
	
	for(int i = 0;i < MSG_CHL_MAX;i++)
	{
		item.data = buf;
		item.len = BUF_LEN_MAX;
		if(MsgPull(i,MSG_QUEUE_TYPE_RX,&item) == 0)
		{
			if(item.type < MSG_HANDLER_MAX)
			{
			    if(MsgHandler[item.type] != 0)
			    {
			    	(MsgHandler[item.type])(i, &item);
			    }
	   		}
		}
	}
}

/**
	@brief 统一的数据发送通道
	@attention 若是通道间数据透传转发则可能将数据发往另外一个通道的RX队列
			   因此不对QType做任何限定
	@param msg 需要发送的数据
	@param priority 优先级
	@retval none
*/
int AppMessageSend(EN_MSG_CHL chl,EN_MSG_QUEUE_TYPE qtype,MSG_ITEM_T *msg,EN_MSG_PRI priority)
{
	return MsgPush(chl,qtype,msg,MSG_PUSH_OPTION,priority);
}




















