#ifndef GAME_SESSION_H
#define GAME_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Restore one complete gameplay session without recreating RTOS resources.
 * The screen owner calls this only while gameplay producers are stopped.
 */
void game_session_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* GAME_SESSION_H */
