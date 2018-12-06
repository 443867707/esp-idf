#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "cli-interpreter.h"

//#define NULL (void*)0


static char ArgStrList[CLI_IPT_ARG_NUM][CLI_IPT_ARG_STR_LEN];
unsigned int CliIptArgList[CLI_IPT_ARG_NUM][CLI_IPT_ARG_LEN];


void printMainMenue(CommandEntry* MainTable);

void ArrayMapInt2Byte(unsigned char* b,unsigned int* i,unsigned int len)
{
    if(len > CLI_IPT_ARG_LEN) return;
    for(unsigned char index =0;index< len;index++)
    {
        b[index] = i[index];
    }
}

int LengthOfMenue(CommandEntry* entry)
{
	int i,len=0;
	for (i = 0; i < CLI_IPT_ENTRY_NUM_MAX; i++)
	{
		if (entry->action == NULL &&
			entry->argumentDescriptions == NULL&&
			entry->argumentTypes == NULL &&
			entry->description == NULL &&
			entry->name == NULL)
		{
			return len;
		}
		else
		{
			len++;
			entry++;
		}
	}
	return 0;
}

int Cmd2Paras(const char* cmd, char* paras,int para_len_max)
{
	int state = 0,row=0;
	char* base;
	
	base = paras;
	while (*cmd != '\0')
	{
		if (*cmd != ' ')
		{
			if (state == 2 || state == 0)
			{
				row++;
				if(row > CLI_IPT_ARG_NUM) return row;
				paras = base + para_len_max*(row-1);
				memset(paras,0,para_len_max);
				state = 1;
			}
			state = 1;
			*paras = *cmd;
			paras++;
			if (paras > base + para_len_max*(row) -1)
			{
				return 0;
			}
		}
		else
		{
			if(state == 1)
			{
				state = 2;
			}
		}
		cmd++;
		
	}
	return row;
}

void PrintEntry(CommandEntry* entry)
{
	int len, i;

	len = LengthOfMenue(entry);
	for (i = 0; i < len; i++)
	{
		if (entry[i].action != NULL)
		{
			CLI_PRINTF("%-30s- (%s)%s\r\n", entry[i].name, entry[i].argumentTypes, entry[i].description);
		}
		else
		{
			CLI_PRINTF("%-30s- (submenue)%s\r\n", entry[i].name, entry[i].description);
		}
	}

}

// entry:CommandEntryActionWithDetails("h2", CommandHello2, "uuu", "Hello 2 with para...", CommandTableHelloCommandArguments),
// h2
// arg1 - u<one-byte> para1
// arg2 - v<two-byte> para2
// arg3 - w<four-byte> para3
void PirntArgDesp(CommandEntry* entry)
{
	int len,i;
	char* d[] = {"<one-byte 1 or 0x01>","<two-byte 1 or 0x01>","<four-byte 1 or 0x01>",\
		"<string \"abc\" >" ,"<array, {1,2,3}>" };
	char* desp;

	len = strlen(entry->argumentTypes);

	// if (len == 0 || entry->argumentDescriptions == NULL) return;
	if(len == 0) return;

	// printf("%s:\r\n",entry->name);
	for (i = 0; i < len; i++)
	{
		if (entry->argumentTypes[i] == 'u') desp = d[0];
		else if(entry->argumentTypes[i] == 'v') desp = d[1];
		else if (entry->argumentTypes[i] == 'w') desp = d[2];
		else if (entry->argumentTypes[i] == 's') desp = d[3];
		else if (entry->argumentTypes[i] == 'a') desp = d[4];
		else desp = "";
		
		if (entry->argumentDescriptions == NULL){
		CLI_PRINTF("arg%d   -   %c%s\r\n", i, entry->argumentTypes[i], desp);
		}else{
			CLI_PRINTF("arg%d   -   %c%s %s\r\n", i, entry->argumentTypes[i], desp, entry->argumentDescriptions[i]);
		}
		
	}

}

static unsigned char hexToInt(unsigned char ch)
{
	return ch - (ch >= 'a' ? 'a' - 10
		: (ch >= 'A' ? 'A' - 10
			: (ch <= '9' ? '0'
				: 0)));
}

static unsigned int stringToUnsignedInt(char* str, unsigned int *array, int para_len_max)
{
	int result = 0;
	int base = 10;
	int i;
	int length;
	int pflag = 0;

	length = strlen(str);
	if (length == 0) return 1;
	if (str[0] == '-') pflag = 1;
	for (i = 0; i < length; i++)
	{
		unsigned char next = str[i];
		if (i == 0 && next == '-') 
		{
			// do nothing
		}
		else if ((next == 'x' || next == 'X')
			  && result == 0
			  && (i == 1 || i == 2))
		{
			base = 16;
		}
		else 
		{
			unsigned char value = hexToInt(next);
			if (value < base) 
			{
				result = result * base + value;
				if (pflag == 1) {
					result = ~result + 1;
				}
				*array = result;
			}
			else 
			{
				return 1;
			}
		}
	}
	return 0;
}

// "s" "abc"
// "str\r\n"
int string2s(char* str, unsigned int *array, int para_len_max)
{
	int i = 0,j=0,bs=0;
	char* ps = (char*)array;
	
	if (str[0] != '"') return 1;
  
	j=0;
	for (i = 0; str[i+1] != '"';)
	{
		if(str[i+1] == '\\')
		{
		    if(str[i+1+1] == 'n' )
		    {
		        ps[j] = '\n';
		        i++;
		    }
		    else if(str[i+1+1] == 'r')
		    {
		    	ps[j] = '\r';
		    	i++;
		    }
		    else
		    {
		        ps[j] = '\\';
		    }
		}
		else
		{
		    ps[j] = str[i+1];
		}
		
		j++;
		if (j + 1 > para_len_max)
		{
			return 1;
		}

		i++;
	}
	ps[j] = 0;

	return 0;
}

// "a" {1,2,3}
int string2a(char* str, unsigned int *array, int para_len_max)
{
	int i,index = 0,count=0,flag=0;
	char s[CLI_IPT_ARG_LEN] = {0};

	if (str[0] != '{') return 1;

	for (i = 0;str[i]!='\0';i++)
	{
		if (str[i] == '{')
		{
			index = 0;
		}
		else if (str[i] == ',')
		{
			s[index] = 0;
			if (stringToUnsignedInt(s, array + count, CLI_IPT_ARG_LEN) != 0) return 1;
			index = 0;
			count++;
		}
		else if (str[i] == '}')
		{
			flag = 1;
			s[index]=0;
			if (stringToUnsignedInt(s, array + count, CLI_IPT_ARG_LEN) != 0) return 1;
		}
		else
		{
			s[index++] = str[i];
		}
		
	}
	if(flag == 0) return 1;

	return 0;
}

// entry:CommandEntryActionWithDetails("h2", CommandHello2, "u", "Hello 2 with para...", CommandTableHelloCommandArguments),
// paras:list[n][100]={"para1","para2"}
// u - one byte (8 or -4 or 0x8)
// v - two bytes
// w - four bytes
// s - string or arrays ("abc" or {97 98 99 00})
int CommandActionExec(CommandEntry *entry, char* paras,int para_len_max)
{
	int len,i,res=0;

	len = strlen(entry->argumentTypes);

	//CLI_PRINTF("arg is:");
	for (i = 0; i < len; i++)
	{
		if (entry->argumentTypes[i] == 'u')
		{
			res = stringToUnsignedInt(paras+ para_len_max*i, CliIptArgList[i],CLI_IPT_ARG_LEN);
		}
		else if (entry->argumentTypes[i] == 'v')
		{
			res = stringToUnsignedInt(paras + para_len_max*i, CliIptArgList[i], CLI_IPT_ARG_LEN);
		}
		else if (entry->argumentTypes[i] == 'w')
		{
			res = stringToUnsignedInt(paras + para_len_max*i, CliIptArgList[i], CLI_IPT_ARG_LEN);
		}
		else if (entry->argumentTypes[i] == 's')
		{
			res = string2s(paras + para_len_max*i, CliIptArgList[i], CLI_IPT_ARG_LEN*sizeof(unsigned int));
		}
		else if (entry->argumentTypes[i] == 'a')
		{
			res = string2a(paras + para_len_max*i, CliIptArgList[i], CLI_IPT_ARG_LEN);
		}
		else
		{

		}
		if (res == 1)
		{
			return res;
		}
		//CLI_PRINTF("%d ", CliIptArgList[i][0]);
	}
	//CLI_PRINTF("\r\n");

	return res;
}

int stricmp(const char* a,const char* b)
{
	int i,len;
	
	if(strlen(a) != strlen(b)) return 1;
	
	len = strlen(a);
	for(i = 0;i < len;i++)
	{
		if(toupper((unsigned int )a[i]) != toupper((unsigned int)b[i]))
		{
			return 1;
		}
	}
	
	return 0;
}

int CommandMatch(const char* cmd,int para_len_max,int num, CommandEntry** entry_in,CommandEntry** entry_out)
{
	int len,i,res = 0;

	len = LengthOfMenue(*entry_in);

	for (i = 0; i < len; i++)
	{
		//printf("%s\r\n",ArgStrList[i]);
		if (stricmp(cmd, (*entry_in)->name) == 0)
		{
			if ((*entry_in)->action != NULL)
			{
				*entry_out =*entry_in;
				res = 1;
			}
			else
			{
				*entry_out = (CommandEntry*)((*entry_in)->argumentTypes);
				res = 2;
			}
			break;
		}
		(*entry_in)++;
	}
	if (i == len)
	{
		res = 0;
	}
	return res;
}

int CommandEntryMatch(CommandEntry* MainTable,const char* cmd)
{
	static CommandEntry *ein,*eout,*ehint;
	int num,res=2;
	char* plist;
    
	ein = MainTable;
	eout = (CommandEntry*)NULL;
	ehint = ein;

	memset(ArgStrList,0,sizeof(ArgStrList));
	num = Cmd2Paras(cmd, (char*)ArgStrList, CLI_IPT_ARG_STR_LEN);
	if (num == 0)
	{
		//printMainMenue(MainTable);
		CLI_PRINTF("# ");
		return 0;
	}

	plist = (char*)ArgStrList;
	while (res==2)
	{
		res = CommandMatch((const char*)plist,CLI_IPT_ARG_STR_LEN,num,&ein,&eout);
		if (res == 0)
		{
			//printf("Commands Or Paras Unmatched!\r\n");
			PrintEntry(ehint);
			CLI_PRINTF("# ");
			return 0;
		}
		else if(res == 1)
		{
			if (CommandActionExec(eout, plist + CLI_IPT_ARG_STR_LEN, CLI_IPT_ARG_STR_LEN) == 0) {
				eout->action();
			}
			else {
				//printf("Arguments error!\r\n");
				PirntArgDesp(eout);
			}
			
			
		}
		else
		{
			plist = plist + CLI_IPT_ARG_STR_LEN;
			ein = eout;
			ehint = ein;
		}
	}
  
	CLI_PRINTF("# ");
    
	return 1;

}

void printMainMenue(CommandEntry* MainTable)
{
	PrintEntry(MainTable);
}

int casechg(char a)
{
	if(a >= 'A' && a <= 'Z') a = a-'A'+'a';
	return a;
}

int strInclu(const char* s,const char*str)
{
	int i;
	
	if(strlen(s) > strlen(str)) return -1;
	
	for(i = 0;i < strlen(s);i++)
	{
		if(casechg(s[i]) != casechg(str[i])) return -1;
	}
	
	return 0;
}


// "led o" -> "on off" return 1
// "led of" -> "led off" return 2
// "led off" -> "led off "return 3
// "xxx xx" -> "" return 0
int CommanEntryHint(CommandEntry* MainTable,const char* cmd,char* out)
{
	static CommandEntry *ein,*eout; //,*ehint;
	int num,res=2,i,len,cnt;
	char* plist,tmp[CLI_IPT_ARG_STR_LEN];
    
	ein = MainTable;
	//eout = (CommandEntry*)NULL;
	eout = ein;
	//ehint = ein;

	memset(ArgStrList,0,sizeof(ArgStrList));
	num = Cmd2Paras(cmd, (char*)ArgStrList, CLI_IPT_ARG_STR_LEN);
	if (num == 0)
	{
		//printMainMenue(MainTable);
		//printf("# ");
		return 0;
	}

	plist = (char*)ArgStrList;
	while (res==2)
	{
		res = CommandMatch((const char*)plist,CLI_IPT_ARG_STR_LEN,num,&ein,&eout);
		if (res == 0)
		{
			//printf("Commands Or Paras Unmatched!\r\n");
			//PrintEntry(ehint);
			//printf("# ");
			if(strlen(plist) == 0)
			{
				if(cmd[strlen(cmd)-1] != ' ') // "s1 s2"
				{
					strcpy(out,cmd);
					strcat(out," ");
					return 3;
				}
			}
			
			len = LengthOfMenue(eout);
			cnt = 0;
			strcpy(out,"");
			for(i = 0;i < len;i++)
			{
				if(strInclu(plist,eout[i].name)==0)
				{
					cnt++;
					strcat(out,eout[i].name);
					strcat(out," ");
				}
			}
			if(cnt == 1)
			{
				strcpy(tmp,out);
				strcpy(out,"");
				for(i = 0;i < (num-1);i++)
				{
					strcat(out,ArgStrList[i]);
					strcat(out," ");
				}
				strcat(out,tmp);
				return 2;
			}
			else
			{
				return 1;
			}
		}
		else if(res == 1)  // Action
		{
			//if (CommandActionExec(eout, plist + CLI_IPT_ARG_STR_LEN, CLI_IPT_ARG_STR_LEN) == 0) {
			//	eout->action();
			//}
			//else {
				//printf("Arguments error!\r\n");
			//	PirntArgDesp(eout);
			//}
			strcpy(out,"");
			for(i = 0;i < (num);i++)
			{
				strcat(out,ArgStrList[i]);
				strcat(out," ");
				if(strlen(out) >= ((unsigned int)CLI_IPT_ARG_STR_LEN>>1))
				{
					strcat(out,"...");
					break;
				}
			}
			return 3;
			
		}
		else  // Submenue
		{
			plist = plist + CLI_IPT_ARG_STR_LEN;
			ein = eout;
			//ehint = ein;
		}
	}
  
	// printf("# ");
    
	return 1;
}
