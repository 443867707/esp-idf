#include "DRV_Nb.h"
#include "DRV_Gpio.h"
#include "OPP_Debug.h"
#include "OS.h"

SemaphoreHandle_t g_NbRxSem = NULL;
//SemaphoreHandle_t g_NbMutex = NULL;
T_MUTEX g_NbMutex;
uint8_t g_NbRxFlag = 0;

typedef enum _NbStatus {
    Nb_NO_INIT,
    Nb_RUNNING,
} NbStatus_t;

NbStatus_t g_NbStatus = Nb_NO_INIT;

static uint8_t NbRstGpioInit()
{
    esp_err_t err;

    gpio_config_t gpio_conf;
    gpio_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask = GPIO_NB_RST_PIN_SEL;
    gpio_conf.pull_down_en = 0;
    gpio_conf.pull_up_en = 0;

    err = gpio_config(&gpio_conf);
    if (err != ESP_OK) {
        return 1;
    }
    
    return BspGpioSetLevel(GPIO_NB_RST_PIN, 0);
}

uint8_t NbSetRstGpioLevel(uint8_t Level)
{
    uint8_t u8Ret;

    if (Level != 0 && Level != 1) {
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_INFO, "unkwon level \n");
        return 1;
    }

    //xSemaphoreTake(g_NbMutex, portMAX_DELAY);
    MUTEX_LOCK(g_NbMutex,MUTEX_WAIT_ALWAYS);
	/*
    if (g_NbStatus != Nb_RUNNING) {
        xSemaphoreGive(g_NbMutex);
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_INFO, "nb is not running %s:%d\n", __func__, __LINE__);
        return 1;
    }
    */

    u8Ret = BspGpioSetLevel(GPIO_NB_RST_PIN, Level);
    if (u8Ret) {
        //xSemaphoreGive(g_NbMutex);
        MUTEX_UNLOCK(g_NbMutex);
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_INFO, "nb is not running %s:%d\n", __func__, __LINE__);
        return 1;
    }

    //xSemaphoreGive(g_NbMutex);
	MUTEX_UNLOCK(g_NbMutex);
    return 0;
}


uint8_t NbReset()
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_NbMutex, portMAX_DELAY);
    MUTEX_LOCK(g_NbMutex,MUTEX_WAIT_ALWAYS);
    u8Ret = BspGpioSetLevel(GPIO_NB_RST_PIN, 1);
    if (u8Ret) {
        //xSemaphoreGive(g_NbMutex);
        MUTEX_UNLOCK(g_NbMutex);
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_ERROR, "set nb reset gpio err %s:%d\n", __func__, __LINE__);
        return 1;
    }

    vTaskDelay(100 / portTICK_RATE_MS);
    u8Ret = BspGpioSetLevel(GPIO_NB_RST_PIN, 0);
    if (u8Ret) {
        //xSemaphoreGive(g_NbMutex);
        MUTEX_UNLOCK(g_NbMutex);
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_ERROR, "set nb reset gpio err %s:%d\n", __func__, __LINE__);
        return 1;
    }

    //xSemaphoreGive(g_NbMutex);
	MUTEX_UNLOCK(g_NbMutex);
    return 0;
}

int NbUartWriteBytes(char* send_buffer, size_t size)
{
    int s32Ret;

    //xSemaphoreTake(g_NbMutex, portMAX_DELAY);
    MUTEX_LOCK(g_NbMutex,MUTEX_WAIT_ALWAYS);
    if (g_NbStatus != Nb_RUNNING) {
        //xSemaphoreGive(g_NbMutex);
		MUTEX_UNLOCK(g_NbMutex);
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_ERROR, "nb is not running %s:%d\n", __func__, __LINE__);
        return -1;
    }

    s32Ret = uart_write_bytes(NB_UART_NUM, send_buffer, size);

    //xSemaphoreGive(g_NbMutex);
	MUTEX_UNLOCK(g_NbMutex);
    return s32Ret; 
}


int NbUartReadBytes(uint8_t *u8RecvBuf, uint32_t u32RecvLen, uint32_t u32Timeout) {
    int s32Ret;

    //xSemaphoreTake(g_NbMutex, portMAX_DELAY);
    MUTEX_LOCK(g_NbMutex,MUTEX_WAIT_ALWAYS);
    if (g_NbStatus != Nb_RUNNING) {
        //xSemaphoreGive(g_NbMutex);
		MUTEX_UNLOCK(g_NbMutex);
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_ERROR, "nb is not running %s:%d\n", __func__, __LINE__);
        return -1;
    }

    s32Ret = BspUartReadBytes(NB_UART_NUM, u8RecvBuf, u32RecvLen, u32Timeout / portTICK_RATE_MS);

    //xSemaphoreGive(g_NbMutex);
	MUTEX_UNLOCK(g_NbMutex);
    return s32Ret; 
}


uint8_t NbUartConfig(uart_config_t *nb_uart_config) 
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_NbMutex, portMAX_DELAY);
    MUTEX_LOCK(g_NbMutex,MUTEX_WAIT_ALWAYS);
    u8Ret = BspUartConfig(NB_UART_NUM, nb_uart_config);
    if (u8Ret) {
        //xSemaphoreGive(g_NbMutex);
		MUTEX_UNLOCK(g_NbMutex);
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_ERROR, "nb uart config fail \n");
        return 1; 
    } 

    //xSemaphoreGive(g_NbMutex);
	MUTEX_UNLOCK(g_NbMutex);
    return 0;
}

uint8_t NbUartInit(int rx_buffer_size, int tx_buffer_size)
{
    uint8_t  u8Ret;

    g_NbRxSem = xSemaphoreCreateBinary();
    if (g_NbRxSem == NULL) {
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_ERROR, "create g_NbRxSem fail \n");
        return 1;
    }

    /*g_NbMutex = xSemaphoreCreateMutex();
    if (g_NbMutex == NULL) {
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_ERROR, "create g_NbMutex fail \n");
        return 1;
    }*/
	MUTEX_CREATE(g_NbMutex);
    //xSemaphoreTake(g_NbMutex, portMAX_DELAY);
    MUTEX_LOCK(g_NbMutex,MUTEX_WAIT_ALWAYS);
    u8Ret = NbRstGpioInit();
    if (u8Ret) {
        //xSemaphoreGive(g_NbMutex);
		MUTEX_UNLOCK(g_NbMutex);
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_ERROR, "nb rst gpio init fail \n");
        return 1;
    }

    u8Ret = BspUartInit(NB_UART_NUM, rx_buffer_size, tx_buffer_size, NB_UART_TXD, NB_UART_RXD);
    if (u8Ret) {
        //xSemaphoreGive(g_NbMutex);
		MUTEX_UNLOCK(g_NbMutex);
        DEBUG_LOG(DEBUG_MODULE_NB, DLL_ERROR, "nb uart init fail \n");
        return 1;
    }
    //xSemaphoreGive(g_NbMutex);

    g_NbStatus = Nb_RUNNING;
	MUTEX_UNLOCK(g_NbMutex);
	return 0;
}
