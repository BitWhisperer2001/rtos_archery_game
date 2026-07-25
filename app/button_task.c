#include <stdbool.h>
#include <stdint.h>

#include "button_task.h"

#include "button.h"
#include "input_task.h"
#include "system_log.h"

#include "FreeRTOS.h"
#include "task.h"

TaskHandle_t task_button_handle = NULL;

typedef struct {
    uint32_t pin;
    bool candidate_pressed;
    bool stable_pressed;
    uint8_t candidate_samples;
    TickType_t pressed_at;
} button_debounce_t;

static button_debounce_t buttons[INPUT_BUTTON_COUNT] = {
    [INPUT_BUTTON_DOWN] = { .pin = BT_DOWN },
    [INPUT_BUTTON_UP] = { .pin = BT_UP },
    [INPUT_BUTTON_OK] = { .pin = BT_OK }
};

static void button_publish_release(input_button_t button_id,
                                   TickType_t pressed_at)
{
    TickType_t held_ticks = xTaskGetTickCount() - pressed_at;
    input_event_t event = {
        .button = button_id,
        .press = (held_ticks >= pdMS_TO_TICKS(BUTTON_LONG_PRESS_MS))
                     ? INPUT_EVENT_LONG_PRESS
                     : INPUT_EVENT_SHORT_PRESS
    };

    /*
     * Publishing on release cleanly distinguishes short and long presses.
     * The button sampler never needs to know which application state consumes
     * the event, keeping hardware input independent of game policy.
     */
    if (!input_event_publish(&event)) {
        DBG_LOG("Input queue full: button=%u press=%u",
                (unsigned int)event.button, (unsigned int)event.press);
    }
}

static void button_sample(input_button_t button_id)
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

        if (button->stable_pressed) {
            button->pressed_at = xTaskGetTickCount();
        } else {
            button_publish_release(button_id, button->pressed_at);
        }
    }
}

static void button_task_handler(void *param)
{
    TickType_t last_wake_time = xTaskGetTickCount();

    (void)param;

    for (;;) {
        for (input_button_t id = INPUT_BUTTON_DOWN;
             id < INPUT_BUTTON_COUNT;
             id = (input_button_t)(id + 1)) {
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
