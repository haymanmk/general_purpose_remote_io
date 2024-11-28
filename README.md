# General Purpose Remote I/O

This project is dedicated on developing a STM32F7xx-based remote I/O which is supposed to offer the following services for clients via TCP:

- Monitor and control general purpose inputs and outputs
- UART
- PWM-based API to control WS28xx LED strips
- Over the Air - remote update

# Outline

- [General Purpose Remote I/O](#general-purpose-remote-io)
- [Outline](#outline)
- [Commands](#commands)
  - [Format](#format)
  - [Functions](#functions)
    - [Services](#services)
    - [Settings](#settings)
- [Hardware Configuration](#hardware-configuration)
  - [Input Mapping](#input-mapping)
  - [Output Mapping](#output-mapping)
  - [UART (Serial)](#uart-serial)
  - [PWM (WS28xx)](#pwm-ws28xx)
- [Error Code](#error-code)

# Commands

## Format

**Command**: `[R/W][ID] [Para_1] ... [Para_N]`

- `[R/W]`: `R` defines the command to read data at `[ID]` with speicified `[Para_1]` to `[Para_N]`; `W` will write data at `[ID]` with specified `[Para_1]` to `[Para_N]`.
- `[ID]`: The `ID` of each function which provides specific service for users as described in [Protocol](#protocol).
- `[Para_N]`: Parameter used to manipulate each function.

**Return**: `[R/W][ID] [VALUE_1] ... [VALUE_N]`

- The device would prepend the command code into return, `[R/W][ID]`, and append values at the end. In such way, client could recognize which reply belongs to which command by the command code at the beginning.
- In case of writing values to a function, the return values would be the same as the device received. It can be used to double check if the command has been correctly transfered to the device.

**Exception** or **Error**: `[R/W][ID] ERR[ID]` or only `ERR[ID]`

- In case user sends an unacceptable command to device, the device would append the error code at the end, `ERR[ID]`, as a handy information for the debug purpose. The error code can be found at [Error Code](#error-code).

> NOTE:
>
> - Every command and return shall end with `\r\n`.
> - The prefix R and W is case-insensitive. Both of `R01` and `r01` are acceptable.

## Functions

At the `Type` column, the symbol `R` stands for **Readable**, and `W` means **Writable**.

### Services

| ID   | Name                       | Description                                                  | Example                                                      | Type |
| :--- | :------------------------- | :----------------------------------------------------------- | :----------------------------------------------------------- | :--- |
| 00   | N/A                        | System reserved.                                             |                                                              |      |
| 01   | Status                     | Read device status.                                          | `R01`<br />The return would be `R01 OK`. Client can use this feature to check the connection with this device. | R    |
| 02   | Info.                      | Read device information.                                     | `R02`<br />The device would return everything from firmware version, command format, and hardware info, etc. | R    |
| 03   | Input                      | Read input status. To get the pin ID, please refer to [Input Mapping](#input-mapping). | `R03 -1`: read all inputs<br />`R03 1`: read input_1         | R    |
| 04   | Output                     | Read or write output status. To get the pin ID, please refer to [Output Mapping](#output-mapping) | - **Read**<br />**format**: `R04 [PIN/NONE]`<br />`R04`: read all outputs<br />`R04 1`: read output_1<br /><br />- **Write**<br />**format**: `W04 [PIN] [VALUE]`<br />`W04 4 0`: write 0 at output_4<br />`W04 4 1`: write 1 at output_4 | R/W  |
| 05   | Subscribe                  | **Subscribing input status** provides an event-driven feature to notify clients the subscribed inputs has been updated without having to polling input status from the client side which is relatively resource-consuming. | `R05`: read which pin has been subscribed. The return would be in the format as `R05 [PIN_2] [PIN_7] [PIN_N]`, which lists all the subscribed inputs by their pin ID.<br />`W05 [PIN_10]`: subscribe input_10.<br />`S05 [PIN_10] [STATUS]`: the format of update message. | R/W  |
| 06   | Unsubscribe                | **Unsubscribing input status** provides an approach for clients to cancel the subscription at particular subscribed input. | Refer to **Subscribe** service.                              | W    |
| 07.x | Serial                     | Send a message through serial.                               | `W07.x [LEN] [MSG]`: `x` specifies which channel to send. `[LEN]` denotes the length of the message. `[MSG]` is the message to be sent via serial which can be in any type like `char`, `string`, or `number`. `<br>` The return to a client would be the response from another device connected with the serial port once it has been received, and the format of the return would be `W07 [CH] [RESPONSE]`. | W    |
| 08   | PWM (WS28xx)               | Control WS28xx LED strip by PWM.                             | `R08 [CH] [LED]`: read RGB setting at `[LED]` LED and `[CH]` channel.<br />Return would be `R08 [CH] [LED] [R] [G] [B]`.<br />e.g. `R08 1 4`, the return could be `R08 1 4 127 23 255`<br /><br />`W08 [CH] [LED] [R] [G] [B]`: write RGB, specified in `[R]`, `[G]`, and `[B]`, respectively, to `[LED]` LED at `[CH]` channel.<br />e.g. `W08 1 19 255 255 0`<br /><br />Note: This device only supports at most two channels for this application. The number specified in `[CH]` should range from 0 to 1. The maximum ID of `[LED]` depends on the number of LEDs configured by **Number of LEDs** as elaborating in [Settings](#settings), which should range from 0 to N-1. | R/W  |
| 09   | Analog input<br />(To-do)  | Read analog data at input.                                   | `R09 [PIN]`: read analog data at `[PIN]` pin.<br />Return would be `R09 [PIN] [FLOAT_VALUE]`. The `[FLOAT_VALUE]` is the analog data represented in floating point. | R    |
| 10   | Analog output<br />(To-do) | Write analog data at output.                                 | `W10 [PIN] [FLOAT_VALUE]`: write `[FLOAT_VALUE]` at output which is usually represented in floating point. | W    |

### Settings

At the `Type` column, the symbols

- `N/A` which represents no symbol in the column means the change would be actived immediately by default. This is the same thing as the symbol `I`.
- `A` means the change would happen only after system reboot.
- `I` means the change would be enrolled right after the modification. (Default mode)
- `F` means the change would be written in the flash, which means the setting shall be nonvolatile even after system reboot.

| ID    | Name                                  | Description                                                  | Example                                                      | Type    |
| :---- | :------------------------------------ | :----------------------------------------------------------- | :----------------------------------------------------------- | :------ |
| 101   | IP setting                            | Configure IP address.                                        | `R101`: read current IP setting.<br />The return looks like `R101 172 16 0 10` when the IP is `172.16.0.10`.<br />`W101 192 168 0 21`: set IP address as `192.168.0.21`. | R/W/A/F |
| 102   | Ethernet port                         | Configure ethernet listening port.                           | `R102`: read port setting.<br />The return would be `R102 8500` when current listening port is `8500`.<br />`W102 8501`: set port as `8501`. | R/W/A/F |
| 103   | Netmask                               | Configure Netmask.                                           | Refer to IP setting.                                         | R/W/A/F |
| 104   | Gateway                               | Configure Gateway.                                           | Refer to IP setting.                                         | R/W/A/F |
| 105.x | Baud rate                             | Configure the serial port baud rate for channel `x`.         | Refer to Ethernet port setting.                              | R/W/A/F |
| 106.x | Data bits                             | Configure the serial port data bits for channel `x`.         | Refer to Ethernet port setting.                              | R/W/A/F |
| 107.x | Parity                                | Configure the serial port parity for channel `x`.            | Refer to Ethernet port setting.                              | R/W/A/F |
| 108.x | Stop bits                             | Configure the serial stop bits for channel `x`.              | Refer to Ethernet port setting.                              | R/W/A/F |
| 109.x | Flow control<br />(under development) | Configure the serial flow control for channel `x`.           | `R109`: read flow control setting.<br />The return would be either `R109 0` means without flow control or `R109 1` means with flow control.<br />`W109 1`: enable flow control, and vice versa. | R/W/A/F |
| 110.x | Number of LEDs                        | Configure the number of LEDs embedded at the strip connected to channel `x`. | Refer to Ethernet port setting.                              | R/W/A/F |

> Note:
>
> 1. The channel ID, `x`, can be found in the [Hardware Configuration](#hardware-configuration). If there is no channel ID specified, the first available channel will be assigned to the function by default.

# Hardware Configuration

## Digital Input Mapping

| Pin ID | STM32 Pin ID | Description   |
| :----- | :----------- | :------------ |
| 1      | PA1          | Digital Input |
| 2      | PA2          | Digital Input |

## Digital Output Mapping

| Pin ID | STM32 Pin ID | Description    |
| :----- | :----------- | :------------- |
| 1      | PB1          | Digital Output |
| 2      | PB2          | Digital Output |

## Analog Input Mapping

| Pin ID | STM32 Pin ID | Description  |
| ------ | ------------ | ------------ |
| 1      | PE1          | Analog Input |
| 2      | PE2          | Analog Input |
|        |              |              |

## Analog Output Mapping

| Pin ID | STM32 Pin ID | Description   |
| ------ | ------------ | ------------- |
| 1      | PD1          | Analog Output |
| 2      | PD2          | Analog Input  |
|        |              |               |



## UART (Serial)

| Channel ID | STM32 Pins              |
| :--------- | :---------------------- |
| 1          | TX: PDx<br />RX: PDx |
| 2          | TX: PDx<br />RX: PDx |

## PWM (WS28xx)

| Channel ID | STM32 Pins |
| :--------- | :--------- |
| 1          | PWM: PDx   |
| 2          | PWM: PDx   |

# Error Code

| ID | Name                     | Description                                                                                              |
| -- | ------------------------ | -------------------------------------------------------------------------------------------------------- |
| 01 | Hard fault               | Serious system fault which could be an unknown error for device to assign it to certain branch of error. |
| 02 | Incorrect command format | Device received a command with incorrect parameters.                                                     |
| 03 | Unacceptable command     | The command user sent is not supported at the device.                                                    |
