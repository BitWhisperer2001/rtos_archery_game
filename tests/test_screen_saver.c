#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "screen_saver.h"

static void assert_all_circles_are_on_screen(const screen_saver_t *saver)
{
    for (uint8_t i = 0U; i < screen_saver_count(saver); i++) {
        int16_t x;
        int16_t y;

        assert(screen_saver_get_circle(saver, i, &x, &y));
        assert(x >= SCREEN_SAVER_CIRCLE_RADIUS);
        assert(x <= (SCREEN_SAVER_DISPLAY_WIDTH - 1 -
                     SCREEN_SAVER_CIRCLE_RADIUS));
        assert(y >= SCREEN_SAVER_CIRCLE_RADIUS);
        assert(y <= (SCREEN_SAVER_DISPLAY_HEIGHT - 1 -
                     SCREEN_SAVER_CIRCLE_RADIUS));
    }
}

int main(void)
{
    screen_saver_t saver;
    int16_t x;
    int16_t y;

    screen_saver_init(&saver, 1U);
    assert(screen_saver_count(&saver) == SCREEN_SAVER_MIN_CIRCLES);
    assert(screen_saver_get_circle(&saver, 0U, &x, &y));
    assert(x == (SCREEN_SAVER_DISPLAY_WIDTH / 2));
    assert(y == (SCREEN_SAVER_DISPLAY_HEIGHT / 2));

    /* Maximum count is enforced even under repeated UP commands. */
    for (uint8_t i = 1U; i < SCREEN_SAVER_MAX_CIRCLES; i++) {
        assert(screen_saver_add_circle(&saver));
    }
    assert(!screen_saver_add_circle(&saver));
    assert(screen_saver_count(&saver) == SCREEN_SAVER_MAX_CIRCLES);

    /*
     * Thousands of frames exercise repeated reflection on all four edges while
     * checking that every radius remains completely visible.
     */
    for (uint32_t frame = 0U; frame < 10000U; frame++) {
        screen_saver_step(&saver);
        assert_all_circles_are_on_screen(&saver);
    }

    {
        screen_saver_circle_t previous_circle =
            saver.circles[SCREEN_SAVER_MAX_CIRCLES - 2U];
        screen_saver_circle_t cleared_circle = {0};

        /*
         * Removing the newest slot must leave the previous slot unchanged,
         * which verifies the requested stack/LIFO behavior directly.
         */
        assert(screen_saver_remove_circle(&saver));
        assert(memcmp(&saver.circles[SCREEN_SAVER_MAX_CIRCLES - 2U],
                      &previous_circle, sizeof(previous_circle)) == 0);
        assert(memcmp(&saver.circles[SCREEN_SAVER_MAX_CIRCLES - 1U],
                      &cleared_circle, sizeof(cleared_circle)) == 0);
        assert(screen_saver_add_circle(&saver));
    }

    /* Repeated DOWN commands stop at one circle and therefore preserve min=1. */
    for (uint8_t i = SCREEN_SAVER_MAX_CIRCLES;
         i > SCREEN_SAVER_MIN_CIRCLES;
         i--) {
        assert(screen_saver_remove_circle(&saver));
    }
    assert(!screen_saver_remove_circle(&saver));
    assert(screen_saver_count(&saver) == SCREEN_SAVER_MIN_CIRCLES);

    return 0;
}
