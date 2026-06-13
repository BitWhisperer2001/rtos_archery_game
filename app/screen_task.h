#ifndef SCREEN_TASK_H
#define SCREEN_TASK_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "event_groups.h"
#include "timers.h"

extern EventGroupHandle_t screen_event_state;
extern TaskHandle_t task_update_screen_handle;
extern TimerHandle_t timer_update_screen_handle;

#define TASK_UPDATE_SCREEN_PRIORITY 2

#define SCREEN_UPDATE_BIT           (1 << 0)
#define SCREEN_GAME_OVER_BIT        (1 << 1)

extern void screen_task_create(void);
extern void task_screen_begin(void);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // SCREEN_TASK_H