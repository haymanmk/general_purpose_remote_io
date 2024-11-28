#include "stm32f7xx_remote_io.h"

settings_t settings;

const settings_t defaults = {
    .settings_version = SETTINGS_VERSION,
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
    .uart = {
        {
            .baudrate = 115200,
            .data_bits = UART_WORDLENGTH_8B,
            .stop_bits = UART_STOPBITS_1,
            .parity = UART_PARITY_NONE,
            .flow_control = 0,
        },
        {
            .baudrate = 9600,
            .data_bits = UART_WORDLENGTH_8B,
            .stop_bits = UART_STOPBITS_1,
            .parity = UART_PARITY_NONE,
            .flow_control = 0,
        },
    },
    .pwmws288xx_1 = {
        .number_of_leds = 25,
    },
};

/* Private functions */
void settings_restore(uint8_t restore_flag);
io_status_t settings_save();
io_status_t settings_load();

void settings_restore(uint8_t restore_flag)
{
    settings = defaults;
    settings_save();
}

void settings_init()
{
    // load settings from flash
    if (settings_load() != STATUS_OK)
    {
        // if failed to load settings, restore defaults
        settings_restore(SETTINGS_RESTORE_DEFAULTS);
    }
}

io_status_t settings_save()
{
    return flash_write_data_with_checksum(FLASH_SECTOR_SETTINGS, (uint8_t *)&settings, sizeof(settings_t));
}

io_status_t settings_load()
{
    if (flash_read_data_with_checksum(FLASH_SECTOR_SETTINGS, (uint8_t *)&settings, sizeof(settings_t)) != STATUS_OK)
    {
        return STATUS_ERROR;
    }

    if (settings.settings_version != SETTINGS_VERSION)
    {
        return STATUS_ERROR;
    }

    return STATUS_OK;
}