#include <stdint.h>

#include "border.h"

#include "app_state.h"
#include "arrow.h"
#include "bitmap.h"
#include "game_state.h"
#include "screen_task.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

game_border_t game_border;
TaskHandle_t task_border_update_handle = NULL;
SemaphoreHandle_t semphr_task_update_border = NULL;

static const unsigned char *border_arrow_bitmap(int16_t arrow_count)
{
    switch (arrow_count) {
        case 0:
            return bitmap_arrow_0;
        case 1:
            return bitmap_arrow_1;
        case 2:
            return bitmap_arrow_2;
        case 3:
            return bitmap_arrow_3;
        case 4:
            return bitmap_arrow_4;
        default:
            return bitmap_arrow_5;
    }
}

static void task_update_border_handler(void *param)
{
    (void)param;

    for (;;) {
        (void)xSemaphoreTake(semphr_task_update_border, portMAX_DELAY);

        game_state_lock();

        /*
         * Checking under the mutation mutex prevents an old semaphore token
         * from modifying the freshly reset score or ammunition display.
         */
        if (!app_state_is_running_locked()) {
            game_state_unlock();
            continue;
        }

        /*
         * Queue occupancy is the authoritative ammunition count.  Legacy
         * "last" fields remain updated for diagnostics, while screen task
         * redraws a complete frame and no longer needs erase bookkeeping.
         */

        game_border.arrow_num = (int16_t)uxQueueMessagesWaiting(arrow_free_queue);
        game_border.last_arrow_num = game_border.arrow_num;
        game_border.last_arrow_num_img = border_arrow_bitmap(game_border.arrow_num);
        game_border.last_game_score = game_border.game_score;

        game_state_unlock();

        /* Rendering is requested, never performed, by an object worker. */
        screen_request_frame();
    }
}

void border_reset_for_new_game(void)
{
    uint32_t preserved_high_score;

    game_state_lock();

    /*
     * The best score belongs to persistent application history; every other
     * field belongs to one session and is restored to its initial value.
     */
    preserved_high_score = game_border.highest_score;
    game_border.action_img = bitmap_border;
    game_border.last_arrow_num_img = bitmap_arrow_5;
    game_border.visible = true;
    game_border.x_border = BORDER_X;
    game_border.y_border = BORDER_Y;
    game_border.game_score = 0U;
    game_border.arrow_num = ARROW_MAX_NUM;
    game_border.last_arrow_num = ARROW_MAX_NUM;
    game_border.last_game_score = 0U;
    game_border.highest_score = preserved_high_score;

    game_state_unlock();
}

void border_task_create(void)
{
    BaseType_t status;

    /*
     * Static storage starts at zero, so the first reset also initializes the
     * preserved high-score field without a separate boot-only code path.
     */
    game_border.highest_score = 0U;
    border_reset_for_new_game();

    semphr_task_update_border = xSemaphoreCreateBinary();
    configASSERT(semphr_task_update_border != NULL);

    status = xTaskCreate(task_update_border_handler, "border update", 384U,
                         NULL, BORDER_TASK_PRIORITY,
                         &task_border_update_handle);
    configASSERT(status == pdPASS);
}
