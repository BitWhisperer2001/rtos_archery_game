#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>

#define TIM3_PRESCALER     (999U)
#define TIM3_PERIOD        (49999U)

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct {
    uint16_t freq;
    int16_t duration;
} Tone_TypeDef;

extern void buzzer_stop(void);
extern void buzzer_start(void);
extern uint32_t buzzer_play_tones(const Tone_TypeDef tones, uint16_t bpm);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // BUZZER_H

