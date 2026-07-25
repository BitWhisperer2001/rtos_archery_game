#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "screen_task.h"

#include "app_settings.h"
#include "app_state.h"
#include "archery.h"
#include "arrow.h"
#include "bang.h"
#include "bitmap.h"
#include "border.h"
#include "buzzer_task.h"
#include "game_session.h"
#include "game_state.h"
#include "game_stats.h"
#include "meteoroid.h"
#include "saver_engine.h"
#include "screen.h"
#include "ssd1306.h"
#include "system_init.h"
#include "system_log.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "timers.h"

EventGroupHandle_t screen_event_state = NULL;
TimerHandle_t timer_update_screen_handle = NULL;
TaskHandle_t task_update_screen_handle = NULL;

#define SCREEN_ALL_BITS \
    (SCREEN_UPDATE_BIT | SCREEN_GAME_OVER_BIT | SCREEN_GAME_START_BIT | \
     SCREEN_SAVER_EXIT_BIT | SCREEN_SAVER_ADD_BIT | \
     SCREEN_SAVER_REMOVE_BIT | SCREEN_RETURN_TO_START_BIT | \
     SCREEN_PAUSE_BIT | SCREEN_RESUME_BIT | SCREEN_SAVER_NEXT_BIT | \
     SCREEN_SAVER_PREVIOUS_BIT | SCREEN_SETTINGS_ENTER_BIT | \
     SCREEN_SETTINGS_EXIT_BIT | SCREEN_SETTINGS_DIFFICULTY_BIT | \
     SCREEN_SETTINGS_EFFECT_BIT | SCREEN_SETTINGS_SOUND_BIT | \
     SCREEN_WAKE_BIT)

#define SCREEN_SAVER_FRAME_PERIOD_MS    (40U)
#define SCREEN_SAVER_DIM_PERIOD_MS      (100U)
#define SCREEN_SAVER_DIM_AFTER_MS       (60000U)

/* uint32_t needs at most ten decimal digits plus the string terminator. */
static char your_score[11] = {0};
static char best_score[11] = {0};

static saver_engine_t saver_engine;
static TickType_t screen_saver_next_frame = 0U;
static TickType_t screen_saver_started_at = 0U;
static bool screen_powered = true;

typedef struct {
    game_archery_t archery;
    game_arrow_t arrows[ARROW_MAX_NUM];
    game_meteoroid_t meteoroids[METEROID_MAX_NUM];
    game_bang_t bangs[BANG_MAX_NUM];
    game_border_t border;
} screen_game_snapshot_t;

static bool screen_present(void)
{
    /*
     * Screen task is the sole runtime owner of both the SSD1306 framebuffer and
     * I2C transfer, so no mutex or interrupt masking is necessary here.
     */
    return SSD1306_UpdateScreen() != 0U;
}

static void screen_display_waiting_for_start(void)
{
    SSD1306_Clear();
    SSD1306_GotoXY(0, 2);
    SSD1306_Puts("ARCHERY GAME", &Font_11x18, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(2, 24);
    SSD1306_Puts("Design by Minh Chi", &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(13, 39);
    SSD1306_Puts("HOLD OK: SETTINGS", &Font_6x8, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(4, 53);
    SSD1306_Puts("PRESS OK TO START", &Font_7x10, SSD1306_COLOR_WHITE);

    if (!screen_present()) {
        DBG_LOG("OLED waiting-screen transfer failed");
        return;
    }

    /*
     * The start melody describes the visible prompt, not the gameplay action.
     * Publish it only after the framebuffer has reached the OLED successfully.
     */
    (void)buzzer_request_cue(AUDIO_CUE_GAME_START);
}

static void screen_render_saver_frame(void)
{
    SSD1306_Clear();

    if (saver_engine_effect(&saver_engine) == SAVER_EFFECT_BUBBLES) {
        for (uint8_t i = 0U;
             i < saver_engine_circle_count(&saver_engine);
             i++) {
            int16_t x;
            int16_t y;

            if (saver_engine_get_circle(&saver_engine, i, &x, &y)) {
                SSD1306_DrawCircle(x, y, SCREEN_SAVER_CIRCLE_RADIUS,
                                   SSD1306_COLOR_WHITE);
            }
        }
    } else if (saver_engine_effect(&saver_engine) ==
               SAVER_EFFECT_STARFIELD) {
        for (uint8_t i = 0U;
             i < saver_engine_star_count(&saver_engine);
             i++) {
            int16_t x;
            int16_t y;

            if (saver_engine_get_star(&saver_engine, i, &x, &y) &&
                (x >= 0) && (y >= 0)) {
                SSD1306_DrawPixel((uint16_t)x, (uint16_t)y,
                                  SSD1306_COLOR_WHITE);
            }
        }
    } else {
        for (uint8_t wave = 0U;
             wave < saver_engine_wave_count(&saver_engine);
             wave++) {
            int16_t previous_y =
                saver_engine_wave_y(&saver_engine, wave, 0U);

            for (uint8_t x = 2U; x < SSD1306_WIDTH; x += 2U) {
                int16_t y = saver_engine_wave_y(&saver_engine, wave, x);
                SSD1306_DrawLine((uint16_t)(x - 2U),
                                 (uint16_t)previous_y,
                                 x, (uint16_t)y,
                                 SSD1306_COLOR_WHITE);
                previous_y = y;
            }
        }
    }

    if (!screen_present()) {
        DBG_LOG("OLED screensaver transfer failed");
    }
}

static void screen_enter_saver(void)
{
    app_settings_t settings = app_settings_get();

    /*
     * A new session always restarts at one centered circle.  The time-based
     * seed changes the random positions and directions of later UP additions.
     */
    saver_engine_init(&saver_engine,
                      sys_get_tick() ^ 0xA511E9B3UL,
                      (saver_effect_t)settings.saver_effect);
    /*
     * Establish the deadline before rendering.  OLED transfer time is therefore
     * included in the 40 ms period instead of being added on top of it.
     */
    screen_saver_started_at = xTaskGetTickCount();
    screen_saver_next_frame =
        screen_saver_started_at +
        pdMS_TO_TICKS(SCREEN_SAVER_FRAME_PERIOD_MS);
    app_state_set(APP_STATE_SCREENSAVER);
    screen_render_saver_frame();
}

static void screen_leave_saver(void)
{
    /*
     * OK exits only to the normal start screen.  A second, deliberate OK press
     * is required to launch gameplay.
     */
    app_state_set(APP_STATE_WAITING_FOR_START);
    screen_display_waiting_for_start();
}

static TickType_t screen_wait_timeout(app_state_t mode)
{
    if (mode == APP_STATE_WAITING_FOR_START) {
        app_settings_t settings = app_settings_get();
        return pdMS_TO_TICKS((uint32_t)settings.saver_timeout_s * 1000U);
    }

    if (mode == APP_STATE_SCREENSAVER) {
        TickType_t now = xTaskGetTickCount();

        /*
         * Signed modular subtraction remains correct across the 32-bit tick
         * wrap as long as the frame period is below half the tick range.
         */
        if ((int32_t)(screen_saver_next_frame - now) <= 0) {
            return 0U;
        }
        return screen_saver_next_frame - now;
    }

    return portMAX_DELAY;
}

static void screen_schedule_next_saver_frame(void)
{
    TickType_t now = xTaskGetTickCount();
    uint32_t elapsed_ms =
        (uint32_t)((now - screen_saver_started_at) * portTICK_PERIOD_MS);
    uint32_t period_ms =
        (elapsed_ms >= SCREEN_SAVER_DIM_AFTER_MS)
            ? SCREEN_SAVER_DIM_PERIOD_MS
            : SCREEN_SAVER_FRAME_PERIOD_MS;
    TickType_t period = pdMS_TO_TICKS(period_ms);

    /*
     * Advance from the previous deadline, not from "now".  If a slow I2C retry
     * misses a frame, skip stale deadlines without accumulating long-term drift.
     */
    do {
        screen_saver_next_frame += period;
    } while ((int32_t)(screen_saver_next_frame - now) <= 0);
}

static void screen_snapshot_game(screen_game_snapshot_t *snapshot)
{
    game_state_lock();

    snapshot->archery = game_archery;
    snapshot->border = game_border;

    for (uint8_t i = 0U; i < ARROW_MAX_NUM; i++) {
        snapshot->arrows[i] = game_arrow[i];
    }

    for (uint8_t i = 0U; i < METEROID_MAX_NUM; i++) {
        snapshot->meteoroids[i] = game_meteoroid[i];
    }

    for (uint8_t i = 0U; i < BANG_MAX_NUM; i++) {
        snapshot->bangs[i] = game_bang[i];
    }

    game_state_unlock();
}

static const unsigned char *screen_arrow_count_bitmap(int16_t arrow_count)
{
    switch (arrow_count) {
        case 0:
            return bitmap_arrow_0;
        case 1:
            return bitmap_arrow_1;
        case 2:
            return bitmap_arrow_2;
        case 3:
            return bitmap_arrow_3;
        case 4:
            return bitmap_arrow_4;
        default:
            return bitmap_arrow_5;
    }
}

static void screen_draw_bang(const game_bang_t *bang)
{
    if (!bang->visible) {
        return;
    }

    /*
     * State 3 uses the smaller final sprite and preserves the original visual
     * offset.  Earlier states share the 15x15 animation footprint.
     */
    if (bang->state == 3U) {
        SSD1306_DrawBitmap(bang->x + 2, bang->y + 3, bang->action_img,
                           BANG_III_WIDTH, BANG_III_HEIGH,
                           SSD1306_COLOR_WHITE);
    } else {
        SSD1306_DrawBitmap(bang->x, bang->y, bang->action_img,
                           BANG_I_WIDTH, BANG_I_HEIGH,
                           SSD1306_COLOR_WHITE);
    }
}

static void screen_render_game_frame(void)
{
    screen_game_snapshot_t snapshot;
    char score_text[11] = {0};

    /*
     * Copy a coherent state quickly, release its mutex, then perform all pixel
     * work on private data.  Gameplay therefore never waits for OLED I2C.
     */
    screen_snapshot_game(&snapshot);

    SSD1306_Clear();
    SSD1306_DrawBitmap(snapshot.border.x_border, snapshot.border.y_border,
                       snapshot.border.action_img, BORDER_WIDTH, BORDER_HEIGH,
                       SSD1306_COLOR_WHITE);

    SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y,
                       screen_arrow_count_bitmap(snapshot.border.arrow_num),
                       ARROW_NUM_WIDTH, ARROW_NUM_HEIGH,
                       SSD1306_COLOR_WHITE);

    snprintf(score_text, sizeof(score_text), "%u",
             (unsigned int)snapshot.border.game_score);
    SSD1306_GotoXY(37, 55);
    SSD1306_Puts(score_text, &Font_6x8, SSD1306_COLOR_WHITE);

    if (snapshot.archery.visible) {
        SSD1306_DrawBitmap(snapshot.archery.x, snapshot.archery.y,
                           snapshot.archery.action_img, ARCHERY_WIDTH,
                           ARCHERY_HEIGH, SSD1306_COLOR_WHITE);
    }

    for (uint8_t i = 0U; i < METEROID_MAX_NUM; i++) {
        if (snapshot.meteoroids[i].visible) {
            SSD1306_DrawBitmap(snapshot.meteoroids[i].x,
                               snapshot.meteoroids[i].y,
                               snapshot.meteoroids[i].action_img,
                               METEOROID_WIDTH, METEOROID_HEIGH,
                               SSD1306_COLOR_WHITE);
        }
    }

    for (uint8_t i = 0U; i < ARROW_MAX_NUM; i++) {
        if (snapshot.arrows[i].visible) {
            SSD1306_DrawBitmap(snapshot.arrows[i].x, snapshot.arrows[i].y,
                               snapshot.arrows[i].action_img, ARROW_WIDTH,
                               ARROW_HEIGHT, SSD1306_COLOR_WHITE);
        }
    }

    for (uint8_t i = 0U; i < BANG_MAX_NUM; i++) {
        screen_draw_bang(&snapshot.bangs[i]);
    }

    if (!screen_present()) {
        DBG_LOG("OLED frame transfer failed");
    }
}

static void screen_update_tick_callback(TimerHandle_t timer)
{
    (void)timer;

    /*
     * Software-timer callbacks must remain bounded.  The callback only starts
     * the gameplay pipeline; rendering and object updates run in worker tasks.
     */
    xEventGroupSetBits(archery_event_state, ARCHERY_INIT_BIT);
}

static void screen_display_game_over(void)
{
    uint32_t final_score;
    uint32_t final_best;
    game_stats_snapshot_t stats = game_stats_get();
    char stats_text[22];

    game_state_lock();
    final_score = game_border.game_score;
    final_best = game_border.highest_score;
    game_state_unlock();

    SSD1306_Clear();
    SSD1306_DrawFilledRectangle(0, 0, SSD1306_WIDTH, SSD1306_HEIGHT,
                                SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(18, 5);
    SSD1306_Puts("GAME OVER", &Font_11x18, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(2, 25);
    SSD1306_Puts("BEST:", &Font_6x8, SSD1306_COLOR_BLACK);
    snprintf(best_score, sizeof(best_score), "%u", (unsigned int)final_best);
    SSD1306_GotoXY(38, 25);
    SSD1306_Puts(best_score, &Font_6x8, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(68, 25);
    SSD1306_Puts("SCORE:", &Font_6x8, SSD1306_COLOR_BLACK);
    snprintf(your_score, sizeof(your_score), "%u", (unsigned int)final_score);
    SSD1306_GotoXY(104, 25);
    SSD1306_Puts(your_score, &Font_6x8, SSD1306_COLOR_BLACK);

    snprintf(stats_text, sizeof(stats_text), "HIT:%lu ACC:%u%%",
             (unsigned long)stats.hits,
             (unsigned int)stats.accuracy_percent);
    SSD1306_GotoXY(20, 38);
    SSD1306_Puts(stats_text, &Font_6x8, SSD1306_COLOR_BLACK);
    /*
     * Match the state machine precisely: only OK performs a software restart.
     */
    SSD1306_GotoXY(37, 53);
    SSD1306_Puts("PRESS OK", &Font_7x10, SSD1306_COLOR_BLACK);

    if (!screen_present()) {
        DBG_LOG("OLED update failed on game-over screen");
    }
}

static void screen_update_high_score(void)
{
    uint32_t current_score;
    uint32_t stored_score;
    uint32_t highest_score;

    game_state_lock();
    current_score = game_border.game_score;
    game_state_unlock();

    /*
     * Flash access is intentionally outside the state mutex: an erase/program
     * cycle must never stall unrelated input or gameplay-state readers.
     */
    stored_score = sys_read_score_in_flash(SCORE_FLASH_ADDRESS);
    highest_score = stored_score;

    if (current_score > stored_score) {
        if (!sys_save_score_into_flash(SCORE_FLASH_ADDRESS, current_score)) {
            DBG_LOG("Failed to persist high score");
        }
        highest_score = current_score;
    }

    game_state_lock();
    game_border.highest_score = highest_score;
    game_state_unlock();
}

static void screen_start_game(void)
{
    /*
     * Publish RUNNING before waking producers so every worker accepts the first
     * tick.  Timers were created during boot but are deliberately started only
     * after the user presses OK.
     */
    app_state_set(APP_STATE_RUNNING);
    game_stats_start();

    screen_render_game_frame();
    xEventGroupSetBits(archery_event_state, ARCHERY_INIT_BIT);

    configASSERT(xTimerStart(timer_update_screen_handle,
                             pdMS_TO_TICKS(10U)) == pdPASS);
    /*
     * xTimerChangePeriod also starts a dormant timer.  Using the freshly reset
     * initial period prevents difficulty from leaking across play-throughs.
     */
    configASSERT(
        xTimerChangePeriod((TimerHandle_t)meteoroid_timer,
                           pdMS_TO_TICKS(meteoroid_timer_period_ms),
                           pdMS_TO_TICKS(10U)) == pdPASS);
}

static void screen_display_paused(void)
{
    screen_render_game_frame();
    SSD1306_DrawFilledRectangle(24, 20, 80, 24, SSD1306_COLOR_BLACK);
    SSD1306_DrawRectangle(24, 20, 80, 24, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(36, 27);
    SSD1306_Puts("PAUSED", &Font_11x18, SSD1306_COLOR_WHITE);

    if (!screen_present()) {
        DBG_LOG("OLED paused-screen transfer failed");
    }
}

static void screen_pause_game(void)
{
    /*
     * The request already published PAUSED, so workers reject stale wake-ups
     * while timer stop commands are being delivered.
     */
    configASSERT(xTimerStop(timer_update_screen_handle,
                            pdMS_TO_TICKS(10U)) == pdPASS);
    configASSERT(xTimerStop((TimerHandle_t)meteoroid_timer,
                            pdMS_TO_TICKS(10U)) == pdPASS);
    game_stats_pause();
    screen_display_paused();
}

static void screen_resume_game(void)
{
    app_state_set(APP_STATE_RUNNING);
    game_stats_resume();
    screen_render_game_frame();

    configASSERT(xTimerStart(timer_update_screen_handle,
                             pdMS_TO_TICKS(10U)) == pdPASS);
    configASSERT(xTimerStart((TimerHandle_t)meteoroid_timer,
                             pdMS_TO_TICKS(10U)) == pdPASS);
}

static const char *screen_difficulty_name(app_difficulty_t difficulty)
{
    static const char *const names[APP_DIFFICULTY_COUNT] = {
        "EASY", "NORMAL", "HARD"
    };
    return (difficulty < APP_DIFFICULTY_COUNT) ? names[difficulty] :
                                                 "NORMAL";
}

static void screen_display_settings(void)
{
    app_settings_t settings = app_settings_get();

    SSD1306_Clear();
    SSD1306_GotoXY(22, 0);
    SSD1306_Puts("SETTINGS", &Font_11x18, SSD1306_COLOR_WHITE);
    SSD1306_DrawFilledRectangle(0, 21, 42, 8, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(2, 22);
    SSD1306_Puts("UP DIFF", &Font_6x8, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(88, 22);
    SSD1306_Puts((char *)screen_difficulty_name(settings.difficulty), &Font_6x8, SSD1306_COLOR_WHITE);
    SSD1306_DrawFilledRectangle(0, 32, 42, 8, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(2, 33);
    SSD1306_Puts("DOWN FX", &Font_6x8, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(88, 33);
    SSD1306_Puts((char *)saver_engine_effect_name((saver_effect_t)settings.saver_effect), &Font_6x8, SSD1306_COLOR_WHITE);
    SSD1306_DrawFilledRectangle(0, 43, 78, 8, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(2, 44);
    SSD1306_Puts("HOLD UP SOUND", &Font_6x8, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(88, 44);
    SSD1306_Puts(settings.sound_enabled ? "ON" : "OFF", &Font_6x8, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(25, 56);
    SSD1306_Puts("OK: SAVE/BACK", &Font_6x8, SSD1306_COLOR_WHITE);

    if (!screen_present()) {
        DBG_LOG("OLED settings transfer failed");
    }
}

static void screen_enter_settings(void)
{
    screen_display_settings();
}

static void screen_exit_settings(void)
{
    if (!app_settings_save()) {
        DBG_LOG("Failed to persist application settings");
    }

    /*
     * Apply the selected difficulty immediately to the next game, including
     * the very first game after boot.  Session reset is safe in SETTINGS and
     * also clears any stale input-to-gameplay pipeline state.
     */
    game_session_reset();
    app_state_set(APP_STATE_WAITING_FOR_START);
    screen_display_waiting_for_start();
}

static void screen_enter_display_sleep(void)
{
    /*
     * The idle hook already executes WFI.  Turning off the OLED charge pump is
     * the remaining large idle-power saving after prolonged inactivity.
     */
    SSD1306_OFF();
    screen_powered = false;
    app_state_set(APP_STATE_DISPLAY_SLEEP);
}

static void screen_wake_display(void)
{
    if (!screen_powered) {
        SSD1306_ON();
        screen_powered = true;
    }
    app_state_set(APP_STATE_WAITING_FOR_START);
    screen_display_waiting_for_start();
}

static void screen_finish_game(void)
{
    /*
     * Stop both periodic producers.  Worker tasks stay alive and blocked, which
     * is safer than externally suspending tasks at arbitrary instruction points.
     */
    configASSERT(xTimerStop(timer_update_screen_handle,
                            pdMS_TO_TICKS(10U)) == pdPASS);
    configASSERT(xTimerStop((TimerHandle_t)meteoroid_timer,
                            pdMS_TO_TICKS(10U)) == pdPASS);

    game_stats_finish();
    screen_update_high_score();
    (void)buzzer_request_cue(AUDIO_CUE_GAME_OVER);

    vTaskDelay(pdMS_TO_TICKS(1500));
    screen_display_game_over();

    /*
     * Input becomes active only after the result screen is actually visible.
     * This replaces external suspension of the button task with explicit state.
     */
    app_state_set(APP_STATE_GAME_OVER);
}

static void screen_return_to_start(void)
{
    /*
     * Keep all RTOS resources alive and reset only per-game data.  This gives
     * deterministic replay without heap churn or a disruptive MCU reset.
     */
    game_session_reset();
    app_state_set(APP_STATE_WAITING_FOR_START);
    screen_display_waiting_for_start();
}

static void task_update_screen(void *param)
{
    (void)param;

    for (;;) {
        app_state_t mode = app_state_get();
        EventBits_t bits = xEventGroupWaitBits(screen_event_state,
                                               SCREEN_ALL_BITS,
                                               pdTRUE,
                                               pdFALSE,
                                               screen_wait_timeout(mode));

        if (bits == 0U) {
            /*
             * A timeout has a different meaning in each non-game state:
             * 10 seconds enters the saver; 40 ms advances one animation frame.
             */
            mode = app_state_get();
            if (mode == APP_STATE_WAITING_FOR_START) {
                screen_enter_saver();
            } else if (mode == APP_STATE_SCREENSAVER) {
                app_settings_t settings = app_settings_get();
                uint32_t elapsed_ms =
                    (uint32_t)((xTaskGetTickCount() -
                                screen_saver_started_at) *
                               portTICK_PERIOD_MS);

                if (elapsed_ms >=
                    ((uint32_t)settings.sleep_timeout_s * 1000U)) {
                    screen_enter_display_sleep();
                } else {
                    saver_engine_step(&saver_engine);
                    screen_render_saver_frame();
                    screen_schedule_next_saver_frame();
                }
            }
            continue;
        }

        if ((bits & SCREEN_GAME_OVER_BIT) != 0U) {
            screen_finish_game();
        } else if ((bits & SCREEN_RETURN_TO_START_BIT) != 0U) {
            screen_return_to_start();
        } else if ((bits & SCREEN_GAME_START_BIT) != 0U) {
            screen_start_game();
        } else if ((bits & SCREEN_PAUSE_BIT) != 0U) {
            screen_pause_game();
        } else if ((bits & SCREEN_RESUME_BIT) != 0U) {
            screen_resume_game();
        } else if ((bits & SCREEN_WAKE_BIT) != 0U) {
            screen_wake_display();
        } else if ((bits & SCREEN_SETTINGS_ENTER_BIT) != 0U) {
            screen_enter_settings();
        } else if ((bits & SCREEN_SETTINGS_EXIT_BIT) != 0U) {
            screen_exit_settings();
        } else if ((bits & SCREEN_SAVER_EXIT_BIT) != 0U) {
            screen_leave_saver();
        } else if (app_state_get() == APP_STATE_SETTINGS) {
            if ((bits & SCREEN_SETTINGS_DIFFICULTY_BIT) != 0U) {
                app_settings_cycle_difficulty(1);
            }
            if ((bits & SCREEN_SETTINGS_EFFECT_BIT) != 0U) {
                app_settings_cycle_saver_effect(1, SAVER_EFFECT_COUNT);
            }
            if ((bits & SCREEN_SETTINGS_SOUND_BIT) != 0U) {
                app_settings_toggle_sound();
            }
            screen_display_settings();
        } else if (app_state_get() == APP_STATE_SCREENSAVER) {
            bool saver_changed = false;

            if ((bits & SCREEN_SAVER_ADD_BIT) != 0U) {
                saver_changed = saver_engine_add(&saver_engine) ||
                                saver_changed;
            }

            if ((bits & SCREEN_SAVER_REMOVE_BIT) != 0U) {
                saver_changed = saver_engine_remove(&saver_engine) ||
                                saver_changed;
            }

            if ((bits & SCREEN_SAVER_NEXT_BIT) != 0U) {
                saver_engine_next(&saver_engine);
                saver_changed = true;
            }

            if ((bits & SCREEN_SAVER_PREVIOUS_BIT) != 0U) {
                saver_engine_previous(&saver_engine);
                saver_changed = true;
            }

            /*
             * At min/max count the command is intentionally a no-op.  Otherwise
             * redraw immediately so UP/DOWN feedback does not wait 40 ms.
             */
            if (saver_changed) {
                screen_render_saver_frame();
            }
        } else if (((bits & SCREEN_UPDATE_BIT) != 0U) &&
                   (app_state_get() == APP_STATE_RUNNING)) {
            screen_render_game_frame();
        }
    }
}

void screen_task_create(void)
{
    BaseType_t status;

    /* Create every dependency before exposing the consumer task. */
    screen_event_state = xEventGroupCreate();
    configASSERT(screen_event_state != NULL);

    timer_update_screen_handle = xTimerCreate("game cadence",
                                               pdMS_TO_TICKS(100U),
                                               pdTRUE,
                                               NULL,
                                               screen_update_tick_callback);
    configASSERT(timer_update_screen_handle != NULL);

    status = xTaskCreate(task_update_screen, "screen owner", 768U, NULL,
                         TASK_UPDATE_SCREEN_PRIORITY,
                         &task_update_screen_handle);
    configASSERT(status == pdPASS);

    /*
     * No gameplay timer is started here.  The splash remains stable until the
     * button task publishes SCREEN_GAME_START_BIT.
     */
}

void task_screen_begin(void)
{
    screen_init();

    /*
     * main() is the sole execution context before vTaskStartScheduler(), so the
     * boot splash can be sent directly.  The audio queues and owner task already
     * exist; the queued start melody runs as soon as scheduling begins.
     */
    screen_display_waiting_for_start();
}

void screen_request_frame(void)
{
    if (app_state_get() == APP_STATE_RUNNING) {
        xEventGroupSetBits(screen_event_state, SCREEN_UPDATE_BIT);
    }
}

void screen_request_game_start(void)
{
    if (app_state_transition(APP_STATE_WAITING_FOR_START,
                             APP_STATE_START_PENDING)) {
        xEventGroupSetBits(screen_event_state, SCREEN_GAME_START_BIT);
    }
}

void screen_request_game_over(void)
{
    /*
     * Change state before publishing so every object worker rejects stale
     * updates even if the screen owner runs slightly later.
     */
    if (app_state_transition(APP_STATE_RUNNING,
                             APP_STATE_GAME_OVER_TRANSITION)) {
        xEventGroupSetBits(screen_event_state, SCREEN_GAME_OVER_BIT);
    }
}

void screen_request_return_to_start(void)
{
    if (app_state_transition(APP_STATE_GAME_OVER,
                             APP_STATE_RETURN_TO_START_PENDING)) {
        xEventGroupSetBits(screen_event_state, SCREEN_RETURN_TO_START_BIT);
    }
}

void screen_request_saver_exit(void)
{
    if (app_state_get() == APP_STATE_SCREENSAVER) {
        xEventGroupSetBits(screen_event_state, SCREEN_SAVER_EXIT_BIT);
    }
}

void screen_request_saver_add_circle(void)
{
    if (app_state_get() == APP_STATE_SCREENSAVER) {
        xEventGroupSetBits(screen_event_state, SCREEN_SAVER_ADD_BIT);
    }
}

void screen_request_saver_remove_circle(void)
{
    if (app_state_get() == APP_STATE_SCREENSAVER) {
        xEventGroupSetBits(screen_event_state, SCREEN_SAVER_REMOVE_BIT);
    }
}

void screen_request_saver_next_effect(void)
{
    if (app_state_get() == APP_STATE_SCREENSAVER) {
        xEventGroupSetBits(screen_event_state, SCREEN_SAVER_NEXT_BIT);
    }
}

void screen_request_saver_previous_effect(void)
{
    if (app_state_get() == APP_STATE_SCREENSAVER) {
        xEventGroupSetBits(screen_event_state, SCREEN_SAVER_PREVIOUS_BIT);
    }
}

void screen_request_pause(void)
{
    if (app_state_transition(APP_STATE_RUNNING, APP_STATE_PAUSED)) {
        xEventGroupSetBits(screen_event_state, SCREEN_PAUSE_BIT);
    }
}

void screen_request_resume(void)
{
    if (app_state_get() == APP_STATE_PAUSED) {
        xEventGroupSetBits(screen_event_state, SCREEN_RESUME_BIT);
    }
}

void screen_request_settings_enter(void)
{
    if (app_state_transition(APP_STATE_WAITING_FOR_START,
                             APP_STATE_SETTINGS)) {
        xEventGroupSetBits(screen_event_state, SCREEN_SETTINGS_ENTER_BIT);
    }
}

void screen_request_settings_exit(void)
{
    if (app_state_get() == APP_STATE_SETTINGS) {
        xEventGroupSetBits(screen_event_state, SCREEN_SETTINGS_EXIT_BIT);
    }
}

void screen_request_settings_difficulty(void)
{
    if (app_state_get() == APP_STATE_SETTINGS) {
        xEventGroupSetBits(screen_event_state,
                           SCREEN_SETTINGS_DIFFICULTY_BIT);
    }
}

void screen_request_settings_effect(void)
{
    if (app_state_get() == APP_STATE_SETTINGS) {
        xEventGroupSetBits(screen_event_state, SCREEN_SETTINGS_EFFECT_BIT);
    }
}

void screen_request_settings_sound(void)
{
    if (app_state_get() == APP_STATE_SETTINGS) {
        xEventGroupSetBits(screen_event_state, SCREEN_SETTINGS_SOUND_BIT);
    }
}

void screen_request_wake(void)
{
    /*
     * Keep DISPLAY_SLEEP until the screen owner has physically enabled OLED.
     * Additional queued input is therefore swallowed as wake intent instead of
     * starting a hidden game while the panel is still off.
     */
    if (app_state_get() == APP_STATE_DISPLAY_SLEEP) {
        xEventGroupSetBits(screen_event_state, SCREEN_WAKE_BIT);
    }
}
