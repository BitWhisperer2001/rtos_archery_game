#ifndef _SYSTEM_INIT_
#define _SYSTEM_INIT_

#ifdef __cplusplus
extern "C" {
#endif   // __cplusplus

#include <stdint.h>
#include <stdbool.h>

extern volatile uint32_t g_cnt;

// #define sys_reset()         NVIC_SystemReset();
#define TIM2_PRESCALER      (499U)
#define TIM2_PERIOD         (203U)
#define unused(x)           (void)(x)

extern void sys_clock_config(void);
extern uint32_t sys_get_tick(void);
extern void sys_tick_config(void);
extern void sys_delay(uint32_t ms);
extern void sys_log_config(void);
extern void sys_save_score_into_flash(uint32_t add, uint32_t data);
extern uint32_t sys_read_score_in_flash(uint32_t add);

#ifdef __cplusplus
}
#endif   // __cplusplus
#endif   //_SYSTEM_INIT_
