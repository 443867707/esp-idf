#ifndef _DRV_WATCHDOG_H
#define _DRV_WATCHDOG_H

#define WATCHDOG_GPIO_PIN        GPIO_PIN_0
#define WATCHDOG_GPIO_PIN_PORT   GPIOA

void WathdogInit(void);
void Watchdog_trigger(void);

#endif
