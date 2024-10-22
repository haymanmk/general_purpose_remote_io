#include "stm32f7xx_remote_io.h"

settings_t settings;

const settings_t defaults = {
    .ip_address_0 = 172,
    .ip_address_1 = 16,
    .ip_address_2 = 0,
    .ip_address_3 = 10,
    .netmask_0 = 255,
    .netmask_1 = 240,
    .netmask_2 = 0,
    .netmask_3 = 0,
    .gateway_0 = 172,
    .gateway_1 = 16,
    .gateway_2 = 0,
    .gateway_3 = 1,
    .mac_address_0 = 0x00,
    .mac_address_1 = 0x80,
    .mac_address_2 = 0xE1,
    .mac_address_3 = 0x01,
    .mac_address_4 = 0x02,
    .mac_address_5 = 0x03,
    .tcp_port = 0, // this value will be added to 8500 as the final tcp port, i.e. 8500 + tcp_port
};

void settings_restore(uint8_t restore_flag)
{
    if (restore_flag & SETTINGS_RESTORE_DEFAULTS)
    {
        settings = defaults;
    }
}

void settings_init()
{
    settings_restore(SETTINGS_RESTORE_DEFAULTS);
}