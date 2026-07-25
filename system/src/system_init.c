#include <stdint.h>
#include <stdbool.h>
#include "system_init.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_flash.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
#include "misc.h"

#include "system_fault.h"
#include "system_log.h"

volatile uint32_t g_cnt = 0;

void sys_clock_config(void)
{
    /*
     * SystemInit() already configured the F411 at 100 MHz before main. Keep
     * this application-level entry point to preserve the original boot flow,
     * but make it validate the clock instead of destructively configuring it
     * a second time.
     */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    SystemCoreClockUpdate();
    if (SystemCoreClock != SYSTEM_CORE_CLOCK_HZ) {
        system_panic(SYSTEM_PANIC_CLOCK_CONFIGURATION);
    }

    /*
     * Keep configurable Cortex-M faults separate instead of escalating all of
     * them into HardFault. Divide-by-zero also becomes an observable UsageFault.
     */
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk |
                  SCB_SHCSR_BUSFAULTENA_Msk |
                  SCB_SHCSR_USGFAULTENA_Msk;
    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
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
    /* SPL has no NVIC_StructInit, so zero-initialize every reserved field. */
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    /* Initialize every SPL field first; no register receives stack garbage. */
    TIM_TimeBaseStructInit(&TIM_InitStructure);
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

    /* GPIO_StructInit documents defaults and protects future SPL extensions. */
    GPIO_StructInit(&GPIO_InitStructure);
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
    /* Zero initialization prevents reserved NVIC fields from containing junk. */
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    /*
     * DMA_Init writes burst/FIFO fields as well. DMA_StructInit is essential
     * even though direct mode is selected below.
     */
    DMA_StructInit(&DMA_InitStructure);
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
    /*
     * Error interrupts are as important as transfer-complete: the logger uses
     * them to release its buffer instead of deadlocking after a DMA failure.
     */
    DMA_ITConfig(DMA2_Stream7, DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_FE,
                 ENABLE);
    DMA_Cmd(DMA2_Stream7, DISABLE);
}

/* Select USART1 to print information for debug */
void sys_log_config(void)
{
    USART_InitTypeDef USART_InitStructure;
    /* Zero initialization prevents reserved NVIC fields from containing junk. */
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    USART_StructInit(&USART_InitStructure);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    USART_DeInit(USART1);
    /* 115200 baud keeps periodic stack diagnostics from monopolizing a task. */
    USART_InitStructure.USART_BaudRate = SYSTEM_LOG_BAUDRATE;
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

    /*
     * Create logger synchronization before the boot report and every app task.
     * A failure is fatal because later diagnostics depend on these resources.
     */
    if (!sys_log_create_resources()) {
        system_panic(SYSTEM_PANIC_RESOURCE_CREATION);
    }
}

/*
 * A small append-only record turns one destructive sector erase per high score
 * into thousands of power-loss-detectable writes. CRC is committed last, so a
 * partially programmed record is never accepted after reset.
 */
#define SCORE_RECORD_MAGIC       (0x53434F52UL) /* ASCII "SCOR" */
#define SCORE_RECORD_CRC_SEED    (0xA5A55A5AUL)

typedef struct
{
    uint32_t magic;
    uint32_t sequence;
    uint32_t score;
    uint32_t crc;
} score_record_t;

/* Journal layout is a persistent ABI; catch accidental padding changes. */
_Static_assert(sizeof(score_record_t) == 16U, "score_record_t Flash layout must remain 16 bytes");

static uint32_t score_record_crc(const score_record_t *record)
{
    return record->magic ^ record->sequence ^ record->score ^ SCORE_RECORD_CRC_SEED;
}

static bool score_record_is_erased(const volatile score_record_t *record)
{
    return (record->magic == UINT32_MAX) &&
           (record->sequence == UINT32_MAX) &&
           (record->score == UINT32_MAX) &&
           (record->crc == UINT32_MAX);
}

static bool score_record_is_valid(const volatile score_record_t *record)
{
    score_record_t snapshot;

    /* Read once into a normal object so validation uses a coherent snapshot. */
    snapshot.magic = record->magic;
    snapshot.sequence = record->sequence;
    snapshot.score = record->score;
    snapshot.crc = record->crc;

    return (snapshot.magic == SCORE_RECORD_MAGIC) &&
           (snapshot.crc == score_record_crc(&snapshot));
}

static bool score_sequence_is_newer(uint32_t candidate, uint32_t current)
{
    /*
     * Signed modular distance keeps ordering correct when the 32-bit journal
     * sequence eventually wraps after an extremely long service life.
     */
    return (int32_t)(candidate - current) > 0;
}

static bool score_address_is_valid(uint32_t address)
{
    return address == (uint32_t)(uintptr_t)&__nvm_start__;
}

uint32_t sys_read_score_in_flash(uint32_t add)
{
    const uintptr_t start = (uintptr_t)&__nvm_start__;
    const uintptr_t end = (uintptr_t)&__nvm_end__;
    const volatile score_record_t *record;
    uint32_t latest_score = 0U;
    uint32_t latest_sequence = 0U;
    bool found = false;

    if (!score_address_is_valid(add)) {
        return 0U;
    }

    for (uintptr_t address = start;
         (address + sizeof(score_record_t)) <= end;
         address += sizeof(score_record_t)) {
        record = (const volatile score_record_t *)address;

        if (score_record_is_erased(record)) {
            break;
        }

        if (score_record_is_valid(record) &&
            ((!found) ||
             score_sequence_is_newer(record->sequence, latest_sequence))) {
            latest_sequence = record->sequence;
            latest_score = record->score;
            found = true;
        }
    }

    return latest_score;
}

bool sys_save_score_into_flash(uint32_t add, uint32_t data)
{
    const uintptr_t start = (uintptr_t)&__nvm_start__;
    const uintptr_t end = (uintptr_t)&__nvm_end__;
    const volatile score_record_t *record;
    uintptr_t target = end;
    uint32_t latest_sequence = 0U;
    bool found_previous = false;
    FLASH_Status status = FLASH_COMPLETE;
    score_record_t next;

    if (!score_address_is_valid(add)) {
        return false;
    }

    /* Locate both the newest valid record and the first unused journal slot. */
    for (uintptr_t address = start;
         (address + sizeof(score_record_t)) <= end;
         address += sizeof(score_record_t)) {
        record = (const volatile score_record_t *)address;

        if (score_record_is_erased(record)) {
            target = address;
            break;
        }

        if (score_record_is_valid(record) &&
            ((!found_previous) ||
             score_sequence_is_newer(record->sequence, latest_sequence))) {
            latest_sequence = record->sequence;
            found_previous = true;
        }
    }

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    /* Erase only after all journal slots have been consumed. */
    if (target == end) {
        status = FLASH_EraseSector(FLASH_Sector_7, VoltageRange_3);
        target = start;
    }

    next.magic = SCORE_RECORD_MAGIC;
    next.sequence = found_previous ? (latest_sequence + 1U) : 0U;
    next.score = data;
    next.crc = score_record_crc(&next);

    if (status == FLASH_COMPLETE) {
        status = FLASH_ProgramWord((uint32_t)(target + 0U), next.magic);
    }
    if (status == FLASH_COMPLETE) {
        status = FLASH_ProgramWord((uint32_t)(target + 4U), next.sequence);
    }
    if (status == FLASH_COMPLETE) {
        status = FLASH_ProgramWord((uint32_t)(target + 8U), next.score);
    }
    if (status == FLASH_COMPLETE) {
        /* CRC is the commit marker and must always be programmed last. */
        status = FLASH_ProgramWord((uint32_t)(target + 12U), next.crc);
    }

    FLASH_Lock();

    /*
     * Read back the committed record. A brownout or marginal Flash write must
     * be reported to the UI instead of being accepted solely from SPL status.
     */
    record = (const volatile score_record_t *)target;
    return (status == FLASH_COMPLETE) &&
           score_record_is_valid(record) &&
           (record->score == data);
}

