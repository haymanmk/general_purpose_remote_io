#include "stm32f7xx_remote_io.h"

// ring buffer for received data
static char rxBuffer[UART_RX_BUFFER_SIZE];
static uint8_t rxBufferHead = 0;
static uint8_t rxBufferTail = 0;

static UART_HandleTypeDef* huart_1;

static TaskHandle_t processRxTaskHandle;

// function prototypes
io_status_t uart_increment_rx_buffer_head();
io_status_t uart_increment_rx_buffer_tail();
// io_status_t uart_increment_tx_buffer_head();
// io_status_t uart_increment_tx_buffer_tail();
void uart_process_rx_task(void *parameters);

// initialize UART
void uart_init()
{
    // check if UART handle is not NULL
    if (huart_1 == NULL)
    {
        Error_Handler();
    }

    // enable UART receive interrupt from HAL library
    if (HAL_UART_Receive_IT(huart_1, (uint8_t*)(rxBuffer+rxBufferHead), 1) != HAL_OK)
    {
        Error_Handler();
    }
    // start rx task
    xTaskCreate(uart_process_rx_task, "UART RX Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &processRxTaskHandle);
}

// In order to make UART parameters become adjustable for users,
// this function is going to override the UART initialization process in the main.c file.
// All the settings for UART initialization will be extracted from the flash memory.
void uart_msp_init(UART_HandleTypeDef *huart)
{
    huart->Init.BaudRate = settings.uart_1.baudrate;
    huart->Init.WordLength = settings.uart_1.data_bits;
    huart->Init.StopBits = settings.uart_1.stop_bits;
    huart->Init.Parity = settings.uart_1.parity;
    if (HAL_UART_Init(huart) != HAL_OK)
    {
        Error_Handler();
    }
    // record the UART handle
    huart_1 = huart;
}

void uart_process_rx_task(void *parameters)
{
    for (;;)
    {
        // wait for notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // process the received data
        uint8_t head = rxBufferHead; // record the volatile variable

        // print prefix beforehand
        api_printf("R%d ", SERVICE_ID_SERIAL);

        // append the received data to the tx buffer
        while (head != rxBufferTail)
        {
            if (api_append_to_tx_ring_buffer(&rxBuffer[rxBufferTail], 1) != STATUS_OK)
            {
                Error_Handler();
            }

            if (uart_increment_rx_buffer_tail() != STATUS_OK)
            {
                Error_Handler();
            }
        }
    }
}

// increment rx buffer head and its pointer
io_status_t uart_increment_rx_buffer_head()
{
    UTILS_INCREMENT_BUFFER_HEAD(rxBufferHead, rxBufferTail, UART_RX_BUFFER_SIZE);
}

// increment rx buffer tail
io_status_t uart_increment_rx_buffer_tail()
{
    UTILS_INCREMENT_BUFFER_TAIL(rxBufferTail, rxBufferHead, UART_RX_BUFFER_SIZE);
}

io_status_t uart_printf(uint8_t ch, const char *format_string, ...)
{
    char buffer[UART_TX_BUFFER_SIZE] = {'\0'};
    va_list args;
    va_start(args, format_string);
    vsnprintf(buffer, UART_TX_BUFFER_SIZE, format_string, args);
    va_end(args);

    uint8_t indexTerminator = 0;
    UTILS_SEARCH_FOR_END_OF_STRING(buffer, indexTerminator, '\0');

    UART_HandleTypeDef *huart = NULL;
    switch (ch)
    {
    case CHANNEL_1:
        huart = huart_1;
        break;
    default:
        return STATUS_ERROR;
    }

    // transmit the data
    if (HAL_UART_Transmit(huart, (uint8_t *)buffer, indexTerminator, HAL_MAX_DELAY) != HAL_OK)
    {
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

// UART receive interrupt callback
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == huart_1->Instance)
  {
    char chr = rxBuffer[rxBufferHead];

    if (chr == '\n')
    {
        // give notification to the UART task
        xTaskNotifyGive(processRxTaskHandle);
    }

    if (uart_increment_rx_buffer_head() != STATUS_OK)
    {
        Error_Handler();
    }

    HAL_UART_Receive_IT(huart, (uint8_t*)(rxBuffer+rxBufferHead), 1);
  }
}