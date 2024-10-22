#include "stm32f7xx_remote_io.h"

Socket_t apiSocket;

// API task handle
TaskHandle_t apiTaskHandle;

// ring buffer for received data
uint8_t rxBuffer[API_RX_BUFFER_SIZE];
uint8_t rxBufferHead = 0;
uint8_t rxBufferTail = 0;

// ring buffer for sending data
uint8_t txBuffer[API_TX_BUFFER_SIZE];
uint8_t txBufferHead = 0;
uint8_t txBufferTail = 0;

// initialize API task
void api_init(Socket_t socket)
{
    // initialize socket
    apiSocket = socket;
}

void api_task(void *parameters)
{
    // process received data stored in buffer
    for (BaseType_t i = 0; i < len; i++)
    {
        // read data from buffer
        char charReceived = rx_data[i];

        if (charReceived == '\n'
            || charReceived == '\r'
            || charReceived <= ' ') 
            {

            }
        else continue;

        switch (charReceived)
        {
        case 'W': // write data
            /* code */
            break;
        case 'R': // read data
            /* code */
            break;
        default:
            break;
        }
    }
}

// Append received data to buffer
io_status_t api_append_data(uint8_t *data, BaseType_t len)
{
    // append data to buffer
    for (uint8_t i = 0; i < len; i++)
    {
        // check if buffer is full
        if (api_is_rx_buffer_full() == STATUS_OK) return STATUS_FAIL;

        // append data to buffer
        rxBuffer[rxBufferHead] = data[i];
        api_increment_rx_buffer_head();
    }

    return STATUS_OK;
}

// increment rx buffer head
io_status_t api_increment_rx_buffer_head()
{
    API_INCREMENT_BUFFER_HEAD(rxBufferHead, rxBufferTail, API_RX_BUFFER_SIZE);
}

// increment rx buffer tail
io_status_t api_increment_rx_buffer_tail()
{
    API_INCREMENT_BUFFER_TAIL(rxBufferTail, rxBufferHead, API_RX_BUFFER_SIZE);
}

// check if rx buffer is empty
io_status_t api_is_rx_buffer_empty()
{
    return (rxBufferHead == rxBufferTail) ? STATUS_OK : STATUS_FAIL;
}

// check if rx buffer is full
io_status_t api_is_rx_buffer_full()
{
    return (((rxBufferHead + 1) % API_RX_BUFFER_SIZE) == rxBufferTail) ? STATUS_OK : STATUS_FAIL;
}

// increment tx buffer head
io_status_t api_increment_tx_buffer_head()
{
    API_INCREMENT_BUFFER_HEAD(txBufferHead, txBufferTail, API_TX_BUFFER_SIZE);
}

// increment tx buffer tail
io_status_t api_increment_tx_buffer_tail()
{
    API_INCREMENT_BUFFER_TAIL(txBufferTail, txBufferHead, API_TX_BUFFER_SIZE);
}

// check if tx buffer is empty
io_status_t api_is_tx_buffer_empty()
{
    return (txBufferHead == txBufferTail) ? STATUS_OK : STATUS_FAIL;
}

// check if tx buffer is full
io_status_t api_is_tx_buffer_full()
{
    return (((txBufferHead + 1) % API_TX_BUFFER_SIZE) == txBufferTail) ? STATUS_OK : STATUS_FAIL;
}