
#ifndef _SYSTEM_IT_H
#define _SYSTEM_IT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// extern void SysTick_Handler(void);
// extern void SVC_Handler(void);
// extern void PendSV_Handler(void);

extern void TIM2_IRQHandler(void);
extern void DMA2_Stream7_IRQHandler(void);
// extern void USART1_IRQHandler(void);
// extern void DMA1_Stream6_IRQHandler(void);

#ifdef __cplusplus
}
#endif  //__cplusplus
#endif  //_SYSTEM_IT_H
