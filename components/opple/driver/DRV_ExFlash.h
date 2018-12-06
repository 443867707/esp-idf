#ifndef __DRV_EXFLASH_H__
#define __DRV_EXFLASH_H__




typedef enum
{
    EXFEC_SUCCESS=0,
    EXFEC_FAIL,
    EXFEC_LEN_ERR,
    EXFEC_ERASE_FAIL,
    EXFEC_PROGRAM_FAIL,
    EXFEC_VERIFY_FAIL,
}t_exflash_err_code;

extern void exflash_init(void);
extern t_exflash_err_code exflash_erase(unsigned int addr,unsigned len);
extern t_exflash_err_code exflash_program(unsigned int addr,unsigned char *pbuf,unsigned int len);
extern t_exflash_err_code exflash_verify(unsigned int addr,unsigned char* pbuf,unsigned int len);
extern t_exflash_err_code exflash_read(unsigned int addr,unsigned char *pbuf,unsigned int len);
extern t_exflash_err_code exflash_write(unsigned int addr,unsigned char *pbuf,unsigned int len);








#endif
