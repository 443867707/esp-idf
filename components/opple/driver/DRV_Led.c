//#include "stm32f4xx_hal.h"
#include "Includes.h"
#include "DRV_Led.h"
#include "DRV_Bc28.h"

U8 g_ucNbLedStatus = 0;
/**
  * @brief  打开系统灯    
  * @param  无     
  * @retval 无
  */
void System_Led_On()
{
	  //HAL_GPIO_WritePin(SYSTEM_LED_GPIO_PIN_PORT, SYSTEM_LED_GPIO_PIN, GPIO_PIN_RESET); 
}

/**
  * @brief  关闭系统灯    
  * @param  无     
  * @retval 无
  */
void System_Led_Off()
{
	  //HAL_GPIO_WritePin(SYSTEM_LED_GPIO_PIN_PORT, SYSTEM_LED_GPIO_PIN, GPIO_PIN_SET); 
}

/**
  * @brief  系统灯初始化   
  * @param  无
  * @retval 无
  */
void System_Led_Init()
{
#if 0
  GPIO_InitTypeDef GPIO_InitStruct;
  __HAL_RCC_GPIOB_CLK_ENABLE();
  /* Configure the GPIO_LED pin */
  GPIO_InitStruct.Pin = SYSTEM_LED_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(SYSTEM_LED_GPIO_PIN_PORT, &GPIO_InitStruct);
#endif
}	

/**
  * @brief  NB 状态灯关闭   
  * @param  无
  * @retval 无
  */
uint8_t NB_Status_Led_Off()
{
    uint8_t u8Ret;

    u8Ret = BspGpioSetLevel(GPIO_NB_LED_PIN, 1);
    
    if (u8Ret) {
        return -1;
    }

	g_ucNbLedStatus = 0;
    return 0;
}

/**
  * @brief  NB 状态灯开启  
  * @param  无
  * @retval 无
  */
uint8_t NB_Status_Led_On()
{
    uint8_t u8Ret;
    
    u8Ret = BspGpioSetLevel(GPIO_NB_LED_PIN, 0);
    if (u8Ret) {
        return 1;
    }

	g_ucNbLedStatus = 1;
    return 0;
}

/**
  * @brief  NB 状态灯初始化
  * @param  无
  * @retval 无
  */
uint8_t NB_Status_Led_Init()
{
    uint8_t u8Ret;
    esp_err_t err;

    gpio_config_t gpio_conf;
    gpio_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask = GPIO_NB_LED_PIN_SEL;
    gpio_conf.pull_down_en = 0;
    gpio_conf.pull_up_en = 0;

    err = gpio_config(&gpio_conf);
    if (err != ESP_OK) {
        return 1;
    }

    u8Ret = BspGpioSetLevel(GPIO_NB_LED_PIN, 1);
    if (u8Ret) {
        return 1;
    }

	u8Ret = NB_Status_Led_On();
    if (u8Ret) {
        return 1;    
    }

    return 0;
}	

void LedLoop(void)
{
	static U32 tick = 0, tick1 = 0;
	static U8 flash = 0;
	static U8 lastNbState = BC28_UNKNOW;
	static U8 times = 0;

	if(lastNbState != NeulBc28GetDevState()){
		if(BC28_INIT == NeulBc28GetDevState()){
			flash = 2;
		}
		else if(BC28_CGA == NeulBc28GetDevState()){
			flash = 3;
		}	
		else if(BC28_GIP == NeulBc28GetDevState()){
			flash = 4;
		}	
		else if(BC28_READY == NeulBc28GetDevState()){
			flash = 5;
		}
		else if(BC28_ERR == NeulBc28GetDevState()){
			flash = 6;
		}
		else if(BC28_UPDATE == NeulBc28GetDevState()){
			flash = 7;
		}		
		else if(BC28_TEST == NeulBc28GetDevState()){
			flash = 1;		
		}

		lastNbState = NeulBc28GetDevState();		
		tick = OppTickCountGet();
	}
	
	if((OppTickCountGet() - tick) > 10000){		
		if((OppTickCountGet() - tick1) > 500){
			if(g_ucNbLedStatus){
				NB_Status_Led_Off();
			}else{
				NB_Status_Led_On();
			}
			tick1 = OppTickCountGet();
			times++;
		}
		if(times == flash*2){
			tick1 = tick = OppTickCountGet();
			times = 0;
		}
	}

}
