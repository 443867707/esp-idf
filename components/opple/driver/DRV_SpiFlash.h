#ifndef _DRV_SPI_FLASH_H
#define _DRV_SPI_FLASH_H
#include "stdint.h"

/*
#define NORFLASH_SPI                             SPI2
#define NORFLASH_SPI_CLK_ENABLE()                __HAL_RCC_SPI2_CLK_ENABLE()
#define NORFLASH_SPI_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOB_CLK_ENABLE()
#define NORFLASH_CS_GPIO_CLK_ENABLE()            __HAL_RCC_GPIOB_CLK_ENABLE()
#define NORFLASH_SPI_FORCE_RESET()               __HAL_RCC_SPI2_FORCE_RESET()
#define NORFLASH_SPI_RELEASE_RESET()             __HAL_RCC_SPI2_RELEASE_RESET()

#define NORFLASH_SPI_SCK_PIN                     GPIO_PIN_13
#define NORFLASH_SPI_MISO_PIN                    GPIO_PIN_14
#define NORFLASH_SPI_MOSI_PIN                    GPIO_PIN_15
#define NORFLASH_SPI_AF                          GPIO_AF5_SPI2

#define NORFLASH_SPI_GPIO_PORT                   GPIOB


#define NORFLASH_SPI_CS_PIN                      GPIO_PIN_12               
#define NORFLASH_SPI_CS_GPIO_PORT                GPIOB 

#define NORFLASH_SPI_IRQn                        SPI2_IRQn
#define NORFLASH_SPI_IRQHandler                  SPI2_IRQHandler
*/
#define BUFFERSIZE                       (COUNTOF(aTxBuffer) - 1)
#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

#define NORFLASH_SPI_TIMEOUT_MAX              ((uint32_t)0x1000)


#define  sFLASH_ID                       0XEF4018     //MT25Q128
#define SPI_FLASH_SectorSize            4096
#define SPI_FLASH_PageSize              256
#define SPI_FLASH_PerWritePageSize      256

#define MT25Q_WriteEnable		      0x06 
#define MT25Q_WriteDisable		      0x04 
#define MT25Q_ReadStatusReg		    0x05 
#define MT25Q_WriteStatusReg		    0x01 
#define MT25Q_ReadData			        0x03 
#define MT25Q_FastReadData		      0x0B 
#define MT25Q_FastReadDual		      0x3B 
#define MT25Q_PageProgram		      0x02 
#define MT25Q_BlockErase			      0xD8 
#define MT25Q_SectorErase		      0x20 
#define MT25Q_ChipErase			      0xC4
#define MT25Q_PowerDown			      0xB9 
#define MT25Q_ReleasePowerDown	  0xAB 
#define MT25Q_DeviceID			        0x9E
#define MT25Q_ManufactDeviceID   	0x90 
#define MT25Q_JedecDeviceID		    0x9F 

#define WIP_Flag                  0x01  /* Write In Progress (WIP) flag */
#define Dummy_Byte                0xFF


#define SPIT_FLAG_TIMEOUT         ((uint32_t)0x1000)
#define SPIT_LONG_TIMEOUT         ((uint32_t)(10 * SPIT_FLAG_TIMEOUT))

#if 0
#define SPI_FLASH_CS_HIGH()  HAL_GPIO_WritePin(NORFLASH_SPI_CS_GPIO_PORT, NORFLASH_SPI_CS_PIN, GPIO_PIN_SET)
#define SPI_FLASH_CS_LOW()   HAL_GPIO_WritePin(NORFLASH_SPI_CS_GPIO_PORT, NORFLASH_SPI_CS_PIN, GPIO_PIN_RESET)
#else
#define SPI_FLASH_CS_HIGH()
#define SPI_FLASH_CS_LOW()
#endif

void SpiFlashInit(void);
void SpiFlashErase(uint32_t u32SartAddr, uint32_t u32NumBytes);
void SpiFlashBufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SpiFlashBufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
int32_t SpiFlashBufferVerify(uint8_t* pBuffer, uint32_t u32VerfyAddr, uint16_t u16NumByteToVerfy);
void NorFlash_Test(void);
uint8_t EmuInitial(void);

#endif

