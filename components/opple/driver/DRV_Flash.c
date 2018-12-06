#include "DRV_Flash.h"
//#include "stm32f4xx.h"
//#include "stm32f4xx_hal_flash.h"
//#include "stm32f4xx_hal_flash_ex.h"
#include "Typedef.h"

/* Base address of the Flash sectors */
#define ADDR_FLASH_SECTOR_0     ((U32)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((U32)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((U32)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((U32)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((U32)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((U32)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((U32)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((U32)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((U32)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((U32)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((U32)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((U32)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */


unsigned int GetSector(unsigned int Address)
{
  unsigned int sector = 0;
#if 0
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;  
  }
	else
	{
		return FLASH_SECTOR_4;
	}
#endif	
  return sector;
}

t_flash_err_code flash_erase(unsigned int addr,unsigned len)
{
#if 0
    unsigned int start_sector,end_sector,i;

    start_sector = GetSector(addr);
    end_sector = GetSector(addr+len-1);

    //FLASH_Unlock(); //解锁FLASH后才能向FLASH中写数据。
    //FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
    //              FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
    HAL_FLASH_Unlock();
    for(i = start_sector;i<=end_sector;i++)
    {
        FLASH_Erase_Sector(i,FLASH_VOLTAGE_RANGE_3);
        // watchdog_feed();
    }
		HAL_FLASH_Lock();

    //FLASH_Lock();  //读FLASH不需要FLASH处于解锁状态。
#endif
    return FEC_SUCCESS;
}

t_flash_err_code flash_program(unsigned int addr,unsigned char *pbuf,unsigned int len)
{
#if 0
    unsigned int start_addr,i=0;

    start_addr = addr;
    //end_addr = addr+len-1;

    //FLASH_Unlock(); //解锁FLASH后才能向FLASH中写数据。
    //FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
    //              FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
    HAL_FLASH_Unlock();
    for(i=0;i<len;i++)
    {
        // if(FLASH_Program_Byte(start_addr+i,pbuf[i]) != FLASH_COMPLETE)
			  if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,start_addr+i,pbuf[i]) != HAL_OK)
        {
             if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,start_addr+i,pbuf[i]) != HAL_OK)
             {
                return FEC_FAIL;
             }
        }
    }
    HAL_FLASH_Lock();
    //FLASH_Lock();  //读FLASH不需要FLASH处于解锁状态
#endif
    return FEC_SUCCESS;
}

t_flash_err_code flash_read(unsigned int addr,unsigned char *pbuf,unsigned int len)
{
    unsigned short i;

    if ((0 == len) || (0 == pbuf))
    {
        return FEC_LEN_ERR;
    }

    for (i = 0; i < len; i++)
    {
        pbuf[i] = *(volatile unsigned char*)addr;
        addr ++;
    }

    return FEC_SUCCESS;
}

t_flash_err_code flash_verify(unsigned int addr,unsigned char* pbuf,unsigned int len)
{
    unsigned int i,data;

    for(i = 0;i < len;i++)
    {
        data = *(volatile unsigned char*)(addr+i);
        if(data != pbuf[i])
        {
            return FEC_FAIL;
        }
    }
    return FEC_SUCCESS;
}

t_flash_err_code flash_write(unsigned int addr,unsigned char *pbuf,unsigned int len)
{
    //flash_erase(addr,len);
    //flash_program(add,pbuf,len);
    //unsigned char buf[200];

    //if(len > 200) return BEC_LEN_ERR;

    // if(flash_read(addr,buf,len) != BEC_SUCCESS) return BEC_FAIL;
    if(flash_erase(addr,len) != FEC_SUCCESS) return FEC_ERASE_FAIL;
    if(flash_program(addr,pbuf,len) != FEC_SUCCESS) return FEC_PROGRAM_FAIL;
    if(flash_verify(addr,pbuf,len) != FEC_SUCCESS) return FEC_VERIFY_FAIL;

    return FEC_SUCCESS;


}

