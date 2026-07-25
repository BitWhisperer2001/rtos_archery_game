#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "system_log.h"
#include "stack_monitor.h"

#define STACK_MONITOR_MAX_TASKS       14U
#define STACK_MONITOR_PERIOD_MS       5000U
#define STACK_MONITOR_STACK_WORDS     256U

static TaskStatus_t task_status[STACK_MONITOR_MAX_TASKS];

static void stack_monitor_task(void *parameter)
{
    (void)parameter;

    for (;;){
        UBaseType_t task_count;

        task_count = uxTaskGetSystemState(task_status, STACK_MONITOR_MAX_TASKS, NULL);
        DBG_LOG("---- Stack watermark ----");
        for (UBaseType_t i = 0; i < task_count; i++){
            uint32_t free_words = (uint32_t)task_status[i].usStackHighWaterMark;

            uint32_t free_bytes = free_words * sizeof(StackType_t);

            DBG_LOG("%s: free=%lu words, %lu bytes", task_status[i].pcTaskName, (unsigned long)free_words, (unsigned long)free_bytes);
        }
        if (task_count >= STACK_MONITOR_MAX_TASKS){
            DBG_LOG("WARNING: task list truncated");
        }
        DBG_LOG("Heap: free=%lu min-ever=%lu bytes",
                (unsigned long)xPortGetFreeHeapSize(),
                (unsigned long)xPortGetMinimumEverFreeHeapSize());
        vTaskDelay(pdMS_TO_TICKS(STACK_MONITOR_PERIOD_MS));
    }
}

void stack_monitor_task_create(void)
{
    BaseType_t status;
    status = xTaskCreate(stack_monitor_task, "StackMon", STACK_MONITOR_STACK_WORDS, NULL, tskIDLE_PRIORITY + 1U, NULL);
    configASSERT(status == pdPASS);
}
