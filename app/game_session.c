#include <stdint.h>

#include "game_session.h"

#include "app_settings.h"
#include "app_state.h"
#include "archery.h"
#include "arrow.h"
#include "bang.h"
#include "border.h"
#include "buzzer_task.h"
#include "difficulty_manager.h"
#include "game_stats.h"
#include "meteoroid.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include "task.h"

static void game_session_drain_binary_semaphore(SemaphoreHandle_t semaphore)
{
    /*
     * Binary semaphores coalesce work, but explicitly draining them documents
     * that a new session never consumes a wake-up produced by the old session.
     */
    while (xSemaphoreTake(semaphore, 0U) == pdTRUE) {
        /* Intentionally empty. */
    }
}

void game_session_reset(void)
{
    configASSERT(app_state_get() != APP_STATE_RUNNING);

    /*
     * Clear old pipeline commands before restoring object state.  The periodic
     * timers are already stopped and workers reject non-RUNNING sessions.
     */
    (void)xEventGroupClearBits(archery_event_state, ALL_EVENT);
    buzzer_flush_gameplay_cues();
    game_session_drain_binary_semaphore(semphr_task_arrow_update);
    game_session_drain_binary_semaphore(semphr_task_bang_update);
    game_session_drain_binary_semaphore(semphr_task_update_border);

    /*
     * A late timer notification must not become an immediate meteor update in
     * the next game.  Clear both the value and the pending notification state.
     */
    (void)ulTaskNotifyValueClear(task_meteoroid_update_handle, UINT32_MAX);
    (void)xTaskNotifyStateClear(task_meteoroid_update_handle);

    /*
     * Reset data in dependency order: queue ownership first, then consumers.
     * Tasks, queues, semaphores and timers remain allocated for system lifetime.
     */
    app_settings_t settings = app_settings_get();
    difficulty_manager_reset(settings.difficulty);
    game_stats_reset();

    arrow_reset_for_new_game();
    archery_reset_for_new_game();
    border_reset_for_new_game();
    meteoroid_reset_for_new_game();
    bang_reset_for_new_game();
}
