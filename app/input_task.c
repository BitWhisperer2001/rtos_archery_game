#include "input_task.h"

#include <stdint.h>

#include "app_state.h"
#include "archery.h"
#include "arrow.h"
#include "border.h"
#include "buzzer_task.h"
#include "game_state.h"
#include "game_stats.h"
#include "screen_task.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "task.h"

#define INPUT_QUEUE_LENGTH       (12U)
#define INPUT_ROUTER_PRIORITY    (3U)
#define INPUT_ROUTER_STACK_WORDS (384U)

static QueueHandle_t input_queue = NULL;
static TaskHandle_t input_router_handle = NULL;

static void input_shoot_arrow(void)
{
    uint8_t arrow_index;

    if (xQueueReceive(arrow_free_queue, &arrow_index, 0U) != pdPASS) {
        (void)buzzer_request_cue(AUDIO_CUE_NO_ARROW);
        return;
    }

    game_state_lock();

    /*
     * Game over can occur after queue ownership changes but before this lock.
     * Return the slot instead of leaking ammunition into a frozen session.
     */
    if (!app_state_is_running_locked()) {
        game_state_unlock();
        configASSERT(xQueueSend(arrow_free_queue, &arrow_index, 0U) == pdPASS);
        return;
    }

    game_arrow[arrow_index].x = game_archery.x;
    game_arrow[arrow_index].y = game_archery.y + 5;
    game_arrow[arrow_index].visible = true;
    game_border.arrow_num = (int16_t)uxQueueMessagesWaiting(arrow_free_queue);

    game_state_unlock();

    game_stats_record_shot();
    (void)buzzer_request_cue(AUDIO_CUE_ARROW_SHOT);
    screen_request_frame();
}

static void input_route_saver(const input_event_t *event)
{
    if (event->press == INPUT_EVENT_LONG_PRESS) {
        if (event->button == INPUT_BUTTON_UP) {
            screen_request_saver_next_effect();
        } else if (event->button == INPUT_BUTTON_DOWN) {
            screen_request_saver_previous_effect();
        } else {
            screen_request_saver_exit();
        }
        return;
    }

    if (event->button == INPUT_BUTTON_UP) {
        screen_request_saver_add_circle();
    } else if (event->button == INPUT_BUTTON_DOWN) {
        screen_request_saver_remove_circle();
    } else {
        screen_request_saver_exit();
    }
}

static void input_route_settings(const input_event_t *event)
{
    if ((event->button == INPUT_BUTTON_OK) ||
        ((event->button == INPUT_BUTTON_DOWN) &&
         (event->press == INPUT_EVENT_LONG_PRESS))) {
        screen_request_settings_exit();
    } else if ((event->button == INPUT_BUTTON_UP) &&
               (event->press == INPUT_EVENT_LONG_PRESS)) {
        screen_request_settings_sound();
    } else if (event->button == INPUT_BUTTON_UP) {
        screen_request_settings_difficulty();
    } else {
        screen_request_settings_effect();
    }
}

static void input_route_running(const input_event_t *event)
{
    if ((event->button == INPUT_BUTTON_OK) &&
        (event->press == INPUT_EVENT_LONG_PRESS)) {
        screen_request_pause();
        return;
    }

    if (event->button == INPUT_BUTTON_DOWN) {
        xEventGroupSetBits(archery_event_state, ARCHERY_DOWN_BIT);
    } else if (event->button == INPUT_BUTTON_UP) {
        xEventGroupSetBits(archery_event_state, ARCHERY_UP_BIT);
    } else {
        input_shoot_arrow();
    }
}

static void input_route(const input_event_t *event)
{
    app_state_t state = app_state_get();

    if (state == APP_STATE_DISPLAY_SLEEP) {
        /* First input only wakes the product; it cannot trigger hidden action. */
        screen_request_wake();
    } else if (state == APP_STATE_SCREENSAVER) {
        input_route_saver(event);
    } else if (state == APP_STATE_SETTINGS) {
        input_route_settings(event);
    } else if (state == APP_STATE_WAITING_FOR_START) {
        if (event->button == INPUT_BUTTON_OK) {
            if (event->press == INPUT_EVENT_LONG_PRESS) {
                screen_request_settings_enter();
            } else {
                screen_request_game_start();
            }
        }
    } else if (state == APP_STATE_RUNNING) {
        input_route_running(event);
    } else if (state == APP_STATE_PAUSED) {
        if (event->button == INPUT_BUTTON_OK) {
            screen_request_resume();
        }
    } else if (state == APP_STATE_GAME_OVER) {
        if (event->button == INPUT_BUTTON_OK) {
            screen_request_return_to_start();
        }
    }
}

static void input_router_task(void *param)
{
    input_event_t event;

    (void)param;

    for (;;) {
        if (xQueueReceive(input_queue, &event, portMAX_DELAY) == pdPASS) {
            input_route(&event);
        }
    }
}

bool input_event_publish(const input_event_t *event)
{
    return (event != NULL) && (input_queue != NULL) &&
           (xQueueSendToBack(input_queue, event, 0U) == pdPASS);
}

void input_task_create(void)
{
    BaseType_t status;

    input_queue = xQueueCreate(INPUT_QUEUE_LENGTH, sizeof(input_event_t));
    configASSERT(input_queue != NULL);

    status = xTaskCreate(input_router_task, "input router",
                         INPUT_ROUTER_STACK_WORDS, NULL,
                         INPUT_ROUTER_PRIORITY, &input_router_handle);
    configASSERT(status == pdPASS);
}
