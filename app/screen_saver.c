#include "screen_saver.h"

#include <stddef.h>
#include <string.h>

#define SCREEN_SAVER_Q8_SHIFT             (8)
#define SCREEN_SAVER_Q8_ONE               (1L << SCREEN_SAVER_Q8_SHIFT)
#define SCREEN_SAVER_VELOCITY_MIN_Q8      (96)
#define SCREEN_SAVER_VELOCITY_MAX_Q8      (192)
#define SCREEN_SAVER_CURRENT_PERIOD        (32U)
#define SCREEN_SAVER_CURRENT_DELTA_Q8      (8)
#define SCREEN_SAVER_DEFAULT_SEED         (0x6D2B79F5UL)

#define SCREEN_SAVER_X_MIN_Q8 \
    ((int32_t)SCREEN_SAVER_CIRCLE_RADIUS * SCREEN_SAVER_Q8_ONE)
#define SCREEN_SAVER_X_MAX_Q8 \
    ((int32_t)(SCREEN_SAVER_DISPLAY_WIDTH - 1 - \
               SCREEN_SAVER_CIRCLE_RADIUS) * SCREEN_SAVER_Q8_ONE)
#define SCREEN_SAVER_Y_MIN_Q8 \
    ((int32_t)SCREEN_SAVER_CIRCLE_RADIUS * SCREEN_SAVER_Q8_ONE)
#define SCREEN_SAVER_Y_MAX_Q8 \
    ((int32_t)(SCREEN_SAVER_DISPLAY_HEIGHT - 1 - \
               SCREEN_SAVER_CIRCLE_RADIUS) * SCREEN_SAVER_Q8_ONE)

static uint32_t screen_saver_random(screen_saver_t *saver)
{
    /*
     * xorshift32 is small, deterministic and independent of libc rand(), so
     * screensaver animation cannot disturb gameplay's meteor sequence.
     */
    uint32_t value = saver->random_state;

    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    saver->random_state = value;

    return value;
}

static int32_t screen_saver_random_range(screen_saver_t *saver,
                                         int32_t minimum,
                                         int32_t maximum)
{
    uint32_t span = (uint32_t)(maximum - minimum + 1);

    return minimum +
           (int32_t)(screen_saver_random(saver) % span);
}

static int16_t screen_saver_random_velocity(screen_saver_t *saver)
{
    int16_t magnitude =
        (int16_t)screen_saver_random_range(
            saver,
            SCREEN_SAVER_VELOCITY_MIN_Q8,
            SCREEN_SAVER_VELOCITY_MAX_Q8);

    return ((screen_saver_random(saver) & 1U) != 0U)
               ? magnitude
               : (int16_t)-magnitude;
}

static void screen_saver_set_velocity(screen_saver_t *saver,
                                      screen_saver_circle_t *circle)
{
    circle->velocity_x_q8 = screen_saver_random_velocity(saver);
    circle->velocity_y_q8 = screen_saver_random_velocity(saver);
}

static void screen_saver_reflect_axis(int32_t *position,
                                      int16_t *velocity,
                                      int32_t minimum,
                                      int32_t maximum)
{
    /*
     * Preserve the small amount of overshoot when reflecting.  This mirrors
     * the incidence angle and avoids a visible pause at the display boundary.
     */
    if (*position < minimum) {
        *position = minimum + (minimum - *position);
        if (*velocity < 0) {
            *velocity = (int16_t)-(*velocity);
        }
    } else if (*position > maximum) {
        *position = maximum - (*position - maximum);
        if (*velocity > 0) {
            *velocity = (int16_t)-(*velocity);
        }
    }
}

static int16_t screen_saver_clamp_velocity(int16_t velocity)
{
    if (velocity > SCREEN_SAVER_VELOCITY_MAX_Q8) {
        return SCREEN_SAVER_VELOCITY_MAX_Q8;
    }
    if (velocity < -SCREEN_SAVER_VELOCITY_MAX_Q8) {
        return -SCREEN_SAVER_VELOCITY_MAX_Q8;
    }
    if ((velocity >= 0) && (velocity < SCREEN_SAVER_VELOCITY_MIN_Q8)) {
        return SCREEN_SAVER_VELOCITY_MIN_Q8;
    }
    if ((velocity < 0) && (velocity > -SCREEN_SAVER_VELOCITY_MIN_Q8)) {
        return -SCREEN_SAVER_VELOCITY_MIN_Q8;
    }
    return velocity;
}

static void screen_saver_apply_current(screen_saver_t *saver)
{
    int16_t current_x =
        (int16_t)screen_saver_random_range(
            saver, -SCREEN_SAVER_CURRENT_DELTA_Q8,
            SCREEN_SAVER_CURRENT_DELTA_Q8);
    int16_t current_y =
        (int16_t)screen_saver_random_range(
            saver, -SCREEN_SAVER_CURRENT_DELTA_Q8,
            SCREEN_SAVER_CURRENT_DELTA_Q8);

    /*
     * One tiny shared perturbation every 1.28 seconds resembles a slow water
     * current without introducing per-frame jitter or floating-point math.
     */
    for (uint8_t i = 0U; i < saver->count; i++) {
        saver->circles[i].velocity_x_q8 =
            screen_saver_clamp_velocity(
                (int16_t)(saver->circles[i].velocity_x_q8 + current_x));
        saver->circles[i].velocity_y_q8 =
            screen_saver_clamp_velocity(
                (int16_t)(saver->circles[i].velocity_y_q8 + current_y));
    }
}

static void screen_saver_resolve_collisions(screen_saver_t *saver)
{
    const int32_t diameter_q8 =
        (2L * SCREEN_SAVER_CIRCLE_RADIUS * SCREEN_SAVER_Q8_ONE);
    const int32_t collision_distance_squared =
        diameter_q8 * diameter_q8;

    for (uint8_t first = 0U; first < saver->count; first++) {
        for (uint8_t second = (uint8_t)(first + 1U);
             second < saver->count;
             second++) {
            screen_saver_circle_t *a = &saver->circles[first];
            screen_saver_circle_t *b = &saver->circles[second];
            int32_t dx = b->x_q8 - a->x_q8;
            int32_t dy = b->y_q8 - a->y_q8;
            int32_t distance_squared = (dx * dx) + (dy * dy);
            int32_t relative_dot =
                ((int32_t)(b->velocity_x_q8 - a->velocity_x_q8) * dx) +
                ((int32_t)(b->velocity_y_q8 - a->velocity_y_q8) * dy);

            if ((distance_squared <= collision_distance_squared) &&
                (relative_dot < 0)) {
                /*
                 * Equal-mass circles exchange velocity vectors.  The approach
                 * test prevents repeated swapping while they move apart.
                 */
                int16_t velocity_x = a->velocity_x_q8;
                int16_t velocity_y = a->velocity_y_q8;
                a->velocity_x_q8 = b->velocity_x_q8;
                a->velocity_y_q8 = b->velocity_y_q8;
                b->velocity_x_q8 = velocity_x;
                b->velocity_y_q8 = velocity_y;
            }
        }
    }
}

void screen_saver_init(screen_saver_t *saver, uint32_t seed)
{
    if (saver == NULL) {
        return;
    }

    memset(saver, 0, sizeof(*saver));
    saver->random_state = (seed != 0U) ? seed : SCREEN_SAVER_DEFAULT_SEED;
    saver->count = SCREEN_SAVER_MIN_CIRCLES;

    /*
     * Every new screensaver session begins with exactly one circle at the
     * physical center, as required by the UI contract.
     */
    saver->circles[0].x_q8 =
        (SCREEN_SAVER_DISPLAY_WIDTH / 2) * SCREEN_SAVER_Q8_ONE;
    saver->circles[0].y_q8 =
        (SCREEN_SAVER_DISPLAY_HEIGHT / 2) * SCREEN_SAVER_Q8_ONE;
    screen_saver_set_velocity(saver, &saver->circles[0]);
}

bool screen_saver_add_circle(screen_saver_t *saver)
{
    screen_saver_circle_t *circle;

    if ((saver == NULL) ||
        (saver->count >= SCREEN_SAVER_MAX_CIRCLES)) {
        return false;
    }

    circle = &saver->circles[saver->count];
    circle->x_q8 =
        screen_saver_random_range(saver, SCREEN_SAVER_X_MIN_Q8,
                                  SCREEN_SAVER_X_MAX_Q8);
    circle->y_q8 =
        screen_saver_random_range(saver, SCREEN_SAVER_Y_MIN_Q8,
                                  SCREEN_SAVER_Y_MAX_Q8);
    screen_saver_set_velocity(saver, circle);
    saver->count++;

    return true;
}

bool screen_saver_remove_circle(screen_saver_t *saver)
{
    if ((saver == NULL) ||
        (saver->count <= SCREEN_SAVER_MIN_CIRCLES)) {
        return false;
    }

    /*
     * Circles occupy a contiguous array, so decrementing count removes the
     * most recently added item and implements LIFO without extra allocation.
     */
    saver->count--;
    memset(&saver->circles[saver->count], 0,
           sizeof(saver->circles[saver->count]));

    return true;
}

void screen_saver_step(screen_saver_t *saver)
{
    if (saver == NULL) {
        return;
    }

    for (uint8_t i = 0U; i < saver->count; i++) {
        screen_saver_circle_t *circle = &saver->circles[i];

        circle->x_q8 += circle->velocity_x_q8;
        circle->y_q8 += circle->velocity_y_q8;

        screen_saver_reflect_axis(&circle->x_q8,
                                  &circle->velocity_x_q8,
                                  SCREEN_SAVER_X_MIN_Q8,
                                  SCREEN_SAVER_X_MAX_Q8);
        screen_saver_reflect_axis(&circle->y_q8,
                                  &circle->velocity_y_q8,
                                  SCREEN_SAVER_Y_MIN_Q8,
                                  SCREEN_SAVER_Y_MAX_Q8);
    }

    screen_saver_resolve_collisions(saver);
    saver->frame_counter++;
    if ((saver->frame_counter % SCREEN_SAVER_CURRENT_PERIOD) == 0U) {
        screen_saver_apply_current(saver);
    }
}

uint8_t screen_saver_count(const screen_saver_t *saver)
{
    return (saver != NULL) ? saver->count : 0U;
}

bool screen_saver_get_circle(const screen_saver_t *saver, uint8_t index,
                             int16_t *x, int16_t *y)
{
    if ((saver == NULL) || (x == NULL) || (y == NULL) ||
        (index >= saver->count)) {
        return false;
    }

    /*
     * Round Q8.8 coordinates to the nearest display pixel instead of always
     * truncating, producing symmetric visual motion in both directions.
     */
    *x = (int16_t)((saver->circles[index].x_q8 +
                    (SCREEN_SAVER_Q8_ONE / 2)) >>
                   SCREEN_SAVER_Q8_SHIFT);
    *y = (int16_t)((saver->circles[index].y_q8 +
                    (SCREEN_SAVER_Q8_ONE / 2)) >>
                   SCREEN_SAVER_Q8_SHIFT);

    return true;
}
