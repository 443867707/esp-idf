#include "cli-interpreter.h"

void ledon(unsigned char led);
void ledoff(unsigned char led);


void CommandLedOn(void);
void CommandLedOff(void);

const char* const CommandTableLedCommandArguments[] = {
  "The id of led,1:Led1,2:Led2",
};


CommandEntry CommandTableLed[] =
{
	CommandEntryActionWithDetails("on", CommandLedOn, "u", "Turn on led (1/2)...", CommandTableLedCommandArguments),
	CommandEntryActionWithDetails("off", CommandLedOff, "u", "Turn off led(1/2)...", CommandTableLedCommandArguments),
	CommandEntryTerminator()
};

void CommandLedOn(void)
{
	ledon(CliIptArgList[0][0]);
}

void CommandLedOff(void)
{
	ledoff(CliIptArgList[0][0]);
}

void ledon(unsigned char led){}
void ledoff(unsigned char led){}
