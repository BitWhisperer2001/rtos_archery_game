#ifndef BUZZER_TASK_H
#define BUZZER_TASK_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "event_groups.h"

#define TASK_BUZZER_PRIORITY 2

#define ARROW_SHOT_BIT              (1U << 0)
#define ARCHERY_NO_ARROW_BIT        (1U << 1)
#define METEOROID_DESTROY           (1U << 2)
#define MUSIC_BACKGROUND            (1U << 3)
#define GAME_OVER_SOUND_BIT         (1U << 4)
#define GAME_START_SOUND_BIT        (1U << 5)

extern EventGroupHandle_t buzzer_event_state;
extern void buzzer_task_create(void);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // BUZZER_TASK_H
