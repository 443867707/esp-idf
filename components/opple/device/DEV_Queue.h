#ifndef __OPP_QUEUE_H__
#define __OPP_QUEUE_H__


/******************************************************************************
*                                Defines                                      *
******************************************************************************/
//#define OPP_QUEUE_ELE_LEN_MAX 30



/******************************************************************************
*                                Typedefs                                     *
******************************************************************************/
//#pragma pack(1)

typedef enum
{
	QEC_SUCCESS=0,
	QEC_LEN_ERROR,
	QEC_QUEUE_FULL,
	QEC_QUEUE_EMPTY,
	QEC_INPUT_PARA_ERROR,
	QEC_MEMORY_ERROR,
	
}t_queue_err_code;

typedef enum
{
	QB_FALSE=0,
	QB_TRUE,
}t_queu_bool;

//typedef unsigned char  t_queue_buf[OPP_QUEUE_ELE_LEN_MAX];
typedef unsigned char* t_queue_buf;


typedef struct
{
		// unsigned short len;
    t_queue_buf data;
}t_queue_element;


//typedef unsigned char* t_queue_element;

typedef struct
{
    int ulFront;
    int ulRear;
    int depth;
    int length;
    //t_queue_element* element;
    unsigned char* buffer;
}t_queue;


//#pragma pack()

/******************************************************************************
*                                Interfaces                                   *
******************************************************************************/
extern t_queue_err_code initQueue(t_queue* this,unsigned char* buf,unsigned short size,\
unsigned short depth,unsigned short length);
extern t_queue_err_code pushQueue(t_queue* this, unsigned char* data,unsigned char option);
extern t_queue_err_code pullQueue(t_queue* this, unsigned char* data,unsigned short inLen);
extern unsigned char lengthQueue(t_queue* this);
extern t_queu_bool isQueueFull(t_queue* this);
extern t_queu_bool isQueueEmpty(t_queue* this);





#endif

