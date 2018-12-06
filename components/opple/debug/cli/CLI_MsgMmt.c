#include "cli-interpreter.h"
#include "SVS_MsgMmt.h"


void CommandQueuePush(void);
void CommandQueuePull(void);
void CommandQueueShow(void);
void CommandIsBusy(void);


const char* const arg_MsgMmt_push[] = {
  "The queue id(0:nbTX,1:nbRX,2:wifiTX,3:wifiRX,4:nbTxImd)",
  "len",
  "data",
  "option(0:no-overwrite,1:overwrite)"
};

const char* const arg_MsgMmt_pull[] = {
  "The queue id(0:nbTX,1:nbRX,2:wifiTX,3:wifiRX,4:nbTxImd)",
};

const char* const arg_MsgMmt_isbusy[] = {
  "The channel id(0:NB_CHL,1:WIFI_CHL)",
};

CommandEntry CommandTable_MsgMmt[] =
{
	CommandEntryActionWithDetails("push", CommandQueuePush, "uau", "push queue...", arg_MsgMmt_push),
	CommandEntryActionWithDetails("pull", CommandQueuePull, "u", "pull queue...", arg_MsgMmt_pull),
	CommandEntryActionWithDetails("show", CommandQueueShow, "u", "show queue...", arg_MsgMmt_pull),
  CommandEntryActionWithDetails("isBusy", CommandIsBusy, "u", "is busy...", arg_MsgMmt_isbusy),
	CommandEntryTerminator()
};

int CliGetQueue(int chl, const t_queue* queue)
{
    return MsgMmtGetQueue(chl,queue);
}

void CommandIsBusy(void)
{
    int res,busy,chl;

    chl = CliIptArgList[0][0];
    
    if((chl+1) > 2)
    {
        CLI_PRINTF("Error,Chl >= CHL_ID_MAX\r\n");
        return;
    }
    
    res = MsgMmtIsBusy(chl,&busy);
    if(res != 0)
    {
        CLI_PRINTF("Error,p_isbusy_chl[%d] is null\r\n",chl);
    }
    else
    {
        CLI_PRINTF("IsBusy = %d\r\n",busy);
    }
}

void CommandQueuePush(void)
{
	int len;
	int option;
	unsigned char buf[60];
  t_queue* queue;
  int res;
	
  if(CliGetQueue(CliIptArgList[0][0],&queue)!=0)
  {
        CLI_PRINTF("Can not find queue!\r\n");
        return;
  }
  else
  {
        CLI_PRINTF("Queue address : 0x%04X\r\n",(unsigned int)queue);
  }
  
	len = CliIptArgList[1][0];
	option = CliIptArgList[3][0];
	if(len == 0) {CLI_PRINTF("No element,input len=0!\r\n");return;}
	for(int i=0;i<len;i++) buf[i] = (unsigned char)CliIptArgList[2][i];
  res = pushQueue(queue,buf,option);
	if(res != 0) CLI_PRINTF("Queue push fail,err=%d!\r\n",res);
}

void CommandQueuePull(void)
{
	unsigned char buf[60];
  t_queue* queue;
  int res;

  if(CliGetQueue(CliIptArgList[0][0],&queue)!=0)
   {
        CLI_PRINTF("Can not find queue!\r\n");
        return;
  }
  else
  {
        CLI_PRINTF("Queue address : 0x%04X\r\n",(unsigned int)queue);
  }
  
  res = pullQueue(queue,buf,60);
	if(res == 0){
		for(int j=0;j< queue->length;j++)
		{
			CLI_PRINTF("0x%02X ",buf[j]);
		}
		CLI_PRINTF("\r\n");
  }else
	{
		CLI_PRINTF("Queue pull fail,err=%d!\r\n",res);
	}
}

void CommandQueueShow(void)
{
	t_queue* queue;
  
  if(CliGetQueue(CliIptArgList[0][0],&queue)!=0)
  {
        CLI_PRINTF("Can not find queue!\r\n");
        return;
  }
  else
  {
        CLI_PRINTF("Queue address : 0x%04X\r\n",(unsigned int)queue);
  }

  CLI_PRINTF("===================================\r\n");
	CLI_PRINTF("Length=%d,Depth=%d,Rear=%d,Font=%d,Num=%d\r\n",queue->length,\
	queue->depth,queue->ulRear,queue->ulFront,lengthQueue(queue) );
  /*
	for(int i=0;i<queue->depth;i++)
	{
		for(int j=0;j< queue->length;j++)
		{
			CLI_PRINTF("0x%02X ",queue->data[i*queue->length + j]);
		}
		CLI_PRINTF("\r\n");
	}
  */
	CLI_PRINTF("===================================\r\n");
}
