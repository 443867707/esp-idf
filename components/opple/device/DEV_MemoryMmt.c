#include "DEV_MemoryMmt.h"
#include "DRV_Flash.h"
#include "DRV_ExFlash.h"

/*
 * 注意：block顺序必须和头文件中一致
 * Attention : The order of blocks must be consistent with the order in header file
 */
const t_memory_mmt memory_mmt[MB_NUM_MAX]=
{
	{.block=MB_BOOTLOADER,     .pos=MP_FLASH_INNER,.addr=0x08000000,.size=0x4000}, /* fixed */
	{.block=MB_BOOT_PARA1,     .pos=MP_FLASH_INNER,.addr=0x08004000,.size=0x4000}, /* fixed */
	{.block=MB_BOOT_PARA2,     .pos=MP_FLASH_INNER,.addr=0x08008000,.size=0x4000}, /* fixed */
	{.block=MB_APP,            .pos=MP_FLASH_INNER,.addr=0x08010000,.size=0x10000},
	//{.block=MB_APP_OTA,        .pos=MP_FLASH_INNER,.addr=0x0800c000,.size=0x4000},
	{.block=MB_APP_OTA,        .pos=MP_FLASH_EX,   .addr=0x00000000,.size=0x4000},
	{.block=MB_APP_BACKUP,     .pos=MP_FLASH_EX,   .addr=0x00000000,.size=0x14000},
	{.block=MB_APP_PARA,       .pos=MP_FLASH_EX,   .addr=0x00014000,.size=0x4000},
	{.block=MB_APP_PARA_BACKUP,.pos=MP_FLASH_EX,   .addr=0x00018000,.size=0x4000},
	{.block=MB_APP_PARA_FCT,   .pos=MP_FLASH_EX,   .addr=0x0001c000,.size=0x4000},
	{.block=MB_DB,             .pos=MP_FLASH_EX,   .addr=0x00020000,.size=0x10000},
};

void memory_init(void)
{
	exflash_init();
}

t_memory_err_code memory_erase(t_memory_block block,unsigned int offset,unsigned int len)
{
	if(block >= MB_NUM_MAX) return MEC_BLOCK_ERROR;
	if((offset+len) > memory_mmt[block].size) return MEC_LEN_ERROR;
	
	if(memory_mmt[block].pos == MP_FLASH_INNER)
	{
		if(flash_erase(memory_mmt[block].addr+offset,len) != 0)
		{
			return MEC_ERASE_FAIL;
		}
	}
	else
	{
		if(exflash_erase(memory_mmt[block].addr+offset,len) != 0)
		{
			return MEC_ERASE_FAIL;
		}
	}
	return MEC_SUCCESS;
}

t_memory_err_code memory_program(t_memory_block block,unsigned int offset,unsigned char *pbuf,unsigned int len)
{
	if(block >= MB_NUM_MAX) return MEC_BLOCK_ERROR;
	if((offset+len) > memory_mmt[block].size) return MEC_LEN_ERROR;
	
	if(memory_mmt[block].pos == MP_FLASH_INNER)
	{
		if(flash_program(memory_mmt[block].addr+offset,pbuf,len) != 0)
		{
			return MEC_PROGRAM_FAIL;
		}
	}
	else
	{
		if(exflash_program(memory_mmt[block].addr+offset,pbuf,len) != 0)
		{
			return MEC_PROGRAM_FAIL;
		}
	}
	return MEC_SUCCESS;
}

t_memory_err_code memory_verify(t_memory_block block,unsigned int offset,unsigned char* pbuf,unsigned int len)
{
	if(block >= MB_NUM_MAX) return MEC_BLOCK_ERROR;
	if((offset+len) > memory_mmt[block].size) return MEC_LEN_ERROR;
	
	if(memory_mmt[block].pos == MP_FLASH_INNER)
	{
		if(flash_verify(memory_mmt[block].addr+offset,pbuf,len) != 0)
		{
			return MEC_VERITY_FAIL;
		}
	}
	else
	{
		if(exflash_verify(memory_mmt[block].addr+offset,pbuf,len) != 0)
		{
			return MEC_VERITY_FAIL;
		}
	}
	return MEC_SUCCESS;
}

t_memory_err_code memory_read(t_memory_block block,unsigned int offset,unsigned char *pbuf,unsigned int len)
{
	if(block >= MB_NUM_MAX) return MEC_BLOCK_ERROR;
	if((offset+len) > memory_mmt[block].size) return MEC_LEN_ERROR;
	
	if(memory_mmt[block].pos == MP_FLASH_INNER)
	{
		if(flash_read(memory_mmt[block].addr+offset,pbuf,len) != 0)
		{
			return MEC_READ_FAIL;
		}
	}
	else
	{
		if(exflash_read(memory_mmt[block].addr+offset,pbuf,len) != 0)
		{
			return MEC_READ_FAIL;
		}
	}
	return MEC_SUCCESS;
}

t_memory_err_code memory_write(t_memory_block block,unsigned int offset,unsigned char *pbuf,unsigned int len)
{
	if(block >= MB_NUM_MAX) return MEC_BLOCK_ERROR;
	if((offset+len) > memory_mmt[block].size) return MEC_LEN_ERROR;
	
	if(memory_mmt[block].pos == MP_FLASH_INNER)
	{
		if(flash_write(memory_mmt[block].addr+offset,pbuf,len) != 0)
		{
			return MEC_WRITE_FAIL;
		}
	}
	else
	{
		if(exflash_write(memory_mmt[block].addr+offset,pbuf,len) != 0)
		{
			return MEC_WRITE_FAIL;
		}
	}
	return MEC_SUCCESS;
}
