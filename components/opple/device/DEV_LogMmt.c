/**
******************************************************************************
* @file    DEV_LogMmt.c
* @author  Liqi
* @version V1.0.0
* @date    2018-6-22
* @brief   
******************************************************************************

******************************************************************************
*/ 

/*****************************************************************
                              How it works
******************************************************************/
/*


*/
/*****************************************************************
                              Includes
******************************************************************/
#include "DEV_LogMmt.h"
#include "esp_partition.h"


/*****************************************************************
                              Defines
******************************************************************/
#define BITMAP_STATE_FISRT_BYTE_EMPTY   1
#define BITMAP_STATE_LAST_BYTE_FULL     2
#define BITMAP_STAET_FIND_WORKING_ITEM  3
#define BITMAP_STATE_SECTOR_FULL        4

/*****************************************************************
                              Typedefs
******************************************************************/
typedef enum
{
	SECTOR_EMPTY=0,
	SECTOR_FULL,
	SECTOR_WORKING,
}sector_state;


/*****************************************************************
                              Data
******************************************************************/
static log_mmt_t log_mmt_partations[LOG_MMT_PARTATIONS]= \
LOG_MMT_PARTATIONS_DEFAULT;

static U32 log_mmt_working_sector[LOG_MMT_PARTATIONS];
static U32 log_mmt_working_item[LOG_MMT_PARTATIONS];
static U8  log_mmt_init[LOG_MMT_PARTATIONS];
static U8  log_mmt_working_state[LOG_MMT_PARTATIONS]; /* 0:Never used ,1:data been written into */



/*****************************************************************
                              Utilities
******************************************************************/
/**
	@brief 获取日志模块相关信息，用于调试
	@attention none
	@param init 初始化状态
	@param wi working item
	@param ws working sector
	@param pt partitions
	@retval 0:success
*/
void LogMmtInfo(const const U8** init,const const U32** wi,const const U32** ws,const log_mmt_t** pt)
{
	*init = log_mmt_init;
	*wi = log_mmt_working_item;
	*ws = log_mmt_working_sector;
	*pt = log_mmt_partations;
}

/**
	@brief 扫描扇区的bitmap，找到当前工作的扇区
	@attention none
	@param address 扇区地址
	@param item_size item的大小
	@param workItem 计算出当前工作的item编号
	@retval
	     -1 items数错误
	     -2 spi_flsah_read 故障
	     其他参照：
	     #define BITMAP_STATE_FISRT_BYTE_EMPTY   1
		 #define BITMAP_STATE_LAST_BYTE_FULL     2
		 #define BITMAP_STAET_FIND_WORKING_ITEM  3
		 #define BITMAP_STATE_SECTOR_FULL        4
*/
int scanSectorBitmap(U32 address,U16 item_size,U16* workdItem)
{
	U8 bitmap[LOG_MMT_BITMAP_BYTES];
	U32 items_per_sector = (((LOG_MMT_SECTOR_SIZE)-(LOG_MMT_BITMAP_BYTES))/(item_size));
	U32 Bytes = (items_per_sector-1)/8 + 1;
	U32 tmp,i;
	U8 n,last_byte;
	esp_err_t err;

	if(items_per_sector == 0) return -1;
	
	err = spi_flash_read(address, bitmap, LOG_MMT_BITMAP_BYTES);
	if(err != 0)
	{
		return -2;
	}

	*workdItem = 0;
	for(i = 0;i < Bytes;i++)
	{
		if(bitmap[i] != 0)
		{
			break;
		}
	}

	last_byte = bitmap[Bytes-1] & (~(0xff>>(items_per_sector%8)));
	if(i == Bytes) 
	{
	    return BITMAP_STATE_SECTOR_FULL;  // all 0
	}
	else if((i == Bytes-1) && (last_byte == 0))
	{
		return BITMAP_STATE_LAST_BYTE_FULL;  // Last byte 00000XXX
	}
	else if((i == 0)&&(bitmap[i]==0xFF))
	{
		return BITMAP_STATE_FISRT_BYTE_EMPTY;  // First byte 11111111
	}
	else 
	{
		for(n=0;n<8;n++)
		{
			if((bitmap[i] & (0x80 >> n) )!= 0)
			{
				break;
			}
		}
		*workdItem = i*8 + n;                // 0b00111111
		return BITMAP_STAET_FIND_WORKING_ITEM;  // 000111xx
	}
	
}

/**
	@brief 获取一个数组bit流中第N位bit的值
	@attention none
	@param array BIT流数组
	@param bit   bit编号,从0开始
	@retval bit0/bit1
*/
U8 ArrayBitGet(const U8* array,U32 bit)
{
	U32 byte_index,bit_index;
	U8 res;

	// array = [0x01,0x02]
	byte_index = bit/8;  // 10/8 = 1
	bit_index  = bit%8;  // 10%8 = 2
	
	res = (array[byte_index] & (0x80 >> (bit_index)));
	if(res != 0) res = 1;

	return res;
}

/**
	@brief 读取log分区的id对应的所在bitmap中的bit值
	@attention none
	@param partation 分区
	@param id 全局id编号
	@param bit bit值
	@retval 0：success
*/
U8 LogMmtBitMapGet(U16 partation,U16 id,U8* bit)
{
	U32 sector_index,item_index,items_per_sector,addr;
	U8 bitmap[LOG_MMT_BITMAP_BYTES];
	
	if(partation >= LOG_MMT_PARTATIONS) return 1;

	items_per_sector = (((LOG_MMT_SECTOR_SIZE)-(LOG_MMT_BITMAP_BYTES))/(log_mmt_partations[partation].item_size));
	sector_index = id/items_per_sector;
	item_index = id%items_per_sector;
	addr = log_mmt_partations[partation].addr_start;  // Base addr
	addr += (U32)LOG_MMT_SECTOR_SIZE*sector_index; // Sector addr

	if(spi_flash_read(addr, bitmap, LOG_MMT_BITMAP_BYTES) != 0) return 2;

	*bit = ArrayBitGet(bitmap,item_index);
	return 0;
}

/**
	@brief 清除log分区的id对应的所在bitmap中的bit值
	@attention none
	@param partation 分区
	@param sector 扇区编号
	@param item 扇区下item编号
	@retval 0：success
*/
U8 LogMmtBitmapClear(U32 partation,U32 sector,U32 item)
{
	U32 addr;
	U16 ByteIndex,BitIndex,mapbit;
	
	if(partation >= LOG_MMT_PARTATIONS) return 1;

	ByteIndex = item/8;
	BitIndex  = item%8;
	addr = log_mmt_partations[partation].addr_start;  // Base addr
	addr += (U32)LOG_MMT_SECTOR_SIZE*log_mmt_working_sector[partation]; // Sector addr
	addr += ByteIndex; // Bitmap of item addr
	mapbit = 0xff>>(BitIndex+1);

	return spi_flash_write(addr, &mapbit, 1);
}
/*****************************************************************
                              Funs
******************************************************************/
/**
	@brief 擦除log分区
	@attention none
	@param partation 分区
	@retval 0：success
*/
U32 LogMmtPartitionErase(U16 partation)
{
	const esp_partition_t* partition1 = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_LOG0, "log0");
	
	const esp_partition_t* partition2 = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_LOG1, "log1");

	if(partation >= LOG_MMT_PARTATIONS) return 1;

	if(partation == 0)
	{
		return esp_partition_erase_range(partition1, 0, partition1->size);
	}
	else if(partation == 1)
	{
		return esp_partition_erase_range(partition2, 0, partition2->size);
	}
	else
	{
		return 2;
	}
}

/**
	@brief 擦除log分区下的一个sector
	@attention none
	@param partation 分区
	@param sector 扇区
	@retval 0：success
*/
U32 LogMmtSectorErase(U16 partition,U16 sector)
{
	U32 addr;
	
	if(partition >= LOG_MMT_PARTATIONS) return;

	addr = log_mmt_partations[partition].addr_start;
	addr += (U32)LOG_MMT_SECTOR_SIZE*sector;

	return (spi_flash_erase_range(addr, LOG_MMT_SECTOR_SIZE));
}

/**
	@brief Log数据写入
	@attention none
	@param partation 分区
	@param data 数据
	@param len 数据长度
	@param id 写入成功后返回的存储id
	@retval 0：success
*/
int   LogMmtWrite(U16 partation,U8* data,U32 len,U32* id)
{
	U32 working_id,sector_next,item_next;
	U32 items_per_sector; 
	U32 addr;
	U8  res;
	
	if(partation >= LOG_MMT_PARTATIONS) return 1;
	if(log_mmt_init[partation] == 0) return 2;
	if(len > log_mmt_partations[partation].item_size) return 3;

	items_per_sector = (((LOG_MMT_SECTOR_SIZE)-(LOG_MMT_BITMAP_BYTES))/(log_mmt_partations[partation].item_size));
	working_id = log_mmt_working_sector[partation]*items_per_sector+log_mmt_working_item[partation];
	*id = working_id;

	/********************************************************************************************
	1. 先判断是否将要写的sector关键item(最后一个item)，是则先擦除下一个sector，如果擦除不成功，
	则不清bitmap也不写入item，以此保证下一个sector能够被保证擦除。
	2. 如果不是关键点item，则先清bitmap，再写入item，以此保证item区域不被破坏，此时
	可能出现bitmap清了，但是item没有写入或者写入部分，需要加上校验,应用层可自己校验
	
	*********************************************************************************************/
	item_next = log_mmt_working_item[partation];
	sector_next = log_mmt_working_sector[partation];
	item_next++;
	if(item_next >= items_per_sector)
	{
		item_next = 0;
		sector_next++;
		if(sector_next >= log_mmt_partations[partation].sectors_per_partation)
		{
			sector_next = 0;
		}

		addr = log_mmt_partations[partation].addr_start;
		addr += (U32)LOG_MMT_SECTOR_SIZE*sector_next;
		if (spi_flash_erase_range(addr, LOG_MMT_SECTOR_SIZE) != 0)
		{
			return 4;
		}

	}

	
	if(LogMmtBitmapClear(partation,log_mmt_working_sector[partation],log_mmt_working_item[partation]) != 0)
	{
		return 5;
	}

	addr = log_mmt_partations[partation].addr_start;
	addr += (U32)LOG_MMT_SECTOR_SIZE*log_mmt_working_sector[partation];
	addr += LOG_MMT_BITMAP_BYTES+log_mmt_working_item[partation]*log_mmt_partations[partation].item_size;
	if(spi_flash_write(addr, data, len) != 0)
	{
		return 6;
	}
	else
	{
		log_mmt_working_item[partation] = item_next;
		log_mmt_working_sector[partation] = sector_next;
		log_mmt_working_state[partation] = 1;
		
		return 0;
	}

}

/**
	@brief Log数据读取
	@attention none
	@param partation 分区
	@param data 数据
	@param len 传入时作为data的大小，传出时为读取的数据长度
	@param id 存储id
	@retval 0：success
*/
int   LogMmtRead(U16 partation,U16 id,U8* data,U32* len)
{
	U32 sector,item,items_per_sector,addr,rlen,idMax;
	U8 bit;
	
	if(partation >= LOG_MMT_PARTATIONS) return 1;
	if(log_mmt_init[partation] == 0) return 2;

	items_per_sector = (((LOG_MMT_SECTOR_SIZE)-(LOG_MMT_BITMAP_BYTES))/(log_mmt_partations[partation].item_size));
	idMax = log_mmt_partations[partation].sectors_per_partation;
	idMax = idMax * items_per_sector -1;
	
	if(id > idMax ) return 3;
	if(LogMmtBitMapGet(partation,id,&bit) != 0) return 4;
	if(bit != 0) return 5;
	
	if(*len < log_mmt_partations[partation].item_size){
		rlen = *len;
	}else {
		rlen = log_mmt_partations[partation].item_size;
	}
	*len = rlen;
	
	sector = id/items_per_sector;
	item = id%items_per_sector;

	addr = log_mmt_partations[partation].addr_start;
	addr += sector*LOG_MMT_SECTOR_SIZE;
	addr += LOG_MMT_BITMAP_BYTES+item*log_mmt_partations[partation].item_size;

	//*len = log_mmt_partations[partation].item_size;
	
	return spi_flash_read(addr, data, rlen );

}

/**
	@brief 获取最新的存储id
	@attention none
	@param partation 分区
	@retval < 0:no data,else : latest id
*/
S32  LogMmtGetLatestId(U16 partation)
{
	U32 id,idMax;
	U32 items_per_sector;
	
	if(partation >= LOG_MMT_PARTATIONS) return -2;
	if(log_mmt_working_state[partation] == 0) return -1;

	items_per_sector = (((LOG_MMT_SECTOR_SIZE)-(LOG_MMT_BITMAP_BYTES))/(log_mmt_partations[partation].item_size));
	id = log_mmt_working_sector[partation];
	id = id*items_per_sector;
	id += log_mmt_working_item[partation];

	idMax = log_mmt_partations[partation].sectors_per_partation;
	idMax = idMax * items_per_sector -1;

	if(id == 0)
	{
		return (S32)idMax;
	}
	else
	{
		id--;
		return (S32)id;
	}
}

/**
	@brief 获取最新的存储id
	@attention none
	@param partation 分区
	@param id_now 当前id
	@retval < 0:no data,else : latest id
	@attention:
*/
S32  LogMmtGetLastId(U16 partation,S32 id_now)
{
	U32 id,idMax;
	U32 items_per_sector;
	U8 resU8,bit;
	
	if(partation >= LOG_MMT_PARTATIONS) return -1;

	items_per_sector = (((LOG_MMT_SECTOR_SIZE)-(LOG_MMT_BITMAP_BYTES))/(log_mmt_partations[partation].item_size));
	idMax = log_mmt_partations[partation].sectors_per_partation;
	idMax = idMax * items_per_sector -1;
	
	if(id_now < 0)
	{
		id = 0;
	}
	else if(id_now == 0)
	{
		
		id = idMax;
	}
	else
	{
		id = id_now-1;
	}

	resU8 = LogMmtBitMapGet(partation,id,&bit);
	if(resU8 != 0) return -1;
	if(bit != 0) return -2;
	
	return (S32)id;
}

/**
	@brief 获取Logmmt分区的总容量和使用情况
	@attention none
	@param partation 分区
	@param total 总容量(满载后在0-items_p_sec之间动态变化)
	@retval 0：success
*/
int LogMmtToalItems(U16 partation,U32* total)
{
	int res=0;
	U32 items_per_sector;
	U32 id,idMax;
	U8 resU8,bit;
	U32 num;

	if(total == NULL) return -2;
	if(partation >= LOG_MMT_PARTATIONS) return -3;

	if(log_mmt_working_state[partation] == 0)
	{
		*total = 0;
		return 0;
	}
	
	items_per_sector = (((LOG_MMT_SECTOR_SIZE)-(LOG_MMT_BITMAP_BYTES))/(log_mmt_partations[partation].item_size));
	idMax = log_mmt_partations[partation].sectors_per_partation;
	idMax = idMax * items_per_sector -1;
	
	resU8 = LogMmtBitMapGet(partation, idMax,&bit);
	if(resU8 != 0) return -3;

	if(bit == 0) // Overwrite
	{
		num = items_per_sector;
		num = num * (log_mmt_partations[partation].sectors_per_partation - 1);
		num = num + log_mmt_working_item[partation];
		*total = num;
	}
	else // Non-overwrite
	{
		num = log_mmt_working_sector[partation];
		num = num*items_per_sector;
		num += log_mmt_working_item[partation];
		*total = num;
	}
	
	return 0;
}

/**
	@brief 读取指定ID的上次ID数据并返回对应上次ID
	@attention none
	@param partation 分区
	@param id_now 当前id
	@param id_last 计算出的上次的id
	@param data 数据
	@param len 传入时作为data的大小，传出时为读取的数据长度
	@retval 0：success
*/
int LogMmtReadLastItem(U16 partation,S32 id_now,S32* id_last,U8* data,U32* len)
{
	S32 id;

	id = LogMmtGetLastId(partation,id_now);
	if(id >= 0)
	return LogMmtRead(partation,id,data,len);
	else 
	return -1;
}

/**
	@brief 读取最新的Log数据并返回对应ID
	@attention none
	@param partation 分区
	@param data 数据
	@param len 传入时作为data的大小，传出时为读取的数据长度
	@param id 返回最新的存储id
	@retval 0：success
*/
int  LogMmtReadLatestItem(U16 partation,S32* id,U8* data,U32* len)
{
	*id = LogMmtGetLatestId(partation);
	if(*id < 0) return 11; /* No data saved*/
	else return LogMmtRead(partation,*id,data,len);
}


/**
	@brief 初始化读取分区信息
	@attention none
	@param none
	@retval none
*/
void LogMmtInitBitmap(void)
{
	U32 partation,sector,addr,i;
	U32 working_sector=0,sector_c;
	U16 working_item;
	int sector_state,sector_state_last=-1,sector_state_first=-1;

	for(partation = 0;partation < LOG_MMT_PARTATIONS;partation++)
	{
		log_mmt_working_sector[partation] = 0;
		log_mmt_working_item[partation] = 0;
		log_mmt_working_state[partation] = 0;
		//for(sector= 0;sector < log_mmt_partations[partation].sectors_per_partation;sector++)
		for(sector_c=log_mmt_partations[partation].sectors_per_partation;sector_c > 0;sector_c--)
		{
			sector = sector_c-1;
			
			addr = log_mmt_partations[partation].addr_start;
			addr += (U32)sector*LOG_MMT_SECTOR_SIZE;
			sector_state = scanSectorBitmap(addr,log_mmt_partations[partation].item_size,&working_item);
			if(sector_state==BITMAP_STAET_FIND_WORKING_ITEM) 
			{
				/* 找到当前工作item所在的sector */
				log_mmt_working_sector[partation] = sector;
				log_mmt_working_item[partation] = working_item;
				log_mmt_working_state[partation] = 1;
				break;
			}
			else
			{
				if((sector_c < log_mmt_partations[partation].sectors_per_partation)&&
				((sector_state_last == BITMAP_STATE_FISRT_BYTE_EMPTY)&& 
				((sector_state == BITMAP_STATE_LAST_BYTE_FULL)||(sector_state== BITMAP_STATE_SECTOR_FULL))))
				{
				    /* 若sector不是首次查询的sector
				       且上次的sector状态是空
				       且当前sector状态时满的
				       则工作sector是上次的sector
				    */
					log_mmt_working_sector[partation] = sector+1;
					log_mmt_working_item[partation] = working_item;
					log_mmt_working_state[partation] = 1;
					break;
				}
				else if((sector_c == 1)&&
				(sector_state_first == BITMAP_STATE_LAST_BYTE_FULL || sector_state_first==BITMAP_STATE_SECTOR_FULL))
				{
				    /* 若本次sector是第一个sector(扫描的最后一个sector)
				       则判断扫描的第一个sector(最后一个sector)是空的
				       则工作sector是本次的sector
				    */
					log_mmt_working_sector[partation] = sector;
					log_mmt_working_item[partation] = working_item;
					log_mmt_working_state[partation] = 1;
					break;
				}
				else
				{
					/* 记录上次的sector状态和首次扫描的sector(从后往前扫描)的状态并继续查找下一个sector */
					sector_state_last = sector_state;
					if(sector_c == log_mmt_partations[partation].sectors_per_partation)
					{
						sector_state_first = sector_state;
					}
				}
				
			}
		}
	}
}

/**
	@brief 读取分区下某个sector的bitmap
	@attention none
	@param partition 分区
	@param sector 扇区
	@param data bitmap
	@retval none
*/
U32 LogMmtReadBitMap(U16 partation,U16 sector,U8* data)
{
	U32 addr;

	if(partation >= LOG_MMT_PARTATIONS) return 0;
	
	addr = log_mmt_partations[partation].addr_start;  // Base addr
	addr += (U32)LOG_MMT_SECTOR_SIZE*sector; // Sector addr
	return spi_flash_read(addr, data, LOG_MMT_BITMAP_BYTES);
}

/**
	@brief 模块初始化
	@attention none
	@param none
	@retval none
*/
void LogMmtInit(void)
{
	/* Init partation */
	const esp_partition_t* partition1 = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_LOG0, "log0");
	
	const esp_partition_t* partition2 = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_LOG1, "log1");

	log_mmt_init[0] = 0;
	log_mmt_init[1] = 0;
    if (partition1 == NULL) {
        log_mmt_partations[0].partation_size = 0;
		
    }else{
    	log_mmt_partations[0].partation_size = partition1->size;
		log_mmt_partations[0].addr_start     = partition1->address;
		log_mmt_init[0] = 1;
	}
	
	if (partition2 == NULL) {
        log_mmt_partations[1].partation_size = 0;
    }else{
    	log_mmt_partations[1].partation_size = partition2->size;
		log_mmt_partations[1].addr_start     = partition2->address;
		log_mmt_init[1] = 1;
	}
	
	/* Init bitmap */    
	LogMmtInitBitmap();
}

void LogMmtLoop(void)
{
    
}









