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
#define SETTINGS_RECORD_MAGIC    (0x53455454UL) /* ASCII "SETT" */
#define JOURNAL_RECORD_CRC_SEED  (0xA5A55A5AUL)

typedef struct
{
    uint32_t magic;
    uint32_t sequence;
    uint32_t value;
    uint32_t crc;
} journal_record_t;

/*
 * This is byte-for-byte compatible with the previous score_record_t.  Existing
 * SCOR entries therefore remain readable after adding settings records.
 */
_Static_assert(sizeof(journal_record_t) == 16U,
               "journal_record_t Flash layout must remain 16 bytes");

typedef struct {
    bool found;
    uint32_t sequence;
    uint32_t value;
} journal_latest_t;

typedef struct {
    uintptr_t free_address;
    journal_latest_t score;
    journal_latest_t settings;
} journal_scan_t;

static uint32_t journal_record_crc(const journal_record_t *record)
{
    return record->magic ^ record->sequence ^ record->value ^
           JOURNAL_RECORD_CRC_SEED;
}

static bool journal_record_is_erased(const volatile journal_record_t *record)
{
    return (record->magic == UINT32_MAX) &&
           (record->sequence == UINT32_MAX) &&
           (record->value == UINT32_MAX) &&
           (record->crc == UINT32_MAX);
}

static bool journal_record_is_valid(const volatile journal_record_t *record)
{
    journal_record_t snapshot;

    /* Read once into a normal object so validation uses a coherent snapshot. */
    snapshot.magic = record->magic;
    snapshot.sequence = record->sequence;
    snapshot.value = record->value;
    snapshot.crc = record->crc;

    return ((snapshot.magic == SCORE_RECORD_MAGIC) ||
            (snapshot.magic == SETTINGS_RECORD_MAGIC)) &&
           (snapshot.crc == journal_record_crc(&snapshot));
}

static bool journal_sequence_is_newer(uint32_t candidate, uint32_t current)
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

static void journal_update_latest(journal_latest_t *latest,
                                  const volatile journal_record_t *record)
{
    if ((!latest->found) ||
        journal_sequence_is_newer(record->sequence, latest->sequence)) {
        latest->found = true;
        latest->sequence = record->sequence;
        latest->value = record->value;
    }
}

static journal_scan_t journal_scan(void)
{
    journal_scan_t scan = {
        .free_address = (uintptr_t)&__nvm_end__
    };
    const uintptr_t start = (uintptr_t)&__nvm_start__;
    const uintptr_t end = (uintptr_t)&__nvm_end__;
    const volatile journal_record_t *record;

    for (uintptr_t address = start;
         (address + sizeof(journal_record_t)) <= end;
         address += sizeof(journal_record_t)) {
        record = (const volatile journal_record_t *)address;

        if (journal_record_is_erased(record)) {
            scan.free_address = address;
            break;
        }

        if (!journal_record_is_valid(record)) {
            continue;
        }

        if (record->magic == SCORE_RECORD_MAGIC) {
            journal_update_latest(&scan.score, record);
        } else if (record->magic == SETTINGS_RECORD_MAGIC) {
            journal_update_latest(&scan.settings, record);
        }
    }

    return scan;
}

static FLASH_Status journal_program_record(uintptr_t address,
                                           uint32_t magic,
                                           uint32_t sequence,
                                           uint32_t value)
{
    FLASH_Status status = FLASH_COMPLETE;
    journal_record_t next = {
        .magic = magic,
        .sequence = sequence,
        .value = value
    };

    next.crc = journal_record_crc(&next);

    status = FLASH_ProgramWord((uint32_t)(address + 0U), next.magic);
    if (status == FLASH_COMPLETE) {
        status = FLASH_ProgramWord((uint32_t)(address + 4U), next.sequence);
    }
    if (status == FLASH_COMPLETE) {
        status = FLASH_ProgramWord((uint32_t)(address + 8U), next.value);
    }
    if (status == FLASH_COMPLETE) {
        /* CRC remains the final power-loss-safe commit marker. */
        status = FLASH_ProgramWord((uint32_t)(address + 12U), next.crc);
    }

    return status;
}

static bool journal_save(uint32_t magic, uint32_t value)
{
    const uintptr_t start = (uintptr_t)&__nvm_start__;
    const uintptr_t end = (uintptr_t)&__nvm_end__;
    journal_scan_t scan = journal_scan();
    journal_latest_t *target_latest =
        (magic == SCORE_RECORD_MAGIC) ? &scan.score : &scan.settings;
    journal_latest_t *other_latest =
        (magic == SCORE_RECORD_MAGIC) ? &scan.settings : &scan.score;
    uint32_t other_magic =
        (magic == SCORE_RECORD_MAGIC) ? SETTINGS_RECORD_MAGIC :
                                        SCORE_RECORD_MAGIC;
    uint32_t next_sequence =
        target_latest->found ? (target_latest->sequence + 1U) : 0U;
    uintptr_t target = scan.free_address;
    FLASH_Status status;

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    if (target == end) {
        /*
         * Sector compaction keeps the newest record of the other type before
         * appending the new value.  Score and settings therefore survive one
         * another even though STM32F411 erases Flash only by whole sector.
         */
        status = FLASH_EraseSector(FLASH_Sector_7, VoltageRange_3);
        target = start;

        if ((status == FLASH_COMPLETE) && other_latest->found) {
            status = journal_program_record(target, other_magic,
                                            other_latest->sequence,
                                            other_latest->value);
            target += sizeof(journal_record_t);
        }
    } else {
        status = FLASH_COMPLETE;
    }

    if (status == FLASH_COMPLETE) {
        status = journal_program_record(target, magic, next_sequence, value);
    }

    /*
     * Read back the committed record. A brownout or marginal Flash write must
     * be reported to the UI instead of being accepted solely from SPL status.
     */
    const volatile journal_record_t *record =
        (const volatile journal_record_t *)target;
    bool valid = (status == FLASH_COMPLETE) &&
                 journal_record_is_valid(record) &&
                 (record->magic == magic) &&
                 (record->value == value);

    FLASH_Lock();
    return valid;
}

uint32_t sys_read_score_in_flash(uint32_t add)
{
    if (!score_address_is_valid(add)) {
        return 0U;
    }

    journal_scan_t scan = journal_scan();
    return scan.score.found ? scan.score.value : 0U;
}

bool sys_save_score_into_flash(uint32_t add, uint32_t data)
{
    return score_address_is_valid(add) &&
           journal_save(SCORE_RECORD_MAGIC, data);
}

uint32_t sys_read_settings_in_flash(uint32_t default_value)
{
    journal_scan_t scan = journal_scan();
    return scan.settings.found ? scan.settings.value : default_value;
}

bool sys_save_settings_into_flash(uint32_t data)
{
    return journal_save(SETTINGS_RECORD_MAGIC, data);
}

