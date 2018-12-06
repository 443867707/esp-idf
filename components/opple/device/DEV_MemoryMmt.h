#ifndef __MEMORY_MMT_H__
#define __MEMORY_MMT_H__


/***************************************************************************
 模块用于管理内置，外置Flash，并对存储区地址
 进行分配，所有需要用到存储区的模块都需要从
 这里进行分配
 
—————————————————————————————————————————————
Bootloader(para)   App(para)    数据记录模块
—————————————————————————————————————————————
        内存管理模块（地址分配，读写）
—————————————————————————————————————————————
    内置FLASH驱动              外置FLASH驱动
***************************************************************************/



/**************************************************************************
                                  Typedefs
**************************************************************************/
typedef enum
{
	MB_BOOTLOADER=0,
	MB_BOOT_PARA1,
	MB_BOOT_PARA2,
	MB_APP,
	MB_APP_OTA,
	MB_APP_BACKUP,
	MB_APP_PARA,
	MB_APP_PARA_BACKUP,
	MB_APP_PARA_FCT,
	MB_DB,
	
	MB_NUM_MAX,
}t_memory_block;

typedef enum
{
	MP_FLASH_INNER=0,
	MP_FLASH_EX,
}t_memoty_position;

typedef struct
{
	t_memory_block block;
	t_memoty_position pos;
	unsigned int addr;
	unsigned int size;
}t_memory_mmt;

typedef enum
{
	MEC_SUCCESS=0,
	MEC_LEN_ERROR,
	MEC_ADDR_ERROR,
	MEC_BLOCK_ERROR,
	MEC_ERASE_FAIL,
	MEC_PROGRAM_FAIL,
	MEC_VERITY_FAIL,
	MEC_READ_FAIL,
	MEC_WRITE_FAIL,
}t_memory_err_code;


/**************************************************************************
                                  extern Data
**************************************************************************/
extern const t_memory_mmt memory_mmt[MB_NUM_MAX];




/**************************************************************************
                                  Interfaces
**************************************************************************/
extern void memory_init(void);
extern t_memory_err_code memory_erase(t_memory_block block,unsigned int offset,unsigned int len);
extern t_memory_err_code memory_program(t_memory_block block,unsigned int offset,unsigned char *pbuf,unsigned int len);
extern t_memory_err_code memory_verify(t_memory_block block,unsigned int offset,unsigned char* pbuf,unsigned int len);
extern t_memory_err_code memory_read(t_memory_block block,unsigned int offset,unsigned char *pbuf,unsigned int len);
extern t_memory_err_code memory_write(t_memory_block block,unsigned int offset,unsigned char *pbuf,unsigned int len);


#endif
