#ifndef SAVER_ENGINE_H
#define SAVER_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "screen_saver.h"

#define SAVER_ENGINE_STAR_MAX       (20U)
#define SAVER_ENGINE_STAR_MIN       (4U)
#define SAVER_ENGINE_AUTO_FRAMES    (375U) /* 15 seconds at 25 FPS. */

typedef enum {
    SAVER_EFFECT_BUBBLES = 0,
    SAVER_EFFECT_STARFIELD,
    SAVER_EFFECT_WAVES,
    SAVER_EFFECT_COUNT
} saver_effect_t;

typedef struct {
    int32_t x_q8;
    int32_t y_q8;
    int16_t velocity_x_q8;
    int16_t velocity_y_q8;
} saver_star_t;

typedef struct {
    saver_effect_t effect;
    screen_saver_t bubbles;
    saver_star_t stars[SAVER_ENGINE_STAR_MAX];
    uint32_t random_state;
    uint16_t frames_in_effect;
    uint8_t star_count;
    uint8_t wave_count;
    uint8_t wave_phase;
} saver_engine_t;

void saver_engine_init(saver_engine_t *engine, uint32_t seed,
                       saver_effect_t effect);
void saver_engine_step(saver_engine_t *engine);
void saver_engine_next(saver_engine_t *engine);
void saver_engine_previous(saver_engine_t *engine);
bool saver_engine_add(saver_engine_t *engine);
bool saver_engine_remove(saver_engine_t *engine);
saver_effect_t saver_engine_effect(const saver_engine_t *engine);
const char *saver_engine_effect_name(saver_effect_t effect);

uint8_t saver_engine_circle_count(const saver_engine_t *engine);
bool saver_engine_get_circle(const saver_engine_t *engine, uint8_t index,
                             int16_t *x, int16_t *y);
uint8_t saver_engine_star_count(const saver_engine_t *engine);
bool saver_engine_get_star(const saver_engine_t *engine, uint8_t index,
                           int16_t *x, int16_t *y);
uint8_t saver_engine_wave_count(const saver_engine_t *engine);
int16_t saver_engine_wave_y(const saver_engine_t *engine, uint8_t wave,
                            uint8_t x);

#ifdef __cplusplus
}
#endif

#endif /* SAVER_ENGINE_H */
