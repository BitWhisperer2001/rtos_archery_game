#include <stdbool.h>
#include <stdint.h>

#include "archery.h"

#include "arrow.h"
#include "bitmap.h"
#include "game_state.h"
#include "screen_task.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

EventGroupHandle_t archery_event_state = NULL;
TaskHandle_t task_archery_update_handle = NULL;
game_archery_t game_archery;

static void archery_task_handler(void *param)
{
    (void)param;

    for (;;) {
        EventBits_t bits = xEventGroupWaitBits(archery_event_state, ALL_EVENT,
                                               pdTRUE, pdFALSE,
                                               portMAX_DELAY);

        game_state_lock();

        /*
         * Recheck lifecycle under the same mutex used for object mutation.
         * Game over can therefore never slip between a mode check and this
         * update, which is essential when a new session reuses the same task.
         */
        if (!screen_gameplay_is_running_locked()) {
            game_state_unlock();
            continue;
        }

        if ((bits & ARCHERY_DOWN_BIT) != 0U) {
            game_archery.y += ARCHERY_Y_STEP;
            if (game_archery.y > ARCHERY_DOWN_THRESHOLD) {
                game_archery.y = ARCHERY_DOWN_THRESHOLD;
            }
        }

        if ((bits & ARCHERY_UP_BIT) != 0U) {
            game_archery.y -= ARCHERY_Y_STEP;
            if (game_archery.y < ARCHERY_UP_THRESHOLD) {
                game_archery.y = ARCHERY_UP_THRESHOLD;
            }
        }

        /*
         * The empty-quiver sprite is derived from queue ownership, avoiding a
         * second counter that could drift from the real free-arrow pool.
         */
        game_archery.action_img =
            (uxQueueMessagesWaiting(arrow_free_queue) == 0U)
                ? bitmap_archery_II
                : bitmap_archery_I;

        game_state_unlock();

        /* Binary semaphore intentionally coalesces missed gameplay ticks. */
        (void)xSemaphoreGive(semphr_task_arrow_update);
    }
}

void archery_reset_for_new_game(void)
{
    game_state_lock();

    /* Restore every mutable field; no state may leak from the previous game. */
    game_archery.action_img = bitmap_archery_I;
    game_archery.visible = true;
    game_archery.x = ARCHERY_X;
    game_archery.y = ARCHERY_Y_START;

    game_state_unlock();
}

void archery_task_create(void)
{
    BaseType_t status;

    /* Boot and soft restart deliberately share one initialization path. */
    archery_reset_for_new_game();

    archery_event_state = xEventGroupCreate();
    configASSERT(archery_event_state != NULL);

    status = xTaskCreate(archery_task_handler, "archery update", 512U, NULL,
                         ARCHERY_TASK_PRIORYTY,
                         &task_archery_update_handle);
    configASSERT(status == pdPASS);

    /*
     * Do not publish ARCHERY_INIT_BIT during boot.  screen_start_game() sends
     * the first event only after the user confirms with OK.
     */
}
