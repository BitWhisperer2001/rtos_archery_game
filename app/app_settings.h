#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define APP_SETTINGS_VERSION             (1U)
#define APP_SETTINGS_SAVER_TIMEOUT_MIN_S (5U)
#define APP_SETTINGS_SLEEP_TIMEOUT_MIN_S (30U)

typedef enum {
    APP_DIFFICULTY_EASY = 0,
    APP_DIFFICULTY_NORMAL,
    APP_DIFFICULTY_HARD,
    APP_DIFFICULTY_COUNT
} app_difficulty_t;

typedef struct {
    uint8_t version;
    app_difficulty_t difficulty;
    uint8_t saver_effect;
    bool sound_enabled;
    uint8_t saver_timeout_s;
    uint16_t sleep_timeout_s;
} app_settings_t;

void app_settings_init(void);
app_settings_t app_settings_get(void);
void app_settings_cycle_difficulty(int8_t direction);
void app_settings_cycle_saver_effect(int8_t direction,
                                     uint8_t effect_count);
void app_settings_toggle_sound(void);
bool app_settings_save(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SETTINGS_H */
