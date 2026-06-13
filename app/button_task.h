#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "timers.h"

extern TimerHandle_t bt_polling_timer_handle;
extern TaskHandle_t task_buzzer_handle;

extern void button_task_create(void);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // BUTTON_TASK_H
