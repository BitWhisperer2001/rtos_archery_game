#include "app_settings.h"

#include "game_state.h"
#include "system_init.h"

#define SETTINGS_DIFFICULTY_SHIFT      (0U)
#define SETTINGS_EFFECT_SHIFT          (2U)
#define SETTINGS_SOUND_SHIFT           (4U)
#define SETTINGS_SAVER_TIMEOUT_SHIFT   (8U)
#define SETTINGS_SLEEP_UNITS_SHIFT     (16U)
#define SETTINGS_VERSION_SHIFT         (28U)
#define SETTINGS_FIELD_MASK            (0xFFUL)
#define SETTINGS_SLEEP_UNIT_SECONDS    (10U)
#define SETTINGS_SUPPORTED_EFFECTS     (3U)

static app_settings_t settings;

static app_settings_t app_settings_defaults(void)
{
    app_settings_t defaults = {
        .version = APP_SETTINGS_VERSION,
        .difficulty = APP_DIFFICULTY_NORMAL,
        .saver_effect = 0U,
        .sound_enabled = true,
        .saver_timeout_s = 10U,
        .sleep_timeout_s = 120U
    };

    return defaults;
}

static uint32_t app_settings_pack(const app_settings_t *value)
{
    uint32_t sleep_units =
        (uint32_t)(value->sleep_timeout_s / SETTINGS_SLEEP_UNIT_SECONDS);

    return ((uint32_t)value->difficulty << SETTINGS_DIFFICULTY_SHIFT) |
           ((uint32_t)value->saver_effect << SETTINGS_EFFECT_SHIFT) |
           ((uint32_t)value->sound_enabled << SETTINGS_SOUND_SHIFT) |
           ((uint32_t)value->saver_timeout_s <<
            SETTINGS_SAVER_TIMEOUT_SHIFT) |
           (sleep_units << SETTINGS_SLEEP_UNITS_SHIFT) |
           ((uint32_t)value->version << SETTINGS_VERSION_SHIFT);
}

static bool app_settings_unpack(uint32_t packed, app_settings_t *value)
{
    value->version =
        (uint8_t)((packed >> SETTINGS_VERSION_SHIFT) & 0x0FUL);
    value->difficulty =
        (app_difficulty_t)((packed >> SETTINGS_DIFFICULTY_SHIFT) & 0x03UL);
    value->saver_effect =
        (uint8_t)((packed >> SETTINGS_EFFECT_SHIFT) & 0x03UL);
    value->sound_enabled =
        ((packed >> SETTINGS_SOUND_SHIFT) & 0x01UL) != 0U;
    value->saver_timeout_s =
        (uint8_t)((packed >> SETTINGS_SAVER_TIMEOUT_SHIFT) &
                  SETTINGS_FIELD_MASK);
    value->sleep_timeout_s =
        (uint16_t)(((packed >> SETTINGS_SLEEP_UNITS_SHIFT) &
                    SETTINGS_FIELD_MASK) * SETTINGS_SLEEP_UNIT_SECONDS);

    return (value->version == APP_SETTINGS_VERSION) &&
           (value->difficulty < APP_DIFFICULTY_COUNT) &&
           (value->saver_effect < SETTINGS_SUPPORTED_EFFECTS) &&
           (value->saver_timeout_s >= APP_SETTINGS_SAVER_TIMEOUT_MIN_S) &&
           (value->sleep_timeout_s >= APP_SETTINGS_SLEEP_TIMEOUT_MIN_S);
}

void app_settings_init(void)
{
    app_settings_t defaults = app_settings_defaults();
    app_settings_t loaded;
    uint32_t packed_default = app_settings_pack(&defaults);
    uint32_t packed =
        sys_read_settings_in_flash(packed_default);

    /*
     * A future or corrupt settings payload falls back safely.  It is not
     * rewritten during boot, avoiding unnecessary Flash wear.
     */
    settings = app_settings_unpack(packed, &loaded) ? loaded : defaults;
}

app_settings_t app_settings_get(void)
{
    app_settings_t snapshot;

    game_state_lock();
    snapshot = settings;
    game_state_unlock();

    return snapshot;
}

void app_settings_cycle_difficulty(int8_t direction)
{
    game_state_lock();

    int16_t next = (int16_t)settings.difficulty + direction;
    if (next < 0) {
        next = APP_DIFFICULTY_COUNT - 1;
    } else if (next >= APP_DIFFICULTY_COUNT) {
        next = 0;
    }
    settings.difficulty = (app_difficulty_t)next;

    game_state_unlock();
}

void app_settings_cycle_saver_effect(int8_t direction,
                                     uint8_t effect_count)
{
    if (effect_count == 0U) {
        return;
    }

    game_state_lock();

    int16_t next = (int16_t)settings.saver_effect + direction;
    if (next < 0) {
        next = (int16_t)effect_count - 1;
    } else if (next >= effect_count) {
        next = 0;
    }
    settings.saver_effect = (uint8_t)next;

    game_state_unlock();
}

void app_settings_toggle_sound(void)
{
    game_state_lock();
    settings.sound_enabled = !settings.sound_enabled;
    game_state_unlock();
}

bool app_settings_save(void)
{
    app_settings_t snapshot = app_settings_get();
    return sys_save_settings_into_flash(app_settings_pack(&snapshot));
}
