/******************************************************************************
*                                Includes                                  
******************************************************************************/
#include "Includes.h"
#include "OPP_Debug.h"
#include <stdio.h>
#include <stdarg.h>
/******************************************************************************
*                                Config                                  
******************************************************************************/



/******************************************************************************
*                                Data Structs                                  
******************************************************************************/
volatile T_MUTEX mutex_debug;


/******************************************************************************
*                                Implementations                                  
******************************************************************************/
#if LOCAL_PRINTF_ENABLE==1
#pragma import(__use_no_semihosting) 
#endif



t_debug_info module_list[DEBUG_MODULE_MAX]= \
DEBUG_MODULE_CONFIG_LIST;

t_debug_info_level level_list[5]= \
DEBUG_LEVEL_CONFIG_LIST;

extern void telnetPrintf(char* msg,...);
extern int telnet_esp32_vprintf(const char *fmt, va_list va);

#if LOCAL_PRINTF_ENABLE==1
/**************************************************************************
                             Local Printf
**************************************************************************/
struct __FILE{
	int handle;
};

FILE __stdout;
FILE __stdin;


int SendChar(int ch)
{
	/********************
	         SPI
	********************/
	
	
	
	/********************
	         UART
	********************/
	LOCAL_PRINTF_SEND_CHAR(ch);
	
	return ch;
}

int GetKey(void)
{
	return 0;
}

int fputc(int ch ,FILE* f)
{
	return (SendChar(ch));
}


int fgetc(FILE *f)
{
	return (SendChar(GetKey()));
}

void _ttywrch(int ch)
{
	SendChar(ch);
}

int ferror(FILE *f)
{
	return EOF;
}

void _sys_exit(int return_code)
{
	label:goto label;
}
/**************************************************************************
                             Local Printf
**************************************************************************/
#endif

void debug_log_init(void)
{
	MUTEX_CREATE(mutex_debug);
}

int findModuleListIndex(t_debug_module module)
{
	int i;
	
	for(i = 0;i < DEBUG_MODULE_MAX;i++)
	{
		if(module_list[i].module == module)
		{
			return i;
		}
	}
	return -1;
}

void debug_log_rtc_printf(void)
{
	DEBUG_LOG_RTC_PRINTF;
}

void debug_log(char* file,int line,t_debug_module module,t_debug_level level,char* log,...)
{
	int index;
	va_list vl;
  //char c[100]={0};
  unsigned short len;
  extern unsigned char g_CliChl;
	
	if(module >= DEBUG_MODULE_MAX) return;
	
	index = findModuleListIndex(module);
	if(index == -1) return;

	if((level < DLL_FETAL) || (level > DLL_DEBUG)) return;

	
	if( ((module_list[index].enable == DISABLE)&&(level >= DLL_WARN))
	   ||((level_list[level-1].enable == DISABLE)&&(level >= DLL_WARN)))
	{
		return;
	}
	/*
	if(module_list[index].enable == DISABLE) return;
    if(level_list[level-1].enable == DISABLE) return;
    */
	
	MUTEX_LOCK(mutex_debug,MUTEX_WAIT_ALWAYS);
	
    // [2018-4-13 15:40:17]
	debug_log_rtc_printf();
	
	// [OppDebug.c,120][ModuleA,Warning] This is the debug info.
  if(g_CliChl == 0)
  {
	    DEBUG_PRINTF("[%s,%d][%s,%s]:",file,line,module_list[index].name,level_list[level-1].name);
  }
  else
  {
      //snprintf(c,99,"[%s,%d][%s,%s]:",file,line,module_list[index].name,level_list[level-1].name);
      //telnetPrintf(c);
      telnetPrintf("[%s,%d][%s,%s]:",file,line,module_list[index].name,level_list[level-1].name);
  }

    
	  if(g_CliChl == 0)
    {
        va_start(vl,log);
        vprintf(log,vl);
        va_end(vl);
    }
    else
    {
        va_start(vl,log);
        //len = vsnprintf(c+strlen(c),319,log,vl);
        //telnetPrintf(c);
        telnet_esp32_vprintf(log,vl);
        va_end(vl);
    }
    
    // Enter
    //if(g_CliChl == 0){
    //  DEBUG_PRINTF("\r\n");
    //}
    
	MUTEX_UNLOCK(mutex_debug);
}

int debug_module_set(t_debug_module module,t_debug_enable en)
{
	if(module >= DEBUG_MODULE_MAX) return 1;
	
	module_list[module].enable = en;
	return 0;
}

int  debug_level_set(t_debug_level level ,t_debug_enable en)
{
	if(level == 0 || level > 5) return 1;

	level_list[level-1].enable = en;
	return 0;
}

int  debug_level_filt(t_debug_level level)
{
	if(level == 0 || level > 5) return 1;

	for(int i = 1;i <= 5;i++ )
	{
		if(i <= level)
		{
			level_list[i-1].enable = 1;
		}
		else
		{
			level_list[i-1].enable = 0;
		}
	}
	return 0;
}

