#include "driver/gpio.h"

uint8_t BspGpioSetLevel(uint8_t u8GpioPin, uint8_t u8Value)
{
    esp_err_t err;

    err = gpio_set_level(u8GpioPin, u8Value);
    if (err != ESP_OK) {
        return 1;
    }
    return 0;
}

uint8_t BspGpioConf(gpio_config_t *io_conf)
{
    esp_err_t err;

    err = gpio_config(io_conf);
    if (err != ESP_OK) {
        return 1;
    }
    return 0;
}

