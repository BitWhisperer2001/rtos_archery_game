#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "system_init.h"
#include "system_log.h"
#include "system_fault.h"
#include "app.h"
#include "game_state.h"
#include "stack_monitor.h"

#include "button_task.h"
#include "screen_task.h"
#include "buzzer_task.h"
#include "led_task.h"

#include "archery.h"
#include "arrow.h"
#include "border.h"
#include "bang.h"
#include "meteoroid.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"

int main(void)
{
    sys_clock_config();
    sys_tick_config();
    sys_log_config();

    /* Report a retained fault only after DMA/UART logging is operational. */
    system_fault_report();
    
    /* Create shared-state synchronization before any worker task is exposed. */
    game_state_init();

    arrow_task_create();
    archery_task_create();
    border_task_create();
    button_task_create();
    screen_task_create();
    meteoroid_task_create();
    buzzer_task_create();
    bang_task_create();
    led_task_create();

    /*
     * Render only after the buzzer event group exists.  task_screen_begin()
     * transfers the prompt first, then queues SOUND_GAME_START; the buzzer task
     * consumes it immediately when the scheduler starts.
     */
    task_screen_begin();

    stack_monitor_task_create();
    vTaskStartScheduler();

    /* FreeRTOS returns only when it cannot create its internal tasks. */
    system_panic(SYSTEM_PANIC_SCHEDULER_RETURNED);
}
