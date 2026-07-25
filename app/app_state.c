#include "app_state.h"

#include "game_state.h"

static app_state_t current_state = APP_STATE_WAITING_FOR_START;

void app_state_init(void)
{
    /*
     * game_state_mutex already exists when this is called.  Initializing via
     * the public setter keeps boot and later lifecycle transitions consistent.
     */
    app_state_set(APP_STATE_WAITING_FOR_START);
}

app_state_t app_state_get(void)
{
    app_state_t snapshot;

    game_state_lock();
    snapshot = current_state;
    game_state_unlock();

    return snapshot;
}

void app_state_set(app_state_t state)
{
    game_state_lock();
    current_state = state;
    game_state_unlock();
}

bool app_state_transition(app_state_t expected, app_state_t desired)
{
    bool changed = false;

    game_state_lock();

    if (current_state == expected) {
        current_state = desired;
        changed = true;
    }

    game_state_unlock();
    return changed;
}

bool app_state_is_running_locked(void)
{
    /*
     * Deliberately lock-free: callers already own game_state_mutex.  Taking a
     * non-recursive FreeRTOS mutex twice would deadlock.
     */
    return current_state == APP_STATE_RUNNING;
}
