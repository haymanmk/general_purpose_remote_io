#include "stm32f7xx_remote_io.h"

// UART parameter handle
uart_handle_t uartHandles[UART_MAX];

static TaskHandle_t processRxTaskHandle;

// function prototypes
io_status_t uart_increment_rx_buffer_head(uart_handle_t *uart_handle);
io_status_t uart_increment_rx_buffer_tail(uart_handle_t *uart_handle);
void uart_process_rx_task(void *parameters);

// initialize UART
void uart_init()
{
    // enable UART receive interrupt from HAL library
    for (uint8_t i = 0; i < UART_MAX; i++)
    {
        uart_handle_t *uart_handle = &uartHandles[i];

        // check if UART handle is not NULL
        if (uart_handle->huart == NULL)
        {
            Error_Handler();
        }

        if (HAL_UART_Receive_IT(uart_handle->huart, (uint8_t*)(uart_handle->rx_buffer + uart_handle->rx_head), 1) != HAL_OK)
        {
            Error_Handler();
        }
    }
    // start rx task
    xTaskCreate(uart_process_rx_task, "UART RX Task", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY+1, &processRxTaskHandle);
}

// In order to make UART parameters become adjustable for users,
// this function is going to override the UART initialization process in the main.c file.
// All the settings for UART initialization will be extracted from the flash memory.
void uart_msp_init(uart_index_t uart_index, UART_HandleTypeDef *huart, uart_settings_t *uart_settings)
{
    huart->Init.BaudRate = uart_settings->baudrate;
    huart->Init.WordLength = uart_settings->data_bits;
    huart->Init.StopBits = uart_settings->stop_bits;
    huart->Init.Parity = uart_settings->parity;
    if (HAL_UART_Init(huart) != HAL_OK)
    {
        Error_Handler();
    }

    // update uart handle
    uart_handle_t *uart_handle = &uartHandles[uart_index];
    uart_handle->huart = huart;
}

void uart_process_rx_task(void *parameters)
{
    for (;;)
    {
        // wait for notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // get index of the UART
        uart_index_t uartIndex = GET_UART_INDEX_FROM_NEW_LINE();

        // get the UART handle
        uart_handle_t *uartHandle = &uartHandles[uartIndex];

        // record the volatile variable
        uint8_t head = uartHandle->rx_head;
        uint8_t tail = uartHandle->rx_tail;

        // print prefix beforehand; format: "R<Service ID>.<UART index> "
        api_printf("R%d.%d ", SERVICE_ID_SERIAL, uartIndex);

        // append the received data to the tx buffer
        while (head != tail)
        {
            if (api_append_to_tx_ring_buffer((char*)&(uartHandle->rx_buffer[tail]), 1) != STATUS_OK)
            {
                vLoggingPrintf("Failed to append to tx buffer\n");
            }

            if (uart_increment_rx_buffer_tail(uartHandle) != STATUS_OK)
            {
                vLoggingPrintf("Failed to increment rx buffer tail\n");
            }

            tail = uartHandle->rx_tail;
        }

        // decrement the new line flag
        uartHandle->is_new_line--;
    }
}

// increment rx buffer head and its pointer
io_status_t uart_increment_rx_buffer_head(uart_handle_t *uart_handle)
{
    UTILS_INCREMENT_BUFFER_HEAD(uart_handle->rx_head, uart_handle->rx_tail, UART_RX_BUFFER_SIZE);
}

// increment rx buffer tail
io_status_t uart_increment_rx_buffer_tail(uart_handle_t *uart_handle)
{
    UTILS_INCREMENT_BUFFER_TAIL(uart_handle->rx_tail, uart_handle->rx_head, UART_RX_BUFFER_SIZE);
}

io_status_t uart_printf(uart_index_t uart_index, const char *format_string, ...)
{
    // assert if uart index is valid
    if (uart_index >= UART_MAX)
    {
        return STATUS_ERROR;
    }

    char buffer[UART_TX_BUFFER_SIZE] = {'\0'};
    va_list args;
    va_start(args, format_string);
    vsnprintf(buffer, UART_TX_BUFFER_SIZE, format_string, args);
    va_end(args);

    uint8_t indexTerminator = 0;
    UTILS_SEARCH_FOR_END_OF_STRING(buffer, indexTerminator, '\0');

    UART_HandleTypeDef *huart = uartHandles[uart_index].huart;

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
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uart_handle_t *uartHandle = GET_UART_HANDLE(huart);

    if (uartHandle != NULL)
    {
        char chr = uartHandle->rx_buffer[uartHandle->rx_head];

        if (chr == '\n')
        {
            // set new line flag
            uartHandle->is_new_line++;
            // give notification to the UART task
            vTaskNotifyGiveFromISR(processRxTaskHandle, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

        if (uart_increment_rx_buffer_head(uartHandle) != STATUS_OK)
        {
            Error_Handler();
        }

        if (HAL_UART_Receive_IT(uartHandle->huart, (uint8_t*)(uartHandle->rx_buffer + uartHandle->rx_head), 1) != HAL_OK)
        {
            Error_Handler();
        }
    }
}