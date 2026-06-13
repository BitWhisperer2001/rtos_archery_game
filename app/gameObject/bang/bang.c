#include "stdint.h"
#include "stdbool.h"
#include "system_init.h"
#include "bang.h"
#include "buzzer_task.h"
#include "border.h"
#include "meteoroid.h"
#include "ssd1306.h"
#include "bitmap.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"

#include "cmsis_gcc.h"

TaskHandle_t task_update_bang_handle = NULL;
SemaphoreHandle_t semphr_task_bang_update = NULL;
game_bang_t game_bang[BANG_MAX_NUM];
static uint8_t count_score = 0;


static void task_bang_update_handler(void *param)
{
    unused(param);
    while(true){
        if(xSemaphoreTake(semphr_task_bang_update, portMAX_DELAY) == pdTRUE){
            for(uint8_t i = 0; i < BANG_MAX_NUM; i++){
                if(game_bang[i].visible == true){
                    switch(game_bang[i].state){
                        case 0:
                            portENTER_CRITICAL();
                            SSD1306_DrawBitmap(game_bang[i].x, game_bang[i].y, game_bang[i].action_img, BANG_I_WIDTH, BANG_I_HEIGH, game_bang[i].visible);
                            portEXIT_CRITICAL();
                            game_bang[i].state = 1;
                            break;
                        case 1:
                            portENTER_CRITICAL();
                            SSD1306_DrawBitmap(game_bang[i].x, game_bang[i].y, game_bang[i].action_img, BANG_I_WIDTH, BANG_I_HEIGH, !game_bang[i].visible);
                            game_bang[i].action_img = bitmap_bang_II;
                            SSD1306_DrawBitmap(game_bang[i].x, game_bang[i].y, game_bang[i].action_img, BANG_II_WIDTH, BANG_II_HEIGH, game_bang[i].visible);
                            game_border.game_score += GAME_SCORE_STEP;
                            count_score++;
                            if(count_score >= 10){
                                count_score = 0;
                                meteoroid_timer_period = meteoroid_timer_period - 15;
                                xTimerChangePeriod(meteoroid_timer, pdMS_TO_TICKS(meteoroid_timer_period), 0);
                            }
                            portEXIT_CRITICAL();
                            game_bang[i].state = 2;
                            xEventGroupSetBits(buzzer_event_state, METEOROID_DESTROY);
                            break;
                        case 2:
                            portENTER_CRITICAL();
                            SSD1306_DrawBitmap(game_bang[i].x, game_bang[i].y, game_bang[i].action_img, BANG_II_WIDTH, BANG_II_HEIGH, !game_bang[i].visible);
                            game_bang[i].action_img = bitmap_bang_III;
                            SSD1306_DrawBitmap(game_bang[i].x + 2, game_bang[i].y + 3, game_bang[i].action_img, BANG_III_WIDTH, BANG_III_HEIGH, game_bang[i].visible);
                            portEXIT_CRITICAL();
                            game_bang[i].state = 3;
                            break;
                        case 3:
                            portENTER_CRITICAL();
                            SSD1306_DrawBitmap(game_bang[i].x + 2, game_bang[i].y + 3, game_bang[i].action_img, BANG_III_WIDTH, BANG_III_HEIGH, !game_bang[i].visible);
                            portEXIT_CRITICAL();
                            game_bang[i].visible = false;
                            game_bang[i].action_img = bitmap_bang_I;
                            game_bang[i].state = 0;
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }
}

void bang_task_create(void)
{
    BaseType_t status;

    for(uint8_t i = 0; i < BANG_MAX_NUM; i++){
        game_bang[i].visible = false;
        game_bang[i].action_img = bitmap_bang_I;
        game_bang[i].state = 0;
    }

    status = xTaskCreate(task_bang_update_handler, "task update bang", 512, NULL, TASK_BANG_PRIORITY, &task_update_bang_handle);
    if(status == pdFAIL){
        __disable_irq();
        while(true);
    }
    semphr_task_bang_update = xSemaphoreCreateBinary();
}
