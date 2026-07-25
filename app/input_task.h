#ifndef INPUT_TASK_H
#define INPUT_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
    INPUT_BUTTON_DOWN = 0,
    INPUT_BUTTON_UP,
    INPUT_BUTTON_OK,
    INPUT_BUTTON_COUNT
} input_button_t;

typedef enum {
    INPUT_EVENT_SHORT_PRESS = 0,
    INPUT_EVENT_LONG_PRESS
} input_press_t;

typedef struct {
    input_button_t button;
    input_press_t press;
} input_event_t;

void input_task_create(void);
bool input_event_publish(const input_event_t *event);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_TASK_H */
