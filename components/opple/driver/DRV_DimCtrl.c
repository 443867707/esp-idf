#include "DRV_DimCtrl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include "freertos/semphr.h" 
#include "OPP_Debug.h"
#include "esp_err.h"
#include "nvs.h"
#include "OS.h"

static uint16_t s_u16Level64Vol = 2960;
static uint16_t s_u16Level192Vol = 8250;

uint16_t g_DacLevel = 0;
uint16_t g_PwmLevel = 0;
uint8_t  g_RelayGpioStatus = 0;
uint8_t  g_DimTypeStatus = 0;

/*mutex lock to used to operate dim func*/
//SemaphoreHandle_t g_DimMutex = NULL; 
T_MUTEX g_DimMutex;

typedef enum _DimStatus {
    DIM_INIT_NONE,
    DIM_INIT_SUCCESS,
    DIM_INIT_FAIL
} DimStatus_t; 


DimStatus_t g_DimStatus = DIM_INIT_NONE;

/*
 *@breif 继电器控制gpio初始化
 *@param none
 *@return 0: 成功， 1：失败
 */
static uint8_t DimRelayGpioInit() 
{
    esp_err_t err;

    gpio_config_t gpio_conf;
    gpio_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask = GPIO_DIM_RELAY_PIN_SEL;
    gpio_conf.pull_down_en = 0;
    gpio_conf.pull_up_en = 0;

    err = gpio_config(&gpio_conf);
    if (err != ESP_OK) {
        return 1;
    }
    return 0;
}

/*
 *@breif 0-10V 和 pwm 调光二选一控制gpio初始化
 *@param none
 *@return 0: 成功， 1：失败
 */
static uint8_t DimSwitchGpioInit() 
{
    esp_err_t err;

    gpio_config_t gpio_conf;
    gpio_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask = GPIO_DIM_SWITCH_PIN_SEL;
    gpio_conf.pull_down_en = 0;
    gpio_conf.pull_up_en = 0;

    err = gpio_config(&gpio_conf);
    if (err != ESP_OK) {
        return 1;
    }
    return 0;
}

/*
 *@breif 继电器关闭 
 *@param none
 *@return 0: 成功， 1：失败
 */
uint8_t DimRelayOff()
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);
    if (g_DimStatus != DIM_INIT_SUCCESS) {
        //xSemaphoreGive(g_DimMutex);
        MUTEX_UNLOCK(g_DimMutex);
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "take mutex fail %s:%d \n", __func__, __LINE__);
        return 1;
    }

    u8Ret = BspGpioSetLevel(GPIO_DIM_RELAY_PIN, 1);
    if (u8Ret == 0) {
        g_RelayGpioStatus = 0;
    }

    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);
    return u8Ret;
}

/*
 *@breif 继电器开启
 *@param none
 *@return 0: 成功， 1：失败
 */
uint8_t DimRelayOn()
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);
    if (g_DimStatus != DIM_INIT_SUCCESS) {
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "take mutex fail %s:%d \n", __func__, __LINE__);
        return 1;
    }

    u8Ret = BspGpioSetLevel(GPIO_DIM_RELAY_PIN, 0);
    if (u8Ret == 0) {
        g_RelayGpioStatus = 1;
    }

    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);
    return u8Ret;
}

/*
 *@breif 切换到pwm 调光 
 *@param none
 *@return 0: 成功， 1：失败
 */
uint8_t DimCtrlSwitchToPwm()
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);
    if (g_DimStatus != DIM_INIT_SUCCESS) {
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "take mutex fail %s:%d \n", __func__, __LINE__);
        return 1;
    }

    u8Ret = BspGpioSetLevel(GPIO_DIM_SWITCH_PIN, 1);
    if (u8Ret == 0) {
        g_DimTypeStatus = 1;
    }
    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);
    return u8Ret;
}

/*
 *@breif 切换到DAC 调光 
 *@param none
 *@return 0: 成功， 1：失败
 */
uint8_t DimCtrlSwitchToDac()
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);
    if (g_DimStatus != DIM_INIT_SUCCESS) {
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "take mutex fail %s:%d \n", __func__, __LINE__);
        return 1;
    }

    u8Ret = BspGpioSetLevel(GPIO_DIM_SWITCH_PIN, 0);
    if (u8Ret == 0) {
        g_DimTypeStatus = 0;
    }

    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);
    return u8Ret;
}

/*
 *@breif pwm 占空比设置 
 *@param u32Level: 占空比level, 取值 0～10000, 0 ：%0, 10000： 100% 
 *@return 0: 成功， 1：失败
 */
uint8_t DimPwmDutyCycleSet(uint32_t u32Level)
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);
    /*level 0 ~ 10000() 转换成硬件实际的level 0~8192 */

    u32Level = u32Level * PWM_RAW_MAX_LEVEL / PWM_MAX_LEVEL;

    if (g_DimStatus != DIM_INIT_SUCCESS) {
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);        
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "take mutex fail %s:%d \n", __func__, __LINE__);
        return 1;
    }

    u8Ret = BspSetDuty(u32Level);
    if (u8Ret == 0) {
        g_PwmLevel = u32Level;
    }

    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);    
    return u8Ret;
}

/*
 *@breif 0~10 输出电压设置 
 *@param u8Level: 输出电压level, 取值 0～255, 0 ：0v, 255： 10v 
 *@return 0: 成功， 1：失败
 */

uint8_t DimDacLevelSet(uint16_t u16Level)
{
    uint8_t u8Ret;
    uint8_t u8RawLevel;

    double f64B, f64K, f64DacVol, f64TmpLevel;

    f64B = CALC_B((double)s_u16Level64Vol, (double)s_u16Level192Vol);
    f64K = CALC_K((double)s_u16Level64Vol, (double)s_u16Level192Vol);

    DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "b: %lf, k = %lf, leve l= %u \n", f64B, f64K, u16Level);

    //f64DacVol = (double)(u16Level / 10000) * 10000(mv);
    f64DacVol =  (double)u16Level;

    f64TmpLevel = CALC_LEVEL(f64DacVol, f64B, f64K);
    f64TmpLevel += 0.5; //四舍五入

    if (f64TmpLevel >= DAC_RAW_MIN_LEVEL && f64TmpLevel <= DAC_RAW_MAX_LEVEL) {
        if (u16Level != DAC_MAX_LEVEL) {
            u8RawLevel = (uint8_t)f64TmpLevel; 
        } else {
            u8RawLevel = (uint8_t)f64TmpLevel; 
            if (u8RawLevel < (DAC_RAW_MAX_LEVEL -2)) {
                u8RawLevel += 2; 
            }
        }
    } else if (f64TmpLevel < 0) {
        u8RawLevel = 0; 
    } else {
        u8RawLevel = 255; 
    }

    DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "raw level %d \n", u8RawLevel);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);
    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
    if (g_DimStatus != DIM_INIT_SUCCESS) {
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "take mutex fail %s:%d \n", __func__, __LINE__);
        return 1;
    }

    u8Ret = BspDacLevelSet(u8RawLevel);
    if (u8Ret == 0) {
        g_DacLevel = u8RawLevel;
    }

    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);    
    return u8Ret;
}

uint8_t DimDacRawLevelSet(uint8_t u8Level)
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);
    if (g_DimStatus != DIM_INIT_SUCCESS) {
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "take mutex fail %s:%d \n", __func__, __LINE__);
        return 1;
    }

    u8Ret = BspDacLevelSet(u8Level);
    if (u8Ret == 0) {
        g_DacLevel = u8Level;
    }
    
    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);	 
    return u8Ret;
}


static void GetCalParam()
{
    esp_err_t err;

    uint16_t u16DacLevel64Vol = 0;
    uint16_t u16DacLevel192Vol = 0;
    size_t len = 2;

    nvs_handle my_handle;

    err = nvs_open("DacParam", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) { 
        err = nvs_get_blob(my_handle, "DacLevel64",  (uint8_t *)&u16DacLevel64Vol, &len);
        if (err != ESP_OK) {
            return;
        } else {
            DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "DacLevel64 = %x \n", u16DacLevel64Vol);
        }

        err = nvs_get_blob(my_handle, "DacLevel192",  (uint8_t *)&u16DacLevel192Vol, &len);
        if (err != ESP_OK) {
            return;
        } else {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "DacLevel192 = %x \n", u16DacLevel192Vol);
        }

        s_u16Level64Vol = u16DacLevel64Vol;
        s_u16Level192Vol = u16DacLevel192Vol;
    }
    nvs_close(my_handle);
}

/*
 *@breif 调光功能模块初始化 
 *@param 无 
 *@return 0: 成功， 1：失败
 */
uint8_t DimCtrlInit()
{
    uint8_t u8Ret;

    //g_DimMutex = xSemaphoreCreateMutex();
    /*if (g_DimMutex == NULL) {
        g_DimStatus = DIM_INIT_FAIL;
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "create g_DimStatus fail \n");
        return 1; 
    }*/
	 MUTEX_CREATE(g_DimMutex);

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);
    u8Ret = DimRelayGpioInit();
    if (u8Ret) {
        g_DimStatus = DIM_INIT_FAIL;
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "relay gpio init fail \n");
        return 1;
    }

    u8Ret = DimSwitchGpioInit();
    if (u8Ret) {
        g_DimStatus = DIM_INIT_FAIL;
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "dim switch gpio init fail \n");
        return 1;
    }

    u8Ret = BspDacEnable();
    if (u8Ret) {
        g_DimStatus = DIM_INIT_FAIL;
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	         
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "dac enable fail \n");
        return 1;
    }

    u8Ret = BspInitPwm();
    if (u8Ret) {
        g_DimStatus = DIM_INIT_FAIL;
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_ERROR, "pwm init fail \n");
        return 1;
    }

    g_DimStatus = DIM_INIT_SUCCESS;

    GetCalParam(); 
    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);	 
    return 0;
}


/*
 *@brief  写level 64 对应的输出电压
 *@param  u32DacOutput: 输出的电压值 
 *@return 0 获取成功，1 获取失败
*/
uint8_t DacLevel64ParamSet(uint16_t u16DacOutput)
{

    nvs_handle my_handle;
    esp_err_t err;

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);

    DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "set level value : %u  \n", u16DacOutput);    

    err = nvs_open("DacParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }

    err = nvs_set_blob(my_handle, "DacLevel64", (uint8_t *)&u16DacOutput, 2);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }
    
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }

    nvs_close(my_handle);

    s_u16Level64Vol = u16DacOutput;

    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);	 
    return 0;
}

/*
 *@brief  写level 192 对应的输出电压
 *@param  u32DacOutput: 输出的电压值 
 *@return 0 获取成功，1 获取失败
*/
uint8_t DacLevel192ParamSet(uint16_t u16DacOutput)
{
    nvs_handle my_handle;
    esp_err_t err;

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);
    DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "set level 192 param value : %u  \n", u16DacOutput);    
    err = nvs_open("DacParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }

    err = nvs_set_blob(my_handle, "DacLevel192", (uint8_t *)&u16DacOutput, 2);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }

    
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }

    nvs_close(my_handle);
    s_u16Level192Vol = u16DacOutput;

    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);	 
    return 0;
}

/*
 *@brief  获取Dac Level 64 电压
 *@param  pDacLevelVol: 存储获取值的指针
 *@return 0 获取成功，1 获取失败
*/
uint8_t DacLevel64ParamGet(uint16_t *pDacLevelVol)
{
    int ret = 0;
    size_t len = 2;

    nvs_handle my_handle;
    esp_err_t err;

    if (pDacLevelVol == NULL) {
        return 1;
    }

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);

    err = nvs_open("DacParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }

    err = nvs_get_blob(my_handle, "DacLevel64",  (uint8_t *)pDacLevelVol, &len);
    if (err == ESP_OK) {
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "DacLevel64 vol : %u \n", *pDacLevelVol);
    } else {
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "read DacLevel64 vol err \n", *pDacLevelVol);
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }

    nvs_close(my_handle);
    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);	 
    return 0;
}

/*
 *@brief  获取Dac Level 192 电压
 *@param  pDacLevelVol: 存储获取值的指针
 *@return 0 获取成功，1 获取失败
*/
uint8_t DacLevel192ParamGet(uint16_t *pDacLevelVol)
{
    int ret = 0;
    size_t len = 2;

    nvs_handle my_handle;
    esp_err_t err;

    if (pDacLevelVol == NULL) {
        return 1;
    }

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);
    err = nvs_open("DacParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }

    err = nvs_get_blob(my_handle, "DacLevel192",  (uint8_t *)pDacLevelVol, &len);
    if (err == ESP_OK) {
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "DacLevel192 vol : %u \n", *pDacLevelVol);
    } else {
        DEBUG_LOG(DEBUG_MODULE_LAMPCTRL, DLL_INFO, "read DacLevel192 vol err \n");
        nvs_close(my_handle);
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }

    nvs_close(my_handle);
    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);	 
    return 0;
}

uint8_t delDacCalParam()
{
    esp_err_t err;

    nvs_handle my_handle;

    //xSemaphoreTake(g_DimMutex, portMAX_DELAY);
	MUTEX_LOCK(g_DimMutex, MUTEX_WAIT_ALWAYS);
    err = nvs_open("DacParam", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }

    err = nvs_erase_key(my_handle, "DacLevel192");
    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            nvs_close(my_handle);
            //xSemaphoreGive(g_DimMutex);
			MUTEX_UNLOCK(g_DimMutex);	 
            return 1;
        }
    }

    err = nvs_erase_key(my_handle, "DacLevel64");
    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            nvs_close(my_handle);
            //xSemaphoreGive(g_DimMutex);
			MUTEX_UNLOCK(g_DimMutex);	 
            return 1;
        }
    }

    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        //xSemaphoreGive(g_DimMutex);
		MUTEX_UNLOCK(g_DimMutex);	 
        return 1;
    }
    
    nvs_close(my_handle);
    //xSemaphoreGive(g_DimMutex);
	MUTEX_UNLOCK(g_DimMutex);	 
    return 0;
}

