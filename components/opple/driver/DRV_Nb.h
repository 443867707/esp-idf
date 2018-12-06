#ifndef _DRV_NB_H
#define _DRV_NB_H
#include "DRV_Uart.h"

#define NB_UART_NUM  UART_NUM_1
#define NB_UART_TXD  (GPIO_NUM_10)
#define NB_UART_RXD  (GPIO_NUM_9)

#define GPIO_NB_RST_PIN 33 
#define GPIO_NB_RST_PIN_SEL (1ULL<<GPIO_NB_RST_PIN)


uint8_t NbReset(void);
int NbUartWriteBytes(char *send_buffer, size_t size);
int NbUartReadBytes(uint8_t *u8RecvBuf, uint32_t u32RecvLen, uint32_t u32Timeout);
uint8_t NbUartConfig(uart_config_t *nb_uart_config);
uint8_t NbSetRstGpioLevel(uint8_t Level);
uint8_t NbUartInit(int rx_buffer_size, int tx_buffer_size);

#endif
