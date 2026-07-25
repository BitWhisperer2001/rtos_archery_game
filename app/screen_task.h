#ifndef SCREEN_TASK_H
#define SCREEN_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "app_state.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "timers.h"

#define TASK_UPDATE_SCREEN_PRIORITY  (2U)

#define SCREEN_UPDATE_BIT            (1U << 0)
#define SCREEN_GAME_OVER_BIT         (1U << 1)
#define SCREEN_GAME_START_BIT        (1U << 2)
#define SCREEN_SAVER_EXIT_BIT        (1U << 3)
#define SCREEN_SAVER_ADD_BIT         (1U << 4)
#define SCREEN_SAVER_REMOVE_BIT      (1U << 5)
#define SCREEN_RETURN_TO_START_BIT   (1U << 6)
#define SCREEN_PAUSE_BIT             (1U << 7)
#define SCREEN_RESUME_BIT            (1U << 8)
#define SCREEN_SAVER_NEXT_BIT        (1U << 9)
#define SCREEN_SAVER_PREVIOUS_BIT    (1U << 10)
#define SCREEN_SETTINGS_ENTER_BIT    (1U << 11)
#define SCREEN_SETTINGS_EXIT_BIT     (1U << 12)
#define SCREEN_SETTINGS_DIFFICULTY_BIT (1U << 13)
#define SCREEN_SETTINGS_EFFECT_BIT   (1U << 14)
#define SCREEN_SETTINGS_SOUND_BIT    (1U << 15)
#define SCREEN_WAKE_BIT              (1U << 16)

extern EventGroupHandle_t screen_event_state;
extern TaskHandle_t task_update_screen_handle;
extern TimerHandle_t timer_update_screen_handle;

void screen_task_create(void);
void task_screen_begin(void);

/*
 * Producers publish intent; only the screen task renders or transfers pixels.
 * These APIs are task-context APIs and must not be called from an ISR.
 */
void screen_request_frame(void);
void screen_request_game_start(void);
void screen_request_game_over(void);
void screen_request_return_to_start(void);
void screen_request_saver_exit(void);
void screen_request_saver_add_circle(void);
void screen_request_saver_remove_circle(void);
void screen_request_saver_next_effect(void);
void screen_request_saver_previous_effect(void);
void screen_request_pause(void);
void screen_request_resume(void);
void screen_request_settings_enter(void);
void screen_request_settings_exit(void);
void screen_request_settings_difficulty(void);
void screen_request_settings_effect(void);
void screen_request_settings_sound(void);
void screen_request_wake(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_TASK_H */
