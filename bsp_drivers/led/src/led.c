#include <stdint.h>
#include <stdbool.h>

#include "led.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

void led_setup(uint32_t leds)
{
    GPIO_InitTypeDef gpio;

    /* PC7: LED_LIFE */
    if ((leds & LED_LIFE) != 0U) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        GPIO_ResetBits(GPIOC, LED_LIFE);
        GPIO_StructInit(&gpio);
        gpio.GPIO_Pin = LED_LIFE;
        gpio.GPIO_Mode = GPIO_Mode_OUT;
        gpio.GPIO_OType = GPIO_OType_PP;
        gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
        gpio.GPIO_Speed = GPIO_Low_Speed;
        GPIO_Init(GPIOC, &gpio);
    }

    /* PA5: LED_ONBOARD */
    if ((leds & LED_ONBOARD) != 0U) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        GPIO_ResetBits(GPIOA, LED_ONBOARD);
        GPIO_StructInit(&gpio);
        gpio.GPIO_Pin = LED_ONBOARD;
        gpio.GPIO_Mode = GPIO_Mode_OUT;
        gpio.GPIO_OType = GPIO_OType_PP;
        gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
        gpio.GPIO_Speed = GPIO_Low_Speed;
        GPIO_Init(GPIOA, &gpio);
    }
}

void led_on(uint32_t leds)
{
    if ((leds & LED_LIFE) != 0U) {
        GPIO_SetBits(GPIOC, LED_LIFE);
    }

    if ((leds & LED_ONBOARD) != 0U) {
        GPIO_SetBits(GPIOA, LED_ONBOARD);
    }
}

void led_off(uint32_t leds)
{
    if ((leds & LED_LIFE) != 0U) {
        GPIO_ResetBits(GPIOC, LED_LIFE);
    }

    if ((leds & LED_ONBOARD) != 0U) {
        GPIO_ResetBits(GPIOA, LED_ONBOARD);
    }
}

void led_toggle(uint32_t leds)
{
    if ((leds & LED_LIFE) != 0U) {
        GPIO_ToggleBits(GPIOC, LED_LIFE);
    }

    if ((leds & LED_ONBOARD) != 0U) {
        GPIO_ToggleBits(GPIOA, LED_ONBOARD);
    }
}
