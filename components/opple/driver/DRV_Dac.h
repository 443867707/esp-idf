#ifndef _DRV_DAC_H
#define _DRV_DAC_H

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "esp_system.h"

#define DIM_DAC_CHANNEL    1 

uint8_t BspDacDisable(void);
uint8_t BspDacEnable(void);
uint8_t BspDacLevelSet(uint8_t u8Level);

#endif
