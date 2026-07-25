#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "system_init.h"
#include "system_log.h"
#include "system_fault.h"
#include "app.h"
#include "app_settings.h"
#include "app_state.h"
#include "difficulty_manager.h"
#include "game_state.h"
#include "input_task.h"
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

#define APP_SCHEDULER_HEAP_RESERVE_BYTES (8U * 1024U)

int main(void)
{
    sys_clock_config();
    sys_tick_config();
    sys_log_config();

    /* Report a retained fault only after DMA/UART logging is operational. */
    system_fault_report();
    
    /* Create shared-state synchronization before any worker task is exposed. */
    game_state_init();
    app_state_init();
    app_settings_init();

    /*
     * Difficulty is data-driven and must be selected before object modules
     * initialize their first-session cadence.
     */
    app_settings_t settings = app_settings_get();
    difficulty_manager_reset(settings.difficulty);

    arrow_task_create();
    archery_task_create();
    border_task_create();
    screen_task_create();
    meteoroid_task_create();
    buzzer_task_create();
    bang_task_create();
    led_task_create();
    input_task_create();
    button_task_create();

    stack_monitor_task_create();

    /*
     * FreeRTOS allocates the Idle and Timer task TCBs/stacks only when the
     * scheduler starts.  Reserve more than their measured requirement so a
     * future task cannot recreate the deceptive "start screen but frozen"
     * failure.  Check before drawing the splash to make boot failure explicit.
     */
    configASSERT(xPortGetFreeHeapSize() >=
                 APP_SCHEDULER_HEAP_RESERVE_BYTES);
    DBG_LOG("Pre-scheduler heap: free=%lu bytes",
            (unsigned long)xPortGetFreeHeapSize());

    /*
     * Render only after every application resource exists and the kernel boot
     * reserve has been verified.  The start cue is queued after the OLED
     * transfer and consumed by the audio owner when scheduling begins.
     */
    task_screen_begin();
    vTaskStartScheduler();

    /* FreeRTOS returns only when it cannot create its internal tasks. */
    system_panic(SYSTEM_PANIC_SCHEDULER_RETURNED);
}
