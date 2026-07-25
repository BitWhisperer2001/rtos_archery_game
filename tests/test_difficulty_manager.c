#include <assert.h>
#include <stdint.h>

#include "difficulty_manager.h"

static uint32_t register_hits_until_change(uint8_t hit_count)
{
    uint32_t period = 0U;
    bool changed = false;

    for (uint8_t i = 0U; i < hit_count; i++) {
        changed = difficulty_manager_register_hit_locked(&period);
    }

    assert(changed);
    return period;
}

int main(void)
{
    difficulty_manager_reset(APP_DIFFICULTY_EASY);
    assert(difficulty_manager_initial_period_ms() == 150U);
    assert(register_hits_until_change(12U) == 140U);

    difficulty_manager_reset(APP_DIFFICULTY_NORMAL);
    assert(difficulty_manager_initial_period_ms() == 120U);
    assert(register_hits_until_change(10U) == 105U);

    difficulty_manager_reset(APP_DIFFICULTY_HARD);
    assert(difficulty_manager_initial_period_ms() == 90U);
    assert(register_hits_until_change(8U) == 75U);

    uint32_t period = 0U;
    for (uint16_t hit = 0U; hit < 1000U; hit++) {
        (void)difficulty_manager_register_hit_locked(&period);
    }
    assert(difficulty_manager_initial_period_ms() >= 25U);
    return 0;
}
