#ifndef __UART_H
#define __UART_H

#define UART_RX_BUFFER_SIZE 128
#define UART_TX_BUFFER_SIZE 64

/* Macros */

/* Function prototypes */
void uart_init();
void uart_msp_init(UART_HandleTypeDef *huart);
io_status_t uart_printf(uint8_t ch, const char *format_string, ...);

#endif