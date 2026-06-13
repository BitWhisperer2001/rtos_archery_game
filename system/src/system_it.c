#include <stdint.h>
#include <stdbool.h>
#include "system_it.h"
#include "system_init.h"
#include "system_log.h"
#include "screen.h"
#include "led.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_dma.h"

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        g_cnt++;
    }
}

// void USART1_IRQHandler(void)
// {
//     if(USART_GetITStatus(USART1, USART_IT_TC) != RESET){
//         USART_ITConfig(USART1, USART_IT_TC, DISABLE);
//         USART_ClearFlag(USART1, USART_FLAG_TC);
//         IsUSART1_TC = false;
//         USART_ClearITPendingBit(USART1, USART_IT_TC);
//     }
// }

void DMA2_Stream7_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA2_Stream7, DMA_IT_TCIF7) != RESET){
        DMA_ClearITPendingBit(DMA2_Stream7, DMA_IT_TCIF7);
        // USART_ClearFlag(USART1, USART_FLAG_TC);    
        // USART_ITConfig(USART1, USART_IT_TC, ENABLE);
        IsDMALogBusy = false;
    }
}

// void DMA1_Stream6_IRQHandler(void)
// {
//     if(DMA_GetITStatus(DMA1_Stream6, DMA_IT_TCIF6) != RESET){
//         DMA_ClearITPendingBit(DMA1_Stream6, DMA_IT_TCIF6);
//         while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF));
//         I2C_GenerateSTOP(I2C1, ENABLE);
//         DMA_Cmd(DMA1_Stream6, DISABLE);
//         IsDMAScreenBusy = false;
//     }
// }



