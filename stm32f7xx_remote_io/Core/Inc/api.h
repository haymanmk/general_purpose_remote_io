#ifndef __API_H
#define __API_H

#include "stm32f7xx_remote_io.h"

#define MAX_INT_DIGITS 8

#define API_RX_BUFFER_SIZE 128 // 1 ~ 254
#define API_TX_BUFFER_SIZE 128 // 1 ~ 254
#define API_ERROR_BUFFER_SIZE API_TX_BUFFER_SIZE // 1 ~ 254

// Parsing step
#define COMMND_LINE_TYPE 0
#define COMMND_LINE_ID 1
#define COMMND_LINE_PARAM 2

// Service ID
#define SERVICE_ID_STATUS 1
#define SERVICE_ID_INPUT 2
#define SERVICE_ID_OUTPUT 3
#define SERVICE_ID_SUBSCRIBE_INPUT 4
#define SERVICE_ID_SERIAL 5
#define SERVICE_ID_PWM_WS28XX_LED 6
#define SERIVCE_ID_ANALOG_INPUT 7
#define SERVICE_ID_ANALOG_OUTPUT 8

// Setting ID
#define SETTING_ID_IP_ADDRESS 101
#define SETTING_ID_ETHERNET_PORT 102
#define SETTING_ID_NETMASK 103
#define SETTING_ID_GATEWAY 104
#define SETTING_ID_BAUD_RATE 105
#define SETTING_ID_DATA_BITS 106
#define SETTING_ID_PARITY 107
#define SETTING_ID_STOP_BITS 108
#define SETTING_ID_FLOW_CONTROL 109
#define SETTING_ID_NUMBER_OF_LEDS 110

enum {
    TOKEN_TYPE_PARAM = 1,
    TOKEN_TYPE_LENGTH,
};

enum {
    PARAM_TYPE_UINT32 = 1,
    PARAM_TYPE_FLOAT,
    PARAM_TYPE_ANY,
};

/* Macros */
#define API_INCREMENT_BUFFER_HEAD(HEAD, TAIL, SIZE) \
    do { \
        /* check if buffer head is allowed to increment */ \
        uint8_t nextHead = (HEAD+1) % SIZE; \
        if (nextHead != TAIL) { \
            HEAD = nextHead; \
            return STATUS_OK; \
        } \
        return STATUS_FAIL; \
    } while (0)

#define API_INCREMENT_BUFFER_TAIL(TAIL, HEAD, SIZE) \
    do { \
        /* check if buffer tail is allowed to increment */ \
        if (TAIL != HEAD) { \
            TAIL = (TAIL+1) % SIZE; \
            return STATUS_OK; \
        } \
        return STATUS_FAIL; \
    } while (0)

// remove all the blank spaces when processing the command
#define API_REMOVE_BLANK_SPACES() \
    do { \
        while (rxBufferHead != rxBufferTail && rxBuffer[rxBufferTail] == ' ') { \
            api_increment_rx_buffer_tail_or_wait(); \
        } \
    } while (0)

// wait until rx buffer is not empty
#define API_WAIT_UNTIL_BUFFER_NOT_EMPTY() \
    do { \
        while (api_is_rx_buffer_empty() == STATUS_OK) { \
            ulTaskNotifyTake(pdFALSE, portMAX_DELAY); \
        } \
    } while (0)

/* Type definition */
typedef struct Token {
    uint8_t type;
    uint8_t value_type;
    union
    {
        /* data */
        uint32_t u32;
        float f;
        void * any;
    };
    struct Token *next;
} token_t;

typedef struct {
    char type; // 'R' for read, 'W' for write
    uint16_t id; // command id
    uint16_t variant; // command variant
    token_t *token; // parameters
} command_line_t;

/* Function prototypes */
void api_init();
io_status_t api_append_to_rx_ring_buffer(char *data, BaseType_t len);
io_status_t api_append_to_tx_ring_buffer(char *data, char terminator);
io_status_t api_increment_rx_buffer_head();
io_status_t api_increment_rx_buffer_tail();
void api_increment_rx_buffer_tail_or_wait();
io_status_t api_is_rx_buffer_empty();
io_status_t api_is_rx_buffer_full();
io_status_t api_increment_tx_buffer_head();
io_status_t api_increment_tx_buffer_tail();
io_status_t api_is_tx_buffer_empty();
io_status_t api_is_tx_buffer_full();

#endif