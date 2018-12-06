#ifndef _DRV_UART_H
#define _DRV_UART_H

#include "Includes.h"
#include "driver/uart.h"

uint8_t BspUartItEnable(uart_port_t uart_num);

uint8_t BspUartItDisable(uart_port_t uart_num);


int BspUartWriteBytes(uart_port_t uart_num, char* send_buffer, size_t size);

int BspUartReadBytes(uart_port_t uart_num, uint8_t* buf, uint32_t length, TickType_t ticks_to_wait);

uint8_t BspUartInit(uart_port_t uart_num, int rx_buffer_size, int tx_buff_size, int tx_io_num, int rx_io_num);

uint8_t BspUartConfig(uart_port_t uart_num, uart_config_t *uart_config);

#endif
