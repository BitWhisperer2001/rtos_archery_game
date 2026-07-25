#ifndef GAME_STATE_H
#define GAME_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Shared game objects are updated by several worker tasks.  This module keeps
 * their synchronization policy in one place instead of scattering RTOS
 * critical sections throughout gameplay code.
 */
void game_state_init(void);
void game_state_lock(void);
void game_state_unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* GAME_STATE_H */
