#ifndef _DRV_PWM_H
#define _DRV_PWM_H

#include "freertos/FreeRTOS.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO       (22)
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0

uint8_t BspInitPwm(void);
uint8_t BspSetDuty(uint32_t u32DutyValue);

#endif
