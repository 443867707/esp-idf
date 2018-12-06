#include "DRV_Uart.h"
#include "esp_err.h"

uint8_t BspUartItEnable(uart_port_t uart_num)
{
    esp_err_t err;

    err = uart_enable_rx_intr(uart_num);
    if (err != ESP_OK) {
        return 1;
    }

	return 0;
}

uint8_t BspUartItDisable(uart_port_t uart_num)
{
    esp_err_t err;

    err = uart_disable_rx_intr(uart_num);
    if (err != ESP_OK) {
        return 1;
    }

	return 0;
}

int BspUartWriteBytes(uart_port_t uart_num, char* send_buffer, size_t size)
{
    if (send_buffer) {
        return -1;
    }

    return uart_write_bytes(uart_num, send_buffer, size);
}

int BspUartReadBytes(uart_port_t uart_num, uint8_t* buf, uint32_t length, TickType_t ticks_to_wait)
{
    if (buf == NULL) {
        return -1;
    }

   	return uart_read_bytes(uart_num, buf, length, ticks_to_wait);
}

uint8_t BspUartConfig(uart_port_t uart_num, uart_config_t *uart_config)
{
    esp_err_t err;

    if (uart_config == NULL) {
        return 1;
    }

    err = uart_param_config(uart_num, uart_config);
    if (err != ESP_OK) {
        return 1;
    }

    return 0;
}

uint8_t BspUartInit(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int tx_io_num, int rx_io_num)
{
    esp_err_t err;

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    err = uart_param_config(uart_num, &uart_config);
    if (err != ESP_OK) {
        return -1;
    }
    
    err = uart_set_pin(uart_num, tx_io_num, rx_io_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        return -1;
    }
    
    err = uart_driver_install(uart_num,  rx_buffer_size, tx_buffer_size, 0, NULL, 0);
    if (err != ESP_OK) {
        return -1;
    }

    return 0;
}
