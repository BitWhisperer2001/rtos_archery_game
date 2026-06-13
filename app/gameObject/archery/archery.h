#ifndef ARCHERY_H
#define ARCHERY_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#define ARCHERY_TASK_PRIORYTY   (2)

#define ARCHERY_INIT_BIT        (1 << 0)
#define ARCHERY_UP_BIT          (1 << 1)
#define ARCHERY_DOWN_BIT        (1 << 22)

#define ARCHERY_DOWN_THRESHOLD  (38)
#define ARCHERY_UP_THRESHOLD    (2)
#define ARCHERY_WIDTH           (13)
#define ARCHERY_HEIGH           (15)
#define ARCHERY_X               (int16_t)(-1)
#define ARCHERY_Y_START         (20)
#define ARCHERY_Y_STEP          (9)
#define ALL_EVENT               (ARCHERY_INIT_BIT | ARCHERY_DOWN_BIT | ARCHERY_UP_BIT)

typedef struct {
    bool visible;
    int16_t x;
    int16_t y;
    const unsigned char *action_img;
}game_archery_t;

extern EventGroupHandle_t archery_event_state;
extern volatile game_archery_t game_archery;
extern TaskHandle_t task_archery_update_handle;

extern void archery_task_create(void);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // ARCHERY_H