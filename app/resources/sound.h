#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

#include "buzzer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A sequence carries an explicit length. This removes the old {0, 0}
 * sentinel, which was accidentally played and caused an integer divide by 0.
 */
typedef struct
{
    const Tone_TypeDef *tones;
    uint16_t length;
} tone_sequence_t;

extern const tone_sequence_t SOUND_ARROW_SHOT;
extern const tone_sequence_t SOUND_NO_ARROW;
extern const tone_sequence_t SOUND_METEOROID_DESTROY;
extern const tone_sequence_t SOUND_GAME_START;
extern const tone_sequence_t SOUND_GAME_OVER;

#ifdef __cplusplus
}
#endif

#endif /* SOUND_H */
