#ifndef GAME_STATS_H
#define GAME_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    uint32_t shots;
    uint32_t hits;
    uint32_t elapsed_ms;
    uint8_t accuracy_percent;
} game_stats_snapshot_t;

void game_stats_reset(void);
void game_stats_start(void);
void game_stats_pause(void);
void game_stats_resume(void);
void game_stats_finish(void);
void game_stats_record_shot(void);
void game_stats_record_hit(void);
game_stats_snapshot_t game_stats_get(void);

#ifdef __cplusplus
}
#endif

#endif /* GAME_STATS_H */
