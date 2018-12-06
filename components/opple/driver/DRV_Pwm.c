#include "DRV_Pwm.h"
#include "OPP_Debug.h"
#include "esp_err.h"

/*pwm 频率 2k, 精度13位*/
ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        //.duty_resolution = 8,
        .freq_hz = 2000,                
        .speed_mode = LEDC_HS_MODE, 
        .timer_num = LEDC_HS_TIMER  
};

ledc_channel_config_t ledc_channel = {
           .channel    = LEDC_HS_CH0_CHANNEL,
            .duty       = 0,
            .gpio_num   = LEDC_HS_CH0_GPIO,
            .speed_mode = LEDC_HS_MODE,
            .timer_sel  = LEDC_HS_TIMER
};

/*
 *@brief PWM 功能初始化
 *@param none 
 *@return 0: success, 1: fail
 */
uint8_t BspInitPwm()
{
    esp_err_t err;
    err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        return 1;
    }

    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        return 1;
    }
    return 0;
}

/*
 *@brief PWM 占空比设置
 *@param u32DutyValue, 占空比的level 值， 0～255  
 *@return 0: success, 1: fail
 */
uint8_t BspSetDuty(uint32_t u32DutyValue)
{
    esp_err_t err;
	DEBUG_LOG(DEBUG_MODULE_PWM, DLL_INFO, "%d\r\n", u32DutyValue);
    err = ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, u32DutyValue);
    if (err != ESP_OK) {
        return 1;
    }

    err = ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
    if (err != ESP_OK) {
        return 1;
    }
    return 0;
}
