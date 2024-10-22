#ifndef __API_H
#define __API_H

#include "stm32f7xx_remote_io.h"

#define API_RX_BUFFER_SIZE 128 // 1 ~ 254
#define API_TX_BUFFER_SIZE 128 // 1 ~ 254

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

/* Function prototypes */
void api_init(Socket_t socket);
void api_process_data(char *rx_data, BaseType_t len, Socket_t socket);
io_status_t api_append_data(uint8_t *data, BaseType_t len);
io_status_t api_increment_rx_buffer_head();
io_status_t api_increment_rx_buffer_tail();
io_status_t api_is_rx_buffer_empty();
io_status_t api_is_rx_buffer_full();
io_status_t api_increment_tx_buffer_head();
io_status_t api_increment_tx_buffer_tail();
io_status_t api_is_tx_buffer_empty();
io_status_t api_is_tx_buffer_full();

#endif