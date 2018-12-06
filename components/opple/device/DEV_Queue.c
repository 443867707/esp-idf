/******************************************************************************
* Version     :                           
* File        :                                                  
* Description :                                                               
*                                                          
* Note        : (none)                                                        
* Author      : 智能控制研发部                                                
* Date:       :                                                   
* Mod History : (none)                                                        
******************************************************************************/



#ifdef __cplusplus
extern "C" {
#endif



/******************************************************************************
*                                Includes                                     *
******************************************************************************/
#include "DEV_Queue.h"
#include "string.h"



/******************************************************************************
*                                Defines                                      *
******************************************************************************/
#define REAR_INDEX_FULL (~((unsigned int)0))


/******************************************************************************
*                                Typedefs                                     *
******************************************************************************/



/******************************************************************************
*                                Globals                                      *
******************************************************************************/


/******************************************************************************
*                          Extern Declarations                                *
******************************************************************************/



/******************************************************************************
*                             Declarations                                    *
******************************************************************************/



/******************************************************************************
*                            Implementations                                  *
******************************************************************************/
// 队列初始化
t_queue_err_code initQueue(t_queue* this,unsigned char* data,unsigned short size,\
	unsigned short depth,unsigned short length)
{
    if((depth*length > size) || (depth*length==0) ) return QEC_MEMORY_ERROR;
    
    memset(this, 0, sizeof(t_queue));
    this->ulFront = this->ulRear = 0;
    this->buffer = data;
    this->depth = depth;
    this->length = length;
    
    return QEC_SUCCESS;
}

// 检查队列元素长度
t_queue_err_code isElementLenValid(t_queue* this,unsigned short len)
{
    if( (len == 0) ||  (len > this->length)) {
        return QEC_LEN_ERROR;
    } else{
        return QEC_SUCCESS;
    }
}

// 莠萤讚聬藝乇煤
t_queu_bool isQueueFull(t_queue* this)
{
	if (this->ulRear == REAR_INDEX_FULL)
  {
        return QB_TRUE;
  }
	else
	{
		return QB_FALSE;
  }
}

t_queu_bool willQueueFull(t_queue* this)
{
	if (((this->ulRear + 1) % (this->depth) ) == this->ulFront)
  {
        return QB_TRUE;
  }
	else
	{
		return QB_FALSE;
  }
}

t_queu_bool isQueueEmpty(t_queue* this)
{
    if (this->ulFront == this->ulRear)
    {
        return QB_TRUE;
    }
    else
	  {
		 return QB_FALSE;
    }
}

//入队
t_queue_err_code pushQueue(t_queue* this, unsigned char* data,unsigned char option)
{
    unsigned char willFull = 0;
    
    if(this->buffer == 0)
    {
        return QEC_INPUT_PARA_ERROR;
    }

    if (isQueueFull(this) == QB_TRUE)
    {
        if(option == 1)
        {
            this->ulRear = this->ulFront;
            this->ulFront = (this->ulFront + 1) % (this->depth); // over
        }
        else
        {
          return QEC_QUEUE_FULL;
        }
    }
    
    if(willQueueFull(this) ==QB_TRUE)
    {
        willFull = 1;
    }
    memcpy(( this->buffer + (this->ulRear) * (this->length)), data, this->length);
    
    if(willFull == 1)
    {
        this->ulRear = REAR_INDEX_FULL;
    }
    else
    {
        this->ulRear = (this->ulRear + 1) % (this->depth);
    }

    return QEC_SUCCESS;
}

//出队
t_queue_err_code pullQueue(t_queue* this, unsigned char* data,unsigned short inLen)
{
    if(this->buffer == 0)
    {
        return QEC_INPUT_PARA_ERROR;
    }
    
    if(inLen < this->length)
    {
    	return QEC_INPUT_PARA_ERROR;
    }
    
    if (isQueueEmpty(this)==QB_TRUE)
    {
        return QEC_QUEUE_EMPTY;
    }
    
    if(isQueueFull(this) == QB_TRUE)
    {
        this->ulRear = this->ulFront;
    }
    
    memcpy(data, (void*)(this->buffer +(this->ulFront)*(this->length)), this->length);

    this->ulFront = (this->ulFront + 1) % (this->depth);

    return  QEC_SUCCESS;
}

//求长度
unsigned char lengthQueue(t_queue* this)
{
   if(this->ulRear == REAR_INDEX_FULL) return this->depth;
   else if(this->depth == 0) return 0;
   else return ((this->ulRear + this->depth - this->ulFront) % (this->depth) );
}



#ifdef __cplusplus
}
#endif


