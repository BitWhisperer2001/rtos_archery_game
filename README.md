# A Game Embed: Archery game

## Introduction
This application was written to delve into the ARM Cortex-M microcontroller architecture and real-time operating systems (RTOS). Inspired by an existing application ([Source here](https://github.com/the-ak-foundation/archery-game)) written using Event-driven Framwok, I ported it to the FreeRTOS platform on STM32F411 Nucleo board

## Technical
* C/C++ Language
* Linker script
* Makefile
* FreeRTOS Porting
* Ring Buffer

## Installation
### Hardware Connection
If you want to build one yourself, researching and developing it according to your preferences, first download the board schematic and connect it according to the following diagram:

| PCB| STM32F411 Nucleo Board|
| ------ | ----------- |
| DI BT_OK | PB3|
| OLED_SDA | PB7 |
| OLED_SDA | PB6 |
| DO LED_LIFE | PC7 |
| DO BUZZER | PA6 |
| DI BT_UP | PA4 |
| DI BT_DOWN | PB0 |
| 3V3 | 3V3 |
| 5V | 5V |
| GND | GND|

### Sofware Installation and Run
#### For Linux
* Step 1: Install GCC for Arm and Debuger
```
sudo apt update
sudo apt install gcc-arm-none-eabi gdb-multiarch
```
* Step 2: Install Make
```
sudo apt install build-essential
```
* Step 3: Install OpenOCD
```
sudo apt install openocd
```
* Step 4: Install ST-Link Driver
```
sudo wget -O /etc/udev/rules.d/99-openocd.rules https://raw.githubusercontent.com/stlink-org/stlink/develop/config/udev/rules.d/49-stlinkv2.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```
> Unplug and plug back in for it to take effect.

* Step 5: Build and Flash
```
make
make flash
```

#### For Windows
* Step 1: Install Git for Windows [here](https://git-scm.com/install/windows)
* Step 2: Download GCC for Arm [here](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
* Step 3: Add path to GCC for Arm to Enviroment Variable
* Step 4: Install ST-Link Driver [here](https://www.st.com/en/development-tools/stsw-link009.html)
* Step 5: Open the root directory of this project with Git bash
* Step 6: Build and Flash
```
make
make flash
```
## Contact
# `Hope You Enjoy`
