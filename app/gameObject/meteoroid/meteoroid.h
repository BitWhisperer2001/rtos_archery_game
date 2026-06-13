
#ifndef METEOROID_H
#define METEOROID_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "timers.h"

#define METEOROID_Y_START       (5)
#define METEOROID_X_START       (129)
#define METEOROID_Y_OFFSET      (9)
#define METEOROID_X_THRESHOLD   (12)
#define METEOROID_WIDTH         (20)
#define METEOROID_HEIGH         (10)
#define METEROID_MAX_NUM        (5)
#define METEOROID_RUN_STEP      (2)

typedef struct {
    bool visible;
    int16_t x;
    int16_t y;
    const unsigned char *action_img;
    uint8_t state;
}game_meteoroid_t;

extern bool isGameOver;
extern volatile TimerHandle_t meteoroid_timer;
extern volatile game_meteoroid_t game_meteoroid[METEROID_MAX_NUM];
extern uint8_t meteoroid_timer_period;

extern void meteoroid_task_create(void);


#ifdef __cplusplus
}
#endif
#endif  // METEOROID_H

