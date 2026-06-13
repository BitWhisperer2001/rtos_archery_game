#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "stm32f4xx_usart.h"
#include "stm32f4xx_dma.h"
#include "system_log.h"

static uint8_t tx_buf[TX_BUF_LOG_SIZE] = {0};
volatile bool IsDMALogBusy = false;

void sys_log_debug(const char* fmt, ...)
{
    while(IsDMALogBusy);
    va_list args;
    va_start(args, fmt);
    uint16_t len = vsnprintf((char*)tx_buf, TX_BUF_LOG_SIZE, fmt, args);
    va_end(args);
    if((len == 0) || (len > TX_BUF_LOG_SIZE))
        return;
    DMA_Cmd(DMA2_Stream7, DISABLE);
    IsDMALogBusy = true;
    while(DMA_GetCmdStatus(DMA2_Stream7) != DISABLE);
    DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_TCIF7 | DMA_FLAG_HTIF7 | DMA_FLAG_TEIF7 | DMA_FLAG_DMEIF7 | DMA_FLAG_FEIF7);
    DMA2_Stream7->NDTR = len;
    DMA2_Stream7->M0AR = (uint32_t)tx_buf;
    DMA_Cmd(DMA2_Stream7, ENABLE);
}

void sys_log_send_char(char c)
{
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, c);
}

void sys_log_send_string(const char* s)
{
    while(*s) sys_log_send_char(*s++);
}











