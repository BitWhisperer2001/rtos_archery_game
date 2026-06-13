#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "system_init.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"
#include "ssd1306.h"
#include "meteoroid.h"
#include "border.h"
#include "screen_task.h"
#include "buzzer_task.h"
#include "event_groups.h"
#include "bitmap.h"

#include "cmsis_gcc.h"

volatile game_meteoroid_t game_meteoroid[METEROID_MAX_NUM];
volatile TimerHandle_t meteoroid_timer = NULL;
static volatile uint8_t meteoroid_index = 0;
static volatile uint32_t timeToRandMeteoroid = 0;
static uint32_t raw = 0;
bool isGameOver = false;
uint8_t meteoroid_timer_period = 120;

void meteoroid_timer_callback(TimerHandle_t xTimer)
{
    unused(xTimer);
    if((sys_get_tick() - timeToRandMeteoroid) >= 300){
        meteoroid_index = (rand() % METEROID_MAX_NUM);
        if(game_meteoroid[meteoroid_index].visible != true){
            game_meteoroid[meteoroid_index].visible = true;
            game_meteoroid[meteoroid_index].x = METEOROID_X_START;
            game_meteoroid[meteoroid_index].state = 0;
        }
        timeToRandMeteoroid = sys_get_tick();
    }

    for(uint8_t i = 0; i < METEROID_MAX_NUM; i++){
        if(game_meteoroid[i].visible == true){
            portENTER_CRITICAL();
            SSD1306_DrawBitmap(game_meteoroid[i].x, game_meteoroid[i].y, game_meteoroid[i].action_img, METEOROID_WIDTH, METEOROID_HEIGH, !game_meteoroid[i].visible);  // clear old img
            game_meteoroid[i].x -= METEOROID_RUN_STEP;

            if(game_meteoroid[i].x < METEOROID_X_THRESHOLD){
                buzzer_play_tone_game_over();
                portENTER_CRITICAL();
                raw = sys_read_score_in_flash(ADD_TO_SAVE_SCORE);
                if(raw == 0xFFFFFFFF){
                    game_border.highest_score = 0;
                }
                else{
                    game_border.highest_score = raw;
                }
                if(game_border.game_score > game_border.highest_score){
                    sys_save_score_into_flash(ADD_TO_SAVE_SCORE, (uint32_t)game_border.game_score);
                    game_border.highest_score = game_border.game_score;
                }
                portEXIT_CRITICAL();
                xEventGroupSetBits(screen_event_state, SCREEN_GAME_OVER_BIT);
                isGameOver = true;
            }

            switch(game_meteoroid[i].state){
                case 0:
                    game_meteoroid[i].action_img = bitmap_meteoroid_I;
                    game_meteoroid[i].state = 1;
                    break;
                case 1:
                    game_meteoroid[i].action_img = bitmap_meteoroid_II;
                    game_meteoroid[i].state = 2;
                    break;
                case 2:
                    game_meteoroid[i].action_img = bitmap_meteoroid_III;
                    game_meteoroid[i].state = 0;
                    break;
                default:
                    break;
            }
            SSD1306_DrawBitmap(game_meteoroid[i].x, game_meteoroid[i].y, game_meteoroid[i].action_img, METEOROID_WIDTH, METEOROID_HEIGH, game_meteoroid[i].visible);
            portEXIT_CRITICAL();
        }
    }
    xEventGroupSetBits(screen_event_state, SCREEN_UPDATE_BIT);
}

void meteoroid_task_create(void)
{
    for(uint8_t i = 0; i < METEROID_MAX_NUM; i++){
        game_meteoroid[i].action_img = bitmap_meteoroid_I;
        game_meteoroid[i].x = METEOROID_X_START;
        if(i == 0){
            game_meteoroid[i].y = METEOROID_Y_START;
        }
        else{
            game_meteoroid[i].y = game_meteoroid[i-1].y + METEOROID_Y_OFFSET;
        }
        game_meteoroid[i].visible = false;
        game_meteoroid[i].state = 0;
    }
    meteoroid_timer = xTimerCreate("timer update meteoroid", pdMS_TO_TICKS(meteoroid_timer_period), pdTRUE, NULL, meteoroid_timer_callback);
    if(meteoroid_timer == NULL){
        __disable_irq();
        while(true);
    }
    srand(sys_get_tick());
    xTimerStart(meteoroid_timer, pdMS_TO_TICKS(500));
}






