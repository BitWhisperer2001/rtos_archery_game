#ifndef SCREEN_TASK_H
#define SCREEN_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

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

/*
 * The screen mode is also the application lifecycle state.  Keeping it here
 * makes the OLED owner responsible for visible state transitions.
 */
typedef enum {
    SCREEN_MODE_WAITING_FOR_START = 0,
    SCREEN_MODE_SCREENSAVER,
    SCREEN_MODE_START_PENDING,
    SCREEN_MODE_RUNNING,
    /*
     * Keep the transition distinct from the interactive game-over screen.
     * Input is ignored while the score is saved and the end sound is playing.
     */
    SCREEN_MODE_GAME_OVER_TRANSITION,
    SCREEN_MODE_GAME_OVER,
    SCREEN_MODE_RETURN_TO_START_PENDING
} screen_mode_t;

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
screen_mode_t screen_get_mode(void);

/*
 * The caller must already hold game_state_mutex.  Workers use this second
 * check immediately before mutation to close the check-then-lock race.
 */
bool screen_gameplay_is_running_locked(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_TASK_H */
