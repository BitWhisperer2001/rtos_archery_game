#include <stdint.h>
#include <stdbool.h>
#include "system_init.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_flash.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
#include "misc.h"

#include "led.h"

volatile uint32_t g_cnt = 0;

void sys_clock_config(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    RCC_DeInit();
    FLASH_SetLatency(FLASH_Latency_3);
    RCC_HSICmd(ENABLE);
    while(!RCC_GetFlagStatus(RCC_FLAG_HSIRDY));
    RCC_PLLConfig(RCC_PLLSource_HSI, 8, 100, 2, 4);
    RCC_PLLCmd(ENABLE);
    while(!RCC_GetFlagStatus(RCC_FLAG_PLLRDY));
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    RCC_ClockSecuritySystemCmd(ENABLE);
    while(RCC_GetSYSCLKSource() != 0x08);
    SystemCoreClockUpdate(); 

    // NVIC_SetPriority(SysTick_IRQn, 1);  
    // SysTick_Config(SystemCoreClock / 1000);
    // NVIC_SetPriority(PendSV_IRQn, 15);
    // NVIC_EnableIRQ(PendSV_IRQn);
    // NVIC_SetPriority(SVCall_IRQn, 2);
    // NVIC_EnableIRQ(SVCall_IRQn);
}

void  sys_delay(uint32_t ms)
{
    volatile uint32_t ms_current = g_cnt;
    while(!((sys_get_tick() - ms_current) >= ms));
}

uint32_t sys_get_tick(void)
{
    return g_cnt;
}

/* Select timer 2 to use system timer base if use RTOS*/
void sys_tick_config(void)
{
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_DeInit(TIM2);
    TIM_InitStructure.TIM_Prescaler = TIM2_PRESCALER;
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStructure.TIM_Period = TIM2_PERIOD;
    TIM_InitStructure.TIM_RepetitionCounter = 0x00;   // This parameter only valid for TIM1 and TIM8
    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);
    TIM_UpdateRequestConfig(TIM2, TIM_UpdateSource_Regular);
    TIM_UpdateDisableConfig(TIM2, DISABLE);
    TIM_ARRPreloadConfig(TIM2, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

static void sys_io_uart1_config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9,  GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void sys_dma_uart1_config(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
    DMA_DeInit(DMA2_Stream7);
    DMA_InitStructure.DMA_Channel = DMA_Channel_4;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)0;
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_InitStructure.DMA_BufferSize = (uint32_t)0;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_Init(DMA2_Stream7, &DMA_InitStructure);
    DMA_ITConfig(DMA2_Stream7, DMA_IT_TC | DMA_IT_HT | DMA_IT_TE | DMA_IT_FE, DISABLE);
    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA2_Stream7, DISABLE);
}

/* Select USART1 to print information for debug */
void sys_log_config(void)
{
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    USART_DeInit(USART1);
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = (USART_Mode_Rx | USART_Mode_Tx);
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_TC | USART_IT_CTS | USART_IT_LBD | USART_IT_TXE | USART_IT_RXNE | USART_IT_IDLE| USART_IT_PE | USART_IT_ERR, DISABLE);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    sys_dma_uart1_config();
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
    USART_Cmd(USART1, ENABLE);
    sys_io_uart1_config();
}

void sys_save_score_into_flash(uint32_t add, uint32_t data)
{
    // led_on(LED_ONBOARD);
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |
                    FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    FLASH_EraseSector(FLASH_Sector_7, VoltageRange_3);
    FLASH_ProgramWord(add, data);
    FLASH_Lock();
}

uint32_t sys_read_score_in_flash(uint32_t add)
{
    uint32_t raw;
    // FLASH_Unlock();
    raw = *(volatile uint32_t*)add;
    // FLASH_Lock();
    return raw;
}

