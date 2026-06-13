#include <stdint.h>
#include <stdbool.h>

#include "led.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

void led_setup(uint32_t led)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    switch(led) {
        case LED_LIFE:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_Pin = led;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
            GPIO_Init(GPIOC, &GPIO_InitStructure);
            GPIO_ResetBits(GPIOC, led);
            break;
        case LED_ONBOARD:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_Pin = led;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            GPIO_ResetBits(GPIOA, led);
            break;
        default:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_Pin = LED_LIFE;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
            GPIO_Init(GPIOC, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_Pin = LED_ONBOARD;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            GPIO_ResetBits(GPIOC, led);
            GPIO_ResetBits(GPIOA, led);
            break;
    }
}

void led_on(uint32_t led)
{
    switch (led) {
        case LED_LIFE:
            GPIO_SetBits(GPIOC, led);
            break;
        case LED_ONBOARD:
            GPIO_SetBits(GPIOA, led);
            break;
        default:
            GPIO_SetBits(GPIOC, LED_LIFE);
            GPIO_SetBits(GPIOA, LED_ONBOARD);
            break;
    }
}

void led_off(uint32_t led)
{
    switch (led) {
        case LED_LIFE:
            GPIO_ResetBits(GPIOC, led);
            break;
        case LED_ONBOARD:
            GPIO_ResetBits(GPIOA, led);
            break;
        default:
            GPIO_ResetBits(GPIOC, LED_LIFE);
            GPIO_ResetBits(GPIOA, LED_ONBOARD);
            break;
    }
}

void led_toggle(uint32_t led)
{
    switch (led) {
        case LED_LIFE:
            GPIO_ToggleBits(GPIOC, led);
            break;
        case LED_ONBOARD:
            GPIO_ToggleBits(GPIOA, led);
            break;
        default:
            GPIO_ToggleBits(GPIOC, LED_LIFE);
            GPIO_ToggleBits(GPIOA, LED_ONBOARD);
            break;
    }
}



