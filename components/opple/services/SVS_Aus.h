#ifndef __SVS_AUS_H__
#define __SVS_AUS_H__

extern int OppOtaPrepare(void);
extern int OppOtaStartHandle(void);
extern int OppOtaReceivingHandle(unsigned short seq,const unsigned char* data,unsigned short len);
extern int OppOtaReceiveEndHandle(void);


#endif
