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

#define ARROW_SHOT_BIT          (1 << 0)
#define ARCHERY_NO_ARROW_BIT    (1 << 1)
#define METEOROID_DESTROY       (1 << 2)
#define MUSIC_BACKGROUND        (1 << 3)

extern EventGroupHandle_t buzzer_event_state;
extern void buzzer_task_create(void);
extern void buzzer_play_tone_game_over(void);
extern void buzzer_play_tone_game_begin(void);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // BUZZER_TASK_H