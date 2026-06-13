#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "button.h"

void button_setup(uint32_t button)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    switch(button) {
        case BT_DOWN:
            GPIO_StructInit(&GPIO_InitStructure);
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_Pin = BT_DOWN;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_InitStructure.GPIO_Speed = GPIO_Medium_Speed;
            GPIO_Init(GPIOB, &GPIO_InitStructure);
            break;
        case BT_UP:
            GPIO_StructInit(&GPIO_InitStructure);
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_Pin = BT_UP;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_InitStructure.GPIO_Speed = GPIO_Medium_Speed;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            break;
        case BT_OK:
            GPIO_StructInit(&GPIO_InitStructure);
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_Pin = BT_OK;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_InitStructure.GPIO_Speed = GPIO_Medium_Speed;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            break;
        default:
            GPIO_StructInit(&GPIO_InitStructure);
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_Pin = BT_DOWN | BT_OK;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_InitStructure.GPIO_Speed = GPIO_Medium_Speed;
            GPIO_Init(GPIOB, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = BT_UP;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            break;
    }
}

bool button_read(uint32_t button)
{
    bool bt_status = false;
    switch(button) {
        case BT_DOWN:
            bt_status = GPIO_ReadInputDataBit(GPIOB, BT_DOWN);
            break;
        case BT_UP:
            bt_status = GPIO_ReadInputDataBit(GPIOA, BT_UP);
            break;
        case BT_OK:
            bt_status = GPIO_ReadInputDataBit(GPIOB, BT_OK);
            break;
        default:
            break;
    }
    return bt_status;
}





