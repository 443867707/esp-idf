#include "Includes.h"
#include "cli.h"
#include <string.h>
#include<stdio.h>
#include "opple_main.h"
#include "SVS_Exep.h"
#include "SVS_Coap.h"

/***************************************************************************************************************
                                          Config
***************************************************************************************************************/
#define CLI_ANSI_ENABLE 1


/***************************************************************************************************************
                                          Local data
***************************************************************************************************************/




/***************************************************************************************************************
                                          Interfaces
***************************************************************************************************************/
void CommandHelp(void);
void CommandVersion(void);
void CommandClear(void);
void CommandReset(void);
void CommandPs(void);


/***************************************************************************************************************
                                          Arg descriptions
***************************************************************************************************************/




/***************************************************************************************************************
                                          Entries
***************************************************************************************************************/
CommandEntry MainTable[] =
{
	/*****************************************************************************************************************/
    // Add Submenue Entry Here
    CLI_COMPONENTS_LIST
    
    /*****************************************************************************************************************/
	CommandEntryActionWithDetails("Help", CommandHelp, "", "Help infomation", (const char* const *)NULL),
	CommandEntryActionWithDetails("Version", CommandVersion, "", "Show version", (const char* const *)NULL),
    CommandEntryActionWithDetails("Reset", CommandReset, "", "Reset system", (const char* const *)NULL),
    CommandEntryActionWithDetails("clear", CommandClear, "", "Clear screen", (const char* const *)NULL),
    //CommandEntryActionWithDetails("ps", CommandPs, "s", "Print string", (const char* const *)NULL),

	CommandEntryTerminator()
};

/***************************************************************************************************************
                                          Implementations
***************************************************************************************************************/
void CommandVersion(void)
{
	CLI_PRINTF("Version 3.0.10,CC by [" __DATE__ " " __TIME__ "] \r\n"); 
} 

void CommandReset(void)
{
	sysModeDecreaseSet();
	ApsCoapWriteExepInfo(CMD_RST);
	ApsCoapDoReboot();
    CLI_RESET();
}

void CommandHelp(void)
{
	PrintEntry(MainTable);
}

void CommandClear(void)
{
	#if CLI_ANSI_ENABLE==1
	CLI_PRINTF("%c[2J",'\033');
	CLI_PRINTF("%c[41A",'\033');
	#else
	for(U8 i= 0;i < 40;i++)
	{
		CLI_PRINTF("# \r\n");
	}
	#endif
}

void cliInit(void)
{
    CLI_COMPONENTS_INIT
}

void cliLoop(void)
{
    CLI_COMONENTS_LOOP
}

void CommandPs(void)
{
	CLI_PRINTF("%s\r\n",&CliIptArgList[0][0]);
}
