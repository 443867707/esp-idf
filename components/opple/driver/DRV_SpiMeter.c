#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "DRV_SpiMeter.h"
#include "OPP_Debug.h"
#include "nvs.h"
#include "OS.h"

#define VSPI_PIN_NUM_MISO 19 
#define VSPI_PIN_NUM_MOSI 23 
#define VSPI_PIN_NUM_CLK  18 
#define VSPI_PIN_NUM_CS   5 
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<VSPI_PIN_NUM_CS)

#define MAX_VALID_VOLTAGE 3200 // 320V
#define MAX_VALID_POWER   8000000 // 800 W

extern uint16_t g_u16LastEnergyReg;

static uint8_t EmuSPIOp(uint8_t Op_Mode );
static uint8_t EmuInitial(void);
static uint8_t CurrentParamWrite(uint16_t IbGain);
static uint8_t VoltageParamWrite(uint16_t Ugain);
static uint8_t PowerParamWrite(uint16_t PbGain);
static uint8_t BPhCalParamWrite(uint8_t u8BPhCal);

/*计量功能初始化状态*/
typedef enum _MeterInitStatus {
    METER_INIT_NONE,
    METER_INIT_SUCCESS,
    METER_INIT_MUTEX_FAIL, 
    METER_INIT_SPI_FAIL, 
    METER_INIT_REG_FAIL, 
} MeterInitStatus_t; 
static MeterInitStatus_t g_MeterInitStatus = METER_INIT_NONE;

spi_device_handle_t g_SpiHandle = NULL;
//SemaphoreHandle_t g_MeterMutex = NULL;
T_MUTEX g_MeterMutex;
/**
 * @brief  init spi cs pin 
 * @param  none 
 * @retval 0: success, 1: fail 
 */
static uint8_t SpiCsGpioInit()
{
    esp_err_t err;

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;

    err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        return 1;
    }

    return 0;
}

/**
 * @brief SPI 读取一个字节数据
 * @param 无 
 * @retval 0: success, 1: fail 
 */
static uint8_t SpiDrt6020ReadByte(uint8_t *u8Recv) 
{
    spi_transaction_t t;

    if (u8Recv == NULL) {
        return 1;
    }

    memset(&t, 0, sizeof(spi_transaction_t));
    t.length=8;
    t.flags = SPI_TRANS_USE_RXDATA;
    t.user = (void*)1;

    esp_err_t ret = spi_device_transmit(g_SpiHandle, &t);
    if (ret != ESP_OK) {
        return 1;
    }

    *u8Recv = *(uint8_t*)t.rx_data;
    return 0;
}


/**
  * @brief  Writes a byte to device.
  * @param  Value: value to be written
  * @return 0: read success  1: read fail
  */
static uint8_t SpiDrt6020SendByte(uint8_t u8Value)
{
    esp_err_t ret;
    spi_transaction_t t;

    memset(&t, 0, sizeof(spi_transaction_t));
    t.length = 8;  //bytes --> bits 
    t.tx_buffer = &u8Value;
    ret=spi_device_transmit(g_SpiHandle, &t);

    if (ret != ESP_OK) {
        return 1;
    }

    return 0;
}


/**
  * @brief  控制片选引脚输出 
  * @param  high: 取值范围 0、1, 0 选中
  * @return 无 
  */
static void Drt6020CtrlCS(int high)
{
    if (high) {
        gpio_set_level(VSPI_PIN_NUM_CS, 1);
    } else { 
        gpio_set_level(VSPI_PIN_NUM_CS, 0);
    }
}

uint8_t g_EmuConBuf[5];               

struct S_MeterVariable g_EmuData;  

static uint8_t EmuOP(uint8_t OpMode)
{
    return EmuSPIOp(OpMode);
}

static uint8_t EMUEnWrEA(void)
{
    uint8_t ret;
    g_EmuConBuf[0] = 0x70;
    g_EmuConBuf[1] = 0xE5;

    ret = EmuOP(SPIWrite);
    return ret;
}

static uint8_t EMUDisWr(void) 
{
    g_EmuConBuf[0] = 0x70;  
    g_EmuConBuf[1] = 0xDC;
    return EmuOP(SPIWrite);
}

static uint8_t reset_meter(void) 
{
    g_EmuConBuf[0] = RESET;  
    g_EmuConBuf[1] = 0x55;  
    return EmuOP(SPIWrite);
}

static uint8_t EMUEnWrEA50(void)
{
    g_EmuConBuf[0] = 0x7F;
    g_EmuConBuf[1] = 0x69;
    return EmuOP(SPIWrite);
}

static uint8_t EMUDisWr50(void) 
{
    g_EmuConBuf[0] = 0x7F;
    g_EmuConBuf[1] = 0x96;
    return EmuOP(SPIWrite);
}

/*
 *@brief  校准电压
 *@param  U: 输入的测试台电压,单位0.1V
 *@return 0 校准成功，1 校准失败 
*/
uint8_t VoltageAdjust(uint32_t U)
{
    uint8_t ret;
    double   f64ErrData;   //误差值
    union    B32_B08    temp32;    //结果缓存
    union    B16_B08    temp16;

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex,MUTEX_WAIT_ALWAYS);
    ret = EMUEnWrEA();            //计量芯片写使能

    if (ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "enable reg writable fail %s:%d\n", __func__, __LINE__);
        //xSemaphoreGive(g_MeterMutex);
     	MUTEX_UNLOCK(g_MeterMutex);   
        return 1;
    }
    //校准前先清除寄存器
    g_EmuConBuf[0] = UGAIN; 
    g_EmuConBuf[1] = 0;
    g_EmuConBuf[2] = 0;  
    ret = EmuOP(SPIWrite);
    if (ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "write reg err %s:%d\n", __func__, __LINE__);
        //xSemaphoreGive(g_MeterMutex);
     	MUTEX_UNLOCK(g_MeterMutex);           
        return 1;
    }

    vTaskDelay(700 / portTICK_RATE_MS); //延时 寄存器320ms刷新一次，详见数据手册

    //读校准前电压
    g_EmuConBuf[0] = URMS;
    ret = EmuOP(SPIRead);
    if (ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "read reg err %s:%d\n", __func__, __LINE__);
        //xSemaphoreGive(g_MeterMutex);
     	MUTEX_UNLOCK(g_MeterMutex);   
        return 1;
    }

    temp32.B08[0] = g_EmuConBuf[3];   //大端转换成小端
    temp32.B08[1] = g_EmuConBuf[2];
    temp32.B08[2] = g_EmuConBuf[1];
    temp32.B08[3] = 0;
    
    //计算测量电压                                    //公式详见数据手册
    temp32.B32 = temp32.B32 * ((double)K_V / 1677721.6);  //1位小数 U = (DATA*K_V*10)/(Gu*2^23) = DATA*(K_V/Gu*1677721.6)    

    if (temp32.B32 == 0) {
        //xSemaphoreGive(g_MeterMutex);
     	MUTEX_UNLOCK(g_MeterMutex);   
        return 1;
    }
  
    f64ErrData = (double)U / temp32.B32 - 1;    //误差计算：   (实际测量-台体理论)/(台体理论)
    
    if (f64ErrData >= 0) {    //转换成补码形式 详见数据手册/应用笔记
        temp32.B32 = f64ErrData * 32768;
    } else {
        temp32.B32 = f64ErrData * 32768 + 65536;
    }
  
    g_EmuConBuf[0] = UGAIN;//写校准结果到计量芯片
    g_EmuConBuf[1] = temp32.B08[1];
    g_EmuConBuf[2] = temp32.B08[0];
    ret = EmuOP(SPIWrite);
    if (ret) {
        //xSemaphoreGive(g_MeterMutex);
     	MUTEX_UNLOCK(g_MeterMutex);   
        return 1;
    }

    temp16.B08[0] = temp32.B08[0];
    temp16.B08[1] = temp32.B08[1];

    if (VoltageParamWrite(temp16.B16) == 1) {
        //xSemaphoreGive(g_MeterMutex);
     	MUTEX_UNLOCK(g_MeterMutex);  
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "voltage adjust param write err\n");

        return 1;
    }
  
    ret = EMUDisWr();   //计量芯片写禁止
    if (ret) {
        //xSemaphoreGive(g_MeterMutex);
     	MUTEX_UNLOCK(g_MeterMutex);   
        return 1;
    }
    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);   

    return 0;
}


static double GetErrData(uint32_t P, double *pErrData)
{
    union    B32_B08;  //台体下发功率
    double   f64TempDouble = 0;
    uint8_t  u8Result, i;

    if (pErrData == NULL) {
        return 1;
    }
    
    for (i=0; i<7; i++)             //多次读取均值，提高精度，但耗时较长
    {
        vTaskDelay(400 / portTICK_RATE_MS); //寄存器320ms刷新一次，详见数据手册
        g_EmuConBuf[0] = E_PB;    //读当前功率
        u8Result = EmuOP(SPIRead);
        if (u8Result) {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "read err %s:%d \n", __func__, __LINE__);
            return 1;
        }
        g_EmuData.P1.B08[0] = g_EmuConBuf[4];
        g_EmuData.P1.B08[1] = g_EmuConBuf[3];
        g_EmuData.P1.B08[2] = g_EmuConBuf[2];
        g_EmuData.P1.B08[3] = g_EmuConBuf[1];
  
        if ((g_EmuConBuf[1] & 0x80) == 0x80) {   //如果是负数
            g_EmuData.P1.B32 = (~g_EmuData.P1.B32) + 1;  //取绝对值
        }

        f64TempDouble  += (double)g_EmuData.P1.B32 * 0.13111;
    }

    f64TempDouble = f64TempDouble / 7;

/*台体有功功率为0时，会有 1 / 10000 的误差 */
    if (P == 0) {
        P = 1;
    }

    *pErrData = (double)f64TempDouble / P - 1 ;  //误差计算：(实际测量功率-台体理论功率)/(台体理论功率)

    return 0;
}

/*
 *@brief  校准功率 
 *@param  P: 输入的测试台功率, 单位0.1 mw
 *@return 0 校准成功，1 校准失败 
*/
uint8_t PowerAdjust(uint32_t P)
{
    union    B32_B08    temp32;
    union    B16_B08    temp16;
    double   f64ErrData;
    uint8_t  u8Ret;
  
    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex,MUTEX_WAIT_ALWAYS);
    //计量芯片写使能
    u8Ret = EMUEnWrEA(); 
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    //校准前先清除寄存器
    g_EmuConBuf[0] = PBGAIN;
    g_EmuConBuf[1] = 0; 
    g_EmuConBuf[2] = 0;  
    u8Ret = EmuOP(SPIWrite);  
    if (u8Ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "write err %s:%d \n", __func__, __LINE__);
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;    
    }

    vTaskDelay(300 / portTICK_RATE_MS);

    u8Ret = GetErrData(P, &f64ErrData);//计算误差
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    f64ErrData = -f64ErrData / (1 + f64ErrData);  //误差调整为：G-1 = (1/(1+f64ErrData))-1
  
    //将(G-1)调整为*2^15的补码形式
    if (f64ErrData >= 0) {
        temp32.B32 = 32768 * f64ErrData;
    } else {
        temp32.B32 = 65536 + 32768 * f64ErrData;
    }
        
    g_EmuConBuf[0] = PBGAIN;   //将校表结果写回计量芯片
    g_EmuConBuf[1] = temp32.B08[1];
    g_EmuConBuf[2] = temp32.B08[0];

    u8Ret = EmuOP(SPIWrite);
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;     
    }

    u8Ret = EMUDisWr();   //计量芯片写禁止
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    temp16.B08[0] = temp32.B08[0];
    temp16.B08[1] = temp32.B08[1];

    if (PowerParamWrite(temp16.B16) == 1) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "power adjust param write err\n");
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);
    return 0;  
}

/*
 *@brief  校准相角
 *@param  P: 输入的测试台功率, 单位0.1 mw
 *@return 0 校准成功，1 校准失败 
*/
uint8_t BPhCalAdjust(uint32_t P)
{
    union    B32_B08    temp32;

    double   f64ErrData;
    uint8_t  u8Ret;
  
    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    //计量芯片写使能
    u8Ret = EMUEnWrEA(); 
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    //校准前先清除寄存器
    g_EmuConBuf[0] = 0x08;
    g_EmuConBuf[1] = 0; 
    u8Ret = EmuOP(SPIWrite);  
    if (u8Ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "write err %s:%d \n", __func__, __LINE__);
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;    
    }

    vTaskDelay(300 / portTICK_RATE_MS);

    u8Ret = GetErrData(P, &f64ErrData);//计算误差
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    f64ErrData = f64ErrData * 3763.74;
    
    if (f64ErrData >= 0) {
        temp32.B32 = f64ErrData;
    } else {
        temp32.B32 = 255 + f64ErrData;
    }
  
    g_EmuConBuf[0] = 0x08;   //将校表结果写回计量芯片
    g_EmuConBuf[1] = temp32.B08[0];

    u8Ret = EmuOP(SPIWrite);
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;     
    }

    u8Ret = EMUDisWr();   //计量芯片写禁止
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    if (BPhCalParamWrite(temp32.B08[0]) == 1) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "BPhCal adjust param write err\n");
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);
    return 0;  
}

/*
 *@brief  校准电流 
 *@param  I: 输入的测试台电流,单位mA 
 *@return 0 校准成功，1 校准失败 
*/
uint8_t CurrentAdjust(uint32_t I) 
{
    double  f64ErrData;
    //union   B32_B08  temp32_ib;
    union   B32_B08  temp32;
    union   B16_B08  temp16;
    uint8_t u8Ret, i;

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);

    //打开计量写使能
    u8Ret = EMUEnWrEA(); 
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    //校准前先清除寄存器
    g_EmuConBuf[0] = IBGAIN; 
    g_EmuConBuf[1] = 0; 
    g_EmuConBuf[2] = 0;
    u8Ret = EmuOP(SPIWrite);  
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1; 
    }
    vTaskDelay(700 / portTICK_RATE_MS); // reg refresh time is 320 ms

    //读当前B线电流
    g_EmuConBuf[0] = IBRMS;  
    u8Ret = EmuOP(SPIRead);
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }
         
    for (i=0; i<3; i++) {     //转换成小端
        temp32.B08[2-i] = g_EmuConBuf[1+i];
    }
    temp32.B08[3] = 0;

    /*读出的电流寄存器为0时，会有 1 / 1000的误差*/
    if (temp32.B32 == 0) {
    	temp32.B32 = 1;
    }
  
   //计算电流数据
    temp32.B32 = (double)temp32.B32 / (RO_B * 33554.432); //3位小数 IA = DATA*1000/(R*Gi*2^23)
    f64ErrData = (double)I / temp32.B32-1;   //误差计算：   (实际测量-台体理论)/(台体理论)

    if (f64ErrData >= 0) {    //转换成补码形式
        temp32.B32 = f64ErrData * 32768;
    } else {
        temp32.B32 = f64ErrData * 32768 + 65536;    
    }
  
    g_EmuConBuf[0] = IBGAIN;    //写校准结果到计量芯片
    g_EmuConBuf[1] = temp32.B08[1];
    g_EmuConBuf[2] = temp32.B08[0];
    u8Ret = EmuOP(SPIWrite);
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;     
    }

    u8Ret = EMUDisWr();   //计量芯片写禁止
    if (u8Ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    temp16.B08[0] = temp32.B08[0];
    temp16.B08[1] = temp32.B08[1];

    if (CurrentParamWrite(temp16.B16) == 1) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "current adjust param write err\n");
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);
    return 0;
}

/*
 *@brief  获取电流的校准参数
 *@param  存储获取值的指针
 *@return 0 获取成功，1 获取失败
*/
uint8_t CurrentParamGet(uint16_t *IbGain) 
{
    size_t len = 2;
    nvs_handle my_handle;

    esp_err_t err;

    if (IbGain == NULL) {
        return 1;
    }

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    err = nvs_open("MeterParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    err = nvs_get_blob(my_handle, "IBGAIN",  IbGain, &len);

    if (err == ESP_OK) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "IbGain = %x \n", *IbGain);
    } else {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "read IbGain fail \n");
        *IbGain = 0;
        nvs_close(my_handle);
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);

        return 1; 
    }

    nvs_close(my_handle);
    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);

    return 0;
}

/*
 *@brief  获取电压的校准参数
 *@param  存储获取值的指针
 *@return 0 获取成功，1 获取失败
*/
uint8_t VoltageParamGet(uint16_t *Ugain) 
{
    size_t len = 2;
    nvs_handle my_handle;
    esp_err_t err;

    if (Ugain == NULL) {
        return 1;
    }


    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    err = nvs_open("MeterParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    err = nvs_get_blob(my_handle, "UGAIN",  Ugain, &len);
    if (err == ESP_OK) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "Ugain = %x \n", *Ugain);
    } else {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "read Ugain fail \n");
        *Ugain = 0;
        nvs_close(my_handle);
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    nvs_close(my_handle);
    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);
    return 0;
}

/*
 *@brief  获取功率的校准参数
 *@param  PbGain: 存储获取值的指针
 *@return 0 获取成功，1 获取失败
*/
uint8_t PowerParamGet(uint16_t *PbGain) 
{
    size_t len = 2;

    nvs_handle my_handle;
    esp_err_t err;

    if (PbGain == NULL) {
        return 1;
    }

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    err = nvs_open("MeterParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    err = nvs_get_blob(my_handle, "PBGAIN",  PbGain, &len);
    if (err == ESP_OK) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "PbGain = %x \n", *PbGain);
    } else {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "read Ugain err \n");
        nvs_close(my_handle);
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    nvs_close(my_handle);
    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);
    return 0;
}

/*
 *@brief  获取相角的校准参数
 *@param  PbGain: 存储获取值的指针
 *@return 0 获取成功，1 获取失败
*/
uint8_t BPhCalParamGet(uint8_t *BPhCal) 
{
    size_t len = 1;

    nvs_handle my_handle;
    esp_err_t err;

    if (BPhCal == NULL) {
        return 1;
    }

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    err = nvs_open("MeterParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    err = nvs_get_blob(my_handle, "BPHCAL",  BPhCal, &len);
    if (err == ESP_OK) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "BPhCal = %x \n", *BPhCal);
    } else {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "read BPhCal err \n");
        nvs_close(my_handle);
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    nvs_close(my_handle);
    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);
    return 0;
}

/*
 *@brief  写电流的校准参数
 *@param  IbGain: 校准值 
 *@return 0 获取成功，1 获取失败
*/
static uint8_t CurrentParamWrite(uint16_t IbGain) 
{
    nvs_handle my_handle;
    esp_err_t err;

    err = nvs_open("MeterParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        return 1;
    }

    err = nvs_set_blob(my_handle, "IBGAIN", &IbGain, 2);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return 1;
    }
    
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return 1;
    }
    
    nvs_close(my_handle);

    return 0;
}

/*
 *@brief  写电流的校准参数
 *@param  IbGain: 校准值 
 *@return 0 获取成功，1 获取失败
*/
uint8_t CurrentParamSet(uint16_t IbGain) 
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    u8Ret = CurrentParamWrite(IbGain);
	MUTEX_UNLOCK(g_MeterMutex);
    //xSemaphoreGive(g_MeterMutex);
    return u8Ret;
}

/*
 *@brief  写电压的校准参数
 *@param  Ugain: 校准值 
 *@return 0 获取成功，1 获取失败
*/
static uint8_t VoltageParamWrite(uint16_t Ugain) 
{
    nvs_handle my_handle;
    esp_err_t err;

    err = nvs_open("MeterParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        return 1;
    }

    err = nvs_set_blob(my_handle, "UGAIN", &Ugain, 2);
    if (err != ESP_OK) {
        nvs_close(my_handle);

        return 1;
    }
    
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);

        return 1;
    }
    
    nvs_close(my_handle);

    return 0;
}

/*
 *@brief  设置电压的校准参数
 *@param  Ugain: 校准值 
 *@return 0 获取成功，1 获取失败
*/
uint8_t VoltageParamSet(uint16_t Ugain) 
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    u8Ret = VoltageParamWrite(Ugain);
	MUTEX_UNLOCK(g_MeterMutex);
    //xSemaphoreGive(g_MeterMutex);
    return u8Ret;
}

/*
 *@brief  写功率的校准参数
 *@param  PbGain: 校准值 
 *@return 0 获取成功，1 获取失败
*/
static uint8_t PowerParamWrite(uint16_t PbGain) 
{
    nvs_handle my_handle;
    esp_err_t err;

    err = nvs_open("MeterParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        return 1;
    }

    err = nvs_set_blob(my_handle, "PBGAIN", &PbGain, 2);
    if (err != ESP_OK) {
        nvs_close(my_handle);

        return 1;
    }
    
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);

        return 1;
    }
    
    nvs_close(my_handle);

    return 0;
}

/*
 *@brief  设置功率校准参数
 *@param  PbGain: 校准值 
 *@return 0 获取成功，1 获取失败
*/
uint8_t PowerParamSet(uint16_t PbGain) 
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    u8Ret = PowerParamWrite(PbGain);
    MUTEX_UNLOCK(g_MeterMutex);
    //xSemaphoreGive(g_MeterMutex);
    return u8Ret;
}

/*
 *@brief  写相角的校准参数
 *@param  u8BPhCal: 校准值 
 *@return 0 获取成功，1 获取失败
*/
static uint8_t BPhCalParamWrite(uint8_t u8BPhCal) 
{
    nvs_handle my_handle;
    esp_err_t err;

    err = nvs_open("MeterParam", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        return 1;
    }

    err = nvs_set_blob(my_handle, "BPHCAL", &u8BPhCal, 1);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return 1;
    }
    
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return 1;
    }
    
    nvs_close(my_handle);

    return 0;
}
/*
 *@brief  设置相角的校准参数
 *@param  IbGain: 校准值 
 *@return 0 获取成功，1 获取失败
*/
uint8_t BPhCalParamSet(uint8_t u8BPhCal) 
{
    uint8_t u8Ret;

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    u8Ret = BPhCalParamWrite(u8BPhCal);
    MUTEX_UNLOCK(g_MeterMutex);
    //xSemaphoreGive(g_MeterMutex);
    return u8Ret;
}

uint8_t delAdjustParam()
{
    esp_err_t err;

    nvs_handle my_handle;

	
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);

    err = nvs_open("MeterParam", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
		
		MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }

    err = nvs_erase_key(my_handle, "UGAIN");
    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            nvs_close(my_handle);
			
			MUTEX_UNLOCK(g_MeterMutex);
            return 1;
        }
    }

    err = nvs_erase_key(my_handle, "IBGAIN");
    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            nvs_close(my_handle);
			
			MUTEX_UNLOCK(g_MeterMutex);
            return 1;
        }
    }

    err = nvs_erase_key(my_handle, "PBGAIN");
    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            nvs_close(my_handle);
			
			MUTEX_UNLOCK(g_MeterMutex);
            return 1;
        }
    }

    err = nvs_erase_key(my_handle, "BPHCAL");
    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            nvs_close(my_handle);
			
			MUTEX_UNLOCK(g_MeterMutex);
            return 1;
        }
    }

    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);
		
		MUTEX_UNLOCK(g_MeterMutex);
        return 1;
    }
    
    nvs_close(my_handle);
	
    MUTEX_UNLOCK(g_MeterMutex);

    return 0;
}


/*
 *@brief  设置功率的校准参数到计量芯片寄存器
 *@param  无 
 *@return 0 获取成功，1 获取失败
*/
static uint8_t writeAdjustParam()
{
    uint8_t ret = 0;
    uint8_t Ugain[2] = {0};
    uint8_t IbGain[2] = {0};
    uint8_t PbGain[2] = {0};
    uint8_t BPhCal = 0;

    esp_err_t err;

    nvs_handle my_handle;

    size_t len = 2;

    err = nvs_open("MeterParam", NVS_READWRITE, &my_handle);

    if (err == ESP_OK) {
        err = nvs_get_blob(my_handle, "UGAIN",  Ugain, &len);
        if (err != ESP_OK) {
                Ugain[0] = 0x37;
                Ugain[1] = 0xef;
        } else {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "Ugain = %02x%02x \n", Ugain[0], Ugain[1]);
        }

        err = nvs_get_blob(my_handle, "IBGAIN", IbGain, &len);
        if (err != ESP_OK) {
                IbGain[0] = 0x88;
                IbGain[1] = 0xf1;
        } else {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "IbGain = %02x%02x \n", IbGain[0], IbGain[1]);
        }

        err = nvs_get_blob(my_handle, "PBGAIN", PbGain, &len);
        if (err != ESP_OK) {
                PbGain[0] = 0x8f;
                PbGain[1] = 0xec;
        } else {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "PbGain = %02x%02x \n", PbGain[0], PbGain[1]);
        }

        len = 1;
        err = nvs_get_blob(my_handle, "BPHCAL", &BPhCal, &len);
        if (err != ESP_OK) {
                BPhCal = 0x1c;
        } else {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "BPhCal = %02x \n", BPhCal);
        }

        nvs_close(my_handle);
    } else {
        Ugain[0] = 0x37;
        Ugain[1] = 0xef;

        IbGain[0] = 0x88;
        IbGain[1] = 0xf1;

        PbGain[0] = 0x8f;
        PbGain[1] = 0xec;

        BPhCal = 0x1c;
    }

    ret = EMUEnWrEA(); 
    if (ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"enable write err \n");
        return 1;
    }

    g_EmuConBuf[0] = UGAIN;
    g_EmuConBuf[1] = Ugain[1];
    g_EmuConBuf[2] = Ugain[0];
    ret = EmuOP(SPIWrite);
    if (ret) {
        return 1;
    }

    g_EmuConBuf[0] = IBGAIN;
    g_EmuConBuf[1] = IbGain[1];
    g_EmuConBuf[2] = IbGain[0];
    ret = EmuOP(SPIWrite);
    if (ret) {
        return 1;
    }

    g_EmuConBuf[0] = PBGAIN;
    g_EmuConBuf[1] = PbGain[1];
    g_EmuConBuf[2] = PbGain[0];
    ret = EmuOP(SPIWrite);
    if (ret) {
        return 1;
    }

    g_EmuConBuf[0] = 0x08;
    g_EmuConBuf[1] = BPhCal;
    ret = EmuOP(SPIWrite);
    if (ret) {
        return 1;
    }
    ret = EMUDisWr();   //计量芯片写禁止
    if (ret) {
        return 1;
    }

    return 0;
}

static uint8_t EmuInitial(void) 
{
    uint8_t ret;
    union   B16_B08   temp16;

    ret = EMUEnWrEA(); 
    if (ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"enable write err \n");
        return 1;
    }

    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
    g_EmuConBuf[0] = RESET;
    ret = EmuSPIOp(SPIRead);
    if (ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
        return 1; // 1 --> fail, 0 success
    }

    if (g_EmuConBuf[1] == 0x55) {

        vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
        g_EmuConBuf[0] = RESET;
        g_EmuConBuf[1] = 0x00;
        ret = EmuOP(SPIWrite);
        if (ret) {
		    DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"write err %s:%d \n", __func__, __LINE__);
            return 1;
        }
    }

    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);

    g_EmuConBuf[0] = PE;
    ret = EmuSPIOp(SPIRead);

    if (ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
        return 1; // 1 --> fail, 0 success
    }

    temp16.B08[0] = g_EmuConBuf[2];
    temp16.B08[1] = g_EmuConBuf[1];
    g_u16LastEnergyReg = temp16.B16;

    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
    g_EmuConBuf[0] = SSW;
    g_EmuConBuf[1] = 0x00;
    ret = EmuOP(SPIWrite);
    if (ret) {
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"write err %s:%d \n", __func__, __LINE__);
        return 1;
    }

    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);

    g_EmuConBuf[0] = PFSET;
    ret = EmuOP(SPIRead);
    if (ret) {
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"read err %s:%d \n", __func__, __LINE__);
        return 1;
    }

    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
		
    if ((g_EmuConBuf[1] != PFSETHHB) || (g_EmuConBuf[2] != PFSETHLB)
 || (g_EmuConBuf[3] != PFSETLHB) || (g_EmuConBuf[4] != PFSETLLB)) {
        g_EmuConBuf[0] = PFSET;
        g_EmuConBuf[1] = PFSETHHB;
        g_EmuConBuf[2] = PFSETHLB;
        g_EmuConBuf[3] = PFSETLHB;
        g_EmuConBuf[4] = PFSETLLB;
        ret = EmuOP(SPIWrite);
        if (ret) {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"write err %s:%d \n", __func__, __LINE__);
            return 1;
        }
    }

    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);

    g_EmuConBuf[0] = EMOD;
    ret = EmuOP(SPIRead);
    if (ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"read err %s:%d \n", __func__, __LINE__);
        return 1;
    }

    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);

    if ((g_EmuConBuf[1] != EMODHB) || (g_EmuConBuf[2] != EMODLB)) {
        g_EmuConBuf[0] = EMOD;
        g_EmuConBuf[1] = EMODHB;
        g_EmuConBuf[2] = EMODLB;
        ret = EmuOP(SPIWrite);
        if (ret) {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"write err %s:%d \n", __func__, __LINE__);
            return 1;
        }
    }

    g_EmuConBuf[0] = PGAC;
    ret = EmuOP(SPIRead);
    if (ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"read err %s:%d \n", __func__, __LINE__);
        return 1;
    }
    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);

    if (g_EmuConBuf[1] != PGACB) {
        g_EmuConBuf[0] = PGAC;
        g_EmuConBuf[1] = PGACB;
        ret = EmuOP(SPIWrite);
        if (ret) {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"write err %s:%d \n", __func__, __LINE__);
            return 1;
        }
    }

    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);

    g_EmuConBuf[0] = PGAC;
    ret = EmuOP(SPIRead);
    if (ret) {
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"read err %s:%d \n", __func__, __LINE__);
        return 1;
    }

    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
    
    g_EmuConBuf[0] = START;                         
    ret = EmuOP(SPIRead);
    if (ret) {
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"read err %s:%d \n", __func__, __LINE__);
        return 1;
    }
		
    if (g_EmuConBuf[1] != STARTHHB) {
        g_EmuConBuf[0] = START;
        g_EmuConBuf[1] = STARTHHB;
        ret = EmuOP(SPIWrite);
        if (ret) {  
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"write err %s:%d \n", __func__, __LINE__);
            return 1; 
        }
    }

    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
    g_EmuConBuf[0] = START;                         
    ret = EmuOP(SPIRead);
    if (ret) {
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"read err %s:%d \n", __func__, __LINE__);
        return 1;
    }
    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);

    g_EmuConBuf[0] = PSTART;                        
    ret = EmuOP(SPIRead);
    if (ret) {
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"read err %s:%d \n", __func__, __LINE__);
        return 1;
    }
		
    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
		
    if ((g_EmuConBuf[1] != PSTARTHB) || (g_EmuConBuf[2] != PSTARTLB)) {
        g_EmuConBuf[0] = PSTART;
        g_EmuConBuf[1] = PSTARTHB;
        g_EmuConBuf[2] = PSTARTLB;
        ret = EmuOP(SPIWrite);
        if (ret) {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"write err %s:%d \n", __func__, __LINE__);
            return 1;
        }
    }
		
    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);

    ret = EMUEnWrEA50();
    if (ret) {
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"enable 0x50 reg write err\n");
        return 1;
    }
    
    vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
    g_EmuConBuf[0] = 0x50;
    g_EmuConBuf[1] = 0x00;
    g_EmuConBuf[2] = 0xC8;
    g_EmuConBuf[3] = 0x28;
    ret = EmuOP(SPIWrite);
    if (ret) {
	DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"write err %s:%d \n", __func__, __LINE__);
        return 1;
    }
    
    EMUDisWr50();  

    ret = writeAdjustParam();

    if (ret) {
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"write adjust param err \n");
        return 1;
    }
 
    return  0; //0 success, 1 fail
}

/*
 *@brief  获取电压 电流 功率 功率因数
 *@param  info: 存储获取的计量数据缓冲区指针 
 *@return 0 获取成功，1 获取失败
*/
uint8_t MeterInstantInfoGet(meter_info_t *info) 
{
    uint8_t  ret;
    union    B32_B08    temp32;	
  
    if (info == NULL) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "info is NULL Point \n");
        return 1;		
    }

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    if (g_MeterInitStatus != METER_INIT_SUCCESS) {
        if (g_MeterInitStatus == METER_INIT_REG_FAIL) {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "Meter reg is not init\n");
            if (EmuInitial() == 1) {
                DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "reinit init Meter reg err\n");
                //xSemaphoreGive(g_MeterMutex);
                MUTEX_UNLOCK(g_MeterMutex);
                return 1;
            }

            g_MeterInitStatus = METER_INIT_SUCCESS;
        } else {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "Meter init fail \n");
			MUTEX_UNLOCK(g_MeterMutex);
            //xSemaphoreGive(g_MeterMutex);
	        return 1;
	    }
    }

    /*read power*/
    g_EmuConBuf[0] = E_PB;
    ret = EmuOP(SPIRead);

    if (ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
        return 1; // 1 --> fail, 0 success
    }
    temp32.B08[0] = g_EmuConBuf[4];
    temp32.B08[1] = g_EmuConBuf[3];
    temp32.B08[2] = g_EmuConBuf[2];
    temp32.B08[3] = g_EmuConBuf[1];

    if ((g_EmuConBuf[1]&0x80) == 0x80) {
        temp32.B32 = (~temp32.B32) + 1;
    }
    //temp32.B32 = (double)temp32.B32 * 0.068212;
    temp32.B32 = (double)temp32.B32 * 0.13111;	
    info->u32Power = temp32.B32;

    /*功率大于最大有效值*/
    if (info->u32Power > MAX_VALID_POWER) {
        DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "power is invalid %u \r\n", info->u32Power);

        if (EmuInitial() == 1) {
            g_MeterInitStatus = METER_INIT_REG_FAIL;
        }
        MUTEX_UNLOCK(g_MeterMutex);
        //xSemaphoreGive(g_MeterMutex);

        return 1; 
    }
		 
    /*read current*/
    g_EmuConBuf[0] = IBRMS;
    ret = EmuOP(SPIRead);
    if (ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
        return 1; // 1 --> fail, 0 success
    }

    temp32.B08[0] = g_EmuConBuf[3];
    temp32.B08[1] = g_EmuConBuf[2];
    temp32.B08[2] = g_EmuConBuf[1];
    temp32.B08[3] = 0;
    temp32.B32 = (double)temp32.B32 / (RO_B * 33554.432);
    info->u32Current = temp32.B32;

    /*read Voltage*/
    g_EmuConBuf[0] = URMS;
    ret = EmuOP(SPIRead);
    if (ret) {
        //xSemaphoreGive(g_MeterMutex);
        MUTEX_UNLOCK(g_MeterMutex);
        DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
        return 1; // 1 --> fail, 0 success
    }

    temp32.B08[0] = g_EmuConBuf[3];  
    temp32.B08[1] = g_EmuConBuf[2];
    temp32.B08[2] = g_EmuConBuf[1];
    temp32.B08[3] = 0; 
    temp32.B32 = temp32.B32 * ((double)K_V / 1677721.6); 
    info->u32Voltage = temp32.B32;

    /*电压大于最大有效值*/
    if (info->u32Voltage > MAX_VALID_VOLTAGE) {
        //xSemaphoreGive(g_MeterMutex);
        DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "volage is invalid %u \r\n", info->u32Voltage);
        
        if (EmuInitial() == 1) {
            g_MeterInitStatus = METER_INIT_REG_FAIL;
        }
        MUTEX_UNLOCK(g_MeterMutex);
        return 1; // 1 --> fail, 0 success
    }
 
    /*calc power factor, factor = u32Power / (u32Voltage * u32Current)  */

    if (info->u32Voltage == 0 || info->u32Current == 0) {
        info->u32PowerFactor = 1000;
        
    } else {
        info->u32PowerFactor = 1000 * ((double)info->u32Power / (info->u32Voltage * info->u32Current)); // 将小数 *1000 转化成整数
        if (info->u32PowerFactor > 1000) {
            info->u32PowerFactor = 1000;
        }
    }

    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);
    return 0;
}

/*
 *@brief  获取电能信息
 *@param  info: 存储获取的计量数据缓冲区指针 
 *@return 0 获取成功，1 获取失败
*/
uint8_t MeterEnergyInfoGet(uint16_t *pEnergy) 
{
    uint8_t  ret;
    union    B16_B08    temp16;
  
    if (pEnergy == NULL) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "pEnergy is NULL Point \n");
        return 1;		
    }

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    if (g_MeterInitStatus != METER_INIT_SUCCESS) {
        if (g_MeterInitStatus == METER_INIT_REG_FAIL) {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "Meter reg is not init\n");
            if (EmuInitial() == 1) {
                DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "reinit init Meter reg err\n");
                //xSemaphoreGive(g_MeterMutex);
				MUTEX_UNLOCK(g_MeterMutex);
                return 1;
            }

            g_MeterInitStatus = METER_INIT_SUCCESS;
        } else {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "Meter init fail \n");
            //xSemaphoreGive(g_MeterMutex);
			MUTEX_UNLOCK(g_MeterMutex);
	        return 1;
	    }
    }

    /*read energy*/
    g_EmuConBuf[0] = PE;
    ret = EmuSPIOp(SPIRead);

    if (ret) {
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
        return 2; // 1 --> fail, 0 success
    }

    temp16.B08[0] = g_EmuConBuf[2];
    temp16.B08[1] = g_EmuConBuf[1];
    *pEnergy = temp16.B16;

    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);    
    return 0;
}

static uint8_t MeterDumpRetByte1()
{
    uint8_t ret, i;

    for (i = 0x00; i <= 0x0c; i ++ ) {
        g_EmuConBuf[0] = i;
        ret = EmuSPIOp(SPIRead);
        if (ret) {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
            return 1; // 1 --> fail, 0 success
        }
        vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
        printf("%02x: %02x \n", g_EmuConBuf[0], g_EmuConBuf[1]);
    }

    return 0;
}

static uint8_t MeterDumpRetByte2()
{

    uint8_t ret, i;

    for (i = 0x10; i <= 0x1d; i ++ ) {
        g_EmuConBuf[0] = i;
        ret = EmuSPIOp(SPIRead);
        if (ret) {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
            return 1; // 1 --> fail, 0 success
        }
        vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
        printf("%02x: %02x %02x \n", g_EmuConBuf[0], g_EmuConBuf[1], g_EmuConBuf[2]);
    }

    for (i = 0x20; i <= 0x2f; i ++ ) {
        g_EmuConBuf[0] = i;
        ret = EmuSPIOp(SPIRead);
        if (ret) {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
            return 1; // 1 --> fail, 0 success
        }
        vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
        printf("%02x: %02x %02x \n", g_EmuConBuf[0], g_EmuConBuf[1], g_EmuConBuf[2]);
    }

    return 0;
}

static uint8_t MeterDumpRetByte3()
{
    uint8_t ret, i;

    for (i = 0x30; i <= 0x32; i ++ ) {
        g_EmuConBuf[0] = i;
        ret = EmuOP(SPIRead);
        if (ret) {
            DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
            return 1; // 1 --> fail, 0 success
        }

        printf("%02x: %02x %02x %02x \n", g_EmuConBuf[0], g_EmuConBuf[1],g_EmuConBuf[2], g_EmuConBuf[3]);

    }

    return 0;
}

static uint8_t MeterDumpRetByte4()
{
    uint8_t ret, i;

    for (i = 0x40; i <= 0x46; i ++ ) {
        g_EmuConBuf[0] = i;
        ret = EmuOP(SPIRead);
        if (ret) {
            DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
            return 1; // 1 --> fail, 0 success
        }

        printf("%02x: %02x %02x %02x %02x \n", g_EmuConBuf[0], g_EmuConBuf[1],g_EmuConBuf[2], g_EmuConBuf[3], g_EmuConBuf[4]);

    }

    return 0;
}

uint8_t MeterDumpReg() 
{
    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    MeterDumpRetByte1();
    MeterDumpRetByte2();
    MeterDumpRetByte3();
    MeterDumpRetByte4();
	MUTEX_UNLOCK(g_MeterMutex);    
    //xSemaphoreGive(g_MeterMutex);
    return 0;
}

uint8_t MeterReinit() 
{
    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    if (EmuInitial() == 1) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "reinit init Meter reg err\n");
		MUTEX_UNLOCK(g_MeterMutex);    
        //xSemaphoreGive(g_MeterMutex);
        return 1;
    }
	MUTEX_UNLOCK(g_MeterMutex);    
    //xSemaphoreGive(g_MeterMutex);
    return 0;
}


uint8_t MeterReset() 
{
    uint8_t ret;

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    ret = EMUEnWrEA(); 
    if (ret) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR,"enable write err \n");
		MUTEX_UNLOCK(g_MeterMutex);		
        return 1;
    }

    ret = reset_meter();    
    if (ret == 1) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "reset meter err\n");
    }

    ret = EMUDisWr();   //计量芯片写禁止
    if (ret) {
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);    
        return 1;
    }
    //xSemaphoreGive(g_MeterMutex);    
	MUTEX_UNLOCK(g_MeterMutex);    
    return ret;
}

/*
 *@brief  获取计量数据 
 *@param  info: 存储获取的计量数据缓冲区指针 
 *@return 0 获取成功，1 获取失败
*/
uint8_t MeterInfoGet(meter_info_t *info) 
{
    uint8_t  ret;
    union    B16_B08    temp16;
    union    B32_B08    temp32;	
  
    if (info == NULL) {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "info is NULL Point \n");
        return 1;		
    }

    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    if (g_MeterInitStatus != METER_INIT_SUCCESS) {
        if (g_MeterInitStatus == METER_INIT_REG_FAIL) {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "Meter reg is not init\n");
            if (EmuInitial() == 1) {
                DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "reinit init Meter reg err\n");
                //xSemaphoreGive(g_MeterMutex);
				MUTEX_UNLOCK(g_MeterMutex);    
                return 1;
            }

            g_MeterInitStatus = METER_INIT_SUCCESS;
        } else {
            DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "Meter init fail \n");
            //xSemaphoreGive(g_MeterMutex);
			MUTEX_UNLOCK(g_MeterMutex);    
	        return 1;
	    }
    }

    /*read energy*/
    g_EmuConBuf[0] = PE;
    ret = EmuSPIOp(SPIRead);

    if (ret) {
        //xSemaphoreGive(g_MeterMutex);        
		MUTEX_UNLOCK(g_MeterMutex);    
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
        return 2; // 1 --> fail, 0 success
    }

    temp16.B08[0] = g_EmuConBuf[2];
    temp16.B08[1] = g_EmuConBuf[1];
    info->u16Energy = temp16.B16;

    /*read power*/
    g_EmuConBuf[0] = E_PB;
    ret = EmuOP(SPIRead);

    if (ret) {
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);    
        DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
        return 1; // 1 --> fail, 0 success
    }
    temp32.B08[0] = g_EmuConBuf[4];
    temp32.B08[1] = g_EmuConBuf[3];
    temp32.B08[2] = g_EmuConBuf[2];
    temp32.B08[3] = g_EmuConBuf[1];

    if ((g_EmuConBuf[1]&0x80) == 0x80) {
        temp32.B32 = (~temp32.B32) + 1;
    }
    //temp32.B32 = (double)temp32.B32 * 0.068212;
    temp32.B32 = (double)temp32.B32 * 0.13111;	
    info->u32Power = temp32.B32;

    /*功率大于最大有效值*/
    if (info->u32Power > MAX_VALID_POWER) {
        DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "power is invalid %u \r\n", info->u32Power);

        if (EmuInitial() == 1) {
            g_MeterInitStatus = METER_INIT_REG_FAIL;
        }

        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);    
        return 1; 
    }
		 
    /*read current*/
    g_EmuConBuf[0] = IBRMS;
    ret = EmuOP(SPIRead);
    if (ret) {
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);    
        DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
        return 1; // 1 --> fail, 0 success
    }

    temp32.B08[0] = g_EmuConBuf[3];
    temp32.B08[1] = g_EmuConBuf[2];
    temp32.B08[2] = g_EmuConBuf[1];
    temp32.B08[3] = 0;
    temp32.B32 = (double)temp32.B32 / (RO_B * 33554.432);
    info->u32Current = temp32.B32;

    /*read Voltage*/
    g_EmuConBuf[0] = URMS;
    ret = EmuOP(SPIRead);
    if (ret) {
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);    
        DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "read err %s:%d\n", __func__, __LINE__);
        return 1; // 1 --> fail, 0 success
    }

    temp32.B08[0] = g_EmuConBuf[3];  
    temp32.B08[1] = g_EmuConBuf[2];
    temp32.B08[2] = g_EmuConBuf[1];
    temp32.B08[3] = 0; 
    temp32.B32 = temp32.B32 * ((double)K_V / 1677721.6); 
    info->u32Voltage = temp32.B32;
    
    /*电压大于最大有效值*/
    if (info->u32Voltage > MAX_VALID_VOLTAGE) {
        DEBUG_LOG(DEBUG_MODULE_METER, DLL_ERROR, "volage is invalid %u \r\n", info->u32Voltage);

        if (EmuInitial() == 1) {
            g_MeterInitStatus = METER_INIT_REG_FAIL;
        }

        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);    
        return 1; // 1 --> fail, 0 success
    }
 
    /*calc power factor, factor = u32Power / (u32Voltage * u32Current)  */
    if (info->u32Voltage == 0 || info->u32Current == 0) {
        info->u32PowerFactor = 1000;
        
    } else {
        info->u32PowerFactor = 1000 * ((double)info->u32Power / (info->u32Voltage * info->u32Current)); // 将小数 *1000 转化成整数
        if (info->u32PowerFactor > 1000) {
            info->u32PowerFactor = 1000;
        }
    }

    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);    
    return 0;
}

static uint8_t EmuSPIOp(uint8_t u8OpMode ) 
{
    uint8_t  i, j, k, u8TempBuf[5];
    uint8_t  r, n, u8RegAddr, u8Recv;


    u8TempBuf[0] = 0x00;
    u8TempBuf[1] = 0x00;
    u8TempBuf[2] = 0x00;
    u8TempBuf[3] = 0x00;
    u8TempBuf[4] = 0x00;

    /*判断寄存器的字节数*/
    if (g_EmuConBuf[0] <= 0x0C) {
        r = 1 + 1;
    } else if ((g_EmuConBuf[0] >= 0x10) && (g_EmuConBuf[0] <= 0x2F)) { 
        r = 2 + 1; 
    } else if ((g_EmuConBuf[0] >= 0x30) && (g_EmuConBuf[0] <= 0x32)) {
        r = 3 + 1;
    } else if ((g_EmuConBuf[0] >= 0x40) && (g_EmuConBuf[0] <= 0x46)) {
        r = 4 + 1;
    } else if (g_EmuConBuf[0] == 0x50) {
        r = 3 + 1;
    } else if (g_EmuConBuf[0] == 0x70) {
        r = 1 + 1;
    } else if (g_EmuConBuf[0] == 0x71) {
        r = 2 + 1;
    } else if (g_EmuConBuf[0] == 0x72) {
        r = 4 + 1;
    } else if (g_EmuConBuf[0] == 0x73) {
        r = 4 + 1;
    } else if (g_EmuConBuf[0] == 0x7F) {
        r = 1 + 1;
    } else {
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "unkwon cmd \n");
        return 1;
    }

    if (u8OpMode) {
        u8RegAddr = 0x72;
        n = 4;
    } else {
        u8RegAddr = 0x73;
        n = 4;
    }

    for (k=0; k<3; k++) {     
        Drt6020CtrlCS(0);
        vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
    
        g_EmuConBuf[0] |= u8OpMode;
    
        if (!u8OpMode) { 
            //read
            for (i=1; i<r; i++) {
                g_EmuConBuf[i] = 0;
            }
            
            #if 0
            if (u8OpMode == 0) {
                DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "cmd : %02x\n", g_EmuConBuf[0]);    
	    }
            #endif
           
             
            if (SpiDrt6020SendByte(g_EmuConBuf[0]) == 1) {
                return 1;         
            }

            vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);

            for (j=1; j<r; j++) {
                if (SpiDrt6020ReadByte(&u8Recv) == 1) {
                    return 1;
                } 
                g_EmuConBuf[j] = u8Recv;

                vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
                //DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_INFO, "%02x\n", g_EmuConBuf[j]);
            }
        } else { 
            //write data
            for (j=0; j<r; j++) {
	            if (SpiDrt6020SendByte(g_EmuConBuf[j]) == 1) {
                    return 1;
                }
                vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
            }			    
        }
        //DEBUG_LOG(DEBUG_MODULE_METER, DLL_INFO, "\n over \n");
				
        vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);

        Drt6020CtrlCS(1);

        vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
				
        if (!u8OpMode) {  //read
            if (g_EmuConBuf[0] >= 0x70) {
                return 0;
            }
        } else { //write
            if (g_EmuConBuf[0] >= 0xF0) {
                return 0;
            }
        }
          
        Drt6020CtrlCS(0);
        vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
    
        *u8TempBuf = u8RegAddr;
				
	    if (SpiDrt6020SendByte(*u8TempBuf) == 1) {
            return 1;
        }

        vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);

        for (j=1; j<(uint8_t)(n+1); j++) {
            if (SpiDrt6020ReadByte(&u8Recv) == 1) {
                return 1;
            } 

            *(u8TempBuf + j) = u8Recv;
            vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
        }

	    Drt6020CtrlCS(1);
        vTaskDelay(SPI_RW_DELAY_TIME_MS / portTICK_RATE_MS);
        
        for (i=1; i<r; i++) {
            if (g_EmuConBuf[i]  != *(u8TempBuf + i + n - r + 1)) {
                break;
            }
        }
				
        if (i == r) {
            g_EmuConBuf[0] &= B0111_1111;					
            return 0;
        }   
    }	  
    // rw err
    return 1;
}

static uint8_t Drt6020SpiInit()
{
    esp_err_t ret;
    spi_bus_config_t buscfg={
        .miso_io_num=VSPI_PIN_NUM_MISO,
        .mosi_io_num=VSPI_PIN_NUM_MOSI,
        .sclk_io_num=VSPI_PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=3125,
        .mode=1, //CPHA = 0; CPOL
        .spics_io_num=-1,               //CS pin
        .queue_size=7,
    };

    //Initialize the SPI bus
    ret=spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    if (ret != ESP_OK) {
        return 1;
    }

    //Attach the drt6020 to the SPI bus
    ret=spi_bus_add_device(VSPI_HOST, &devcfg, &g_SpiHandle);

    if (ret != ESP_OK) {
       return 1;
    }

    return 0;
}

/*
 *@breif 计量功能初始化
 *@param 无
 *@return 0:初始化成功， 1：初始化失败
*/
uint8_t MeterInit(void)
{
    uint8_t u8Ret;

    /*g_MeterMutex = xSemaphoreCreateMutex();

    if (g_MeterMutex == NULL) {
        g_MeterInitStatus = METER_INIT_MUTEX_FAIL;
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "create g_MeterMutex fail \n");
        return 1;
    }*/
	MUTEX_CREATE(g_MeterMutex);
    //xSemaphoreTake(g_MeterMutex, portMAX_DELAY); 
    MUTEX_LOCK(g_MeterMutex, MUTEX_WAIT_ALWAYS);
    u8Ret = SpiCsGpioInit();
    if (u8Ret) {
        g_MeterInitStatus = METER_INIT_SPI_FAIL;
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);
		DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "init Meter spi cs pnerr\n");		
        return 1;
    }

    if (Drt6020SpiInit() != 0) {
        g_MeterInitStatus = METER_INIT_SPI_FAIL;
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "init Meter spi err\n");
        return 1;
    }

    Drt6020CtrlCS(1);


    if (EmuInitial() == 1) {
        g_MeterInitStatus = METER_INIT_REG_FAIL;
        //xSemaphoreGive(g_MeterMutex);
		MUTEX_UNLOCK(g_MeterMutex);
        DEBUG_LOG(DEBUG_MODULE_ELEC, DLL_ERROR, "init Meter reg err\n");
        return 1;
    }

    g_MeterInitStatus = METER_INIT_SUCCESS;
    //xSemaphoreGive(g_MeterMutex);
	MUTEX_UNLOCK(g_MeterMutex);
    return 0;
}


