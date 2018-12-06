#ifndef _DRV_LED_H
#define _DRV_LED_H
#include "DRV_Gpio.h"

#define GPIO_NB_LED_PIN 27 
#define GPIO_NB_LED_PIN_SEL (1ULL<<GPIO_NB_LED_PIN)

void System_Led_On(void);
void System_Led_Off(void);
void System_Led_Init(void);

uint8_t NB_Status_Led_On(void);
uint8_t NB_Status_Led_Off(void);
uint8_t NB_Status_Led_Init(void);
void LedLoop(void);

#endif
