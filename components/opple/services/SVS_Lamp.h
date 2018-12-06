#ifndef __SVS_LAMP_H__
#define __SVS_LAMP_H__

#include "Includes.h"

typedef enum
{
	LAMP_OUT_DAC=0, /* 0-10V */
	LAMP_OUT_PWM,   /* PWM */
}EN_LAMP_OUT;

extern void ApsLampInit(void);
extern void ApsLampLoop(void);

extern int LampBlink(U32 state,U32 lowLevel,U32 highLevel,U32 lowTime,U32 highTime,U32 lastTime);
extern U32 LampOnOff(unsigned char src,int flag);
extern U32 LampDim(unsigned char src,int level);
extern U32 LampMoveToLevel(unsigned char src,int level,int ms);
extern int LampStateRead(int* state,int* level);

extern int LampOuttypeSet(EN_LAMP_OUT type);/*return 0:success,else:fail*/
extern unsigned char LampOuttypeGet(void);


#endif


