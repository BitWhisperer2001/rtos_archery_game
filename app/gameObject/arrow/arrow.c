#include <stdbool.h>
#include <stdint.h>

#include "arrow.h"

#include "app_state.h"
#include "archery.h"
#include "bang.h"
#include "bitmap.h"
#include "border.h"
#include "game_state.h"
#include "meteoroid.h"
#include "screen_task.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

game_arrow_t game_arrow[ARROW_MAX_NUM];
QueueHandle_t arrow_free_queue = NULL;

TaskHandle_t task_arrow_update_handle = NULL;
SemaphoreHandle_t semphr_task_arrow_update = NULL;

static void arrow_release(uint8_t index)
{
    BaseType_t status = xQueueSend(arrow_free_queue, &index, 0U);

    /*
     * A full queue here means an arrow slot was released twice, which is a
     * gameplay ownership bug and must be captured instead of silently ignored.
     */
    configASSERT(status == pdPASS);
}

static bool game_check_bang_locked(uint8_t arrow_index)
{
    /*
     * Callers hold game_state_mutex.  Lanes are uniformly spaced, so collision
     * logic stays data-driven instead of duplicating five near-identical blocks.
     */
    for (uint8_t lane = 0U; lane < METEROID_MAX_NUM; lane++) {
        int16_t lane_y = (int16_t)(METEOROID_Y_START + 2 +
                         ((int16_t)lane * METEOROID_Y_OFFSET));

        if (game_arrow[arrow_index].y != lane_y) {
            continue;
        }

        bool collision =
            game_meteoroid[lane].visible &&
            ((game_arrow[arrow_index].x + 5) >= game_meteoroid[lane].x);

        if (collision) {
            /*
             * Arrow, meteor and explosion become visible as one transaction.
             * Screen task can never snapshot a half-applied collision.
             */
            game_arrow[arrow_index].visible = false;
            game_meteoroid[lane].visible = false;
            game_bang[arrow_index].x = game_arrow[arrow_index].x + 2;
            game_bang[arrow_index].y = game_arrow[arrow_index].y - 5;
            game_bang[arrow_index].action_img = bitmap_bang_I;
            game_bang[arrow_index].state = 0U;
            game_bang[arrow_index].visible = true;
        }

        return collision;
    }

    return false;
}

static void task_arrow_update_handler(void *param)
{
    uint8_t released[ARROW_MAX_NUM];

    (void)param;

    for (;;) {
        (void)xSemaphoreTake(semphr_task_arrow_update, portMAX_DELAY);

        uint8_t released_count = 0U;

        game_state_lock();

        /*
         * The lifecycle check and mutation are one mutex transaction.  A
         * GAME_OVER transition cannot be followed by a stale arrow update.
         */
        if (!app_state_is_running_locked()) {
            game_state_unlock();
            continue;
        }

        for (uint8_t i = 0U; i < ARROW_MAX_NUM; i++) {
            if (!game_arrow[i].visible) {
                continue;
            }

            game_arrow[i].x += ARROW_STEP;

            if (game_check_bang_locked(i)) {
                released[released_count++] = i;
                continue;
            }

            if (game_arrow[i].x >= ARROW_X_THRESHOLD) {
                game_arrow[i].visible = false;
                released[released_count++] = i;
            }
        }

        game_state_unlock();

        /*
         * Queue operations are deliberately outside the state mutex.  This
         * avoids lock-order coupling between input, projectile and render paths.
         */
        for (uint8_t i = 0U; i < released_count; i++) {
            arrow_release(released[i]);
        }

        /* Bang and border workers keep the original object-update pipeline. */
        (void)xSemaphoreGive(semphr_task_bang_update);
        (void)xSemaphoreGive(semphr_task_update_border);
    }
}

void arrow_reset_for_new_game(void)
{
    BaseType_t status;

    /*
     * Queue ownership is authoritative.  Rebuilding it guarantees all five
     * arrow slots are returned exactly once, regardless of the previous game.
     */
    configASSERT(xQueueReset(arrow_free_queue) == pdPASS);

    game_state_lock();

    for (uint8_t i = 0U; i < ARROW_MAX_NUM; i++) {
        game_arrow[i].action_img = bitmap_arrow;
        game_arrow[i].visible = false;
        game_arrow[i].x = 0;
        game_arrow[i].y = 0;
    }

    game_state_unlock();

    for (uint8_t i = 0U; i < ARROW_MAX_NUM; i++) {
        status = xQueueSend(arrow_free_queue, &i, 0U);
        configASSERT(status == pdPASS);
    }
}

void arrow_task_create(void)
{
    BaseType_t status;

    arrow_free_queue = xQueueCreate(ARROW_MAX_NUM, sizeof(uint8_t));
    configASSERT(arrow_free_queue != NULL);

    /* Boot and soft restart deliberately share one initialization path. */
    arrow_reset_for_new_game();

    semphr_task_arrow_update = xSemaphoreCreateBinary();
    configASSERT(semphr_task_arrow_update != NULL);

    status = xTaskCreate(task_arrow_update_handler, "arrow update", 512U,
                         NULL, TASK_ARROW_UPDATE_PRIORITY,
                         &task_arrow_update_handle);
    configASSERT(status == pdPASS);
}
