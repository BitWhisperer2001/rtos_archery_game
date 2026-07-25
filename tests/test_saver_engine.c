#include <assert.h>
#include <stdint.h>

#include "saver_engine.h"

static void test_each_effect_stays_renderable(void)
{
    saver_engine_t engine;

    for (saver_effect_t effect = SAVER_EFFECT_BUBBLES;
         effect < SAVER_EFFECT_COUNT;
         effect = (saver_effect_t)((uint8_t)effect + 1U)) {
        saver_engine_init(&engine, 1234U, effect);

        for (uint16_t frame = 0U; frame < 300U; frame++) {
            saver_engine_step(&engine);

            if (saver_engine_effect(&engine) == SAVER_EFFECT_BUBBLES) {
                for (uint8_t i = 0U;
                     i < saver_engine_circle_count(&engine);
                     i++) {
                    int16_t x;
                    int16_t y;
                    assert(saver_engine_get_circle(&engine, i, &x, &y));
                    assert(x >= SCREEN_SAVER_CIRCLE_RADIUS);
                    assert(x < (SCREEN_SAVER_DISPLAY_WIDTH -
                                SCREEN_SAVER_CIRCLE_RADIUS));
                    assert(y >= SCREEN_SAVER_CIRCLE_RADIUS);
                    assert(y < (SCREEN_SAVER_DISPLAY_HEIGHT -
                                SCREEN_SAVER_CIRCLE_RADIUS));
                }
            } else if (saver_engine_effect(&engine) ==
                       SAVER_EFFECT_STARFIELD) {
                for (uint8_t i = 0U;
                     i < saver_engine_star_count(&engine);
                     i++) {
                    int16_t x;
                    int16_t y;
                    assert(saver_engine_get_star(&engine, i, &x, &y));
                    assert(x >= 0);
                    assert(x < SCREEN_SAVER_DISPLAY_WIDTH);
                    assert(y >= 0);
                    assert(y < SCREEN_SAVER_DISPLAY_HEIGHT);
                }
            } else {
                for (uint8_t wave = 0U;
                     wave < saver_engine_wave_count(&engine);
                     wave++) {
                    for (uint8_t x = 0U;
                         x < SCREEN_SAVER_DISPLAY_WIDTH;
                         x += 4U) {
                        int16_t y =
                            saver_engine_wave_y(&engine, wave, x);
                        assert(y >= 0);
                        assert(y < SCREEN_SAVER_DISPLAY_HEIGHT);
                    }
                }
            }
        }
    }
}

static void test_manual_and_automatic_switching(void)
{
    saver_engine_t engine;

    saver_engine_init(&engine, 42U, SAVER_EFFECT_BUBBLES);
    saver_engine_next(&engine);
    assert(saver_engine_effect(&engine) == SAVER_EFFECT_STARFIELD);
    saver_engine_previous(&engine);
    assert(saver_engine_effect(&engine) == SAVER_EFFECT_BUBBLES);

    for (uint16_t frame = 0U; frame < SAVER_ENGINE_AUTO_FRAMES; frame++) {
        saver_engine_step(&engine);
    }
    assert(saver_engine_effect(&engine) == SAVER_EFFECT_STARFIELD);
}

int main(void)
{
    test_each_effect_stays_renderable();
    test_manual_and_automatic_switching();
    return 0;
}
