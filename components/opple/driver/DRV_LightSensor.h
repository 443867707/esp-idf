#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define DEFAULT_VREF    944  // 1100 -->  944 = 3400 mv /3.6     //internel default vref
#define NO_OF_SAMPLES   64          //Multisampling

uint8_t BspLightSensorInit(void);
uint8_t BspLightSensorVoltageGet(uint32_t *pVoltage);
uint8_t BspLightSensorLuxGet(uint32_t *pLux);
uint8_t LightSensorParamSet(uint16_t u16Voltage);
uint8_t LightSensorParamGet(uint16_t *pLiSenVol);
uint8_t DelLightSensorParam(void);
