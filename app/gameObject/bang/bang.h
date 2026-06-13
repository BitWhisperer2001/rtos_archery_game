#ifndef BANG_H
#define BANG_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define TASK_BANG_PRIORITY  (2)

#define BANG_MAX_NUM        (5)
#define BANG_I_WIDTH        (15)
#define BANG_I_HEIGH        (15)
#define BANG_II_WIDTH       (15)
#define BANG_II_HEIGH       (15)
#define BANG_III_WIDTH      (10)
#define BANG_III_HEIGH      (10)

typedef struct {
    bool visible;
    int16_t x;
    int16_t y;
    const unsigned char *action_img;
    uint8_t state;
}game_bang_t;

extern TaskHandle_t task_update_bang_handle;

extern SemaphoreHandle_t semphr_task_bang_update;
extern game_bang_t game_bang[BANG_MAX_NUM];

extern void bang_task_create(void);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // BANG_H