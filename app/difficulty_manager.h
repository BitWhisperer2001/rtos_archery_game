#ifndef DIFFICULTY_MANAGER_H
#define DIFFICULTY_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "app_settings.h"

void difficulty_manager_reset(app_difficulty_t level);
uint32_t difficulty_manager_initial_period_ms(void);

/*
 * Caller holds game_state_mutex.  Returns true only when this hit advances the
 * cadence and writes the new saturated period to new_period_ms.
 */
bool difficulty_manager_register_hit_locked(uint32_t *new_period_ms);

#ifdef __cplusplus
}
#endif

#endif /* DIFFICULTY_MANAGER_H */
