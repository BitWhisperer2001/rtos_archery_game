#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "stm32f4xx_usart.h"
#include "stm32f4xx_dma.h"
#include "cmsis_gcc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "system_log.h"

static uint8_t tx_buf[TX_BUF_LOG_SIZE] = {0};
static volatile bool is_dma_log_busy = false;
static volatile bool scheduler_has_started = false;
static SemaphoreHandle_t log_mutex = NULL;
static SemaphoreHandle_t dma_complete = NULL;

static bool sys_log_try_lock(void)
{
    uint32_t interrupt_state = __get_PRIMASK();
    bool acquired = false;

    /*
     * A very short interrupt mask makes the test-and-set atomic across tasks.
     * It protects the shared formatter buffer without introducing an RTOS
     * dependency, so boot-time logging remains available before the scheduler.
     */
    __disable_irq();
    if (!is_dma_log_busy) {
        is_dma_log_busy = true;
        acquired = true;
    }
    if (interrupt_state == 0U) {
        __enable_irq();
    }

    return acquired;
}

static void sys_log_unlock(void)
{
    uint32_t interrupt_state = __get_PRIMASK();

    __disable_irq();
    is_dma_log_busy = false;
    if (interrupt_state == 0U) {
        __enable_irq();
    }
}

bool sys_log_create_resources(void)
{
    /*
     * The mutex provides priority inheritance between logging tasks; the binary
     * semaphore sleeps the owner until DMA completes instead of busy-spinning.
     */
    log_mutex = xSemaphoreCreateMutex();
    dma_complete = xSemaphoreCreateBinary();
    return (log_mutex != NULL) && (dma_complete != NULL);
}

void sys_log_debug(const char* fmt, ...)
{
    int formatted_length;
    uint16_t transfer_length;
    va_list args;
    bool scheduler_is_running;

    if (fmt == NULL) {
        return;
    }

    /* This API is deliberately task/boot context only; ISR diagnostics are dropped. */
    if (__get_IPSR() != 0U) {
        return;
    }

    scheduler_is_running =
        xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED;

    /*
     * Before the scheduler, TIM/DMA interrupts serialize the few boot records.
     * At runtime a FreeRTOS mutex prevents priority inversion while formatting.
     */
    if (scheduler_is_running) {
        BaseType_t status;

        scheduler_has_started = true;
        status = xSemaphoreTake(log_mutex, portMAX_DELAY);
        configASSERT(status == pdTRUE);

        /*
         * A boot-time transfer may still be finishing when the scheduler starts.
         * Consume that completion before reusing the shared DMA buffer.
         */
        if (is_dma_log_busy) {
            status = xSemaphoreTake(dma_complete, portMAX_DELAY);
            configASSERT(status == pdTRUE);
        }

        /*
         * Close the boot-to-runtime race: completion can occur between the busy
         * test and this task running. Drain any resulting stale token before a
         * new transfer, otherwise it could falsely complete the next message.
         */
        while (xSemaphoreTake(dma_complete, 0U) == pdTRUE) {
            /* Binary semaphore: at most one stale completion is present. */
        }
    } else {
        while (!sys_log_try_lock()) {
            if (__get_PRIMASK() != 0U) {
                return;
            }
            __NOP();
        }
    }

    va_start(args, fmt);
    formatted_length = vsnprintf((char*)tx_buf, TX_BUF_LOG_SIZE, fmt, args);
    va_end(args);

    if (formatted_length <= 0) {
        if (scheduler_is_running) {
            (void)xSemaphoreGive(log_mutex);
        } else {
            sys_log_unlock();
        }
        return;
    }

    /*
     * vsnprintf reports the untruncated size. Clamp DMA to bytes that really
     * exist and never transmit the terminating NUL on a long message.
     */
    transfer_length = (formatted_length < (int)TX_BUF_LOG_SIZE)
        ? (uint16_t)formatted_length
        : (uint16_t)(TX_BUF_LOG_SIZE - 1U);

    /*
     * Runtime ownership comes from the mutex, while boot ownership was already
     * acquired by sys_log_try_lock(). Mark DMA busy in both cases before start.
     */
    if (scheduler_is_running) {
        uint32_t interrupt_state = __get_PRIMASK();
        __disable_irq();
        is_dma_log_busy = true;
        if (interrupt_state == 0U) {
            __enable_irq();
        }
    }

    DMA_Cmd(DMA2_Stream7, DISABLE);
    while(DMA_GetCmdStatus(DMA2_Stream7) != DISABLE);
    DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_TCIF7 | DMA_FLAG_HTIF7 | DMA_FLAG_TEIF7 | DMA_FLAG_DMEIF7 | DMA_FLAG_FEIF7);
    DMA2_Stream7->NDTR = transfer_length;
    DMA2_Stream7->M0AR = (uint32_t)tx_buf;
    DMA_Cmd(DMA2_Stream7, ENABLE);

    if (scheduler_is_running) {
        BaseType_t status = xSemaphoreTake(dma_complete, portMAX_DELAY);
        configASSERT(status == pdTRUE);
        configASSERT(xSemaphoreGive(log_mutex) == pdTRUE);
    }
}

void sys_log_dma_complete_from_isr(void)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    /*
     * A normal-mode DMA stream has stopped at this point. Releasing ownership
     * here lets the next message reuse the static buffer safely.
     */
    is_dma_log_busy = false;

    /*
     * Boot-time transfers have no waiting task. Once runtime logging has begun,
     * wake the mutex owner and request an immediate context switch if needed.
     */
    if (scheduler_has_started && (dma_complete != NULL)) {
        (void)xSemaphoreGiveFromISR(dma_complete,
                                    &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

void sys_log_send_char(char c)
{
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, c);
}

void sys_log_send_string(const char* s)
{
    if (s != NULL) {
        while(*s) sys_log_send_char(*s++);
    }
}











