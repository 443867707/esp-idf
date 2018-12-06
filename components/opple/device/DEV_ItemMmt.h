#ifndef __ITEM_MMT_H__
#define __ITEM_MMT_H__


/*********************************************************************************
Module-Id: Lab a item
Module : 0 - ...
Id     : 0 - ...
*********************************************************************************/
#include "Typedef.h"


extern void ItemMmtInit(void);

/* return 0:success,1:fail*/
extern int DataItemRead(U32 module,U32 id,unsigned char* data,unsigned int* len);
extern int DataItemWrite(U32 module,U32 id,unsigned char* data,unsigned int len);
extern int DataItemReadU32(U32 module,U32 id,unsigned int* data);
extern int DataItemWriteU32(U32 module,U32 id,unsigned int data);
extern int DataItemReadU16(U32 module,U32 id,unsigned short* data);
extern int DataItemWriteU16(U32 module,U32 id,unsigned short data);
extern int DataItemReadU8(U32 module,U32 id,unsigned char* data);
extern int DataItemWriteU8(U32 module,U32 id,unsigned char data);











#endif


