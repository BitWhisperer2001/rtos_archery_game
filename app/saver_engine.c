#include "saver_engine.h"

#include <stddef.h>
#include <string.h>

#define SAVER_Q8_ONE          (256L)
#define SAVER_CENTER_X_Q8     (64L * SAVER_Q8_ONE)
#define SAVER_CENTER_Y_Q8     (32L * SAVER_Q8_ONE)
#define SAVER_DEFAULT_SEED    (0x9E3779B9UL)

static const int8_t sine_lut[32] = {
     0,  3,  6,  9, 11, 13, 15, 16,
    16, 16, 15, 13, 11,  9,  6,  3,
     0, -3, -6, -9,-11,-13,-15,-16,
   -16,-16,-15,-13,-11, -9, -6, -3
};

static const int8_t direction_x[16] = {
    16, 15, 11,  6,  0, -6,-11,-15,
   -16,-15,-11, -6,  0,  6, 11, 15
};

static const int8_t direction_y[16] = {
     0,  6, 11, 15, 16, 15, 11,  6,
     0, -6,-11,-15,-16,-15,-11, -6
};

static uint32_t saver_random(saver_engine_t *engine)
{
    uint32_t value = engine->random_state;

    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    engine->random_state = value;
    return value;
}

static void saver_reset_star(saver_engine_t *engine, saver_star_t *star)
{
    uint8_t direction = (uint8_t)(saver_random(engine) & 0x0FU);
    int16_t speed = (int16_t)(12U + (saver_random(engine) % 13U));

    star->x_q8 = SAVER_CENTER_X_Q8;
    star->y_q8 = SAVER_CENTER_Y_Q8;
    star->velocity_x_q8 = (int16_t)(direction_x[direction] * speed);
    star->velocity_y_q8 = (int16_t)(direction_y[direction] * speed);
}

static void saver_prepare_effect(saver_engine_t *engine)
{
    engine->frames_in_effect = 0U;

    if (engine->effect == SAVER_EFFECT_BUBBLES) {
        screen_saver_init(&engine->bubbles, saver_random(engine));
    } else if (engine->effect == SAVER_EFFECT_STARFIELD) {
        engine->star_count = 12U;
        for (uint8_t i = 0U; i < SAVER_ENGINE_STAR_MAX; i++) {
            saver_reset_star(engine, &engine->stars[i]);

            /*
             * Advance each initial star by a different amount so the first
             * frame is already a field rather than one bright center pixel.
             */
            uint8_t warmup = (uint8_t)(saver_random(engine) % 30U);
            for (uint8_t step = 0U; step < warmup; step++) {
                engine->stars[i].x_q8 += engine->stars[i].velocity_x_q8;
                engine->stars[i].y_q8 += engine->stars[i].velocity_y_q8;
            }
        }
    } else {
        engine->wave_count = 2U;
        engine->wave_phase = 0U;
    }
}

void saver_engine_init(saver_engine_t *engine, uint32_t seed,
                       saver_effect_t effect)
{
    if (engine == NULL) {
        return;
    }

    memset(engine, 0, sizeof(*engine));
    engine->random_state = (seed != 0U) ? seed : SAVER_DEFAULT_SEED;
    engine->effect =
        (effect < SAVER_EFFECT_COUNT) ? effect : SAVER_EFFECT_BUBBLES;
    saver_prepare_effect(engine);
}

void saver_engine_step(saver_engine_t *engine)
{
    if (engine == NULL) {
        return;
    }

    if (engine->effect == SAVER_EFFECT_BUBBLES) {
        screen_saver_step(&engine->bubbles);
    } else if (engine->effect == SAVER_EFFECT_STARFIELD) {
        for (uint8_t i = 0U; i < engine->star_count; i++) {
            saver_star_t *star = &engine->stars[i];

            star->x_q8 += star->velocity_x_q8;
            star->y_q8 += star->velocity_y_q8;

            /* A tiny acceleration produces a smooth perspective starfield. */
            star->velocity_x_q8 +=
                (star->velocity_x_q8 > 0) ? 1 :
                ((star->velocity_x_q8 < 0) ? -1 : 0);
            star->velocity_y_q8 +=
                (star->velocity_y_q8 > 0) ? 1 :
                ((star->velocity_y_q8 < 0) ? -1 : 0);

            int16_t x = (int16_t)(star->x_q8 / SAVER_Q8_ONE);
            int16_t y = (int16_t)(star->y_q8 / SAVER_Q8_ONE);
            if ((x < 0) || (x >= SCREEN_SAVER_DISPLAY_WIDTH) ||
                (y < 0) || (y >= SCREEN_SAVER_DISPLAY_HEIGHT)) {
                saver_reset_star(engine, star);
            }
        }
    } else {
        engine->wave_phase = (uint8_t)((engine->wave_phase + 1U) & 31U);
    }

    engine->frames_in_effect++;
    if (engine->frames_in_effect >= SAVER_ENGINE_AUTO_FRAMES) {
        saver_engine_next(engine);
    }
}

void saver_engine_next(saver_engine_t *engine)
{
    if (engine == NULL) {
        return;
    }

    engine->effect =
        (saver_effect_t)(((uint8_t)engine->effect + 1U) %
                         (uint8_t)SAVER_EFFECT_COUNT);
    saver_prepare_effect(engine);
}

void saver_engine_previous(saver_engine_t *engine)
{
    if (engine == NULL) {
        return;
    }

    engine->effect =
        (engine->effect == SAVER_EFFECT_BUBBLES)
            ? (saver_effect_t)(SAVER_EFFECT_COUNT - 1)
            : (saver_effect_t)((uint8_t)engine->effect - 1U);
    saver_prepare_effect(engine);
}

bool saver_engine_add(saver_engine_t *engine)
{
    if (engine == NULL) {
        return false;
    }

    if (engine->effect == SAVER_EFFECT_BUBBLES) {
        return screen_saver_add_circle(&engine->bubbles);
    }
    if (engine->effect == SAVER_EFFECT_STARFIELD) {
        if (engine->star_count >= SAVER_ENGINE_STAR_MAX) {
            return false;
        }
        saver_reset_star(engine, &engine->stars[engine->star_count]);
        engine->star_count++;
        return true;
    }
    if (engine->wave_count < 3U) {
        engine->wave_count++;
        return true;
    }
    return false;
}

bool saver_engine_remove(saver_engine_t *engine)
{
    if (engine == NULL) {
        return false;
    }

    if (engine->effect == SAVER_EFFECT_BUBBLES) {
        return screen_saver_remove_circle(&engine->bubbles);
    }
    if (engine->effect == SAVER_EFFECT_STARFIELD) {
        if (engine->star_count <= SAVER_ENGINE_STAR_MIN) {
            return false;
        }
        engine->star_count--;
        return true;
    }
    if (engine->wave_count > 1U) {
        engine->wave_count--;
        return true;
    }
    return false;
}

saver_effect_t saver_engine_effect(const saver_engine_t *engine)
{
    return (engine != NULL) ? engine->effect : SAVER_EFFECT_BUBBLES;
}

const char *saver_engine_effect_name(saver_effect_t effect)
{
    static const char *const names[SAVER_EFFECT_COUNT] = {
        "BUBBLES", "FLUSH", "WAVES"
    };

    return (effect < SAVER_EFFECT_COUNT) ? names[effect] : "UNKNOWN";
}

uint8_t saver_engine_circle_count(const saver_engine_t *engine)
{
    return (engine != NULL) ? screen_saver_count(&engine->bubbles) : 0U;
}

bool saver_engine_get_circle(const saver_engine_t *engine, uint8_t index,
                             int16_t *x, int16_t *y)
{
    return (engine != NULL) &&
           screen_saver_get_circle(&engine->bubbles, index, x, y);
}

uint8_t saver_engine_star_count(const saver_engine_t *engine)
{
    return (engine != NULL) ? engine->star_count : 0U;
}

bool saver_engine_get_star(const saver_engine_t *engine, uint8_t index,
                           int16_t *x, int16_t *y)
{
    if ((engine == NULL) || (x == NULL) || (y == NULL) ||
        (index >= engine->star_count)) {
        return false;
    }

    *x = (int16_t)(engine->stars[index].x_q8 / SAVER_Q8_ONE);
    *y = (int16_t)(engine->stars[index].y_q8 / SAVER_Q8_ONE);
    return true;
}

uint8_t saver_engine_wave_count(const saver_engine_t *engine)
{
    return (engine != NULL) ? engine->wave_count : 0U;
}

int16_t saver_engine_wave_y(const saver_engine_t *engine, uint8_t wave,
                            uint8_t x)
{
    if ((engine == NULL) || (wave >= engine->wave_count)) {
        return SCREEN_SAVER_DISPLAY_HEIGHT / 2;
    }

    uint8_t phase = (uint8_t)(((x / 4U) + engine->wave_phase +
                               (wave * 11U)) & 31U);
    int16_t center = (int16_t)(20 + (wave * 12U));
    return (int16_t)(center + sine_lut[phase]);
}
