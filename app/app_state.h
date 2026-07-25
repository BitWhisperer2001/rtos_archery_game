#ifndef APP_STATE_H
#define APP_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
    APP_STATE_WAITING_FOR_START = 0,
    APP_STATE_SCREENSAVER,
    APP_STATE_DISPLAY_SLEEP,
    APP_STATE_START_PENDING,
    APP_STATE_RUNNING,
    APP_STATE_PAUSED,
    APP_STATE_GAME_OVER_TRANSITION,
    APP_STATE_GAME_OVER,
    APP_STATE_RETURN_TO_START_PENDING,
    APP_STATE_SETTINGS
} app_state_t;

void app_state_init(void);
app_state_t app_state_get(void);
void app_state_set(app_state_t state);
bool app_state_transition(app_state_t expected, app_state_t desired);

/*
 * Caller must hold game_state_mutex.  Object workers use this form so the
 * lifecycle check and their state mutation remain one indivisible transaction.
 */
bool app_state_is_running_locked(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_H */
