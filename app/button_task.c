#include <stdbool.h>
#include <stdint.h>

#include "button_task.h"

#include "archery.h"
#include "arrow.h"
#include "border.h"
#include "button.h"
#include "buzzer_task.h"
#include "game_state.h"
#include "screen_task.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "task.h"

TaskHandle_t task_button_handle = NULL;

typedef enum {
    BUTTON_ID_DOWN = 0,
    BUTTON_ID_UP,
    BUTTON_ID_OK,
    BUTTON_ID_COUNT
} button_id_t;

typedef struct {
    uint32_t pin;
    bool candidate_pressed;
    bool stable_pressed;
    uint8_t candidate_samples;
} button_debounce_t;

static button_debounce_t buttons[BUTTON_ID_COUNT] = {
    [BUTTON_ID_DOWN] = { .pin = BT_DOWN },
    [BUTTON_ID_UP] = { .pin = BT_UP },
    [BUTTON_ID_OK] = { .pin = BT_OK }
};

static void button_shoot_arrow(void)
{
    uint8_t arrow_index;

    if (xQueueReceive(arrow_free_queue, &arrow_index, 0U) != pdPASS) {
        xEventGroupSetBits(buzzer_event_state, ARCHERY_NO_ARROW_BIT);
        return;
    }
    game_state_lock();

    /*
     * Game over may occur after the queue receive but before this lock.  Return
     * the slot instead of publishing an arrow into a non-running session.
     */
    if (!screen_gameplay_is_running_locked()) {
        game_state_unlock();
        configASSERT(xQueueSend(arrow_free_queue, &arrow_index, 0U) == pdPASS);
        return;
    }

    /*
     * Queue ownership selects a unique arrow slot; the state mutex then makes
     * publishing all of that arrow's fields one coherent operation.
     */
    game_arrow[arrow_index].x = game_archery.x;
    game_arrow[arrow_index].y = game_archery.y + 5;
    game_arrow[arrow_index].visible = true;
    game_border.arrow_num = (int16_t)uxQueueMessagesWaiting(arrow_free_queue);

    game_state_unlock();

    xEventGroupSetBits(buzzer_event_state, ARROW_SHOT_BIT);
    screen_request_frame();
}

static void button_handle_press(button_id_t button_id)
{
    screen_mode_t mode = screen_get_mode();

    if (mode == SCREEN_MODE_GAME_OVER) {
        /*
         * A completed game-over screen accepts only OK.  The screen owner then
         * performs a deterministic software session reset; the MCU and RTOS
         * kernel remain alive.
         */
        if (button_id == BUTTON_ID_OK) {
            screen_request_return_to_start();
        }
        return;
    }

    if (mode == SCREEN_MODE_SCREENSAVER) {
        /*
         * Screensaver controls are intentionally handled by its screen owner:
         * UP pushes, DOWN pops (LIFO), and OK returns to the start prompt.
         */
        switch (button_id) {
            case BUTTON_ID_UP:
                screen_request_saver_add_circle();
                break;

            case BUTTON_ID_DOWN:
                screen_request_saver_remove_circle();
                break;

            case BUTTON_ID_OK:
                screen_request_saver_exit();
                break;

            default:
                configASSERT(false);
                break;
        }
        return;
    }

    if ((mode == SCREEN_MODE_WAITING_FOR_START) ||
        (mode == SCREEN_MODE_START_PENDING)) {
        if (button_id == BUTTON_ID_OK) {
            /* Only OK is allowed to leave the startup screen. */
            screen_request_game_start();
        }
        return;
    }

    if (mode != SCREEN_MODE_RUNNING) {
        return;
    }

    switch (button_id) {
        case BUTTON_ID_DOWN:
            xEventGroupSetBits(archery_event_state, ARCHERY_DOWN_BIT);
            break;

        case BUTTON_ID_UP:
            xEventGroupSetBits(archery_event_state, ARCHERY_UP_BIT);
            break;

        case BUTTON_ID_OK:
            button_shoot_arrow();
            break;

        default:
            configASSERT(false);
            break;
    }
}

static void button_sample(button_id_t button_id)
{
    button_debounce_t *button = &buttons[button_id];
    bool pressed = !button_read(button->pin);

    if (pressed != button->candidate_pressed) {
        button->candidate_pressed = pressed;
        button->candidate_samples = 1U;
        return;
    }

    if (button->candidate_samples < BUTTON_DEBOUNCE_SAMPLES) {
        button->candidate_samples++;
    }

    if ((button->candidate_samples >= BUTTON_DEBOUNCE_SAMPLES) &&
        (button->stable_pressed != button->candidate_pressed)) {
        button->stable_pressed = button->candidate_pressed;

        /*
         * Generate exactly one action on the stable pressed edge.  Holding a
         * button cannot repeatedly move, fire, start, or reset the game.
         */
        if (button->stable_pressed) {
            button_handle_press(button_id);
        }
    }
}

static void button_task_handler(void *param)
{
    TickType_t last_wake_time = xTaskGetTickCount();

    (void)param;

    for (;;) {
        for (button_id_t id = BUTTON_ID_DOWN; id < BUTTON_ID_COUNT; id = (button_id_t)(id + 1)) {
            button_sample(id);
        }

        /*
         * vTaskDelayUntil keeps polling phase-stable even when handling an
         * event takes slightly longer than normal.
         */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(BUTTON_POLL_PERIOD_MS));
    }
}

void button_task_create(void)
{
    BaseType_t status;

    button_setup(BT_DOWN | BT_UP | BT_OK);

    status = xTaskCreate(button_task_handler, "button input",
                         BUTTON_TASK_STACK_WORDS, NULL,
                         BUTTON_TASK_PRIORITY, &task_button_handle);
    configASSERT(status == pdPASS);
}
