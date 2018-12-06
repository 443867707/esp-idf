#ifndef __OPP_LOG_MMT_H__
#define __OPP_LOG_MMT_H__

/*******************************************************************************
                                Includes
********************************************************************************/
#include "Includes.h"


/*******************************************************************************
                                Config
********************************************************************************/
#define LOG_MMT_PARTITION_LOG   0
#define LOG_MMT_PARTITION_METER 1


/*******************************************************************************
                                Defines
********************************************************************************/
#define LOG_MMT_BITMAP_BYTES                 32    /* fixed 32 */
#define LOG_MMT_SECTOR_SIZE                  4096  /* hardware size */
#define LOG_MMT_PARTATIONS                   2


#define LOG_MMT_PARTATIONS_DEFAULT \
{\
	{.item_size = 32,.sectors_per_partation = 250,.addr_start = 0,.partation_size = 0,},\
	{.item_size = 16,.sectors_per_partation = 10, .addr_start = 0,.partation_size = 0,},\
}


/*******************************************************************************
                                Typedefs
********************************************************************************/
typedef struct
{
	U16 item_size; /* 16 <= size <= sector_size-bitmap_bytes */
	U16 sectors_per_partation;/* 2 <= num <= partation_size */
	U32 addr_start;
	U32 partation_size;
	// items_per_sector; /*(((LOG_MMT_SECTOR_SIZE)-(LOG_MMT_BITMAP_BYTES))/(LOG_MMT_ITEM_SIZE))*/
}log_mmt_t;





/*******************************************************************************
                                Interfaces
********************************************************************************/
void LogMmtInit(void);
void LogMmtLoop(void);
int  LogMmtWrite(U16 partation,U8* data,U32 len,U32* id);
int  LogMmtRead(U16 partation,U16 id,U8* data,U32* len);
S32  LogMmtGetLatestId(U16 partation);
int  LogMmtReadLatestItem(U16 partation,S32* id,U8* data,U32* len);
U32  LogMmtReadBitMap(U16 partation,U16 sector,U8* data);
U32  LogMmtSectorErase(U16 partition,U16 sector);
U32  LogMmtPartitionErase(U16 partation);
void LogMmtInfo(const const U8** init,const const U32** wi,const const U32** ws,const log_mmt_t** pt); /* DEBUG */
int LogMmtToalItems(U16 partation,U32* total);
int LogMmtReadLastItem(U16 partation,S32 id_now,S32* id_last,U8* data,U32* len);
S32  LogMmtGetLastId(U16 partation,S32 id_now);





#endif

























