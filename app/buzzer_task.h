#ifndef BUZZER_TASK_H
#define BUZZER_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define TASK_BUZZER_PRIORITY       (2U)

typedef enum {
    AUDIO_CUE_ARROW_SHOT = 0,
    AUDIO_CUE_NO_ARROW,
    AUDIO_CUE_METEOR_DESTROYED,
    AUDIO_CUE_GAME_OVER,
    AUDIO_CUE_GAME_START
} audio_cue_t;

void buzzer_task_create(void);
bool buzzer_request_cue(audio_cue_t cue);
void buzzer_flush_gameplay_cues(void);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_TASK_H */
