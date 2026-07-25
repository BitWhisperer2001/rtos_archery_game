#include <stdbool.h>
#include <stdint.h>

#include "bang.h"

#include "bitmap.h"
#include "border.h"
#include "buzzer_task.h"
#include "game_state.h"
#include "meteoroid.h"
#include "screen_task.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

TaskHandle_t task_update_bang_handle = NULL;
SemaphoreHandle_t semphr_task_bang_update = NULL;
game_bang_t game_bang[BANG_MAX_NUM];

static uint8_t count_score = 0U;

static void task_bang_update_handler(void *param)
{
    (void)param;

    for (;;) {
        bool animation_changed = false;
        bool meteor_destroyed = false;
        bool period_changed = false;
        uint32_t new_period_ms = 0U;

        (void)xSemaphoreTake(semphr_task_bang_update, portMAX_DELAY);

        game_state_lock();

        /*
         * A pending animation token from the old session is discarded under
         * the same mutex that protects score and explosion state.
         */
        if (!screen_gameplay_is_running_locked()) {
            game_state_unlock();
            continue;
        }

        for (uint8_t i = 0U; i < BANG_MAX_NUM; i++) {
            if (!game_bang[i].visible) {
                continue;
            }

            animation_changed = true;

            switch (game_bang[i].state) {
                case 0U:
                    game_bang[i].action_img = bitmap_bang_I;
                    game_bang[i].state = 1U;
                    break;

                case 1U:
                    game_bang[i].action_img = bitmap_bang_II;
                    game_bang[i].state = 2U;
                    game_border.game_score += GAME_SCORE_STEP;
                    meteor_destroyed = true;

                    count_score++;
                    if (count_score >= 10U) {
                        count_score = 0U;

                        /*
                         * Saturating subtraction guarantees the game cadence
                         * can never underflow into a multi-day timer period.
                         */
                        if (meteoroid_timer_period_ms >
                            METEOROID_PERIOD_MIN_MS) {
                            if (meteoroid_timer_period_ms >
                                (METEOROID_PERIOD_MIN_MS +
                                 METEOROID_PERIOD_STEP_MS)) {
                                meteoroid_timer_period_ms -=
                                    METEOROID_PERIOD_STEP_MS;
                            } else {
                                meteoroid_timer_period_ms =
                                    METEOROID_PERIOD_MIN_MS;
                            }

                            period_changed = true;
                            new_period_ms = meteoroid_timer_period_ms;
                        }
                    }
                    break;

                case 2U:
                    game_bang[i].action_img = bitmap_bang_III;
                    game_bang[i].state = 3U;
                    break;

                default:
                    game_bang[i].visible = false;
                    game_bang[i].action_img = bitmap_bang_I;
                    game_bang[i].state = 0U;
                    break;
            }
        }

        game_state_unlock();

        if (period_changed) {
            BaseType_t status =
                xTimerChangePeriod((TimerHandle_t)meteoroid_timer,
                                   pdMS_TO_TICKS(new_period_ms),
                                   pdMS_TO_TICKS(10U));
            configASSERT(status == pdPASS);
        }

        if (meteor_destroyed) {
            xEventGroupSetBits(buzzer_event_state, METEOROID_DESTROY);
        }

        if (animation_changed) {
            /*
             * Border task coalesces score/count changes and requests one frame
             * after this animation step.
             */
            (void)xSemaphoreGive(semphr_task_update_border);
        }
    }
}

void bang_reset_for_new_game(void)
{
    game_state_lock();

    for (uint8_t i = 0U; i < BANG_MAX_NUM; i++) {
        game_bang[i].visible = false;
        game_bang[i].action_img = bitmap_bang_I;
        game_bang[i].state = 0U;
        game_bang[i].x = 0;
        game_bang[i].y = 0;
    }

    /* Difficulty progression must always start at the same score boundary. */
    count_score = 0U;

    game_state_unlock();
}

void bang_task_create(void)
{
    BaseType_t status;

    /* Boot and soft restart deliberately share one initialization path. */
    bang_reset_for_new_game();

    semphr_task_bang_update = xSemaphoreCreateBinary();
    configASSERT(semphr_task_bang_update != NULL);

    status = xTaskCreate(task_bang_update_handler, "bang update", 512U,
                         NULL, TASK_BANG_PRIORITY,
                         &task_update_bang_handle);
    configASSERT(status == pdPASS);
}
