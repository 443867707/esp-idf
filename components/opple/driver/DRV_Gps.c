#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <OPP_Debug.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "DRV_Gps.h"
#include "DRV_Gpio.h"

static uint8_t GpsReceiveData(uint32_t u32Timeout);

//unsigned char Real_Process_Enabled = Valid;
unsigned char g_GpsSum = 0;  
unsigned char arryDhIdSep[32] = {0};

SemaphoreHandle_t g_GpsMutex = NULL;

typedef enum _GpsStatus {
    GPS_NO_INIT,
    GPS_RUNNING,
    GPS_NO_WORK,
} GpsStatus_t;

static GpsStatus_t g_GpsStatus = GPS_NO_INIT;

#if 0
struct _GPS_Information GPS_Information ={
	.Latitude = "3102.1883",
	.NS_Indicator = 'N',
	.Longitude = "12047.4652",
	.EW_Indicator = 'E'
};
#else
//huawei
struct _GPS_Information strGpsInformation ={0};
#endif

struct _GPS_Real_buf strGpsRealbuf;


/*
 *@brief  开启GPS uart 中断 
 *@param  none
 *@return 0: success, 1: fail 
*/
static uint8_t GpsItEnable()
{ 
   return BspUartItEnable(GPS_UART_NUM);
}

/*
 *@brief  关闭GPS uart 中断 
 *@param  none
 *@return 0: success, 1: fail 
*/
static uint8_t GpsItDisable()
{
   return BspUartItDisable(GPS_UART_NUM);
}

/*
 *@brief  GPS reset pin init
 *@param  none
 *@return 0: success, 1: fail 
*/
static uint8_t GpsResetGpioInit()
{
    esp_err_t err;

    gpio_config_t gpio_conf;
    gpio_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask = GPIO_GPS_RST_PIN_SEL;
    gpio_conf.pull_down_en = 0;
    gpio_conf.pull_up_en = 0;

    err = gpio_config(&gpio_conf);
    if (err != ESP_OK) {
       return 1; 
    }

    return BspGpioSetLevel(GPIO_GPS_RST_PIN, 0);
}

/*
 *@brief  set GPS reset pin output level
 *@param  u8Level:the output level of pin , 1 high, 0 low
 *@return 0: success, 1: fail 
*/
uint8_t GpsSetRstGpioLevel(uint8_t u8Level)
{
    if (u8Level != 0 && u8Level != 1) {
        return 1;
    }

    BspGpioSetLevel(GPIO_GPS_RST_PIN, u8Level);

    return 0;
}

/*
 *@brief  复位gps, 复位引脚高电平持续100ms
 *@param  无
 *@return 无
*/
uint8_t GpsReset()
{
    uint8_t u8Ret;

    u8Ret = BspGpioSetLevel(GPIO_GPS_RST_PIN, 1);
    if (u8Ret) {
        return 1;
    }

    vTaskDelay(100 / portTICK_RATE_MS);

    u8Ret = BspGpioSetLevel(GPIO_GPS_RST_PIN, 0);
    if (u8Ret) {
        return 1;
    }

    return 0;
}

/*
 *@brief 获取GPS位置信息。 
 *@param info: 存储获取到的GPS信息缓冲区的指针, u32Mstimeout: 本次获取等待的最大时间。
 *@return 0: 读取成功， 1: GPS 未初始化， 2：GPS 没有工作 3：获取互斥锁失败, 4: 为收到有效的数据。 
*/
uint8_t GpsLocationInfoGet(gps_location_info_t *info, uint32_t u32MsTimeout)
{
    int ret = 0;

#if 0
    if (g_GpsStatus != GPS_RUNNING) {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "GPS is not running %s:%d\n", __func__, __LINE__);
        return 1;
    }
#endif

    if (g_GpsStatus == GPS_NO_INIT) {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "GPS not init %s:%d \n", __func__, __LINE__);
        return 1;
    }

    if (g_GpsStatus == GPS_NO_WORK) {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "GPS not work %s:%d \n", __func__, __LINE__);
        return 2;
    }

    //if (xSemaphoreTake(g_GpsMutex, u32MsTimeout) == 0) {
    if (xSemaphoreTake(g_GpsMutex, portMAX_DELAY) == 0) {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "get g_GpsMutex timeout %s:%d \n", __func__, __LINE__);
        return 3;
    }


    ret = GpsReceiveData(u32MsTimeout);

    if(ret == 0) {
        memcpy( info->Latitude, strGpsInformation.Latitude, 10 );  
        info->NS_Indicator = strGpsInformation.NS_Indicator;	
        memcpy(info->Longitude, strGpsInformation.Longitude, 10 );  	
        info->EW_Indicator = strGpsInformation.EW_Indicator; 		
        xSemaphoreGive(g_GpsMutex);
        return 0;
    }

    xSemaphoreGive(g_GpsMutex);
    return 4;
}

/*
 *@brief 获取GPS时间信息。 
 *@param info: 存储获取到的GPS信息缓冲区的指针, u32Timeout :本次获取等待的最大时间。 
 *@return 0: 读取成功， 1: GPS 未初始化， 2：GPS 没有工作 3：获取互斥锁失败, 4: 为收到有效的数据。 
*/
uint8_t GpsTimeInfoGet(gps_time_info_t *info, uint32_t u32Timeout)
{
    int ret = 0;

#if 0
    if (g_GpsStatus != GPS_RUNNING) {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "GPS is not running %s:%d\n", __func__, __LINE__);
        return 1;
    }
#endif

    if (g_GpsStatus == GPS_NO_INIT) {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "GPS not init");
        return 1;
    }

    if (g_GpsStatus == GPS_NO_WORK) {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "GPS no work");
        return 2;
    }

    if (xSemaphoreTake(g_GpsMutex, portMAX_DELAY) == 0) {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "get g_GpsMutex timeout %s:%d \n", __func__, __LINE__);
        return 3;
    }


    ret = GpsReceiveData(u32Timeout);

    if(ret == 0) {
        memcpy(info->UTC_Time, strGpsInformation.UTC_Time, 6); 
        memcpy(info->UTC_Date, strGpsInformation.UTC_Date, 6);
        xSemaphoreGive(g_GpsMutex);
        return 0;
    }

    xSemaphoreGive(g_GpsMutex);
    return 4;
}

char* RealProcessDh(char* buffer, unsigned char num)  
{  
    if (num < 1)  {
        return  &buffer[0];  
    }
    return  &buffer[arryDhIdSep[num - 1] + 1];  
}  

void CreatDhIndex(char* buffer)  
{  
    unsigned short i, len;  
    unsigned char idj;  
  
    memset(arryDhIdSep, 0, sizeof(arryDhIdSep));  
    len = strlen( buffer );  
    for (i = 0, idj = 0; i < len; i++) {  
        if (buffer[i] == ',')  {  
            arryDhIdSep[idj] = i;  
            idj++;  
            buffer[i] = 0x00;  
        }  
    }  
}  

uint8_t RealGpsCommandProcess(void)  
{  
    char* temp;  
    unsigned char i, j, zhen;  
  
    if (strstr(strGpsRealbuf.data, "GPGGA")) {  
        strGpsInformation.Use_EPH_Sum = atof(RealProcessDh(strGpsRealbuf.data, 7));
        strGpsInformation.MSL_Altitude = atof(RealProcessDh(strGpsRealbuf.data, 9));
        return 1;  
    }  
  
    if (strstr(strGpsRealbuf.data, "GPGSA")) {  
        temp = RealProcessDh(strGpsRealbuf.data, 2);
        if ((*temp == '2') || (*temp == '3')) {  
            strGpsInformation.Locate_Mode = *temp;  
        } else { 
            strGpsInformation.Locate_Mode = Invalid;  
        }

        for ( i = 0; i < 12; i++ ) {  
            strGpsInformation.User_EPH[i] = atof(RealProcessDh(strGpsRealbuf.data, i + 3));
        }  
      
        strGpsInformation.PDOP = atof(RealProcessDh(strGpsRealbuf.data, 15)); 
        strGpsInformation.HDOP = atof(RealProcessDh(strGpsRealbuf.data, 16));
        strGpsInformation.VDOP = atof(RealProcessDh(strGpsRealbuf.data, 17));
      
        return 1;  
    }  
  
    if (strstr(strGpsRealbuf.data, "GPRMC")) {	
        temp = RealProcessDh(strGpsRealbuf.data, 1);
        if (*temp != 0) {
            memcpy(strGpsInformation.UTC_Time, temp, 6);  
            strGpsInformation.UTC_Time[6] = '\0';
	} else {
            return 1;
        }
  
        if ( *(RealProcessDh(strGpsRealbuf.data, 2)) == 'A') {  
            strGpsInformation.Real_Locate = Valid;
            strGpsInformation.Located = Valid;
        } else {  
            strGpsInformation.Real_Locate = Invalid;
            return 1;
        }  
      
        temp = RealProcessDh(strGpsRealbuf.data, 3);
        if ((*temp >= 0x31) && (*temp <= 0x39)) {  
            memcpy(strGpsInformation.Latitude, temp, 9);  
            strGpsInformation.Latitude[9] = 0;  
        } else {  
            strGpsInformation.Latitude[0] = '0';  
            strGpsInformation.Latitude[1] = 0;  
            return 1;
        }  
  
        strGpsInformation.NS_Indicator = *(RealProcessDh(strGpsRealbuf.data, 4));
      
        temp = RealProcessDh(strGpsRealbuf.data, 5);
        if ((*temp >= 0x31) && (*temp <= 0x39)) {   
            memcpy(strGpsInformation.Longitude, temp, 10);  	
        } else {  
            strGpsInformation.Longitude[0] = '0';  
            strGpsInformation.Longitude[1] = 0;  
            return 1;
        }
      
        strGpsInformation.EW_Indicator = *(RealProcessDh(strGpsRealbuf.data, 6));
        strGpsInformation.Speed = atof(RealProcessDh(strGpsRealbuf.data, 7));
        strGpsInformation.Course = atof(RealProcessDh(strGpsRealbuf.data, 8));
      
        temp = RealProcessDh(strGpsRealbuf.data, 9);
        if (*temp != 0) {  
            memcpy(strGpsInformation.UTC_Date, temp, 6);  
            return 0;
        }
    }  
  
    if (strstr(strGpsRealbuf.data, "GPGSV")) {  
        zhen = atof(RealProcessDh(strGpsRealbuf.data, 2));
        if ((zhen <= 3) && (zhen != 0)) {  
            for (i = (zhen - 1) * 4, j = 4; i < zhen * 4; i++) {
                strGpsInformation.EPH_State[i][0] = atof(RealProcessDh(strGpsRealbuf.data, j++));
                strGpsInformation.EPH_State[i][1] = atof(RealProcessDh(strGpsRealbuf.data, j++ ));
                strGpsInformation.EPH_State[i][2] = atof(RealProcessDh(strGpsRealbuf.data, j++));
                strGpsInformation.EPH_State[i][3] = atof(RealProcessDh(strGpsRealbuf.data, j++));
            }  
        }  
        return 1;  
    }

	return 1;
}  

unsigned char CalcGpsSum(const char* Buffer)  
{  
    unsigned char i, j, k, sum;  
    sum = 0;  
  
    for ( i = 1; i < 255; i++ ) {  
        if ((Buffer[i] != '*') && (Buffer[i] != 0x00)) {  
            sum ^= Buffer[i];
        } else {  
            break;  
        }  
    }  
    j = Buffer[i + 1];
    k = Buffer[i + 2];  
  
    if (isalpha( j )) {  
        if (isupper( j )) {  
            j -= 0x37;
        } else {  
            j -= 0x57;
        }  
    } else {  
        if ((j >= 0x30) && (j <= 0x39)) {  
            j -= 0x30;
        }  
    }  
  
    if (isalpha( k )) {  
        if (isupper( k )) {  
            k -= 0x37;
        } else {  
            k -= 0x57;
        }  
    } else {  
        if ((k >= 0x30) && (k <= 0x39)) {  
            k -= 0x30;
        }  
    }  
  
    j = ( j << 4 ) + k;
    g_GpsSum = j;  
  
    if (sum == j) {  
        return Valid;
    } else {  
        return Invalid;
    }  
} 


static uint8_t GpsReceiveData(uint32_t u32Timeout)
{
    uint8_t u8Ret, u8Recv;
    int flag = 0;
    uint32_t u32StartTick, u32CurTick, u32TimeoutTick; 

    u8Ret = GpsItEnable();
    if (u8Ret) {
        return 1;
    }

    u32StartTick = xTaskGetTickCount();
    u32TimeoutTick = u32Timeout / portTICK_RATE_MS;

    while (flag != 1) {
        u32CurTick = xTaskGetTickCount();
        if (u32CurTick - u32StartTick > u32TimeoutTick) {
            return 1;
        }
        
        GpsUartReadBytes(&u8Recv, 1, u32Timeout);

        if (u8Recv == '$') {  
            strGpsRealbuf.rx_pc = 0;  
        } else {  
            if (strGpsRealbuf.rx_pc < sizeof(strGpsRealbuf.data) - 1) {
                strGpsRealbuf.rx_pc++;  
            }  
        } 
	 
        strGpsRealbuf.data[strGpsRealbuf.rx_pc] = u8Recv; 
        //DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "%c", u8Recv);
			
        if (u8Recv == '\r') {  
            //DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "\n"); // "\\n"
            CreatDhIndex( strGpsRealbuf.data );  
            u8Ret = RealGpsCommandProcess();		
	        memset(strGpsRealbuf.data, 0, sizeof(strGpsRealbuf.data));
	        strGpsRealbuf.rx_pc = 0;
            if (u8Ret == 0) {
                uart_flush_input(GPS_UART_NUM);
                GpsItDisable();
                return 0;
            }  
        } 
    }
	return 1;
}

int GpsUartReadBytes(uint8_t *u8RecvBuf, uint32_t u32RecvLen, uint32_t u32Timeout) {
    return BspUartReadBytes(GPS_UART_NUM, u8RecvBuf, u32RecvLen, u32Timeout / portTICK_RATE_MS);
}

uint8_t GpsUartConfig(uart_config_t *gps_uart_config) 
{
    return BspUartConfig(GPS_UART_NUM, gps_uart_config);
}


/*
 *@brief check if Gps is in work
 *@param none
 *@return 0: in work, 1: not in work
 *
*/
static uint8_t CheckGpsIsWork()
{
    uint8_t u8Recv, u8RecvLen, u8Ret;
    esp_err_t err;

    err = uart_flush_input(GPS_UART_NUM);
    if (err != ESP_OK) {
        return 1;
    }

    u8Ret = GpsItEnable();
    if (u8Ret) {
        return 1;
    }

    u8RecvLen = GpsUartReadBytes(&u8Recv, 1, 1000);
    GpsItDisable();

    if (u8RecvLen == 1) {
        g_GpsStatus = GPS_RUNNING;
        u8Ret = 0;
    } else {
        g_GpsStatus = GPS_NO_WORK;
        u8Ret = 1 ;
    }

    return u8Ret;
}

/*
 *@brief bsp gps function test
 *@param none
 *@return 1: get mutex err, 0: sucess, 2: fail
*/
uint8_t GpsTest()
{
    uint8_t u8Ret;

    if (xSemaphoreTake(g_GpsMutex, portMAX_DELAY) == 0) {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "get g_GpsMutex timeout %s:%d \n", __func__, __LINE__);
        return 1;
    }

    u8Ret = CheckGpsIsWork();
	if(u8Ret == 1) {
        xSemaphoreGive(g_GpsMutex);
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "gps not work \n");
        return 2;
    } 

    xSemaphoreGive(g_GpsMutex);
	return 0;
}

/*
 *@brief reset gps for fct test
 *@param none
 *@retturn 0:success, 1:fail
 */
uint8_t L80Reset(void)
{
	int ret;

    if (xSemaphoreTake(g_GpsMutex, portMAX_DELAY) == 0) {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, "get g_GpsMutex timeout %s:%d \n", __func__, __LINE__);
        return 1;
    }
	
	GpsSetRstGpioLevel(1);
	vTaskDelay(200 / portTICK_RATE_MS);// rst gpio high level keep more than 100ms

    ret = CheckGpsIsWork();
	if(ret == 0) {
        /*reseting : should be no work, reset fail*/
        xSemaphoreGive(g_GpsMutex);
        return 1;
    }
	
	GpsSetRstGpioLevel(0);
	vTaskDelay(200 / portTICK_RATE_MS);

    ret = CheckGpsIsWork();
	if(ret != 0) {
        xSemaphoreGive(g_GpsMutex);
        return 1;
    }

    xSemaphoreGive(g_GpsMutex);
    
    return 0;
}

/*
 *@breif  GPS init
 *@param  NONE
 *@return 0: success, 1 create mutex fail, 2 Gps device not found
*/
uint8_t GpsUartInit()
{
    uint8_t u8Ret;

    g_GpsMutex = xSemaphoreCreateMutex();

    if (g_GpsMutex == NULL) {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_ERROR, "create g_GpsMutex fail \n");
        return 1;
    }

    xSemaphoreTake(g_GpsMutex, portMAX_DELAY);

    GpsResetGpioInit();
    BspUartInit(GPS_UART_NUM, 256, 0, GPS_UART_TXD, GPS_UART_RXD);

    if (CheckGpsIsWork() == 0) {
        g_GpsStatus = GPS_RUNNING;
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_INFO, " gps is running \n");
        u8Ret = 0;
    } else {
        DEBUG_LOG(DEBUG_MODULE_GPS, DLL_ERROR, "gps is no work\n");
        g_GpsStatus = GPS_NO_WORK;
        u8Ret = 2;
    }

    xSemaphoreGive(g_GpsMutex);
    return u8Ret;
}
