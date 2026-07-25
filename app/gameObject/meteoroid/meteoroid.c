#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "meteoroid.h"

#include "bitmap.h"
#include "game_state.h"
#include "screen_task.h"
#include "system_init.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

game_meteoroid_t game_meteoroid[METEROID_MAX_NUM];
TimerHandle_t meteoroid_timer = NULL;
TaskHandle_t task_meteoroid_update_handle = NULL;
uint32_t meteoroid_timer_period_ms = METEOROID_PERIOD_INITIAL_MS;

static uint32_t last_spawn_tick = 0U;
static bool cadence_started = false;

static void meteoroid_timer_callback(TimerHandle_t timer)
{
    (void)timer;

    /*
     * FreeRTOS executes every software timer callback in one daemon task.
     * Notify a worker and return immediately so OLED, random generation and
     * collision state can never delay unrelated timers.
     */
    xTaskNotifyGive(task_meteoroid_update_handle);
}

static void meteoroid_spawn_locked(uint32_t now)
{
    if (!cadence_started) {
        /*
         * Start the 300 ms spawn interval from the OK press, not from boot.
         * Waiting on the splash therefore cannot create an instant meteor.
         */
        cadence_started = true;
        last_spawn_tick = now;
        return;
    }

    if ((uint32_t)(now - last_spawn_tick) < 300U) {
        return;
    }

    uint8_t index = (uint8_t)(rand() % METEROID_MAX_NUM);

    if (!game_meteoroid[index].visible) {
        game_meteoroid[index].visible = true;
        game_meteoroid[index].x = METEOROID_X_START;
        game_meteoroid[index].state = 0U;
        game_meteoroid[index].action_img = bitmap_meteoroid_I;
    }

    last_spawn_tick = now;
}

static bool meteoroid_update_locked(void)
{
    for (uint8_t i = 0U; i < METEROID_MAX_NUM; i++) {
        if (!game_meteoroid[i].visible) {
            continue;
        }

        game_meteoroid[i].x -= METEOROID_RUN_STEP;

        switch (game_meteoroid[i].state) {
            case 0U:
                game_meteoroid[i].action_img = bitmap_meteoroid_I;
                game_meteoroid[i].state = 1U;
                break;

            case 1U:
                game_meteoroid[i].action_img = bitmap_meteoroid_II;
                game_meteoroid[i].state = 2U;
                break;

            default:
                game_meteoroid[i].action_img = bitmap_meteoroid_III;
                game_meteoroid[i].state = 0U;
                break;
        }

        if (game_meteoroid[i].x < METEOROID_X_THRESHOLD) {
            return true;
        }
    }

    return false;
}

static void meteoroid_task_handler(void *param)
{
    (void)param;

    for (;;) {
        bool game_over;

        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        game_state_lock();

        /*
         * Check mode while holding the object mutex.  Once game over publishes
         * its transition, no delayed timer notification can mutate new state.
         */
        if (!screen_gameplay_is_running_locked()) {
            game_state_unlock();
            continue;
        }

        meteoroid_spawn_locked(sys_get_tick());
        game_over = meteoroid_update_locked();
        game_state_unlock();

        if (game_over) {
            screen_request_game_over();
        } else {
            screen_request_frame();
        }
    }
}

void meteoroid_reset_for_new_game(void)
{
    game_state_lock();

    for (uint8_t i = 0U; i < METEROID_MAX_NUM; i++) {
        game_meteoroid[i].action_img = bitmap_meteoroid_I;
        game_meteoroid[i].x = METEOROID_X_START;
        game_meteoroid[i].y =
            (int16_t)(METEOROID_Y_START +
                      ((int16_t)i * METEOROID_Y_OFFSET));
        game_meteoroid[i].visible = false;
        game_meteoroid[i].state = 0U;
    }

    /*
     * These private cadence fields are session state too.  Resetting them makes
     * the first spawn delay identical on every play-through.
     */
    last_spawn_tick = 0U;
    cadence_started = false;
    meteoroid_timer_period_ms = METEOROID_PERIOD_INITIAL_MS;

    game_state_unlock();
}

void meteoroid_task_create(void)
{
    BaseType_t status;

    /* Boot and soft restart deliberately share one initialization path. */
    meteoroid_reset_for_new_game();

    status = xTaskCreate(meteoroid_task_handler, "meteoroid update", 512U,
                         NULL, 2U, &task_meteoroid_update_handle);
    configASSERT(status == pdPASS);

    meteoroid_timer =
        xTimerCreate("meteoroid cadence",
                     pdMS_TO_TICKS(meteoroid_timer_period_ms),
                     pdTRUE, NULL, meteoroid_timer_callback);
    configASSERT(meteoroid_timer != NULL);

    srand(sys_get_tick());

    /*
     * Timer remains dormant until screen_start_game() handles the user's OK
     * press.  This is the key behavioral change from the countdown design.
     */
}
