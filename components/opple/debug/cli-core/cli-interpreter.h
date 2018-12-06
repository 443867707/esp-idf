#ifndef __CLI_INTERPRETER_H__
#define __CLI_INTERPRETER_H__

#include <stdio.h>

extern void CliPrintf(char* msg,...);
//#define CLI_PRINTF(info,args...)  printf(info,##args)
#define CLI_PRINTF(info,args...) CliPrintf(info,##args)

#define CLI_IPT_ARG_NUM 12           //  支持解析后的参数个数
#define CLI_IPT_ARG_LEN 50           //　每个参数解析后的最大长度
#define CLI_IPT_ARG_STR_LEN 100      //　每个参数解析前字符串形式的最大长度
#define CLI_IPT_ENTRY_NUM_MAX 100    //  菜单中子菜单最大个数（不占用内存）


typedef void (*CommandAction)();

#define CommandEntryActionWithDetails(name,            \
                                           action,          \
                                           argumentTypes,   \
                                           description,     \
                                           argumentDescriptionArray)  \
{ (name), (CommandAction)(action), (argumentTypes), (description), (argumentDescriptionArray) }

#define CommandEntrySubMenu(name, subMenu, description)     \
{ (name), (CommandAction)NULL, (const char*)(subMenu), (description), (const char* const*)NULL }

#define CommandEntryTerminator() \
{ (const char*)NULL, (CommandAction )NULL, (const char*)NULL, (const char*)NULL, (const char*const*)NULL }


typedef const struct {
  const char* name;
  CommandAction action; // action==NULL means argumentTypes is a subentry
  const char* argumentTypes;
  const char* description;
  const char* const * argumentDescriptions;
}CommandEntry;


extern unsigned int CliIptArgList[CLI_IPT_ARG_NUM][CLI_IPT_ARG_LEN];


extern void ArrayMapInt2Byte(unsigned char* b,unsigned int* i,unsigned int len);
extern int CommandEntryMatch(CommandEntry* MainTable,const char* cmd); // return 0:fail,1:success
extern int CommanEntryHint(CommandEntry* MainTable,const char* cmd,char* out);
extern void PrintEntry(CommandEntry* entry);


#endif


