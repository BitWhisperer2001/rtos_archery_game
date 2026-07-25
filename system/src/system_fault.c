#include "system_fault.h"

#include "cmsis_gcc.h"
#include "stm32f4xx.h"
#include "system_log.h"

#define SYSTEM_CRASH_MAGIC       (0x43524153UL) /* ASCII "CRAS" */
#define SYSTEM_RESET_RETRY_LIMIT (3UL)

/* Linker bounds let the fault path validate a possibly corrupted stack frame. */
extern uint32_t __SramStart;
extern uint32_t __SramEnd;

/*
 * .noinit is intentionally excluded from Reset_Handler's zeroing loop. The
 * magic field distinguishes a real crash record from random power-on SRAM.
 */
volatile system_crash_record_t g_system_crash_record
    __attribute__((section(".noinit.system_crash"), used));

static bool system_debugger_is_attached(void)
{
    return (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U;
}

static void system_fault_reset_or_halt(void) __attribute__((noreturn));
static void system_fault_reset_or_halt(void)
{
    __DSB();
    __ISB();

    /* A breakpoint keeps the exact fault context visible during development. */
    if (system_debugger_is_attached()) {
        __BKPT(0);
    }

    /*
     * Repeated deterministic failures stop after a few resets, preventing an
     * endless reset storm while still allowing transient faults to recover.
     */
    if (g_system_crash_record.reset_count >= SYSTEM_RESET_RETRY_LIMIT) {
        __disable_irq();
        for (;;) {
            __WFI();
        }
    }

    NVIC_SystemReset();
    for (;;) {
        __NOP();
    }
}

void system_panic(system_panic_reason_t reason)
{
    __disable_irq();

    if (g_system_crash_record.magic == SYSTEM_CRASH_MAGIC) {
        g_system_crash_record.reset_count++;
    } else {
        g_system_crash_record.reset_count = 1U;
    }

    g_system_crash_record.magic = SYSTEM_CRASH_MAGIC;
    g_system_crash_record.reason = (uint32_t)reason;
    /* IPSR identifies the exception/IRQ even when it uses Default_Handler. */
    g_system_crash_record.active_vector = __get_IPSR();
    g_system_crash_record.exception_return = 0U;

    /*
     * Register fields are meaningful only for a captured HardFault. Clear
     * stale values so an ordinary assert cannot masquerade as a CPU exception.
     */
    g_system_crash_record.r0 = 0U;
    g_system_crash_record.r1 = 0U;
    g_system_crash_record.r2 = 0U;
    g_system_crash_record.r3 = 0U;
    g_system_crash_record.r12 = 0U;
    g_system_crash_record.lr = 0U;
    g_system_crash_record.pc = 0U;
    g_system_crash_record.xpsr = 0U;
    if (reason != SYSTEM_PANIC_ASSERT) {
        g_system_crash_record.source_line = 0U;
    }
    g_system_crash_record.cfsr = SCB->CFSR;
    g_system_crash_record.hfsr = SCB->HFSR;
    g_system_crash_record.dfsr = SCB->DFSR;
    g_system_crash_record.afsr = SCB->AFSR;
    g_system_crash_record.mmfar = SCB->MMFAR;
    g_system_crash_record.bfar = SCB->BFAR;

    system_fault_reset_or_halt();
}

void system_assert_failed(const char *file, uint32_t line)
{
    /*
     * File is deliberately not copied into the retained record: storing a
     * pointer is enough in a live debugger, while line remains valid offline.
     */
    (void)file;
    g_system_crash_record.source_line = line;
    system_panic(SYSTEM_PANIC_ASSERT);
}

/*
 * Every Cortex-M fault wrapper passes the active exception frame and EXC_RETURN
 * to one C recorder. Strong symbols override startup's weak default aliases.
 */
#define SYSTEM_DEFINE_FAULT_WRAPPER(handler_name)                            \
    void handler_name(void) __attribute__((naked));                           \
    void handler_name(void)                                                   \
    {                                                                         \
        __asm volatile (                                                      \
            "tst lr, #4                         \n"                            \
            "ite eq                             \n"                            \
            "mrseq r0, msp                      \n"                            \
            "mrsne r0, psp                      \n"                            \
            "mov r1, lr                         \n"                            \
            "b system_cpu_fault_handler_c       \n"                            \
        );                                                                    \
    }

SYSTEM_DEFINE_FAULT_WRAPPER(HardFault_Handler)
SYSTEM_DEFINE_FAULT_WRAPPER(MemManage_Handler)
SYSTEM_DEFINE_FAULT_WRAPPER(BusFault_Handler)
SYSTEM_DEFINE_FAULT_WRAPPER(UsageFault_Handler)

void system_cpu_fault_handler_c(uint32_t *stack, uint32_t exception_return)
    __attribute__((used, noreturn));
void system_cpu_fault_handler_c(uint32_t *stack, uint32_t exception_return)
{
    uint32_t active_vector;
    uintptr_t stack_address = (uintptr_t)stack;
    uintptr_t sram_start = (uintptr_t)&__SramStart;
    uintptr_t sram_end = (uintptr_t)&__SramEnd;
    bool stack_frame_is_valid;

    __disable_irq();

    if (g_system_crash_record.magic == SYSTEM_CRASH_MAGIC) {
        g_system_crash_record.reset_count++;
    } else {
        g_system_crash_record.reset_count = 1U;
    }

    active_vector = __get_IPSR();
    g_system_crash_record.magic = SYSTEM_CRASH_MAGIC;
    g_system_crash_record.reason = (active_vector == 3U)
        ? SYSTEM_PANIC_HARDFAULT
        : SYSTEM_PANIC_CPU_FAULT;
    g_system_crash_record.active_vector = active_vector;
    g_system_crash_record.exception_return = exception_return;
    g_system_crash_record.source_line = 0U;

    /*
     * The Cortex-M core frame starts at the selected SP for both basic and
     * floating-point-extended exceptions. Validate it before reading because a
     * stacking BusFault can corrupt SP itself.
     */
    stack_frame_is_valid = (stack_address >= sram_start) &&
                           (stack_address <= (sram_end - (8U * sizeof(uint32_t)))) &&
                           ((stack_address & 0x3U) == 0U);
    if (stack_frame_is_valid) {
        g_system_crash_record.r0 = stack[0];
        g_system_crash_record.r1 = stack[1];
        g_system_crash_record.r2 = stack[2];
        g_system_crash_record.r3 = stack[3];
        g_system_crash_record.r12 = stack[4];
        g_system_crash_record.lr = stack[5];
        g_system_crash_record.pc = stack[6];
        g_system_crash_record.xpsr = stack[7];
    } else {
        g_system_crash_record.r0 = 0U;
        g_system_crash_record.r1 = 0U;
        g_system_crash_record.r2 = 0U;
        g_system_crash_record.r3 = 0U;
        g_system_crash_record.r12 = 0U;
        g_system_crash_record.lr = 0U;
        g_system_crash_record.pc = 0U;
        g_system_crash_record.xpsr = 0U;
    }
    g_system_crash_record.cfsr = SCB->CFSR;
    g_system_crash_record.hfsr = SCB->HFSR;
    g_system_crash_record.dfsr = SCB->DFSR;
    g_system_crash_record.afsr = SCB->AFSR;
    g_system_crash_record.mmfar = SCB->MMFAR;
    g_system_crash_record.bfar = SCB->BFAR;

    system_fault_reset_or_halt();
}

void system_fault_report(void)
{
    if (g_system_crash_record.magic != SYSTEM_CRASH_MAGIC) {
        return;
    }

    /*
     * Keep the boot report compact because UART logging uses a 128-byte DMA
     * buffer. Multiple calls serialize through the logging driver.
     */
    DBG_LOG("Previous crash: reason=%lu count=%lu line=%lu vector=%lu",
            (unsigned long)g_system_crash_record.reason,
            (unsigned long)g_system_crash_record.reset_count,
            (unsigned long)g_system_crash_record.source_line,
            (unsigned long)g_system_crash_record.active_vector);
    DBG_LOG("PC=%08lx LR=%08lx xPSR=%08lx",
            (unsigned long)g_system_crash_record.pc,
            (unsigned long)g_system_crash_record.lr,
            (unsigned long)g_system_crash_record.xpsr);
    DBG_LOG("CFSR=%08lx HFSR=%08lx BFAR=%08lx",
            (unsigned long)g_system_crash_record.cfsr,
            (unsigned long)g_system_crash_record.hfsr,
            (unsigned long)g_system_crash_record.bfar);

    /*
     * Do not clear here. If later initialization crashes again, reset_count
     * must survive so the retry limiter can detect a deterministic boot loop.
     */
}

void system_fault_mark_boot_success(void)
{
    /*
     * Reaching the idle task proves that kernel and application resources were
     * created successfully. Retained register data stays debugger-readable.
     */
    g_system_crash_record.magic = 0U;
    g_system_crash_record.reset_count = 0U;
}
