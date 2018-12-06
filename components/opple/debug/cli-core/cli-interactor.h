#ifndef __CLI_INTERACTOR_H__
#define __CLI_INTERACTOR_H__

#include "cli-interpreter.h"


#define CLI_ITT_LIST_NUM   10    // 历史记录命令个数
#define CLI_ITT_LIST_LEN   CLI_IPT_ARG_STR_LEN    // 每个命令的最大长度

typedef enum{
  CLI_CHL_COM=0,
  CLI_CHL_TELNET,
}CLI_CHL;

extern void CliIttInput(CLI_CHL chl,char c);


#endif


