#include <stdint.h>
#include <stdbool.h>
#include "system_init.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "led_task.h"
#include "led.h"
#include "cmsis_gcc.h"

TimerHandle_t timer_led_life_handle = NULL;

void vApplicationIdleHook(void)
{
    led_toggle(LED_ONBOARD);
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
    if(timer_led_life_handle == NULL){
        __disable_irq();
        while(true);
    }
    xTimerStart(timer_led_life_handle, pdMS_TO_TICKS(500));
}