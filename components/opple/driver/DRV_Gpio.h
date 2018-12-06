#ifndef _DRV_DPIO_H
#define _DRV_DPIO_H

#include "driver/gpio.h"

uint8_t BspGpioSetLevel(uint8_t gpio_pin, uint8_t value);
uint8_t BspGpioConf(gpio_config_t *io_conf);

#endif
