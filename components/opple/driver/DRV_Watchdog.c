//#include "stm32f4xx_hal.h"
#include "Includes.h"
#include "DRV_Watchdog.h"

/**
  * @brief  翻转GPIO喂狗    
  * @param  无        
  * @retval 无
  */
void Watchdog_trigger(void)
{
  //HAL_GPIO_TogglePin(WATCHDOG_GPIO_PIN_PORT, WATCHDOG_GPIO_PIN); 
}

/**
  * @brief  看门狗GPIO初始化   
  * @param  无
  * @retval 无
  */
static void WatchdogGpioInit(void)
{
#if 0
  GPIO_InitTypeDef GPIO_InitStruct;
  __HAL_RCC_GPIOA_CLK_ENABLE();
  /* Configure the GPIO_LED pin */
  GPIO_InitStruct.Pin = WATCHDOG_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(WATCHDOG_GPIO_PIN_PORT, &GPIO_InitStruct);
#endif
}	

/**
  * @brief  看门狗功能初始化  
  * @param  无
  * @retval 无
  */
void WathdogInit(void)
{ 
	WatchdogGpioInit();
}

