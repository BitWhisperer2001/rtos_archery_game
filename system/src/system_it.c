#include <stdint.h>
#include <stdbool.h>
#include "system_it.h"
#include "system_init.h"
#include "system_log.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_dma.h"

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        g_cnt++;
    }
}

void DMA2_Stream7_IRQHandler(void)
{
    /*
     * Release the logger on success or error. Without the error path, one bus
     * fault would leave every later DBG_LOG call blocked forever.
     */
    if ((DMA_GetITStatus(DMA2_Stream7, DMA_IT_TCIF7) != RESET) ||
        (DMA_GetITStatus(DMA2_Stream7, DMA_IT_TEIF7) != RESET) ||
        (DMA_GetITStatus(DMA2_Stream7, DMA_IT_DMEIF7) != RESET) ||
        (DMA_GetITStatus(DMA2_Stream7, DMA_IT_FEIF7) != RESET)) {
        DMA_Cmd(DMA2_Stream7, DISABLE);
        DMA_ClearITPendingBit(DMA2_Stream7,
                              DMA_IT_TCIF7 | DMA_IT_TEIF7 |
                              DMA_IT_DMEIF7 | DMA_IT_FEIF7);
        sys_log_dma_complete_from_isr();
    }
}



