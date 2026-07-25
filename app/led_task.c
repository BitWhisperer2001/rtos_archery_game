#include <stdint.h>
#include <stdbool.h>
#include "system_init.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "led_task.h"
#include "led.h"
#include "system_fault.h"
#include "cmsis_gcc.h"

TimerHandle_t timer_led_life_handle = NULL;
static bool is_boot_marked_successful = false;

void vApplicationIdleHook(void)
{
    /*
     * Idle is the first trustworthy boot-success checkpoint: all startup
     * resources exist and the scheduler is dispatching normally.
     */
    if (!is_boot_marked_successful) {
        system_fault_mark_boot_success();
        is_boot_marked_successful = true;
    }
    led_toggle(LED_ONBOARD);

    /* Sleep until the next interrupt instead of burning power in the idle loop. */
    __WFI();
}

static void led_life_task_callback(TimerHandle_t xTimer)
{
    unused(xTimer);
    led_toggle(LED_LIFE);
}

void led_task_create(void)
{
    led_setup(LED_LIFE | LED_ONBOARD);
    timer_led_life_handle = xTimerCreate("timer toggle led life", pdMS_TO_TICKS(700), pdTRUE, NULL, led_life_task_callback);
    /* Fail observably instead of freezing in an anonymous local loop. */
    configASSERT(timer_led_life_handle != NULL);
    configASSERT(xTimerStart(timer_led_life_handle, 0U) == pdPASS);
}
