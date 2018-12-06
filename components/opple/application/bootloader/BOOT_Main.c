
/**********************************************************************************************
                                             Includes
***********************************************************************************************/
#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal.h"
#include "BOOT_Main.h"
#include "BOOT_Typedef.h"
#include "DEV_MemoryMmt.h"
#include "LIB_CRC32.h"
#include "DRV_Flash.h"
#include "DRV_ExFlash.h"

/**********************************************************************************************
                                             Defines
***********************************************************************************************/
/*
#define FLASH_BOOT_OFFSET         0
#define FLASH_BOOT_PARA1_OFFSET   0x4000
#define FLASH_BOOT_PARA2_OFFSET   0x8000
#define FLASH_APP_OFFSET          0xC000
#define FLASH_APP2_OFFSET         0x10000
*/

#define FLASH_ADDR_BASE           0x08000000
#define FLASH_ADDR_END            0x801FFFF
#define FLASH_OFFSET              0x20000

#define FLASH_RESULT_CHECK(para) if((para)!=0)\
			                           {\
				                          debug_err_line=__LINE__;\
				                          return;\
			                           }
																 
#define FLASH_ERR_CHECK(para) if((para)!=0)\
			                           {\
				                          debug_err_line=__LINE__;\
				                          return BEC_FAIL;\
			                           }

/**********************************************************************************************
                                             Typedefs
***********************************************************************************************/										 
typedef enum
{
    BOOT_NONE=0,
    BOOT_CP21_BOOT_AP1,
    BOOT_APP1,
    BOOT_APP2,
}t_boot_type;

typedef enum
{
    BOOT_FAIL_KEEP=0,
    BOOT_FAIL_BOOT_ANOTHER_AREA,
}t_boot_fail_option;

typedef enum
{
	BOOT_IMAGE_WITHOUT_CHECK=0,
	BOOT_IMAGE_WITH_CHECK,
}t_boot_option;

typedef enum
{
	FLASH_POS_INNER=0,
	FLASH_POS_EXTERNAL,
}t_flash_postion;

typedef struct
{
    unsigned int flash_position;
	  unsigned int boot_option;
	  unsigned int area_start;
    unsigned int area_size;
    unsigned int run_start;
    unsigned int checksum;
}t_app_info;

typedef void (*pboot)(void);

/**********************************************************************************************
                                             Data Structs
***********************************************************************************************/
typedef struct
{
    unsigned int bootType;
    unsigned int failOption;
    t_app_info app1;
    t_app_info app2;
    unsigned int crc;
}t_boot_para;

static volatile unsigned int debug_err_line=0;

/**********************************************************************************************
                                             Utilities
***********************************************************************************************/
unsigned int byteArrayCompare(unsigned char* p1,unsigned char* p2,unsigned char len)
{
    unsigned char i;

    for(i = 0;i < len;i++)
    {
        if(p1[i] != p2[i])
        {
            return 1;
        }
    }
    return 0; // The same
}

/**********************************************************************************************
                                             Implementations
***********************************************************************************************/
t_boot_err_code checkBootPara(t_app_info* app)
{
	if(app->run_start < app->area_start)
	{
		return BEC_FAIL;
	}
	else if(app->flash_position != FLASH_POS_INNER)
	{
		return BEC_FAIL;
	}
	else if(app->flash_position == FLASH_POS_INNER)
	{
			if(app->area_start < (memory_mmt[MB_BOOTLOADER].addr+memory_mmt[MB_BOOTLOADER].size))
			{
				return BEC_FAIL;
			}
			else if(app->area_start < (memory_mmt[MB_BOOT_PARA1].addr+memory_mmt[MB_BOOT_PARA1].size))
			{
				return BEC_FAIL;
			}
			else if(app->area_start < (memory_mmt[MB_BOOT_PARA2].addr+memory_mmt[MB_BOOT_PARA2].size))
			{
				return BEC_FAIL;
			}
		  else
			{
				return BEC_SUCCESS;
			}
	}
	else
	{
			return BEC_SUCCESS;
	}
	
	
}

t_boot_err_code copy_app2_to_app1(t_boot_para* para)
{
    t_flash_err_code err;
	  t_exflash_err_code err_2;
	  static unsigned char buf[1024];
	  unsigned int i,k,addr_2=0,addr=0;
	  
	  // Protect Bootloader,BootPara1,BootPara2 Area
	  if(para->app1.flash_position == FLASH_POS_INNER)
		{
				if(para->app1.area_start < (memory_mmt[MB_BOOTLOADER].addr+memory_mmt[MB_BOOTLOADER].size))
				{
					return BEC_FAIL;
				}
				if(para->app1.area_start < (memory_mmt[MB_BOOT_PARA1].addr+memory_mmt[MB_BOOT_PARA1].size))
				{
					return BEC_FAIL;
				}
				if(para->app1.area_start < (memory_mmt[MB_BOOT_PARA2].addr+memory_mmt[MB_BOOT_PARA2].size))
				{
					return BEC_FAIL;
				}
	  }
	  // check
	  if((para->app1.area_start+para->app2.area_size) > FLASH_ADDR_END) return BEC_FAIL;
	  
	  // erase APP1
	  err = flash_erase(para->app1.area_start,para->app2.area_size);
	  FLASH_ERR_CHECK(err);
	  
		k = (para->app2.area_size) >> 10;
		for(i = 0;i < k;i++)
		{
			// read APP2 
			addr_2 = para->app2.area_start + (i<<10);
			if(para->app2.flash_position == FLASH_POS_EXTERNAL)
			{
				err_2 = exflash_read(addr_2,buf,1024);
				FLASH_ERR_CHECK(err_2);
			}
			else
			{
				err = flash_read(addr_2,buf,1024);
				FLASH_ERR_CHECK(err);
			}
			
			
			// program APP1
			addr = para->app1.area_start + (i<<10);
			err = flash_program(addr,buf,1024);
			FLASH_ERR_CHECK(err);
		
			// verity APP1
			err = flash_verify(addr,buf,1024);
			FLASH_ERR_CHECK(err);
	  }
		return BEC_SUCCESS;
}

t_boot_err_code boot_app(t_app_info* app)
{
	unsigned int crc;
	
	// check image
	if(app->boot_option == BOOT_IMAGE_WITH_CHECK)
	{
		crc = getCrc32((unsigned char*)app->area_start,app->area_size);
		if(crc != app->checksum){
			return BEC_BOOT_FAIL;
		}
	}

	// check
	if(checkBootPara(app) != BEC_SUCCESS)
	{
			return BEC_BOOT_FAIL;
	}
	
	// boot
	(*(pboot*)app->run_start)();
	return BEC_SUCCESS;
}

void system_boot_app(t_app_info* app,t_app_info* app_back,unsigned int failOption)
{
	t_boot_err_code err;
	
	err = boot_app(app);
	if(err != BEC_SUCCESS)
	{
		if(failOption == BOOT_FAIL_BOOT_ANOTHER_AREA)
		{
			err = boot_app(app_back);
			if(err != BEC_SUCCESS){return;}
		}
		else{return;}
	}
	else{return;}
}

void initBoot(void)
{
    static t_boot_para para1,para2;
    static unsigned char valid_p1=0,valid_p2=0;
    static unsigned int crc_p1,crc_p2;
    // pboot boot;
    t_flash_err_code err;
	  t_boot_err_code boot_err;
    /********************************************************************************************
                     Boot Para Reparing
    *********************************************************************************************/
    err = flash_read(memory_mmt[MB_BOOT_PARA1].addr,(unsigned char*)&para1,sizeof(t_boot_para)); // endlian
    FLASH_RESULT_CHECK(err);
    err = flash_read(memory_mmt[MB_BOOT_PARA2].addr,(unsigned char*)&para2,sizeof(t_boot_para));
    FLASH_RESULT_CHECK(err);
    
    crc_p1 = getCrc32((unsigned char*)&para1,sizeof(t_boot_para)-4);
    crc_p2 = getCrc32((unsigned char*)&para2,sizeof(t_boot_para)-4);

    valid_p1 = 0;
		if(crc_p1 == para1.crc)
    {
					valid_p1 = 1;
    }
		valid_p2 = 0;
    if(crc_p2 == para2.crc)
    {
        valid_p2 = 1;
    }

    if(valid_p1 != 0)
    {
        if(valid_p2 == 0)
        {
            err = flash_write(memory_mmt[MB_BOOT_PARA2].addr,(unsigned char*)&para1,sizeof(t_boot_para));
            FLASH_RESULT_CHECK(err);
        }
        else
        {
            // Both valid
            // if para1!=para2,then copy para1 to para2
            if(byteArrayCompare((unsigned char*)&para1,(unsigned char*)&para2,sizeof(t_boot_para)) != 0)
            {
                err = flash_write(memory_mmt[MB_BOOT_PARA2].addr,(unsigned char*)&para1,sizeof(t_boot_para));
							  FLASH_RESULT_CHECK(err);
            }
        }
    }
    else
    {
        if(valid_p2 != 0)
        {
            err = flash_write(memory_mmt[MB_BOOT_PARA1].addr,(unsigned char*)&para2,sizeof(t_boot_para));
					  FLASH_RESULT_CHECK(err);
        }
        else
        {
            // Both invalid then boot APP by default
            //return;
					  para1.bootType = BOOT_APP1;
					  para1.failOption = BOOT_FAIL_KEEP;
					  para1.app1.area_start = memory_mmt[MB_APP].addr;
					  para1.app1.run_start = para1.app1.area_start+4;
					  para1.app1.boot_option = BOOT_IMAGE_WITHOUT_CHECK;
					  para1.app1.flash_position = memory_mmt[MB_APP].pos;
        }
    }

    /*******************************************************************************************
                                                Boot
    ********************************************************************************************/
    if(para1.bootType == BOOT_NONE)
    {
        return;
    }
    else if(para1.bootType == BOOT_CP21_BOOT_AP1)
    {
        // copy app2 to app1 and boot app1
        boot_err =copy_app2_to_app1(&para1);
			  if(boot_err == BEC_SUCCESS)
				{
					para1.bootType = BOOT_APP1;
					para1.app1.area_size = para1.app2.area_size;
					para1.app1.checksum = para1.app2.checksum;
					para1.crc = getCrc32((unsigned char*)&para1,sizeof(t_boot_para)-4);
					err = flash_write(memory_mmt[MB_BOOT_PARA1].addr,(unsigned char*)&para1,sizeof(t_boot_para));
	        FLASH_RESULT_CHECK(err);
					
					system_boot_app(&para1.app1,&para1.app2,para1.failOption);
				}
				else{return;}
    }
    else if(para1.bootType == BOOT_APP1)
    {
        system_boot_app(&para1.app1,&para1.app2,para1.failOption);
    }
    else if(para1.bootType == BOOT_APP2)
    {
        system_boot_app(&para1.app2,&para1.app1,para1.failOption);
    }
    else
    {
        return;
    }

}

void loopBoot(void)
{
    // restart;
    // delay_ms(1000);
    //NVIC_SystemReset();
}

void initLed(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	//RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	GPIO_InitStructure.Pin = GPIO_PIN_14|GPIO_PIN_15;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure); 
	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_RESET);
	
}

void ota_test_init(void)
{
	t_boot_para para1,para2;
	t_flash_err_code err;
	
  para1.bootType = BOOT_APP1;//BOOT_CP21_BOOT_AP1,BOOT_APP1,BOOT_APP2;
	para1.failOption = BOOT_FAIL_KEEP;
	
	para1.app2.area_start = memory_mmt[MB_APP_OTA].addr;
	para1.app2.area_size = memory_mmt[MB_APP_OTA].size;
	para1.app2.run_start = para1.app2.area_start+4;
	para1.app2.boot_option = BOOT_IMAGE_WITHOUT_CHECK;
	para1.app2.flash_position = memory_mmt[MB_APP_OTA].pos;

	para1.app1.area_start = memory_mmt[MB_APP].addr;
	para1.app1.run_start = para1.app1.area_start+4;
	para1.app1.area_size = memory_mmt[MB_APP].size;
	para1.app1.boot_option = BOOT_IMAGE_WITHOUT_CHECK;
	para1.app1.flash_position = memory_mmt[MB_APP].pos;
	
	para1.crc = getCrc32((unsigned char*)&para1,sizeof(t_boot_para)-4);
	para2.crc = getCrc32((unsigned char*)&para2,sizeof(t_boot_para)-4);
	err = flash_write(memory_mmt[MB_BOOT_PARA1].addr,(unsigned char*)&para1,sizeof(t_boot_para));
	FLASH_RESULT_CHECK(err);
	err = flash_write(memory_mmt[MB_BOOT_PARA2].addr,(unsigned char*)&para2,sizeof(t_boot_para));
	FLASH_RESULT_CHECK(err);
}

int main(void)
{
    __disable_irq();
	  HAL_Init();
	
	  init_crc32();
	  memory_init();
	
	  //initLed();
	  //ota_test_init();
	  
    initBoot();                
	  
	  //flash_erase(0x0800c000,0x4000);
	  //(*(pboot*)0x0800c004)();
	  //(*(pboot*)0x08010004)();
	
    //__enable_irq();
    while(1)
    {
        //loopBoot();
    }
		
		return 0;
}
