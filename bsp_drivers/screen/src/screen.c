#include <stdint.h>
#include <stdbool.h>
#include "screen.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_dma.h"
#include "ssd1306.h"
#include "system_init.h"
#include "led.h"
#include "system_log.h"

#define SCREEN_IO_SCL       (GPIO_Pin_6)
#define SCREEN_IO_SDA       (GPIO_Pin_7)
#define SCREEN_IO_PORT      (GPIOB)
#define SCREEN_IO_BUS_CLK   (RCC_AHB1Periph_GPIOB)
#define SCREEN_BUS_CLK      (RCC_APB1Periph_I2C1)
#define SCREEN_PERPH        (I2C1)

#define SCREEN_DMA_BUS_CLK  (RCC_AHB1Periph_DMA1)
#define SCREEN_DMA_STREAM   (DMA1_Stream6)
#define SCREEN_DMA_CHANNEL  (DMA_Channel_1)

static void screen_io_i2c_config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(SCREEN_IO_BUS_CLK, ENABLE);
    GPIO_PinAFConfig(SCREEN_IO_PORT, GPIO_PinSource6,  GPIO_AF_I2C1);
    GPIO_PinAFConfig(SCREEN_IO_PORT, GPIO_PinSource7, GPIO_AF_I2C1);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Pin = SCREEN_IO_SCL | SCREEN_IO_SDA;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_Init(SCREEN_IO_PORT, &GPIO_InitStructure);
}

// static void screen_dma_config(void)
// {
//     NVIC_InitTypeDef NVIC_InitStructure;
//     NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream6_IRQn;
//     NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
//     NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//     NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//     NVIC_Init(&NVIC_InitStructure);
//     DMA_InitTypeDef DMA_InitStructure;
//     RCC_AHB1PeriphClockCmd(SCREEN_DMA_BUS_CLK, ENABLE);
//     DMA_DeInit(SCREEN_DMA_STREAM);
//     DMA_InitStructure.DMA_Channel = SCREEN_DMA_CHANNEL;
//     DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SCREEN_PERPH->DR;
//     DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)0;
//     DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
//     DMA_InitStructure.DMA_BufferSize = (uint32_t)0;
//     DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//     DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//     DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//     DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//     DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//     DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
//     DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
//     DMA_Init(SCREEN_DMA_STREAM, &DMA_InitStructure);
//     DMA_ITConfig(SCREEN_DMA_STREAM, DMA_IT_TC | DMA_IT_HT | DMA_IT_TE | DMA_IT_FE, DISABLE);
//     DMA_ITConfig(SCREEN_DMA_STREAM, DMA_IT_TC, ENABLE);
//     DMA_ClearFlag(SCREEN_DMA_STREAM, DMA_FLAG_TCIF6 | DMA_FLAG_HTIF6 | DMA_FLAG_TEIF6 | DMA_FLAG_DMEIF6 | DMA_FLAG_FEIF6);
//     DMA_Cmd(SCREEN_DMA_STREAM, DISABLE);
// }

static void screen_start(void)
{
    SSD1306_Init();
}

void screen_init(void)
{
    I2C_InitTypeDef I2C1_InitStructure;
    RCC_APB1PeriphClockCmd(SCREEN_BUS_CLK, ENABLE);
    I2C_DeInit(SCREEN_PERPH);
    I2C1_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C1_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C1_InitStructure.I2C_ClockSpeed = 400000;
    I2C1_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C1_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C1_InitStructure.I2C_OwnAddress1 = 0x00;
    screen_io_i2c_config();
    // screen_dma_config();
    I2C_Init(SCREEN_PERPH, &I2C1_InitStructure);
    // I2C_DMACmd(SCREEN_PERPH, ENABLE);
    I2C_ITConfig(SCREEN_PERPH, I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR, DISABLE);
    I2C_Cmd(SCREEN_PERPH, ENABLE);
    screen_start();
}

// static void dma_begin_transfer(uint8_t *data, uint16_t len)
// {
//     DMA_Cmd(SCREEN_DMA_STREAM, DISABLE);
//     while(DMA_GetCmdStatus(SCREEN_DMA_STREAM) != DISABLE);
//     SCREEN_DMA_STREAM->M0AR = (uint32_t)data;
//     SCREEN_DMA_STREAM->NDTR = (uint32_t)len;
//     DMA_ClearFlag(SCREEN_DMA_STREAM, DMA_FLAG_TCIF6 | DMA_FLAG_HTIF6 | DMA_FLAG_TEIF6 | DMA_FLAG_DMEIF6 | DMA_FLAG_FEIF6);
//     DMA_Cmd(SCREEN_DMA_STREAM, ENABLE);
// }

void screen_data_write(uint8_t address, uint8_t *data, uint16_t len)
{
    while(I2C_GetFlagStatus(SCREEN_PERPH, I2C_FLAG_BUSY));

    I2C_GenerateSTART(SCREEN_PERPH, ENABLE);
    while(!I2C_GetFlagStatus(SCREEN_PERPH, I2C_FLAG_SB));
    while(!I2C_CheckEvent(SCREEN_PERPH, I2C_EVENT_MASTER_MODE_SELECT));

    // (void)I2C1->SR1;

    I2C_Send7bitAddress(SCREEN_PERPH, address, I2C_Direction_Transmitter);
    while (!I2C_GetFlagStatus(SCREEN_PERPH, I2C_FLAG_ADDR));

    // (void)SCREEN_PERPH->SR1;
    (void)SCREEN_PERPH->SR2;

    for(uint16_t i = 0; i < len; i++)
    {
        while(!I2C_GetFlagStatus(SCREEN_PERPH, I2C_FLAG_TXE));  // chờ TXE
        I2C_SendData(SCREEN_PERPH, data[i]);
    }

    while(!I2C_GetFlagStatus(SCREEN_PERPH, I2C_FLAG_BTF));
    I2C_GenerateSTOP(SCREEN_PERPH, ENABLE);
}









