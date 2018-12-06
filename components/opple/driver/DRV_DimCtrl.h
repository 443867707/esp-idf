#ifndef _DRV_DIM_CTRL_H
#define _DRV_DIM_CTRL_H

#include "DRV_Dac.h"
#include "DRV_Pwm.h"
#include "DRV_Gpio.h"

#define GPIO_DIM_RELAY_PIN 26 
#define GPIO_DIM_SWITCH_PIN 32 
#define GPIO_DIM_RELAY_PIN_SEL  (1ULL<<GPIO_DIM_RELAY_PIN)
#define GPIO_DIM_SWITCH_PIN_SEL  (1ULL<<GPIO_DIM_SWITCH_PIN)

/* 
 * level = ((u32DacVol * 3 / 10) -b ) / k 
 * b = (9 * u1 - 3 * u2) / 20
 * k =  3 * (u2 -u1) / 1280
 * level = ()
 * u2: level 192  输出的电压
 * u1: level 64  输出的电压
 */

#define CALC_B(u1, u2)  (double)((9 * u1 - 3 * u2) / 20)
#define CALC_K(u1, u2)  (double)(3 * (u2 - u1) / 1280)
#define CALC_LEVEL(volOutput, b, k)  ((((double)((volOutput * 3) / 10) - b) / k))

#define DAC_RAW_MIN_LEVEL 0
#define DAC_RAW_MAX_LEVEL 255 
#define DAC_MAX_LEVEL 10000 

#define PWM_RAW_MAX_LEVEL 8192
#define PWM_MAX_LEVEL     10000


uint8_t DimDacLevelSet(uint16_t u16Level);
uint8_t DimDacRawLevelSet(uint8_t u8Level);
uint8_t DimPwmDutyCycleSet(uint32_t u8DutyCyle);

uint8_t DacLevel64ParamSet(uint16_t u16DacOutput);
uint8_t DacLevel192ParamSet(uint16_t u16DacOutput);
uint8_t DacLevel64ParamGet(uint16_t *pDacLevelVol);
uint8_t DacLevel192ParamGet(uint16_t *pDacLevelVol);
uint8_t delDacCalParam(void);

uint8_t DimRelayOff(void);
uint8_t DimRelayOn(void);
uint8_t DimCtrlSwitchToPwm(void);
uint8_t DimCtrlSwitchToDac(void);
uint8_t DimCtrlInit(void);
#endif
