#ifndef SCREEN_SAVER_H
#define SCREEN_SAVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define SCREEN_SAVER_DISPLAY_WIDTH      (128)
#define SCREEN_SAVER_DISPLAY_HEIGHT     (64)
#define SCREEN_SAVER_CIRCLE_RADIUS      (8)
#define SCREEN_SAVER_MIN_CIRCLES        (1U)
#define SCREEN_SAVER_MAX_CIRCLES        (20U)

/*
 * Q8.8 coordinates retain sub-pixel motion.  The OLED still displays integer
 * pixels, but fractional accumulation avoids jerky one-pixel jumps every frame.
 */
typedef struct {
    int32_t x_q8;
    int32_t y_q8;
    int16_t velocity_x_q8;
    int16_t velocity_y_q8;
} screen_saver_circle_t;

typedef struct {
    screen_saver_circle_t circles[SCREEN_SAVER_MAX_CIRCLES];
    uint32_t random_state;
    uint8_t count;
} screen_saver_t;

void screen_saver_init(screen_saver_t *saver, uint32_t seed);
bool screen_saver_add_circle(screen_saver_t *saver);
bool screen_saver_remove_circle(screen_saver_t *saver);
void screen_saver_step(screen_saver_t *saver);
uint8_t screen_saver_count(const screen_saver_t *saver);
bool screen_saver_get_circle(const screen_saver_t *saver, uint8_t index,
                             int16_t *x, int16_t *y);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_SAVER_H */
