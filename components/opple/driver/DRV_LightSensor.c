#include "DRV_LightSensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <OPP_Debug.h>
#include "nvs.h"


static esp_adc_cal_characteristics_t *pLightChars;

/*adc channel the light sensor used*/
static const adc_channel_t light_channel = ADC_CHANNEL_6;

/*The input voltage of ADC will be reduced to about 1/3.6 */
static const adc_atten_t atten = ADC_ATTEN_DB_11;

/*light sensor use adc1*/
static const adc_unit_t unit = ADC_UNIT_1;

/*mutex lock to used to operate light sensor*/
SemaphoreHandle_t g_LightSensorMutex = NULL;

/*light sensor status*/
typedef enum _LightSensorStatus {
    LIGHT_SENSOR_NO_INIT,
    LIGHT_SENSOR_RUNNING,
} LightSensorStatus_t;

static LightSensorStatus_t g_LightSensorStatus = LIGHT_SENSOR_NO_INIT;

/*
 *@breif light sensor init
 *@param none
 *@return  1: success, 0: fail
*/
uint8_t BspLightSensorInit()
{
    g_LightSensorMutex = xSemaphoreCreateMutex();
    if (g_LightSensorMutex == NULL) {
        DEBUG_LOG(DEBUG_MODULE_LIGHTSENSOR, DLL_ERROR, "create LightSenosrMutex fail \n");
        return 1;
    }

    xSemaphoreTake(g_LightSensorMutex, portMAX_DELAY);
    
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(light_channel, atten);

    pLightChars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    if (pLightChars == NULL) {
        return 1;
    }

    esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, pLightChars);
    g_LightSensorStatus = LIGHT_SENSOR_RUNNING;

    xSemaphoreGive(g_LightSensorMutex);

    return 0;
}

/*
 *@brief get the voltage of light sensor
 *@pVoltage the point of buffer that saved the voltage
 *@return 0: success, 1: fail 
*/
uint8_t BspLightSensorVoltageGet(uint32_t *pVoltage)
{
    if (pVoltage == NULL) {
        DEBUG_LOG(DEBUG_MODULE_LIGHTSENSOR, DLL_ERROR, "pVoltage is NULL point \n");
        return 1;
    }

    if (g_LightSensorStatus != LIGHT_SENSOR_RUNNING) {
        DEBUG_LOG(DEBUG_MODULE_LIGHTSENSOR, DLL_ERROR, "light sensor is not running \n");
        return 1;
    }

    xSemaphoreTake(g_LightSensorMutex, portMAX_DELAY);

    //Multisampling
    uint32_t u32AdcReading = 0;
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        u32AdcReading += adc1_get_raw((adc1_channel_t)light_channel);
    }
    u32AdcReading /= NO_OF_SAMPLES;
    //Convert adc_reading to voltage in mV
    *pVoltage = esp_adc_cal_raw_to_voltage(u32AdcReading, pLightChars);
    DEBUG_LOG(DEBUG_MODULE_LIGHTSENSOR, DLL_INFO, "Raw: %u\t light sensor Voltage: %umV\n", u32AdcReading, *pVoltage);
    xSemaphoreGive(g_LightSensorMutex);

    return 0;
}

/*
 *@breif get lux of light sensor's sample
 *@param the point of data buffer that saved pLux
 *@return  1: success, 0: fail
*/
uint8_t BspLightSensorLuxGet(uint32_t *pLux)
{
    if (pLux == NULL) {
        return 1;
    }

    if (g_LightSensorStatus != LIGHT_SENSOR_RUNNING) {
        return 1;
    }

    xSemaphoreTake(g_LightSensorMutex, portMAX_DELAY);

    uint32_t u32AdcReading = 0, u32Voltage = 0;
    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        u32AdcReading += adc1_get_raw((adc1_channel_t)light_channel);
    }
    u32AdcReading /= NO_OF_SAMPLES;
    //Convert adc_reading to voltage in mV
    u32Voltage = esp_adc_cal_raw_to_voltage(u32AdcReading, pLightChars);

    if (u32Voltage < 231) {
        *pLux = u32Voltage * 15 / 231; // 231 mv -->15 lux
    } else if (u32Voltage >= 231 && u32Voltage < 281) {
        *pLux = 15 + (u32Voltage - 231) * (25 - 15) / (281 -231); // 231 mv -->15 lux
    } else if  (u32Voltage >= 281 && u32Voltage < 1068) {
        *pLux = 25 + (u32Voltage - 281) * (50 - 25) / (1068 -281); // 281 mv -->25 lux
    } else if  (u32Voltage >= 1068 && u32Voltage < 1258) {
        *pLux = 50 + (u32Voltage - 1068) * (100 - 50) / (1258 -1068); // 1068 mv -->50 lux
    } else if  (u32Voltage >= 1258 && u32Voltage < 1934) {
        *pLux = 100 + (u32Voltage - 1256) * (200 - 100) / (1934 - 1258); // 1258 mv -->100 lux
    } else if  (u32Voltage >= 1934 && u32Voltage < 3115) {
        *pLux = 200 + (u32Voltage - 1934) * (500 - 200) / (3115 -1934); // 1934 mv -->200 lux
    } else if  (u32Voltage >= 3115) {
        *pLux = 500; // 3115 mv -->500 lux, to be confirmed
    }

    DEBUG_LOG(DEBUG_MODULE_LIGHTSENSOR, DLL_INFO, "Raw: %u\t light sensor Voltage: %u mV \t, lux: %u \n", u32AdcReading, u32Voltage, *pLux);

    xSemaphoreGive(g_LightSensorMutex);

    return 0;
}

/*
 *@brief  写光敏校准参数
 *@param  u16Voltage: 开关灯电压临界值 
 *@return 0 获取成功，1 获取失败
*/
uint8_t LightSensorParamSet(uint16_t u16Voltage)
{

    nvs_handle my_handle;
    esp_err_t err;

    xSemaphoreTake(g_LightSensorMutex, portMAX_DELAY);

    err = nvs_open("LiSenParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        xSemaphoreGive(g_LightSensorMutex);
        return 1;
    }

    err = nvs_set_blob(my_handle, "LiSenVol", (uint8_t *)&u16Voltage, 2);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        xSemaphoreGive(g_LightSensorMutex);

        return 1;
    }
    
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        xSemaphoreGive(g_LightSensorMutex);

        return 1;
    }

    nvs_close(my_handle);
    xSemaphoreGive(g_LightSensorMutex);

    return 0;
}

/*
 *@brief  获取light sensor校准参数
 *@param  pLiSenVol: 存储获取值的指针
 *@return 0 获取成功，1 获取失败
*/
uint8_t LightSensorParamGet(uint16_t *pLiSenVol)
{
    int ret = 0;
    size_t len = 2;

    nvs_handle my_handle;
    esp_err_t err;

    if (pLiSenVol == NULL) {
        return 1;
    }

    xSemaphoreTake(g_LightSensorMutex, portMAX_DELAY);
    err = nvs_open("LiSenParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        xSemaphoreGive(g_LightSensorMutex);
        return 1;
    }

    err = nvs_get_blob(my_handle, "LiSenVol",  (uint8_t *)pLiSenVol, &len);
    if (err == ESP_OK) {
        DEBUG_LOG(DEBUG_MODULE_LIGHTSENSOR, DLL_ERROR, "light sensor parma : %u \n", *pLiSenVol);
    } else {
        DEBUG_LOG(DEBUG_MODULE_LIGHTSENSOR, DLL_ERROR, "read light sensor param err \n");
        xSemaphoreGive(g_LightSensorMutex);
        return 1;
    }

    nvs_close(my_handle);
    xSemaphoreGive(g_LightSensorMutex);

    return 0;
}

uint8_t DelLightSensorParam()
{
    esp_err_t err;

    nvs_handle my_handle;

    xSemaphoreTake(g_LightSensorMutex, portMAX_DELAY);
    err = nvs_open("LiSenParam", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        xSemaphoreGive(g_LightSensorMutex);
        return 1;
    }

    err = nvs_erase_key(my_handle, "LiSenVol");
    if (err != ESP_OK) {
        nvs_close(my_handle);
        xSemaphoreGive(g_LightSensorMutex);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            return 0;
        }
        return 1;
    }

    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        xSemaphoreGive(g_LightSensorMutex);
        return 1;
    }
    
    nvs_close(my_handle);
    xSemaphoreGive(g_LightSensorMutex);

    return 0;
}

