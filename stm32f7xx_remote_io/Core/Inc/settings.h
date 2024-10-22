#ifndef __SETTINGS_H
#define __SETTINGS_H

// flags for instructing restoring function to restore settings
#define SETTINGS_RESTORE_DEFAULTS (1 << 0)

// type of settings
typedef struct
{
    uint8_t ip_address_0;
    uint8_t ip_address_1;
    uint8_t ip_address_2;
    uint8_t ip_address_3;
    uint8_t netmask_0;
    uint8_t netmask_1;
    uint8_t netmask_2;
    uint8_t netmask_3;
    uint8_t gateway_0;
    uint8_t gateway_1;
    uint8_t gateway_2;
    uint8_t gateway_3;
    uint8_t mac_address_0;
    uint8_t mac_address_1;
    uint8_t mac_address_2;
    uint8_t mac_address_3;
    uint8_t mac_address_4;
    uint8_t mac_address_5;
    uint8_t tcp_port;
} settings_t;

extern settings_t settings;

/* Export functions */
void settings_init();

#endif