#include "DRV_ExFlash.h"
#include "DRV_SpiFlash.h"


void exflash_init()
{
    SpiFlashInit();
	//return EXFEC_SUCCESS;
}


t_exflash_err_code exflash_erase(unsigned int addr,unsigned len)
{
    SpiFlashErase(addr, len);
	return EXFEC_SUCCESS;
}

t_exflash_err_code exflash_program(unsigned int addr,unsigned char *pbuf,unsigned int len)
{
	SpiFlashBufferWrite(pbuf, addr, len);
	return EXFEC_SUCCESS;
}
	
t_exflash_err_code exflash_verify(unsigned int addr,unsigned char* pbuf,unsigned int len)
{
	return SpiFlashBufferVerify(pbuf, addr, len);
	//return EXFEC_SUCCESS;
}
	
t_exflash_err_code exflash_read(unsigned int addr,unsigned char *pbuf,unsigned int len)
{
	SpiFlashBufferRead(pbuf, addr, len);
	return EXFEC_SUCCESS;
}
	
t_exflash_err_code exflash_write(unsigned int addr,unsigned char *pbuf,unsigned int len)
{
	SpiFlashErase(addr, len);
	SpiFlashBufferWrite(pbuf, addr, len);
	return EXFEC_SUCCESS;
}









