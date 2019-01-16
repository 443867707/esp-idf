#ifndef __OS_H__
#define __OS_H__


/******************************************************************************************************************
                            ABOUT 
******************************************************************************************************************/
/*
    Used for :
         Thread
         Mutex/sem
         Malloc/free
         Systick
**/

/******************************************************************************************************************
                            INCLUDES 
******************************************************************************************************************/
#include "Config.h"

#if defined(PLATFORM_OS_LITEOS)
    #include "los_sys.h"
    #include "los_tick.h"
    #include "los_task.ph"
    #include "los_config.h"
    #include "los_bsp_uart.h"
    #include "los_sem.h"
	#include "los_base.ph"
	#include "los_hwi.h"
	#include "los_sem.h"

	//BSP header files
	#include "stm32f4xx_hal.h"    
#elif defined(PLATFORM_OS_FREERTOS)
    #include "freertos/FreeRTOS.h"
	#include "freertos/task.h"
	#include "freertos/semphr.h"
	#include "freertos/event_groups.h"
#elif defined(PLATFORM_OS_NONE)
    
#endif
/******************************************************************************************************************
                            MUTEX 
******************************************************************************************************************/
#if defined(PLATFORM_OS_LITEOS)
    #define T_MUTEX                     unsigned int               // xSemaphoreHandle
    #define MUTEX_CREATE(mutex)         LOS_MuxCreate(&mutex)      // mutex=xSemaphoreCreateMutex()
    #define MUTEX_LOCK(mutex,timeout)   LOS_MuxPend(mutex,timeout) // xSemaphoreTake((mutex),3)
    #define MUTEX_UNLOCK(mutex)         LOS_MuxPost(mutex)         // xSemaphoreGive((mutex))
    #define MUTEX_WAIT_ALWAYS           
#elif defined(PLATFORM_OS_FREERTOS)
    /*#define T_MUTEX                     xSemaphoreHandle    
    #define MUTEX_CREATE(mutex)         do{mutex=xSemaphoreCreateBinary(); xSemaphoreGive(mutex);}while(0)    //mutex=xSemaphoreCreateMutex()
    #define MUTEX_LOCK(mutex,timeout)   xSemaphoreTake((mutex),timeout)
    #define MUTEX_UNLOCK(mutex)         xSemaphoreGive((mutex))
    #define MUTEX_WAIT_ALWAYS           portMAX_DELAY*/
	/*#define T_MUTEX                    xSemaphoreHandle 
    #define MUTEX_CREATE(mutex)        
    #define MUTEX_LOCK(mutex,timeout)   
    #define MUTEX_UNLOCK(mutex)         
    #define MUTEX_WAIT_ALWAYS*/
	#define T_MUTEX					  	xSemaphoreHandle	  
	#define MUTEX_CREATE(mutex) 		do{mutex=xSemaphoreCreateBinary(); xSemaphoreGive(mutex);}while(0)	  //mutex=xSemaphoreCreateMutex()
	#define MUTEX_LOCK(mutex,timeout)	xSemaphoreTake((mutex),timeout)
	#define MUTEX_UNLOCK(mutex) 		xSemaphoreGive((mutex))
	#define MUTEX_WAIT_ALWAYS			portMAX_DELAY
#elif defined(PLATFORM_OS_NONE)
    #define T_MUTEX                     unsigned int         
    #define MUTEX_CREATE(mutex)     
    #define MUTEX_LOCK(mutex,timeout)   
    #define MUTEX_UNLOCK(mutex) 
    #define MUTEX_WAIT_ALWAYS           0
#endif


/******************************************************************************************************************
                            Semaphore 
******************************************************************************************************************/
/**
	Can not be used in ISR.
*/
#if defined(PLATFORM_OS_LITEOS)
    #define OS_SEMAPHORE_T                    unsigned int
    #define OS_SEMAPHORE_CREATE(sem,cntMax)  
	#define OS_SEMAPHORE_TAKE(sem,timeout)
	#define OS_SEMAPHORE_GIVE(sem)
	#define SEM_WAIT_ALWAYS                   0
#elif defined(PLATFORM_OS_FREERTOS)
    #define OS_SEMAPHORE_T		              xSemaphoreHandle
    #define OS_SEMAPHORE_CREATE(sem,cntMax)   sem=xSemaphoreCreateCounting(cntMax,0)
	#define OS_SEMAPHORE_TAKE(sem,timeout)    xSemaphoreTake((sem),timeout)
	#define OS_SEMAPHORE_GIVE(sem)            xSemaphoreGive((sem))
	#define SEM_WAIT_ALWAYS                   portMAX_DELAY
#elif defined(PLATFORM_OS_NONE)
    #define OS_SEMAPHORE_T		  unsigned int
	#define OS_SEMAPHORE_CREATE(sem,cntMax)	  
	#define OS_SEMAPHORE_TAKE(sem,timeout)
	#define OS_SEMAPHORE_GIVE(sem)
	#define SEM_WAIT_ALWAYS                   0
#endif

/******************************************************************************************************************
                            Eventgroup 
******************************************************************************************************************/
#if defined(PLATFORM_OS_LITEOS)
    #define OS_EVENTGROUP_T unsigned int
    #define OS_EVENTGROUP_WAITBIT(event,bit,timeout)
    #define OS_EVENTGROUP_SETBIT(event,bit)
    #define OS_EVENTGROUP_CLEARBIT(event,bit)
    #define OS_EVENTGROUP_WAIT_ALWAYS    
#elif defined(PLATFORM_OS_FREERTOS)
    #define OS_EVENTGROUP_T EventGroupHandle_t
    #define OS_EVENTGROUP_CREATE(event) event=xEventGroupCreate()
    #define OS_EVENTGROUP_WAITBIT(event,bit,timeout) xEventGroupWaitBits(event, bit,false, true, timeout);
    #define OS_EVENTGROUP_SETBIT(event,bit) xEventGroupSetBits(event,bit);
    #define OS_EVENTGROUP_CLEARBIT(event,bit) xEventGroupClearBits(event,bit);
    #define OS_EVENTGROUP_WAIT_ALWAYS    portMAX_DELAY
#elif defined(PLATFORM_OS_NONE)
	#define OS_EVENTGROUP_T unsigned int
    #define OS_EVENTGROUP_WAITBIT(event,bit,timeout)
    #define OS_EVENTGROUP_SETBIT(event,bit)
    #define OS_EVENTGROUP_CLEARBIT(event,bit)
    #define OS_EVENTGROUP_WAIT_ALWAYS 0
#endif

/******************************************************************************************************************
                            Thread 
******************************************************************************************************************/





/******************************************************************************************************************
                            Systick 
******************************************************************************************************************/
#if defined(PLATFORM_OS_LITEOS)
    #define TASK_DELAY(m)               
	#define osDelay(m)                  
	#define OppTickCountGet()           
#elif defined(PLATFORM_OS_FREERTOS)
	#define OS_TICK_T                   unsigned int
	#define OS_GET_TICK()               xTaskGetTickCount()
	#define OS_TICK_DIFF(last,now)      ((now)-(last))
	#define TASK_DELAY(m)               vTaskDelay(m)
	#define osDelay(m)                  vTaskDelay(m)
	#define OS_DELAY_MS(m)              vTaskDelay((m)/portTICK_RATE_MS);
	#define OppTickCountGet()           xTaskGetTickCount()
	#define OS_REBOOT()                 esp_restart()
#elif defined(PLATFORM_OS_NONE)
    #define TASK_DELAY(m)               
	#define osDelay(m)                  
	#define OppTickCountGet() 
#endif





/******************************************************************************************************************
                            Other 
******************************************************************************************************************/












#endif
