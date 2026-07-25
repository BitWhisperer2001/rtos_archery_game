#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

#include "button.h"

void button_setup(uint32_t buttons)
{
    GPIO_InitTypeDef gpio;

    /* PB0: BT_DOWN, PB3: BT_OK */
    uint16_t gpio_b_pins = (uint16_t)(buttons & (BT_DOWN | BT_OK));

    if (gpio_b_pins != 0U) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        GPIO_StructInit(&gpio);
        gpio.GPIO_Pin = gpio_b_pins;
        gpio.GPIO_Mode = GPIO_Mode_IN;
        gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
        gpio.GPIO_Speed = GPIO_Medium_Speed;
        GPIO_Init(GPIOB, &gpio);
    }

    /* PA4: BT_UP */
    if ((buttons & BT_UP) != 0U) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        GPIO_StructInit(&gpio);
        gpio.GPIO_Pin = BT_UP;
        gpio.GPIO_Mode = GPIO_Mode_IN;
        gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
        gpio.GPIO_Speed = GPIO_Medium_Speed;
        GPIO_Init(GPIOA, &gpio);
    }
}

bool button_read(uint32_t button)
{
    switch (button) {
        case BT_DOWN:
            return GPIO_ReadInputDataBit(GPIOB, BT_DOWN) != Bit_RESET;
        case BT_UP:
            return GPIO_ReadInputDataBit(GPIOA, BT_UP) != Bit_RESET;
        case BT_OK:
            return GPIO_ReadInputDataBit(GPIOB, BT_OK) != Bit_RESET;
        default:
            return true;
    }
}





