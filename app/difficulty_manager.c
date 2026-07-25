#include "difficulty_manager.h"

#include <stddef.h>

typedef struct {
    uint32_t initial_period_ms;
    uint32_t minimum_period_ms;
    uint32_t period_step_ms;
    uint8_t hits_per_level;
} difficulty_profile_t;

static const difficulty_profile_t profiles[APP_DIFFICULTY_COUNT] = {
    [APP_DIFFICULTY_EASY] = {
        .initial_period_ms = 150U,
        .minimum_period_ms = 45U,
        .period_step_ms = 10U,
        .hits_per_level = 12U
    },
    [APP_DIFFICULTY_NORMAL] = {
        .initial_period_ms = 120U,
        .minimum_period_ms = 30U,
        .period_step_ms = 15U,
        .hits_per_level = 10U
    },
    [APP_DIFFICULTY_HARD] = {
        .initial_period_ms = 90U,
        .minimum_period_ms = 25U,
        .period_step_ms = 15U,
        .hits_per_level = 8U
    }
};

static const difficulty_profile_t *active_profile =
    &profiles[APP_DIFFICULTY_NORMAL];
static uint32_t active_period_ms = 120U;
static uint8_t hit_progress = 0U;

void difficulty_manager_reset(app_difficulty_t level)
{
    if (level >= APP_DIFFICULTY_COUNT) {
        level = APP_DIFFICULTY_NORMAL;
    }

    active_profile = &profiles[level];
    active_period_ms = active_profile->initial_period_ms;
    hit_progress = 0U;
}

uint32_t difficulty_manager_initial_period_ms(void)
{
    return active_period_ms;
}

bool difficulty_manager_register_hit_locked(uint32_t *new_period_ms)
{
    if (new_period_ms == NULL) {
        return false;
    }

    hit_progress++;
    if (hit_progress < active_profile->hits_per_level) {
        return false;
    }
    hit_progress = 0U;

    if (active_period_ms <= active_profile->minimum_period_ms) {
        return false;
    }

    if (active_period_ms >
        (active_profile->minimum_period_ms + active_profile->period_step_ms)) {
        active_period_ms -= active_profile->period_step_ms;
    } else {
        active_period_ms = active_profile->minimum_period_ms;
    }

    *new_period_ms = active_period_ms;
    return true;
}
