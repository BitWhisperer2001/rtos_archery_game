#ifndef SYSTEM_FAULT_H
#define SYSTEM_FAULT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Stable reason codes make crash records useful even without debug symbols. */
typedef enum
{
    SYSTEM_PANIC_NONE = 0,
    SYSTEM_PANIC_ASSERT,
    SYSTEM_PANIC_CLOCK_CONFIGURATION,
    SYSTEM_PANIC_RTOS_STACK_OVERFLOW,
    SYSTEM_PANIC_RTOS_MALLOC_FAILED,
    SYSTEM_PANIC_RESOURCE_CREATION,
    SYSTEM_PANIC_SCHEDULER_RETURNED,
    SYSTEM_PANIC_HARDFAULT,
    SYSTEM_PANIC_UNHANDLED_EXCEPTION,
    SYSTEM_PANIC_CPU_FAULT,
    SYSTEM_PANIC_LIBC_EXIT
} system_panic_reason_t;

typedef struct
{
    uint32_t magic;
    uint32_t reason;
    uint32_t reset_count;
    uint32_t active_vector;
    uint32_t exception_return;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t dfsr;
    uint32_t afsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t source_line;
} system_crash_record_t;

/*
 * All fatal exits converge here so behavior is observable and consistent.
 * The function records context, optionally breaks under a debugger, then reset.
 */
void system_panic(system_panic_reason_t reason)
    __attribute__((noreturn));
void system_assert_failed(const char *file, uint32_t line)
    __attribute__((noreturn));

/* Report after UART init; clear only after the scheduler reaches idle safely. */
void system_fault_report(void);
void system_fault_mark_boot_success(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_FAULT_H */
