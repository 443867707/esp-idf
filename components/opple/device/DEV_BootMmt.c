#include "DEV_BootMmt.h"
#include "LIB_CRC32.h"
#include "DEV_MemoryMmt.h"


t_bootmmt_err_code readBootPara(t_boot_para* para)
{
	  t_memory_err_code err;
	  unsigned int crc;
	  
	  err = memory_read(MB_BOOT_PARA1,0,(unsigned char*)para,sizeof(t_boot_para));
    if(err != MEC_SUCCESS)
		{
			return FLASH_READ_ERR;
		}
    
    crc = getCrc32((unsigned char*)para,sizeof(t_boot_para)-4);

		if(crc == para->crc)
    {
				return BEC_CRC_ERROR;
    }
		
		return BEC_SUCCESS;
}

t_bootmmt_err_code writeBootPara(t_boot_para* para)
{
	t_memory_err_code err;
	
	para->crc = getCrc32((unsigned char*)para,sizeof(t_boot_para)-4);
	err = memory_write(MB_BOOT_PARA1,0,(unsigned char*)para,sizeof(t_boot_para));
	
	if(err != MEC_SUCCESS)
	{
		return BEC_FLASH_WRITE_ERR;
	}
	
	return BEC_SUCCESS;
}

