#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "system_init.h"
#include "system_log.h"
#include "app.h"
#include "meteoroid.h"
#include "button_task.h"
#include "screen_task.h"
#include "buzzer_task.h"
#include "led_task.h"
#include "archery.h"
#include "arrow.h"
#include "border.h"
#include "bang.h"
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
    
    task_screen_begin();

    arrow_task_create();
    archery_task_create();
    border_task_create();
    button_task_create();
    screen_task_create();
    meteoroid_task_create();
    buzzer_task_create();
    bang_task_create();
    led_task_create();

    vTaskStartScheduler();
    return 0;
}

// void task_delete_all(void)
// {
//     // vTaskEndScheduler();
//     // delete all task
//     vTaskDelete(task_arrow_update_handle);
//     vTaskDelete(task_archery_update_handle);
//     vTaskDelete(task_border_update_handle);
//     vTaskDelete(task_update_screen_handle);
//     vTaskDelete(task_buzzer_handle);
//     vTaskDelete(task_update_bang_handle);
//     // delete all timer
//     xTimerDelete(timer_update_screen_handle, 0);
//     xTimerDelete(meteoroid_timer, 0);
//     // delete all semaphore
//     vSemaphoreDelete(semphr_task_arrow_update);
//     vSemaphoreDelete(semphr_task_update_border);
//     vSemaphoreDelete(semphr_task_bang_update);
//     // delete all event group
//     vEventGroupDelete(archery_event_state);
//     vEventGroupDelete(screen_event_state);
//     vEventGroupDelete(buzzer_event_state);
// }

// void game_restart(void)
// {
//     // vTaskEndScheduler();

//     arrow_task_create();
//     archery_task_create();
//     border_task_create();
//     // button_task_create();
//     screen_task_create();
//     meteoroid_task_create();
//     buzzer_task_create();
//     bang_task_create();
//     // led_task_create();

//     // vTaskStartScheduler();
// }





