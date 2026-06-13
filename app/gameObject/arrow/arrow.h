#ifndef ARROW_H
#define ARROW_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "ring_buffer.h"

#define TASK_ARROW_UPDATE_PRIORITY  (2)

#define ARROW_MAX_NUM               (5)
#define ARROW_WIDTH                 (10)
#define ARROW_HEIGHT                (5)
#define ARROW_STEP                  (4)
#define ARROW_X_THRESHOLD           (127)

typedef struct {
    bool visible;
    int16_t x;
    int16_t y;
    const unsigned char *action_img;
}game_arrow_t;

extern game_arrow_t game_arrow[ARROW_MAX_NUM + 1];
extern ring_buff_t arrow_buff;
extern SemaphoreHandle_t semphr_task_arrow_update;
extern volatile uint8_t arrow_num_index;
extern TaskHandle_t task_arrow_update_handle;

extern void arrow_task_create(void);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // ARROW_H