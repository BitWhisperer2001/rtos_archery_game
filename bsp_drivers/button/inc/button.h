#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include "stm32f4xx_gpio.h"

#define BT_UP       GPIO_Pin_4  // PA4
#define BT_DOWN     GPIO_Pin_0  // PB0
#define BT_OK       GPIO_Pin_3  // PB3

extern void button_setup(uint32_t button);
extern bool button_read(uint32_t button);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif //BUTTON_H




