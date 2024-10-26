#ifndef __STM32F7XX_REMOTE_IO_H
#define __STM32F7XX_REMOTE_IO_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

#include "main.h"
#include "FreeRTOS.h"
#include "cpu_map.h"
#include "utils.h"
#include "freertos.h"
#include "settings.h"
#include "ethernet_if.h"
#include "api.h"

typedef enum {
    STATUS_OK = 0,
    STATUS_ERROR = 1,
    STATUS_FAIL = 2,
} io_status_t

/* Exported functions */

#endif