#include <stdint.h>
#include <stdbool.h>
#include "screen_task.h"
#include "buzzer_task.h"
#include "system_init.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "arrow.h"
#include "archery.h"
#include "border.h"
#include "meteoroid.h"
#include "bang.h"
#include "ring_buffer.h"
#include "bitmap.h"
#include "ssd1306.h"

#include "cmsis_gcc.h"

game_arrow_t game_arrow[ARROW_MAX_NUM + 1];
ring_buff_t arrow_buff;
TaskHandle_t task_arrow_update_handle = NULL;
SemaphoreHandle_t semphr_task_arrow_update = NULL;
volatile uint8_t arrow_num_index;

static void game_check_bang(uint8_t i)
{
    switch(game_arrow[i].y){
        case 7:
            if(game_meteoroid[0].visible == true){
                if(game_arrow[i].x + 5 >=  game_meteoroid[0].x){
                    game_arrow[i].visible = false;
                    game_meteoroid[0].visible = false;
                    portENTER_CRITICAL();
                    SSD1306_DrawBitmap(game_arrow[i].x, game_arrow[i].y, game_arrow[i].action_img, ARROW_WIDTH, ARROW_HEIGHT, game_arrow[i].visible);
                    SSD1306_DrawBitmap(game_meteoroid[0].x, game_meteoroid[0].y, game_meteoroid[0].action_img, METEOROID_WIDTH, METEOROID_HEIGH, game_meteoroid[0].visible);
                    portEXIT_CRITICAL();
                    game_bang[i].x = game_arrow[i].x + 2;
                    game_bang[i].y = game_arrow[i].y - 5;
                    game_bang[i].visible = true;
                    ring_buff_post(&arrow_buff, (int)i);
                }
            }
            break;
        case 16:
            if(game_meteoroid[1].visible == true){
                if(game_arrow[i].x + 5 >=  game_meteoroid[1].x){
                    game_arrow[i].visible = false;
                    game_meteoroid[1].visible = false;
                    portENTER_CRITICAL();
                    SSD1306_DrawBitmap(game_arrow[i].x, game_arrow[i].y, game_arrow[i].action_img, ARROW_WIDTH, ARROW_HEIGHT, game_arrow[i].visible);
                    SSD1306_DrawBitmap(game_meteoroid[1].x, game_meteoroid[1].y, game_meteoroid[1].action_img, METEOROID_WIDTH, METEOROID_HEIGH, game_meteoroid[1].visible);
                    portEXIT_CRITICAL();
                    game_bang[i].x = game_arrow[i].x + 2;
                    game_bang[i].y = game_arrow[i].y - 5;
                    game_bang[i].visible = true;
                    ring_buff_post(&arrow_buff, (int)i);
                }
            }
            break;
        case 25:
            if(game_meteoroid[2].visible == true){
                if(game_arrow[i].x + 5 >=  game_meteoroid[2].x){
                    game_arrow[i].visible = false;
                    game_meteoroid[2].visible = false;
                    portENTER_CRITICAL();
                    SSD1306_DrawBitmap(game_arrow[i].x, game_arrow[i].y, game_arrow[i].action_img, ARROW_WIDTH, ARROW_HEIGHT, game_arrow[i].visible);
                    SSD1306_DrawBitmap(game_meteoroid[2].x, game_meteoroid[2].y, game_meteoroid[2].action_img, METEOROID_WIDTH, METEOROID_HEIGH, game_meteoroid[2].visible);
                    portEXIT_CRITICAL();
                    game_bang[i].x = game_arrow[i].x + 2;
                    game_bang[i].y = game_arrow[i].y - 5;
                    game_bang[i].visible = true;
                    ring_buff_post(&arrow_buff, (int)i);
                }
            }
            break;
        case 34:
            if(game_meteoroid[3].visible == true){
                if(game_arrow[i].x + 5 >=  game_meteoroid[3].x){
                    game_arrow[i].visible = false;
                    game_meteoroid[3].visible = false;
                    portENTER_CRITICAL();
                    SSD1306_DrawBitmap(game_arrow[i].x, game_arrow[i].y, game_arrow[i].action_img, ARROW_WIDTH, ARROW_HEIGHT, game_arrow[i].visible);
                    SSD1306_DrawBitmap(game_meteoroid[3].x, game_meteoroid[3].y, game_meteoroid[3].action_img, METEOROID_WIDTH, METEOROID_HEIGH, game_meteoroid[3].visible);
                    portEXIT_CRITICAL();
                    game_bang[i].x = game_arrow[i].x + 2;
                    game_bang[i].y = game_arrow[i].y - 5;
                    game_bang[i].visible = true;
                    ring_buff_post(&arrow_buff, (int)i);
                }
            }
            break;
        case 43:
            if(game_meteoroid[4].visible == true){
                if(game_arrow[i].x + 5 >=  game_meteoroid[4].x){
                    game_arrow[i].visible = false;
                    game_meteoroid[4].visible = false;
                    portENTER_CRITICAL();
                    SSD1306_DrawBitmap(game_arrow[i].x, game_arrow[i].y, game_arrow[i].action_img, ARROW_WIDTH, ARROW_HEIGHT, game_arrow[i].visible);
                    SSD1306_DrawBitmap(game_meteoroid[4].x, game_meteoroid[4].y, game_meteoroid[4].action_img, METEOROID_WIDTH, METEOROID_HEIGH, game_meteoroid[4].visible);
                    portEXIT_CRITICAL();
                    game_bang[i].x = game_arrow[i].x + 2;
                    game_bang[i].y = game_arrow[i].y - 5;
                    game_bang[i].visible = true;
                    ring_buff_post(&arrow_buff, (int)i);
                }
            }
            break;
        default:
            break;
    }
}

static void task_arrow_update_handler(void *param)
{
    unused(param);
    while(true){
        if(xSemaphoreTake(semphr_task_arrow_update, portMAX_DELAY) == pdTRUE){
            for(uint8_t i = 0; i < 5; i++){
                if(game_arrow[i].visible == true){
                    portENTER_CRITICAL();
                    SSD1306_DrawBitmap(game_arrow[i].x, game_arrow[i].y, game_arrow[i].action_img, ARROW_WIDTH, ARROW_HEIGHT, !game_arrow[i].visible);   // clear old image
                    game_arrow[i].x += ARROW_STEP;
                    SSD1306_DrawBitmap(game_arrow[i].x, game_arrow[i].y, game_arrow[i].action_img, ARROW_WIDTH, ARROW_HEIGHT, game_arrow[i].visible);
                    portEXIT_CRITICAL();
                    if(game_arrow[i].x >= ARROW_X_THRESHOLD){
                        game_arrow[i].visible = false;
                        ring_buff_post(&arrow_buff, (int)i);
                    }
                    game_check_bang(i);
                }
                if(game_bang[i].visible == true){
                    xSemaphoreGive(semphr_task_bang_update);
                }
            }
            game_border.arrow_num = ring_get_element_count(&arrow_buff);
            xSemaphoreGive(semphr_task_update_border);
        }
    }
}

void arrow_task_create(void)
{
    BaseType_t status;
    for(uint8_t i = 0; i <= ARROW_MAX_NUM; i++){
       game_arrow[i].action_img = bitmap_arrow;
       game_arrow[i].visible = false;
    }

    ring_buff_init(&arrow_buff, (int)ARROW_MAX_NUM);
    for(uint8_t i = 0; i < arrow_buff.size; i++){
        ring_buff_post(&arrow_buff, (int)i);
    }

    status = xTaskCreate(task_arrow_update_handler, "task game_arrow update", 512, NULL, TASK_ARROW_UPDATE_PRIORITY, &task_arrow_update_handle);
    if(status == pdFAIL){
        __disable_irq();
        while(1);
    }

    semphr_task_arrow_update = xSemaphoreCreateBinary();
    if(semphr_task_arrow_update == NULL){
        __disable_irq();
        while(1);
    }
}
