#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "system_log.h"
#include "system_init.h"
#include "screen_task.h"
#include "button_task.h"
#include "buzzer_task.h"
#include "screen.h"
#include "ssd1306.h"
#include "archery.h"
#include "arrow.h"
#include "border.h"
#include "meteoroid.h"
#include "bang.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"
#include "event_groups.h"

#include "cmsis_gcc.h"
#include "bitmap.h"

EventGroupHandle_t screen_event_state = NULL;
TimerHandle_t timer_update_screen_handle = NULL;
TaskHandle_t task_update_screen_handle = NULL;

#define ALL_BIT                 (SCREEN_UPDATE_BIT | SCREEN_GAME_OVER_BIT)

static char your_score[4] = "0000";
static char best_score[4] = "0000";

static void update_screen_befor_display_handler(TimerHandle_t xTimer)
{
    unused(xTimer);
    xEventGroupSetBits(archery_event_state, ARCHERY_INIT_BIT); // update archery object
    // xTimerStop(timer_update_screen_handle, pdMS_TO_TICKS(0));
}

static void screen_display_game_over(void)
{
    SSD1306_Clear();
    SSD1306_DrawFilledRectangle(0, 0, 128, 64, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(18, 5);
    SSD1306_Puts("GAME OVER", &Font_11x18, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(2, 27);
    SSD1306_Puts("BEST SCORE:", &Font_7x10, SSD1306_COLOR_BLACK);
    sprintf(best_score, "%d", game_border.highest_score);
    SSD1306_GotoXY(79, 27);
    SSD1306_Puts(best_score, &Font_7x10, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(2, 39);
    SSD1306_Puts("YOUR SCORE:", &Font_7x10, SSD1306_COLOR_BLACK);
    sprintf(your_score, "%d", game_border.game_score);
    SSD1306_GotoXY(79, 39);
    SSD1306_Puts(your_score, &Font_7x10, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(9, 55);
    SSD1306_Puts("PRESS ANY BUTTON", &Font_7x10, SSD1306_COLOR_BLACK);
    SSD1306_UpdateScreen();
}

static void task_update_screen(void* param)
{
    unused(param);
    EventBits_t uxBits;
    while(true){
        uxBits = xEventGroupWaitBits(screen_event_state, ALL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        if(uxBits & SCREEN_UPDATE_BIT){
            portENTER_CRITICAL();
            SSD1306_UpdateScreen();
            portEXIT_CRITICAL();
        }
        else{
            xTimerStop(meteoroid_timer, 0);
            vTaskSuspend(task_arrow_update_handle);
            vTaskSuspend(task_archery_update_handle);
            vTaskSuspend(task_border_update_handle);
            vTaskSuspend(task_update_bang_handle);
            vTaskDelay(1300);
            screen_display_game_over();
            vTaskSuspend(NULL);
        }
    }
}

void screen_task_create(void)
{
    BaseType_t status;
    SSD1306_Clear();
    status = xTaskCreate(task_update_screen, "task update screen", 512, NULL, TASK_UPDATE_SCREEN_PRIORITY, &task_update_screen_handle);
    if(status == pdFAIL){
        __disable_irq();
        while(true);
    }
    
    screen_event_state =  xEventGroupCreate();
    if(screen_event_state == NULL){
        __disable_irq();
        while(true);
    }

    timer_update_screen_handle = xTimerCreate("timer update screen", pdMS_TO_TICKS(100), pdTRUE, NULL, update_screen_befor_display_handler);
    if(timer_update_screen_handle == NULL){
        __disable_irq();
        while(true);
    }
    xTimerStart(timer_update_screen_handle, pdMS_TO_TICKS(100));
}

void task_screen_begin(void)
{
    char cnt[2] = "00";
    uint8_t _cnt = 7;
    screen_init();
    SSD1306_GotoXY(0, 2);
    SSD1306_Puts("ARCHERY GAME", &Font_11x18, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(2, 22);
    SSD1306_Puts("Design by Minh Chi", &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
    buzzer_play_tone_game_begin();
    SSD1306_GotoXY(7, 39);
    SSD1306_Puts("Game start after", &Font_7x10, SSD1306_COLOR_WHITE);
    while(--_cnt){
        sprintf(cnt, "%d", _cnt);
        SSD1306_DrawRectangle(60, 51, 7, 10, SSD1306_COLOR_BLACK);
        SSD1306_GotoXY(60, 51);
        SSD1306_Puts(cnt, &Font_7x10, SSD1306_COLOR_WHITE);
        SSD1306_UpdateScreen();
        sys_delay(1000);
    }
}


