#ifndef _LAMP_GPS_UART_TEST_H
#define _LAMP_GPS_UART_TEST_H

#include "DRV_Uart.h"

#define GPS_UART_NUM  UART_NUM_2
#define GPS_UART_TXD  (GPIO_NUM_17)
#define GPS_UART_RXD  (GPIO_NUM_16)

#define GPIO_GPS_RST_PIN 21 
#define GPIO_GPS_RST_PIN_SEL (1ULL<<GPIO_GPS_RST_PIN)

#define Invalid    0
#define Valid      1

struct _GPS_Real_buf  
{  
    char data[256];
    volatile unsigned short rx_pc;
} ;
  
struct _GPS_Information  
{  
    unsigned char Real_Locate;
    unsigned char Located;   
    unsigned char Locate_Mode;
    char UTC_Time[7];        
    char UTC_Date[7];       
    char Latitude[10];     
    char NS_Indicator;    
    char Longitude[11];  
    char EW_Indicator;  
    double Speed;      
    double Course;    
    double PDOP;     
    double HDOP;    
    double VDOP;   
    double MSL_Altitude;
    unsigned char Use_EPH_Sum;
    unsigned char User_EPH[12]; 
    unsigned short EPH_State[12][4];
} ; 

typedef struct _gps_location_info {
    char Latitude[10];       
    char NS_Indicator;      
    char Longitude[11];    
    char EW_Indicator;   
} gps_location_info_t;

typedef struct _gps_time_info {
    char UTC_Time[7];
    char UTC_Date[7];
} gps_time_info_t;


uint8_t GpsUartInit(void);
uint8_t GpsUartConfig(uart_config_t *nb_uart_config);
uint8_t GpsLocationInfoGet(gps_location_info_t *info, uint32_t u32MsTimout);
uint8_t GpsTimeInfoGet(gps_time_info_t *info, uint32_t u32MsTimout);
int ddmm2dd(const char *ddmm, char *dd);
int GpsUartReadBytes(uint8_t *u8RecvBuf, uint32_t u32RecvLen, uint32_t u32Timeout);
uint8_t GpsSetRstGpioLevel(uint8_t Level);
uint8_t L80Reset(void);
uint8_t GpsTest(void);
#endif
