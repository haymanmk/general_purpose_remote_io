#include "stm32f7xx_remote_io.h"

// function prototypes
void api_task(void *parameters);
token_t* api_create_token();
void api_free_tokens(token_t* token);
void api_reset_command_line();
io_status_t api_process_data();
void api_execute_command();
void api_error(uint16_t error_code);

// API task handle
TaskHandle_t apiTaskHandle;
extern TaskHandle_t processTxTaskHandle;

// ring buffer for received data
static char rxBuffer[API_RX_BUFFER_SIZE];
static uint8_t rxBufferHead = 0;
static uint8_t rxBufferTail = 0;

// ring buffer for sending data
static char txBuffer[API_TX_BUFFER_SIZE];
static uint8_t txBufferHead = 0;
static uint8_t txBufferTail = 0;

static command_line_t commandLine = {0}; // store command line data
static char paramBuffer[UART_TX_BUFFER_SIZE] = {'\0'}; // store data for ANY type
token_t* lastToken = NULL;               // store the last token

SemaphoreHandle_t apiAppendTxSemaphoreHandle = NULL;

// initialize API task
void api_init()
{
    // create semacphore for appending data to tx buffer
    apiAppendTxSemaphoreHandle = xSemaphoreCreateBinary();

    // give semaphore
    xSemaphoreGive(apiAppendTxSemaphoreHandle);

    // initialize api task
    xTaskCreate(api_task, "API Task", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY+1, &apiTaskHandle);
}

void api_task(void *parameters)
{
    for (;;)
    {
        // check if rx buffer is empty
        if (api_is_rx_buffer_empty() == STATUS_OK)
        {
            // wait for new data
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }

        if (api_process_data() == STATUS_OK)
        {
            // execute the command
            api_execute_command();
        }
        else
        {
            while (rxBufferHead != rxBufferTail)
            {
                char chr = rxBuffer[rxBufferTail];
                // increment rx buffer tail
                api_increment_rx_buffer_tail();
                if (chr == '\n' || chr == '\r')
                {
                    break;
                }
            }
        }

        // clear the command line
        commandLine.type = 0;
        commandLine.id = 0;
        commandLine.variant = 0;
        // free linked list of tokens
        api_free_tokens(commandLine.token);
        commandLine.token = NULL;
        lastToken = NULL;
    }
}

// Append received data to buffer
io_status_t api_append_to_rx_ring_buffer(char *data, BaseType_t len)
{
    // append data to buffer
    for (uint8_t i = 0; i < len; i++)
    {
        // check if buffer is full
        if (api_is_rx_buffer_full() == STATUS_OK)
            return STATUS_FAIL;

        // append data to buffer
        rxBuffer[rxBufferHead] = data[i];
        api_increment_rx_buffer_head();
    }

    return STATUS_OK;
}

// Append data to tx buffer with specified length
io_status_t api_append_to_tx_ring_buffer(char *data, BaseType_t len)
{
    if (processTxTaskHandle == NULL)
    {
        return STATUS_FAIL;
    }

    // check if semaphore is created
    if (apiAppendTxSemaphoreHandle == NULL)
    {
        return STATUS_FAIL;
    }

    // take semaphore
    if (xSemaphoreTake(apiAppendTxSemaphoreHandle, portMAX_DELAY) != pdTRUE)
    {
        return STATUS_FAIL;
    }

    // append data to buffer
    for (uint8_t i = 0; i < len; i++)
    {
        // check if buffer is full
        if (api_is_tx_buffer_full() == STATUS_OK)
        {
            // give semaphore
            xSemaphoreGive(apiAppendTxSemaphoreHandle);
            return STATUS_FAIL;
        }

        // append data to buffer
        txBuffer[txBufferHead] = data[i];
        api_increment_tx_buffer_head();

        // notify tx task that there are new data to be sent
        xTaskNotifyGive(processTxTaskHandle);
    }

    // give semaphore
    xSemaphoreGive(apiAppendTxSemaphoreHandle);
    return STATUS_OK;
}

// Append data to buffer until terminator is met
io_status_t api_append_to_tx_ring_buffer_until_term(char *data, char terminator)
{
    if (processTxTaskHandle == NULL)
    {
        return STATUS_FAIL;
    }

    // check if semaphore is created
    if (apiAppendTxSemaphoreHandle == NULL)
    {
        return STATUS_FAIL;
    }

    // take semaphore
    if (xSemaphoreTake(apiAppendTxSemaphoreHandle, portMAX_DELAY) != pdTRUE)
    {
        return STATUS_FAIL;
    }

    // append data to buffer
    uint8_t i = 0;
    char chr = data[i];
    for (; chr != terminator; chr = data[++i])
    {
        // check if buffer is full
        if (api_is_tx_buffer_full() == STATUS_OK)
        {
            // give semaphore
            xSemaphoreGive(apiAppendTxSemaphoreHandle);
            return STATUS_FAIL;
        }

        // append data to buffer
        txBuffer[txBufferHead] = chr;
        api_increment_tx_buffer_head();

        // notify tx task that there are new data to be sent
        xTaskNotifyGive(processTxTaskHandle);
    }

    // give semaphore
    xSemaphoreGive(apiAppendTxSemaphoreHandle);

    return STATUS_OK;
}

// increment rx buffer head
io_status_t api_increment_rx_buffer_head()
{
    UTILS_INCREMENT_BUFFER_HEAD(rxBufferHead, rxBufferTail, API_RX_BUFFER_SIZE);
}

// increment rx buffer tail
io_status_t api_increment_rx_buffer_tail()
{
    UTILS_INCREMENT_BUFFER_TAIL(rxBufferTail, rxBufferHead, API_RX_BUFFER_SIZE);
}

// increment rx buffer tail or wait for new data
void api_increment_rx_buffer_tail_or_wait()
{
    uint8_t nextTail = (rxBufferTail + 1) % API_RX_BUFFER_SIZE;
    if (nextTail == rxBufferHead)
    {
        // wait for new data
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    // increment rx buffer tail
    rxBufferTail = nextTail;
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
    UTILS_INCREMENT_BUFFER_HEAD(txBufferHead, txBufferTail, API_TX_BUFFER_SIZE);
}

// increment tx buffer tail
io_status_t api_increment_tx_buffer_tail()
{
    UTILS_INCREMENT_BUFFER_TAIL(txBufferTail, txBufferHead, API_TX_BUFFER_SIZE);
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

// pop data from tx buffer
char api_pop_tx_buffer()
{
    char chr = txBuffer[txBufferTail];
    if (api_increment_tx_buffer_tail() != STATUS_OK)
    {
        return '\0';
    }
    return chr;
}

// functions for tokenizing data
// create token
token_t* api_create_token()
{
    // create token
    token_t* token = (token_t*)pvPortMalloc(sizeof(token_t));
    if (token == NULL)
    {
        api_error(API_ERROR_CODE_FAIL_ALLOCATE_MEMORY_FOR_TOKEN);
        return NULL;
    }

    // initialize token
    token->type = 0;
    token->value_type = 0;
    token->any = NULL;
    token->next = NULL;

    return token;
}

// free linked list of tokens
void api_free_tokens(token_t* token)
{
    token_t* next = NULL;
    while (token != NULL)
    {
        next = token->next;
        token->next = NULL;
        vPortFree(token);
        token = next;
    }
}

// reset command line
void api_reset_command_line()
{
    memset(&commandLine, 0, sizeof(command_line_t));
    lastToken = NULL;
}

// parse received data
io_status_t api_process_data()
{
    char chr = rxBuffer[rxBufferTail];          // current character
    char param_str[MAX_INT_DIGITS+2] = {'\0'};
    bool isVariant = false;                     // check if there is a variant for the command
    bool isLengthRequired = false;              // check if length is required for the command

    // [Commnad Type]
    // check if command type is valid
    switch (chr)
    {
    case 'W': case 'w': // write data
        commandLine.type = 'W';
        break;
    case 'R': case 'r': // read data
        commandLine.type = 'R';
        break;

    case '\n': case '\r': // end of command
        return STATUS_FAIL;

    default:
        api_error(API_ERROR_CODE_INVALID_COMMAND_TYPE);
        return STATUS_FAIL;
    }

    // increment rx buffer tail
    api_increment_rx_buffer_tail_or_wait();

    // [ID]
    while (rxBufferHead != rxBufferTail)
    {
        chr = rxBuffer[rxBufferTail];

        if (chr >= '0' && chr <= '9')
        {
            if (isVariant)
            {
                commandLine.variant = commandLine.variant * 10 + (chr - '0');
            }
            else
            {
                commandLine.id = commandLine.id * 10 + (chr - '0');
            }
        }
        else if ((chr == '.') && (commandLine.id != 0))
        {
            if (!isVariant) isVariant = true;
            else
            {
                api_error(API_ERROR_CODE_INVALID_COMMAND_VARIANT);
                return STATUS_FAIL;
            }
        }
        else if ((chr == ' ') && (commandLine.id != 0))
        {
            if (isVariant && commandLine.variant == 0) isVariant = false;
            //remove all the blank spaces when processing the command
            API_REMOVE_BLANK_SPACES();
            // proceed to next parsing step
            break;
        }
        else if (chr == '\n' || chr == '\r')
        {
            // end of command line
            return STATUS_OK;
        }
        else
        {
            api_error(API_ERROR_CODE_INVALID_COMMAND_ID);
            return STATUS_FAIL;
        }

        // increment rx buffer tail
        api_increment_rx_buffer_tail_or_wait();
    }

    // [Param]
    token_t* token = api_create_token();

    // check if length parameter is required for the command
    switch (commandLine.id)
    {
    case SERVICE_ID_SERIAL:
        isLengthRequired = true;
        break;
    }

    // check if first character is a digit
    chr = rxBuffer[rxBufferTail];
    if (!isdigit(chr))
    {
        api_error(API_ERROR_CODE_INVALID_COMMAND_PARAMETER);
        api_free_tokens(token);
        return STATUS_FAIL;
    }
    else
    {
        if (token == NULL)
        {
            return STATUS_FAIL;
        }

        // check if length is required for the command
        if (isLengthRequired)
        {
            token->type = TOKEN_TYPE_LENGTH;
        }
        else
        {
            token->type = TOKEN_TYPE_PARAM;
        }

        token->value_type = PARAM_TYPE_UINT32;

        // add token to the linked list
        commandLine.token = token;
        lastToken = token;
    }

    uint8_t param_index = 0;
    bool isDecimal = false;
    bool isAParam = false;
    bool isError = false;

    while (rxBufferHead != rxBufferTail)
    {
        chr = rxBuffer[rxBufferTail];

        // process data based on the value type of the parameter
        // type of number
        if (token->value_type == PARAM_TYPE_UINT32)
        {
            if (chr >= '0' && chr <= '9')
            {
                param_str[param_index++] = chr;
            }
            else if ((chr == ' ') && (param_index > 0))
            {
                // end of parameter
                isAParam = true;
            }
            else if ((chr == '\r' || chr == '\n') && (token->type != TOKEN_TYPE_LENGTH) && (param_index > 0))
            {
                // end of parameter
                isAParam = true;
            }
            else if (chr == '.' && !isDecimal)
            {
                param_str[param_index++] = chr;
                isDecimal = true;
            }
            else
            {
                api_error(API_ERROR_CODE_INVALID_COMMAND_PARAMETER);
                isError = true;
                break;
            }
        }
        // ANY type, push any data into the parameter
        else if (token->value_type == PARAM_TYPE_ANY)
        {
            if (param_index < lastToken->u32)
            {
                paramBuffer[param_index++] = chr;
            }
            
            // check if it is the end of the parameter based on the length
            if (param_index >= lastToken->u32)
            {
                // check if next character is a `\r` or `\n`
                uint8_t nextTail = (rxBufferTail+1) % API_RX_BUFFER_SIZE;
                if (nextTail != rxBufferHead)
                {
                    char chr = rxBuffer[nextTail];
                    if (chr == '\n' || chr == '\r')
                    {
                        // end of parameter
                        isAParam = true;
                    }
                    else
                    {
                        api_error(API_ERROR_CODE_INVALID_COMMAND_PARAMETER);
                        isError = true;
                        break;
                    }
                }
                else // wait for new data
                {
                    API_WAIT_UNTIL_BUFFER_NOT_EMPTY();
                }
            }
        }

        // check if it is the end of the parameter
        if (isAParam)
        {
            // check if the parameter is a float
            if (isDecimal)
            {
                token->value_type = PARAM_TYPE_FLOAT;
                token->f = atof(param_str);
            }
            else if (token->value_type == PARAM_TYPE_UINT32)
            {
                token->u32 = atoi(param_str);
            }
            else
            {
                token->any = &paramBuffer;
            }

            // reset parameters
            memset(param_str, '\0', sizeof(param_str));
            param_index = 0;
            isDecimal = false;
            isAParam = false;

            // check if it is the end of the command line
            // wait until rx buffer is not empty
            API_WAIT_UNTIL_BUFFER_NOT_EMPTY();

            // increment rx buffer tail
            api_increment_rx_buffer_tail_or_wait();
            // remove all the blank spaces when processing the command
            API_REMOVE_BLANK_SPACES();
            chr = rxBuffer[rxBufferTail];
            if ((chr == '\n' || chr == '\r') && (token->type != TOKEN_TYPE_LENGTH))
            {
                // increment rx buffer tail
                api_increment_rx_buffer_tail();
                // end of command line
                return STATUS_OK;
            }

            // update the last token
            lastToken = token;

            // not the end of the command line. Create a new token.
            token = api_create_token();
            if (token == NULL)
            {
                api_error(API_ERROR_CODE_FAIL_ALLOCATE_MEMORY_FOR_TOKEN);
                isError = true;
                break;
            }

            // add new token to the linked list
            lastToken->next = token;

            // check if the type of the parameter is supposed to be any
            if (lastToken != NULL && lastToken->type == TOKEN_TYPE_LENGTH)
            {
                token->value_type = PARAM_TYPE_ANY;
            }
            else
            {
                token->value_type = PARAM_TYPE_UINT32;
            }
        }
        // check if the digit has reached the maximum
        else if ((token->value_type != PARAM_TYPE_ANY) && (param_index >= MAX_INT_DIGITS))
        {
            api_error(API_ERROR_CODE_TOO_MANNY_DIGITS);
            isError = true;
            break;
        }
        else
        {
            // increment rx buffer tail
            api_increment_rx_buffer_tail_or_wait();
        }

    } // while loop

    // free linked list of tokens if there is an error
    if (isError)
    {
        api_free_tokens(commandLine.token);
        return STATUS_FAIL;
    }

    // should not reach here
    api_free_tokens(commandLine.token);
    api_error(API_ERROR_CODE_INVALID_COMMAND_PARAMETER);
    return STATUS_FAIL;
}

// execute the command
void api_execute_command()
{
    uint16_t error_code = 0;

    // execute the command
    switch (commandLine.id)
    {
    case SERVICE_ID_SERIAL:
        // execute serial command
        if (commandLine.type == 'W')
        {
            // search for the message token and ingore the length token
            token_t* token = commandLine.token;
            while (token != NULL)
            {
                if (token->value_type == PARAM_TYPE_ANY)
                {
                    break;
                }
                token = token->next;
            }
            // check if the token is found
            if (token == NULL)
            {
                error_code = API_ERROR_CODE_INVALID_COMMAND_PARAMETER;
            }
            else
            {
                // send the message to the serial port
                uart_printf(CHANNEL_1, "%s", (char*)token->any);
                //clear param buffer
                memset(paramBuffer, '\0', sizeof(paramBuffer));
            }
        }
        else
        {
            error_code = API_ERROR_CODE_INVALID_COMMAND_TYPE;
        }
        break;
    case SERVICE_ID_INPUT:
        // execute input command
        if (commandLine.type == 'R')
        {
            // check if parameter is valid
            if (commandLine.token == NULL ||
            commandLine.token->value_type != PARAM_TYPE_UINT32 ||
            commandLine.token->u32 == 0 ||
            commandLine.token->u32 >= DIGITAL_INPUT_MAX)
            {
                error_code = API_ERROR_CODE_INVALID_COMMAND_PARAMETER;
                break;
            }
            // read the state of the digital input
            bool state = digital_input_read(commandLine.token->u32);
            // send the state to the client, format: "R<Service ID> <Input Index> <State>"
            api_printf("R%d %d %d\r\n", SERVICE_ID_INPUT, commandLine.token->u32, state);
        }
        else
        {
            error_code = API_ERROR_CODE_INVALID_COMMAND_TYPE;
        }
        break;
    case SERVICE_ID_SUBSCRIBE_INPUT:
        // execute subscribe input command
        // check if token is valid
        if (commandLine.token == NULL)
        {
            error_code = API_ERROR_CODE_INVALID_COMMAND_PARAMETER;
            break;
        }

        if (commandLine.type == 'W')
        {
            token_t* token = commandLine.token;
            while (token != NULL)
            {
                // check if parameter is valid
                if (token->value_type != PARAM_TYPE_UINT32 ||
                token->u32 == 0 ||
                token->u32 >= DIGITAL_INPUT_MAX)
                {
                    error_code = API_ERROR_CODE_INVALID_COMMAND_PARAMETER;
                    break;
                }
                // subscribe to the digital input
                digital_input_subscribe(token->u32);
                token = token->next;
            }
            API_DEFAULT_RESPONSE();
        }
        else if (commandLine.type == 'R')
        {
            // print header
            api_printf("R%d ", SERVICE_ID_SUBSCRIBE_INPUT);
            // print current subscribed digital inputs
            digital_input_print_subscribed_inputs();
            // print new line
            api_printf("\r\n");
        }
        else
        {
            error_code = API_ERROR_CODE_INVALID_COMMAND_TYPE;
        }
        break;
    case SERVICE_ID_UNSUBSCRIBE_INPUT:
        // execute unsubscribe input command
        // check if token is valid
        if (commandLine.token == NULL)
        {
            error_code = API_ERROR_CODE_INVALID_COMMAND_PARAMETER;
            break;
        }

        if (commandLine.type == 'W')
        {
            token_t* token = commandLine.token;
            while (token != NULL)
            {
                // check if parameter is valid
                if (token->value_type != PARAM_TYPE_UINT32 ||
                token->u32 == 0 ||
                token->u32 >= DIGITAL_INPUT_MAX)
                {
                    error_code = API_ERROR_CODE_INVALID_COMMAND_PARAMETER;
                    break;
                }
                // unsubscribe to the digital input
                digital_input_unsubscribe(token->u32);
                token = token->next;
            }
            API_DEFAULT_RESPONSE();
        }
        else
        {
            error_code = API_ERROR_CODE_INVALID_COMMAND_TYPE;
        }
        break;
    default:
        // debug print the command
        vLoggingPrintf("Command: %c.%d \r\n", commandLine.type, commandLine.id);
        api_printf("%c%d", commandLine.type, commandLine.id);
        if (commandLine.variant > 0)
        {
            vLoggingPrintf("Variant: %d \r\n", commandLine.variant);
            api_printf(".%d", commandLine.variant);
        }
        // print the parameters
        token_t* token = commandLine.token;
        while (token != NULL)
        {
            if (token->type == TOKEN_TYPE_LENGTH)
            {
                vLoggingPrintf("Length: %d \r\n", token->u32);
            }
            else
            {
                if (token->value_type == PARAM_TYPE_UINT32)
                {
                    vLoggingPrintf("Param: %d \r\n", token->u32);
                }
                else if (token->value_type == PARAM_TYPE_FLOAT)
                {
                    vLoggingPrintf("Param: %f \r\n", token->f);
                }
                else // PARAM_TYPE_ANY
                {
                    vLoggingPrintf("Param: %s \r\n", (char*)token->any);
                    // clear the buffer
                    memset(paramBuffer, '\0', sizeof(paramBuffer));
                }
            }
            token = token->next;
        }

        break;
    }

    // check if there is an error
    if (error_code != 0)
    {
        api_error(error_code);
    }
    // free linked list of tokens
    api_free_tokens(commandLine.token);
    // clear the command line
    api_reset_command_line();
}

io_status_t api_printf(const char *format_string, ...)
{
    char buffer[API_TX_BUFFER_SIZE] = {'\0'};
    va_list args;
    va_start(args, format_string);
    vsnprintf(buffer, API_TX_BUFFER_SIZE, format_string, args);
    va_end(args);

    // append data to tx ring buffer
    if (api_append_to_tx_ring_buffer_until_term(buffer, '\0') != STATUS_OK)
    {
        // error handling
        vLoggingPrintf("Error: Append to TX buffer failed\n");
    }

    return STATUS_OK;
}

void api_error(uint16_t error_code)
{
    // print error code
    api_printf(ERROR_CODE_FORMAT, error_code);
    vLoggingPrintf("Error: %d\n", error_code);
}