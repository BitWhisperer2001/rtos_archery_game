#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

#define BUTTON_TASK_PRIORITY       (3U)
#define BUTTON_TASK_STACK_WORDS    (384U)
#define BUTTON_POLL_PERIOD_MS      (20U)
#define BUTTON_DEBOUNCE_SAMPLES    (3U)
#define BUTTON_LONG_PRESS_MS       (800U)

extern TaskHandle_t task_button_handle;

void button_task_create(void);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_TASK_H */
