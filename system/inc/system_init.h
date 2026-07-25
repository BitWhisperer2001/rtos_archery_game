#ifndef _SYSTEM_INIT_
#define _SYSTEM_INIT_

#ifdef __cplusplus
extern "C" {
#endif   // __cplusplus

#include <stdint.h>
#include <stdbool.h>

extern volatile uint32_t g_cnt;

extern uint8_t __nvm_start__;
extern uint8_t __nvm_end__;

#define TIM2_PRESCALER      (499U)
#define TIM2_PERIOD         (199U)
#define unused(x)           (void)(x)
#define SCORE_FLASH_ADDRESS ((uint32_t)(uintptr_t)&__nvm_start__)
#define SYSTEM_CORE_CLOCK_HZ (100000000UL)
/* One explicit console contract keeps firmware and documentation synchronized. */
#define SYSTEM_LOG_BAUDRATE  (115200UL)

extern void sys_clock_config(void);
extern uint32_t sys_get_tick(void);
extern void sys_tick_config(void);
extern void sys_delay(uint32_t ms);
extern void sys_log_config(void);

/*
 * Persistent-score APIs retain the original address argument so existing
 * learning code can still see the linker-defined NVM boundary. The
 * implementation validates that address and stores journaled records.
 */
extern bool sys_save_score_into_flash(uint32_t add, uint32_t data);
extern uint32_t sys_read_score_in_flash(uint32_t add);

#ifdef __cplusplus
}
#endif   // __cplusplus
#endif   //_SYSTEM_INIT_
