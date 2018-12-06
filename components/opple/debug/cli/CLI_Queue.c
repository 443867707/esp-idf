#include "cli-interpreter.h"
#include "DEV_Queue.h"



static t_queue queue;
static unsigned char array[60];


const char* const arg_queue_init[] = {
    "Buffer size(<=60)",
	"Depth",
	"Length"
};


void CommandQueueInit(void);
void CommandQueuePush(void);
void CommandQueuePull(void);
void CommandQueueShow(void);

CommandEntry CommandQueueTable[] =
{
	CommandEntryActionWithDetails("init", CommandQueueInit, "uuu", "init queue...", arg_queue_init),
	CommandEntryActionWithDetails("push", CommandQueuePush, "uau", "push queue...", (const char* const *)NULL),
	CommandEntryActionWithDetails("pull", CommandQueuePull, "", "pull queue...", (const char* const *)NULL),
	CommandEntryActionWithDetails("show", CommandQueueShow, "", "show queue...", (const char* const *)NULL),
	CommandEntryTerminator()
};

void CommandQueueInit(void)
{
	if(CliIptArgList[0][0] > 60)
    {
        CLI_PRINTF("Fail (Buffer size over 60)!\r\n");
        return;
    }
    if(initQueue(&queue,array,CliIptArgList[0][0],CliIptArgList[1][0],CliIptArgList[2][0])!=0)
		CLI_PRINTF("Queue init fail!\r\n");
}

void CommandQueuePush(void)
{
	int len;
	int option;
	unsigned char buf[60];
	
	len = CliIptArgList[0][0];
	option = CliIptArgList[2][0];
	if(len == 0) {CLI_PRINTF("No element!\r\n");return;}
	for(int i=0;i<len;i++) buf[i] = (unsigned char)CliIptArgList[1][i];
	if(pushQueue(&queue,buf,option) != 0) CLI_PRINTF("Queue push fail!\r\n");
}

void CommandQueuePull(void)
{
	unsigned char buf[60];
	if(pullQueue(&queue,buf,60) == 0){
		for(int j=0;j< queue.length;j++)
		{
			CLI_PRINTF("0x%02X ",buf[j]);
		}
		CLI_PRINTF("\r\n");
  }else
	{
		CLI_PRINTF("Queue empty!\r\n");
	}
}

void CommandQueueShow(void)
{
	CLI_PRINTF("===================================\r\n");
	CLI_PRINTF("Length=%d,Depth=%d,Rear=%d,Font=%d,Num=%d\r\n",queue.length,\
	queue.depth,queue.ulRear,queue.ulFront,lengthQueue(&queue) );
	for(int i=0;i<queue.depth;i++)
	{
		for(int j=0;j< queue.length;j++)
		{
			CLI_PRINTF("0x%02X ",array[i*queue.length + j]);
		}
		CLI_PRINTF("\r\n");
	}
	CLI_PRINTF("===================================\r\n");
}
