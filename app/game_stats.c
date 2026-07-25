#include "game_stats.h"

#include <stdbool.h>

#include "game_state.h"
#include "system_init.h"

static uint32_t shots;
static uint32_t hits;
static uint32_t accumulated_ms;
static uint32_t running_since;
static bool running;

static void game_stats_accumulate_locked(uint32_t now)
{
    if (running) {
        accumulated_ms += now - running_since;
        running_since = now;
    }
}

void game_stats_reset(void)
{
    game_state_lock();
    shots = 0U;
    hits = 0U;
    accumulated_ms = 0U;
    running_since = 0U;
    running = false;
    game_state_unlock();
}

void game_stats_start(void)
{
    game_state_lock();
    running_since = sys_get_tick();
    running = true;
    game_state_unlock();
}

void game_stats_pause(void)
{
    game_state_lock();
    game_stats_accumulate_locked(sys_get_tick());
    running = false;
    game_state_unlock();
}

void game_stats_resume(void)
{
    game_state_lock();
    if (!running) {
        running_since = sys_get_tick();
        running = true;
    }
    game_state_unlock();
}

void game_stats_finish(void)
{
    game_stats_pause();
}

void game_stats_record_shot(void)
{
    game_state_lock();
    shots++;
    game_state_unlock();
}

void game_stats_record_hit(void)
{
    game_state_lock();
    hits++;
    game_state_unlock();
}

game_stats_snapshot_t game_stats_get(void)
{
    game_stats_snapshot_t snapshot;

    game_state_lock();
    game_stats_accumulate_locked(sys_get_tick());
    snapshot.shots = shots;
    snapshot.hits = hits;
    snapshot.elapsed_ms = accumulated_ms;
    snapshot.accuracy_percent =
        (shots != 0U) ? (uint8_t)((hits * 100U) / shots) : 0U;
    game_state_unlock();

    return snapshot;
}
