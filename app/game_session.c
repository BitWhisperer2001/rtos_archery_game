#include <stdint.h>

#include "game_session.h"

#include "archery.h"
#include "arrow.h"
#include "bang.h"
#include "border.h"
#include "buzzer_task.h"
#include "meteoroid.h"
#include "screen_task.h"

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
    configASSERT(screen_get_mode() != SCREEN_MODE_RUNNING);

    /*
     * Clear old pipeline commands before restoring object state.  The periodic
     * timers are already stopped and workers reject non-RUNNING sessions.
     */
    (void)xEventGroupClearBits(archery_event_state, ALL_EVENT);
    (void)xEventGroupClearBits(
        buzzer_event_state,
        ARROW_SHOT_BIT | ARCHERY_NO_ARROW_BIT | METEOROID_DESTROY);
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
    arrow_reset_for_new_game();
    archery_reset_for_new_game();
    border_reset_for_new_game();
    meteoroid_reset_for_new_game();
    bang_reset_for_new_game();
}
