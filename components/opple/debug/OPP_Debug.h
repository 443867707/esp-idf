#ifndef __OPP_DEBUG_H__
#define __OPP_DEBUG_H__

/******************************************************************************
*                                Version                                 
******************************************************************************
V1.3

*/
/******************************************************************************
*                                How to use                                  
******************************************************************************
1.Define a module in "t_debug_module"
2.Name the module and enable it in "DEBUG_MODULE_LIST"
3.Use "DEBUG_LOG()" to print the log
4.Use the warning level in "t_debug_level" to label the log
5.Module can be disable/enable in "DEBUG_MODULE_LIST"
6.Use "DEBUG_LOG_FILTER_LEVEL_A" and "DEBUG_LOG_FILTER_LEVEL_A" to 
   filte the log
7.Use "DEBUG_LOG_ENABLE" to enable/disable OppDebug
8.Fill the "DEBUG_PRINTF" with accurate funtion in line 120

*/
/******************************************************************************
*                                Enable Config                                   
******************************************************************************/
#define DEBUG_LOG_ENABLE                     1        /* 0:Disable ,1:Enable */
#define DEBUG_LOG_RTC_ENABLE                 0        /* 0:Disable ,1:Enable */
#define LOCAL_PRINTF_ENABLE                  0        /* 0:Disable ,1:Enable */
#define DEBUG_LOG_MULTITASK_SUPPORT_ENABLE   0        /* 0:Disable ,1:Enable */

/******************************************************************************
*                                Includes                                  
******************************************************************************/
#include <string.h>

#if LOCAL_PRINTF_ENABLE==1
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#endif

#if DEBUG_LOG_MULTITASK_ENABLE==1
#include "los_sys.h"
#include "los_mux.h"
#endif

#if DEBUG_LOG_RTC_ENABLE==1
#include "SVS_Time.h"
#endif
	
/******************************************************************************
*                                Defines                                  
******************************************************************************/



/******************************************************************************
*                                Utilities Define                                  
******************************************************************************/
//#define filename(x) strrchr(x,'\\')?strrchr(x,'\\')+1:x
#define filename(x) strrchr(x,'/')?strrchr(x,'/')+1:x	
	
/******************************************************************************
*                                Multi-task support                                    
******************************************************************************/
/*#if DEBUG_LOG_MULTITASK_SUPPORT_ENABLE==1
#define T_MUTEX                 unsigned int              // xSemaphoreHandle
#define MUTEX_CREATE(mutex)     LOS_MuxCreate(&mutex)     // mutex=xSemaphoreCreateMutex()
#define MUTEX_LOCK(mutex)       LOS_MuxPend(mutex,100)    // xSemaphoreTake((mutex),3)
#define MUTEX_UNLOCK(mutex)     LOS_MuxPost(mutex)        // xSemaphoreGive((mutex))
#else
#define T_MUTEX                 unsigned int         
#define MUTEX_CREATE(mutex)     
#define MUTEX_LOCK(mutex)    
#define MUTEX_UNLOCK(mutex)   
#endif
*/
/******************************************************************************
*                                RTC Printf                                    
******************************************************************************/
#if DEBUG_LOG_RTC_ENABLE==1
  #define DEBUG_LOG_RTC_PRINTF \
  unsigned long long tick; \
	ST_TIME t; \
	Opple_Get_RTC(&t);\
	tick = OppTickCountGet();\
	DEBUG_PRINTF("[%04d-%02d-%02d %02d:%02d:%02d,%lld]",t.Year,t.Month,t.Day,t.Hour,t.Min,t.Sencond,tick)
#else
 #define DEBUG_LOG_RTC_PRINTF
#endif

/******************************************************************************
*                                Local Printf                                    
******************************************************************************/
#if LOCAL_PRINTF_ENABLE==1
 #define LOCAL_PRINTF_SEND_CHAR(ch) \
  while((USART1->SR & 0x40) == 0){};\
  USART1->DR = (unsigned char)ch
#else 
 #define LOCAL_PRINTF_SEND_CHAR(ch)
#endif

/******************************************************************************
*                                Module Config                                     
******************************************************************************/
typedef enum
{
	/********************************/
	DEBUG_MODULE_MAIN=0,
	DEBUG_MODULE_TIME,
	DEBUG_MODULE_BC28,
	DEBUG_MODULE_NB,
	DEBUG_MODULE_LAMPCTRL,
	DEBUG_MODULE_COAP,
	DEBUG_MODULE_OTA,
	DEBUG_MODULE_GPS,
	DEBUG_MODULE_LOC,
	DEBUG_MODULE_METER,	
	DEBUG_MODULE_PWM,
  	DEBUG_MODULE_APSLAMP,
  	DEBUG_MODULE_AUS,
  	DEBUG_MODULE_DAEMON,
	DEBUG_MODULE_ELEC,
	DEBUG_MODULE_PARA,
	DEBUG_MODULE_WIFI,
	DEBUG_MODULE_MSGMMT,
	DEBUG_MODULE_LIGHTSENSOR,
	DEBUG_MODULE_EXEP,
	/********************************/
	DEBUG_MODULE_MAX,
}t_debug_module;

#define DEBUG_MODULE_CONFIG_LIST {\
{DEBUG_MODULE_MAIN,"MAIN",ENABLE},\
{DEBUG_MODULE_TIME,"TIME",DISABLE},\
{DEBUG_MODULE_BC28,"BC28",DISABLE},\
{DEBUG_MODULE_NB,"NB",DISABLE},\
{DEBUG_MODULE_LAMPCTRL,"LAMPCTRL",DISABLE},\
{DEBUG_MODULE_COAP,"COAP",DISABLE},\
{DEBUG_MODULE_OTA,"OTA",ENABLE},\
{DEBUG_MODULE_GPS,"GPS",DISABLE},\
{DEBUG_MODULE_LOC,"LOC",DISABLE},\
{DEBUG_MODULE_METER,"METER",DISABLE},\
{DEBUG_MODULE_PWM,"PWM",DISABLE},\
{DEBUG_MODULE_APSLAMP,"APSLAMP",DISABLE},\
{DEBUG_MODULE_AUS,"AUS",ENABLE},\
{DEBUG_MODULE_DAEMON,"DAEMON",ENABLE},\
{DEBUG_MODULE_ELEC,"ELEC",DISABLE},\
{DEBUG_MODULE_PARA,"PARA",DISABLE},\
{DEBUG_MODULE_WIFI,"WIFI",DISABLE},\
{DEBUG_MODULE_MSGMMT,"MSGMMT",ENABLE},\
{DEBUG_MODULE_LIGHTSENSOR,"LIGHTSENSOR",DISABLE},\
{DEBUG_MODULE_EXEP,"EXEP",ENABLE},\
}

/******************************************************************************
*                                Level Config                                     
******************************************************************************/
typedef enum
{
	DLL_FETAL=1, 
	DLL_ERROR=2,
	DLL_WARN=3,  
	DLL_INFO=4,  
	DLL_DEBUG=5, 
}t_debug_level;

#define DEBUG_LEVEL_CONFIG_LIST {\
{DLL_FETAL,"Fetal",ENABLE},\
{DLL_ERROR,"Error",ENABLE},\
{DLL_WARN,"Warn",ENABLE},\
{DLL_INFO,"Info",ENABLE},\
{DLL_DEBUG,"Debug",ENABLE},\
}

/******************************************************************************
*								 Typedefs								  
******************************************************************************/
/*
typedef enum
{
	ENABLE=0,
	DISABLE,
}t_debug_enable;
*/
#define ENABLE 1
#define DISABLE 0
typedef unsigned char t_debug_enable;

typedef struct
{
	t_debug_module module;
	char* name;
	t_debug_enable enable;
}t_debug_info;

typedef struct
{
	t_debug_level level;
	char* name;
	t_debug_enable enable;
}t_debug_info_level;


/******************************************************************************
*                                extern fun                                   *
******************************************************************************/
extern void debug_log_init(void);
extern void debug_log(char* file,int line,t_debug_module module,t_debug_level level,char* log,...);


/******************************************************************************
*                                Interfaces                                   *
******************************************************************************/
extern int debug_module_set(t_debug_module module,t_debug_enable en);
extern int debug_level_set(t_debug_level level ,t_debug_enable en);
extern int debug_level_filt(t_debug_level level);


#if DEBUG_LOG_ENABLE==0
    #define DEBUG_LOG_INIT()
    #define DEBUG_LOG(module,level,log,args...) 
    #define DEBUG_PRINTF(info,args...) 
#else
    #define DEBUG_LOG(module,level,log,args...) \
        debug_log(filename(__FILE__),__LINE__,module,level,log,##args);
    #define DEBUG_LOG_INIT() debug_log_init();
    #define DEBUG_PRINTF(info,args...) printf(info,##args);
#endif









#endif


