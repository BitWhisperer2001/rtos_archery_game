#include "FreeRTOS.h"
#include "task.h"
#include "system_fault.h"

typedef enum
{
    FREERTOS_FAULT_NONE = 0,
    FREERTOS_FAULT_STACK_OVERFLOW,
    FREERTOS_FAULT_MALLOC_FAILED
} freertos_fault_t;

volatile freertos_fault_t g_freertos_fault = FREERTOS_FAULT_NONE;
TaskHandle_t volatile g_freertos_fault_task = NULL;
const char * volatile g_freertos_fault_task_name = NULL;

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /* Preserve the offending task for a live debugger before centralized reset. */
    g_freertos_fault = FREERTOS_FAULT_STACK_OVERFLOW;
    g_freertos_fault_task = xTask;
    g_freertos_fault_task_name = pcTaskName;

    system_panic(SYSTEM_PANIC_RTOS_STACK_OVERFLOW);
}

void vApplicationMallocFailedHook(void)
{
    g_freertos_fault = FREERTOS_FAULT_MALLOC_FAILED;
    g_freertos_fault_task = xTaskGetCurrentTaskHandle();

    system_panic(SYSTEM_PANIC_RTOS_MALLOC_FAILED);
}
