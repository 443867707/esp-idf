#include "DRV_Dac.h"
#include "esp_err.h"

uint8_t BspDacDisable()
{
    esp_err_t err;
    err = dac_output_disable(DIM_DAC_CHANNEL);
    if (err != ESP_OK) {
        return 1;
    }
    return 0;
}

uint8_t BspDacEnable()
{
    esp_err_t err;
    err = dac_output_enable(DIM_DAC_CHANNEL);
    if (err != ESP_OK) {
        return 1;
    }
    return 0;
}

uint8_t BspDacLevelSet(uint8_t u8Level)
{
    esp_err_t err;
    err = dac_output_voltage(DIM_DAC_CHANNEL, u8Level);
    if (err != ESP_OK) {
        return 1;
    }
    return 0;
}
