//#include "stm32f4xx_hal.h"
//#include "stm32f4xx_hal_spi.h"
#include "Includes.h"
#include "DRV_SpiFlash.h"

//SPI_HandleTypeDef NorFlashSpiHandle;

//static __IO uint32_t  SPITimeout = SPIT_LONG_TIMEOUT; 
static uint32_t  SPITimeout = SPIT_LONG_TIMEOUT;   
uint32_t NorFlashSpiTimeoutx = NORFLASH_SPI_TIMEOUT_MAX;

static  uint16_t SpiTimeoutUserCallback(uint8_t errorCode)
{
  //NBDebug("SPI 等待超时， errcode = %d", errorCode);
  return 0;
}

#if 0
static void NorFlash_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  NORFLASH_SPI_CLK_ENABLE();
  NORFLASH_SPI_GPIO_CLK_ENABLE();

  /* configure SPI SCK, MOSI and MISO */    
  GPIO_InitStructure.Pin    = NORFLASH_SPI_SCK_PIN | NORFLASH_SPI_MISO_PIN | NORFLASH_SPI_MOSI_PIN;
  GPIO_InitStructure.Mode   = GPIO_MODE_AF_PP;
  GPIO_InitStructure.Pull   = GPIO_NOPULL;
  GPIO_InitStructure.Speed  = GPIO_SPEED_FAST;
  GPIO_InitStructure.Alternate = NORFLASH_SPI_AF;
  HAL_GPIO_Init(NORFLASH_SPI_GPIO_PORT, &GPIO_InitStructure); 
}
#endif
 /**
  * @brief  flash spi 片选gpio初始化
  * @param  无
  * @retval 无
  */
void SpiFlashInitCS(void)
{
#if 0
  GPIO_InitTypeDef  GPIO_InitStruct;
  
  NORFLASH_CS_GPIO_CLK_ENABLE();
  
  GPIO_InitStruct.Pin =   NORFLASH_SPI_CS_PIN;
  GPIO_InitStruct.Mode =  GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull =  GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  
  HAL_GPIO_Init(NORFLASH_SPI_CS_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(NORFLASH_SPI_CS_GPIO_PORT, NORFLASH_SPI_CS_PIN, GPIO_PIN_RESET); 
#endif  
}


 /**
  * @brief  flash spi、gpio 初始化
  * @param  无
  * @retval 无
  */
static void SpiFlashDriverInit(void)
{
#if 0
  if(HAL_SPI_GetState(&NorFlashSpiHandle) == HAL_SPI_STATE_RESET)
  {
    /* SPI configuration -----------------------------------------------------*/
    NorFlashSpiHandle.Instance = NORFLASH_SPI;
	
    /* SPI baudrate is set to 0.195 MHz (PCLK2/SPI_BaudRatePrescaler = 50/256 = 0.195 MHz) 
       to verify these constraints:
       - PCLK2 frequency is set to 50 MHz 
    */
    
    NorFlashSpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    NorFlashSpiHandle.Init.Direction      = SPI_DIRECTION_2LINES;
    NorFlashSpiHandle.Init.CLKPhase       = SPI_PHASE_1EDGE;
    NorFlashSpiHandle.Init.CLKPolarity    = SPI_POLARITY_LOW;
    NorFlashSpiHandle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
    NorFlashSpiHandle.Init.CRCPolynomial  = 7;
    NorFlashSpiHandle.Init.DataSize       = SPI_DATASIZE_8BIT;
    NorFlashSpiHandle.Init.FirstBit       = SPI_FIRSTBIT_MSB;
    NorFlashSpiHandle.Init.NSS            = SPI_NSS_SOFT;
    NorFlashSpiHandle.Init.TIMode         = SPI_TIMODE_DISABLED;
    NorFlashSpiHandle.Init.Mode           = SPI_MODE_MASTER;
  
    NorFlash_SPI_MspInit(&NorFlashSpiHandle);
    HAL_SPI_Init(&NorFlashSpiHandle);
		
	  SpiFlashInitCS();
  } 
#endif
}

static void SpiFlashError(void)
{
#if 0
  /* De-initialize the SPI communication BUS */
  HAL_SPI_DeInit(&NorFlashSpiHandle);
  
  /* Re- Initialize the SPI communication BUS */
  SpiFlashDriverInit();
#endif  
}


 /**
  * @brief  读取一个字节
  * @param  无
  * @retval 读取到的值
  */
static uint8_t SpiFlashReadByte(void)
{
#if 0
  HAL_StatusTypeDef enStatus = HAL_OK;
  uint8_t u8Readvalue, u8Dummy_value;

  u8Dummy_value = Dummy_Byte;

  enStatus = HAL_SPI_TransmitReceive(&NorFlashSpiHandle, &u8Dummy_value, &u8Readvalue, 1, NorFlashSpiTimeoutx);
  
  /* Check the communication status */
  if(enStatus != HAL_OK)
  {
    /* Re-Initialize the BUS */
    SpiFlashError();
  }
 
  return u8Readvalue;
#endif
	return 0;
}


/**
  * @brief  Writes a byte to device.
  * @param  Value: value to be written
  * @retval 无
  */
static void SpiFlashSendByte(uint8_t u8Value)
{
#if 0
  HAL_StatusTypeDef status = HAL_OK;
  
  status = HAL_SPI_Transmit(&NorFlashSpiHandle, &u8Value, 1, NorFlashSpiTimeoutx);
  
  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* Re-Initialize the BUS */
    SpiFlashError();
  }
#endif  
}

 /**
  * @brief  使能flash的写操作
  * @param  无
  * @retval 无
  */
static void SpiFlashWriteEnable(void)
{
  SPI_FLASH_CS_LOW();
  SpiFlashSendByte(MT25Q_WriteEnable);
  SPI_FLASH_CS_HIGH();
}

 /**
  * @brief  等待写操作完成
  * @param  无
  * @retval 无
  */
static void SpiFlashWaitForWriteEnd(void)
{
  uint8_t FLASH_Status = 0;

  SPI_FLASH_CS_LOW();
#if 0
  SpiFlashSendByte(MT25Q_ReadStatusReg);

  SPITimeout = SPIT_FLAG_TIMEOUT;
  
  do
  {
    FLASH_Status = SpiFlashReadByte();	 

    {
      if((SPITimeout--) == 0) 
      {
        SpiTimeoutUserCallback(4);
        return;
      }
    } 
  }
  while ((FLASH_Status & WIP_Flag) == SET);
#endif
  SPI_FLASH_CS_HIGH();
}

 /**
  * @brief  擦除flash扇区
  * @param  u32SectorAddr 要擦除的扇区地址
  * @retval 无
  */
static void SpiFlashSectorErase(uint32_t u32SectorAddr)
{
  /*send write enable cmd*/
 
  SpiFlashWriteEnable();
  SpiFlashWaitForWriteEnd();
  
  SPI_FLASH_CS_LOW();
  SpiFlashSendByte(MT25Q_SectorErase);
  SpiFlashSendByte((u32SectorAddr & 0xFF0000) >> 16);
  SpiFlashSendByte((u32SectorAddr & 0xFF00) >> 8);
  SpiFlashSendByte(u32SectorAddr & 0xFF);
  SPI_FLASH_CS_HIGH();
  
  SpiFlashWaitForWriteEnd();
}

 /**
  * @brief  根据起始地址，和擦除字节大小，擦除flash
  * @param  u32SartAddr, 起始地址
  * @param  u32NumBytes, 擦除大小，注 此大小可能小于实际擦除的大小（非sector 大小的整除倍）
  * @retval 无
  */
void SpiFlashErase(uint32_t u32SartAddr, uint32_t u32NumBytes)
{
	
	uint32_t i  = 0, u32SectorCounts, u32SectorAddr;
 
	u32SectorCounts = u32NumBytes / SPI_FLASH_SectorSize + 1;

  while (i < u32SectorCounts) {
		u32SectorAddr = u32SartAddr + i * SPI_FLASH_SectorSize;
	  SpiFlashSectorErase(u32SectorAddr);
		i++;
	}	
}


 /**
  * @brief  擦除一叠
  * @param  无
  * @retval 无
  */
void SPI_Flash_BulkErase(void)
{
  SpiFlashWriteEnable();
  SpiFlashSendByte(MT25Q_ChipErase);
  SpiFlashWaitForWriteEnd();
}


 /**
  * @brief   对flash按页写入数据（最大256字节）
  * @param	 pBuffer：     写入数据的指针
  * @param   u32WriteAddr：写入的地址
  * @param   u16NumByteToWrite, 写入数据长度，要小于等于SPI_FLASH_PerWritePageSize
  * @retval  无
  */
static void SpiFlashPageWrite(uint8_t* pBuffer, uint32_t u32WriteAddr, uint16_t u16NumByteToWrite)
{
  SpiFlashWriteEnable();
  SPI_FLASH_CS_LOW();
  SpiFlashSendByte(MT25Q_PageProgram);
  SpiFlashSendByte((u32WriteAddr & 0xFF0000) >> 16);
  SpiFlashSendByte((u32WriteAddr & 0xFF00) >> 8);
  SpiFlashSendByte(u32WriteAddr & 0xFF);

  if(u16NumByteToWrite > SPI_FLASH_PerWritePageSize)
  {
     u16NumByteToWrite = SPI_FLASH_PerWritePageSize;
     //NBDebug("SPI_FLASH_PageWrite too large!");
  }

  while (u16NumByteToWrite--)
  {
    SpiFlashSendByte(*pBuffer);
    pBuffer++;
  }

  SPI_FLASH_CS_HIGH();

  SpiFlashWaitForWriteEnd();
}


 /**
  * @brief  对flash，按数据块入数据
  * @param	pBuffer 要写入数据的指针
  * @param  u32WriteAddr 写入地址	
  * @param  u16NumByteToWrite 写入数据长度
  * @retval
  */
void SpiFlashBufferWrite(uint8_t* pBuffer, uint32_t u32WriteAddr, uint16_t u16NumByteToWrite)
{
  uint8_t u8NumOfPage = 0, u8NumOfSingle = 0, u8Addr = 0, u8count = 0, u8temp = 0;
	
  u8Addr = u32WriteAddr % SPI_FLASH_PageSize;
	
  u8count = SPI_FLASH_PageSize - u8Addr;	
  u8NumOfPage =  u16NumByteToWrite / SPI_FLASH_PageSize;
  u8NumOfSingle = u16NumByteToWrite % SPI_FLASH_PageSize;

  if (u8Addr == 0) 
  {
		/* NumByteToWrite < SPI_FLASH_PageSize */
    if (u8NumOfPage == 0) 
    {
      SpiFlashPageWrite(pBuffer, u32WriteAddr, u16NumByteToWrite);
    }
    else /* NumByteToWrite > SPI_FLASH_PageSize */
    {
      while (u8NumOfPage--)
      {
        SpiFlashPageWrite(pBuffer, u32WriteAddr, SPI_FLASH_PageSize);
        u32WriteAddr +=  SPI_FLASH_PageSize;
        pBuffer += SPI_FLASH_PageSize;
      }
			
      SpiFlashPageWrite(pBuffer, u32WriteAddr, u8NumOfSingle);
    }
  }
  else 
  {
		/* NumByteToWrite < SPI_FLASH_PageSize */
    if (u8NumOfPage == 0) 
    {
      if (u8NumOfSingle > u8count) 
      {
        u8temp = u8NumOfSingle - u8count;
				
        SpiFlashPageWrite(pBuffer, u32WriteAddr, u8count);
        u32WriteAddr +=  u8count;
        pBuffer += u8count;
				
        SpiFlashPageWrite(pBuffer, u32WriteAddr, u8temp);
      }
      else 
      {				
        SpiFlashPageWrite(pBuffer, u32WriteAddr, u16NumByteToWrite);
      }
    }
    else /* NumByteToWrite > SPI_FLASH_PageSize */
    {

      u16NumByteToWrite -= u8count;
      u8NumOfPage =  u16NumByteToWrite / SPI_FLASH_PageSize;
      u8NumOfSingle = u16NumByteToWrite % SPI_FLASH_PageSize;

      SpiFlashPageWrite(pBuffer, u32WriteAddr, u8count);
      u32WriteAddr +=  u8count;
      pBuffer += u8count;
			
      while (u8NumOfPage--)
      {
        SpiFlashPageWrite(pBuffer, u32WriteAddr, SPI_FLASH_PageSize);
        u32WriteAddr +=  SPI_FLASH_PageSize;
        pBuffer += SPI_FLASH_PageSize;
      }
      if (u8NumOfSingle != 0)
      {
        SpiFlashPageWrite(pBuffer, u32WriteAddr, u8NumOfSingle);
      }
    }
  }
}


 /**
  * @brief   对flash按数据块读数据
  * @param   pBuffer：          存储读取数据缓冲区的指针	
  * @param   u32ReadAddr：      flash中读取数据的起始地址
  * @param   u16NumByteToRead： 读取的数据块长度
  * @retval
  */
void SpiFlashBufferRead(uint8_t* pBuffer, uint32_t u32ReadAddr, uint16_t u16NumByteToRead)
{
  SPI_FLASH_CS_LOW();
  SpiFlashSendByte(MT25Q_ReadData);

  SpiFlashSendByte((u32ReadAddr & 0xFF0000) >> 16);
  SpiFlashSendByte((u32ReadAddr& 0xFF00) >> 8);
  SpiFlashSendByte(u32ReadAddr & 0xFF);
  
  while (u16NumByteToRead--)
  {
    *pBuffer = SpiFlashReadByte();
     pBuffer++;
  }

  SPI_FLASH_CS_HIGH();
}

 /**
  * @brief   对写入flash的数据块进行校验
  * @param   pBuffer：          校验数据内存中缓冲区的指针
  * @param   u32ReadAddr：      flash中校验的起始地址
  * @param   u16NumByteToRead： 校验的长度
  * @retval  0：校验成功， -1：校验失败
  */
int32_t SpiFlashBufferVerify(uint8_t* pBuffer, uint32_t u32VerfyAddr, uint16_t u16NumByteToVerfy)
{
	uint8_t u8TmpValue;
	
  SPI_FLASH_CS_LOW();
  SpiFlashSendByte(MT25Q_ReadData);
  SpiFlashSendByte((u32VerfyAddr & 0xFF0000) >> 16);
  SpiFlashSendByte((u32VerfyAddr& 0xFF00) >> 8);
  SpiFlashSendByte(u32VerfyAddr & 0xFF);
  
  while (u16NumByteToVerfy--)
  {
    u8TmpValue = SpiFlashReadByte();
		if (*pBuffer != u8TmpValue) {
		  return -1;
		}
     pBuffer++;
  }

  SPI_FLASH_CS_HIGH();
	
	return 0;
}

#if 0
 /**
  * @brief  读取flash的设备ID
  * @param
  * @retval FLASH ID
  */
uint32_t SPI_Flash_ReadID(void)
{
  uint32_t u32Temp = 0, u32Temp0 = 0, u32Temp1 = 0, u32Temp2 = 0;

  SPI_FLASH_CS_LOW();

  SpiFlashSendByte(MT25Q_JedecDeviceID);

  u32Temp0 = SpiFlashReadByte();

  u32Temp1 = SpiFlashReadByte();

  u32Temp2 = SpiFlashReadByte();

  SPI_FLASH_CS_HIGH();

  u32Temp = (u32Temp0 << 16) | (u32Temp1 << 8) | u32Temp2;

  return u32Temp;
}
#endif


 /**
  * @brief  读取flash id
  * @param  无
  * @retval FLASH Device ID
  */
uint32_t SPI_Flash_ReadDeviceID(void)
{
  uint32_t u32Temp = 0;

  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();

  /* Send "RDID " instruction */
  SpiFlashSendByte(MT25Q_JedecDeviceID);
  SpiFlashSendByte(Dummy_Byte);
  SpiFlashSendByte(Dummy_Byte);
  SpiFlashSendByte(Dummy_Byte);
  
  /* Read a byte from the FLASH */
  u32Temp = SpiFlashReadByte();

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

  return u32Temp;
}

void SpiFlashInit(void)
{
	SpiFlashDriverInit();
}

