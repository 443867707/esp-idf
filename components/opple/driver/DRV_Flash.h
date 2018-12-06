#ifndef __DRV_FLASH_H__
#define __DRV_FLASH_H__

//#include "BootTypedef.h"


typedef enum
{
    FEC_SUCCESS=0,
    FEC_FAIL,
    FEC_LEN_ERR,
    FEC_ERASE_FAIL,
    FEC_PROGRAM_FAIL,
    FEC_VERIFY_FAIL,
}t_flash_err_code;

extern t_flash_err_code flash_erase(unsigned int addr,unsigned int len);
extern t_flash_err_code flash_program(unsigned int addr,unsigned char *pbuf,unsigned int len);
extern t_flash_err_code flash_verify(unsigned int addr,unsigned char* pbuf,unsigned int len);
extern t_flash_err_code flash_read(unsigned int addr,unsigned char *pbuf,unsigned int len);
extern t_flash_err_code flash_write(unsigned int addr,unsigned char *pbuf,unsigned int len);

#endif
