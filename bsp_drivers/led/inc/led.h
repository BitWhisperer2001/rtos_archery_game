#ifndef _LED_H_
#define _LED_H_

#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C"{
#endif  //__cplusplus

#include "stm32f4xx_gpio.h"

#define LED_ONBOARD     GPIO_Pin_5   // PA5
#define LED_LIFE        GPIO_Pin_7   // PC7

extern void led_setup(uint32_t led);
extern void led_on(uint32_t led);
extern void led_off(uint32_t led);
extern void led_toggle(uint32_t led);

#ifdef __cplusplus
}
#endif  //__cplusplus



#endif  // _LED_H_