#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "system_init.h"
#include "system_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "ssd1306.h"
#include "border.h"
#include "bitmap.h"
#include "screen_task.h"

#include "cmsis_gcc.h"

game_border_t game_border;
TaskHandle_t task_border_update_handle = NULL;
SemaphoreHandle_t semphr_task_update_border = NULL;
char score[4] = "0000";

static void task_update_border_handler(void *param)
{
    unused(param);
    while(true){
        if(xSemaphoreTake(semphr_task_update_border, portMAX_DELAY) == pdTRUE){
            if(game_border.arrow_num != game_border.last_arrow_num){
                switch(game_border.arrow_num){
                    case 0:
                        portENTER_CRITICAL();
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, !game_border.visible);
                        game_border.last_arrow_num_img = bitmap_arrow_0;
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, game_border.visible);
                        portEXIT_CRITICAL();
                        break;
                    case 1:
                        portENTER_CRITICAL();
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, !game_border.visible);
                        game_border.last_arrow_num_img = bitmap_arrow_1;
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, game_border.visible);
                        portEXIT_CRITICAL();
                        break;
                    case 2:
                        portENTER_CRITICAL();
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, !game_border.visible);
                        game_border.last_arrow_num_img = bitmap_arrow_2;
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, game_border.visible);
                        portEXIT_CRITICAL();
                        break;
                    case 3:
                        portENTER_CRITICAL();
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, !game_border.visible);
                        game_border.last_arrow_num_img = bitmap_arrow_3;
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, game_border.visible);
                        portEXIT_CRITICAL();
                        break;
                    case 4:
                        portENTER_CRITICAL();
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, !game_border.visible);
                        game_border.last_arrow_num_img = bitmap_arrow_4;
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, game_border.visible);
                        portEXIT_CRITICAL();
                        break;
                    case 5:
                        portENTER_CRITICAL();
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, !game_border.visible);
                        game_border.last_arrow_num_img = bitmap_arrow_5;
                        SSD1306_DrawBitmap(ARROW_NUM_X, ARROW_NUM_Y, game_border.last_arrow_num_img, ARROW_NUM_WIDTH, ARROW_NUM_HEIGH, game_border.visible);
                        portEXIT_CRITICAL();
                        break;
                    default:
                        break;
                }
                game_border.last_arrow_num = game_border.arrow_num;
            }
            portENTER_CRITICAL();
            /* Update game score */
            if(game_border.last_game_score != game_border.game_score){
                SSD1306_DrawFilledRectangle(37, 55, 7, 40, SSD1306_COLOR_BLACK);
                sprintf(score, "%d", game_border.game_score);
                SSD1306_GotoXY(37, 55);
                SSD1306_Puts(score, &Font_7x10, 1);
                game_border.last_game_score = game_border.game_score;
            }
            
            SSD1306_DrawBitmap(game_border.x_border, game_border.y_border, game_border.action_img, BORDER_WIDTH, BORDER_HEIGH, game_border.visible);
            portEXIT_CRITICAL();
            xEventGroupSetBits(screen_event_state, SCREEN_UPDATE_BIT);
        }
    }
}

void border_task_create(void)
{   
    SSD1306_GotoXY(37, 54);
    SSD1306_Puts(score, &Font_7x10, 1);
    BaseType_t status;
    game_border.action_img = bitmap_border;
    game_border.last_arrow_num_img = bitmap_arrow_5;
    game_border.visible = true;
    game_border.x_border = BORDER_X;
    game_border.y_border = BORDER_Y;
    game_border.game_score = 0;
    game_border.arrow_num = 5;
    game_border.last_game_score = game_border.game_score;
    game_border.highest_score = 0;

    semphr_task_update_border = xSemaphoreCreateBinary();
    if(semphr_task_update_border == NULL){
        __disable_irq();
        while(true);
    }
    status = xTaskCreate(task_update_border_handler, "task game_border", 512, NULL, BORDER_TASK_PRIORITY, &task_border_update_handle);
    if(status != pdPASS){
        __disable_irq();
        while(true);
    }

}

