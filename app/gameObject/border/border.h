#ifndef BORDER_H
#define BORDER_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define BORDER_TASK_PRIORITY    (2)

#define BORDER_X                (0)
#define BORDER_Y                (0)
#define BORDER_WIDTH            (128)
#define BORDER_HEIGH            (64)
#define ARROW_X                 (86)
#define ARROW_Y                 (55)
#define ARROW_NUM_X             (86)
#define ARROW_NUM_Y             (55)
#define ARROW_NUM_WIDTH         (39)
#define ARROW_NUM_HEIGH         (7)

#define GAME_SCORE_STEP         (10)
#define ADD_TO_SAVE_SCORE       (0x08060000)

typedef struct {
    bool visible;
    int16_t x_border;
    int16_t y_border;
    const unsigned char *action_img;
    const unsigned char *last_arrow_num_img;
    int16_t arrow_num;
    int16_t last_arrow_num;
    uint16_t game_score;
    uint16_t last_game_score;
    uint16_t highest_score;
}game_border_t;

extern game_border_t game_border;
extern SemaphoreHandle_t semphr_task_update_border;
extern TaskHandle_t task_border_update_handle;

extern void border_task_create(void);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // BORDER_H