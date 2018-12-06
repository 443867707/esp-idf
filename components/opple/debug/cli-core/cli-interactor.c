#include "cli-interactor.h"
#include <string.h>
#include <stdio.h>
#include "cli.h" 
#include <stdarg.h>


#define CLI_ITT_ANSI_ENABLE 0


static char CliIttList[CLI_ITT_LIST_NUM][CLI_ITT_LIST_LEN]={0};
static char CliIttListHintIndex=0,CliIttListHisCnt=0;
static unsigned char CliIttListIndex=0;
unsigned char g_CliChl=CLI_CHL_COM;

//extern void telnetPrintf(char* msg);
extern int telnet_esp32_vprintf(const char *fmt, va_list va);

void eoch(char c)
{
    if(g_CliChl == CLI_CHL_COM)
    CLI_PRINTF("%c",c);
}

void CliIttListAdd(unsigned char* item)
{
    strcpy(CliIttList[CliIttListIndex],(const char*)item);
    CliIttListIndex = (CliIttListIndex+1)%CLI_ITT_LIST_NUM;
	  CliIttListHisCnt++;
	  if(CliIttListHisCnt >= CLI_ITT_LIST_NUM) CliIttListHisCnt = CLI_ITT_LIST_NUM;
}

unsigned char DiffNum(unsigned char a,unsigned char b)
{
	return a>b?a-b:b-a;
}

unsigned char CliIttListIndexLast(void)
{
	unsigned char index;
	
	index  = (CliIttListHintIndex+CLI_ITT_LIST_NUM-1)%CLI_ITT_LIST_NUM;
	if((index != CliIttListIndex)
	 &&(DiffNum(CliIttListHintIndex,CliIttListIndex) < CliIttListHisCnt)){
		CliIttListHintIndex = index;
	}
	return CliIttListHintIndex;
}

unsigned char CliIttListIndexNext(void)
{
	unsigned char index;
	
	index = (CliIttListHintIndex+1)%CLI_ITT_LIST_NUM;
	
	if((index != CliIttListIndex)
		&&(DiffNum(CliIttListHintIndex,CliIttListIndex) <= CliIttListHisCnt))
	{
	  CliIttListHintIndex = index;
	}else{
		//CliIttListHintIndex = CliIttListIndex;
	}
	
	return CliIttListHintIndex;
}

void clear_line(void)
{
		CLI_PRINTF("\r");
		for(char i=0 ;i < CLI_ITT_LIST_LEN;i++)
		{
			CLI_PRINTF(" ");
		}
		CLI_PRINTF("\r");
}

void clear_line_ANSI(unsigned int cursor)
{
	CLI_PRINTF("%c[%dD",'\033',cursor);
	CLI_PRINTF("%c[K",'\033');
}

void CliPrintf(char* msg,...)
{
    va_list vl;

    if(g_CliChl == CLI_CHL_COM)
    {
        va_start(vl,msg);
        vprintf(msg,vl);
        va_end(vl);
    }
    else
    {
        va_start(vl,msg);
        telnet_esp32_vprintf(msg,vl);
        va_end(vl);
    }
}

void CliIttInput(CLI_CHL chl,char c)
{
	//static unsigned char enter=0;
	static unsigned char dir=0,ch;
	static char item[CLI_ITT_LIST_LEN];
	static unsigned char item_index=0;
	int res;
	char hint[CLI_ITT_LIST_LEN];
	
  g_CliChl = chl;
  
	ch = (unsigned char) c;
	//CLI_PRINTF("%02x",ch);
	//return;
	switch(ch)
	{
		case '\r':
			//enter = 1;
			break;
		case '\n':
			item[item_index]='\0';
		  if(g_CliChl == CLI_CHL_COM)CLI_PRINTF("\r\n");
		  if(strlen(item) != 0)
			{
		      CliIttListAdd((unsigned char*)item);
		      CliIttListHintIndex = CliIttListIndex;
			}
			CommandEntryMatch(MainTable,item);
			memset(item,0,CLI_ITT_LIST_LEN);
			//enter = 0;
			item_index = 0;
			break;
		case '\t': // Tab
			// "led o" -> "on off" return 1
			// "led of" -> "led off" return 2
			// "led off" -> "led off "return 3
			// "xxx xx" -> "" return 0
			res = CommanEntryHint(MainTable,item,hint);
		  if(res == 0){
				
			}else if(res == 1){
				CLI_PRINTF("\r\n%s\r\n# %s",hint,item);
			}else if(res == 2){
				#if CLI_ITT_ANSI_ENABLE==0
				clear_line();
				#else
				clear_line_ANSI(item_index);
				#endif
				CLI_PRINTF("\r# %s",hint);
				strcpy(item,hint);
				item_index = strlen(item);
			}else if(res == 3){
				#if CLI_ITT_ANSI_ENABLE==0
				clear_line();
				#else
				clear_line_ANSI(item_index);
				#endif
				CLI_PRINTF("\r# %s",hint);
				strcpy(item,hint);
				item_index = strlen(item);
			}else{
			}
			break;
		case 0x1b: // ESC/UP-1
			dir = 1;
		  break;
		case 0x5b:
			dir = 2;
		  break;
		//case 0x1b: // ESC
		//	memcpy(item,0,sizeof(item));
		//	item_index = 0;
		//	CLI_PRINTF("\r%s",item);
		//	break;
		case 8:   // BackSpace
			if(item_index > 0){
			  item_index--;
				
				#if CLI_ITT_ANSI_ENABLE==1
				CLI_PRINTF("%c[1D",'\033');
			  CLI_PRINTF("%c[K",'\033');
				#endif
			}
			item[item_index]='\0';
			#if CLI_ITT_ANSI_ENABLE==0
			clear_line();
		  CLI_PRINTF("\r# %s",item);
			#endif
			
			break;
		default:
		if((ch==0x41 || ch==0x42)&&(dir == 2))
			{
		    if(ch == 0x41) { // UP
					strcpy(item,CliIttList[CliIttListIndexLast()]);
				}
				else
				{ // DOWN
					if(CliIttListHintIndex == CliIttListIndex)
					{
					}
					else if(((CliIttListHintIndex+CLI_ITT_LIST_NUM+1)%CLI_ITT_LIST_NUM) == CliIttListIndex)
					{
					  CliIttListHintIndex = CliIttListIndex;
					  memset(item,0,CLI_ITT_LIST_LEN);  
					}
					else{
						strcpy(item,CliIttList[CliIttListIndexNext()]);
					}
				}
				
				#if CLI_ITT_ANSI_ENABLE==1
				clear_line_ANSI(item_index);
				#endif
				item_index = strlen(item);
				#if CLI_ITT_ANSI_ENABLE==0
		    clear_line();
				#endif
			  CLI_PRINTF("\r# %s",item);
				dir = 0;
				break;
		  }
			
			if((ch==0x43 || ch==0x44)&&(dir == 2))
			{
				break;
			}
		  
			eoch(c);
			if(item_index <= (CLI_ITT_LIST_LEN-2))
			{
				item[item_index++] = c;
			}
			break;
	}
	
	
	
}


