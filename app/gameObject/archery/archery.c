#include <stdint.h>
#include <stdbool.h>
#include "system_log.h"
#include "system_init.h"
#include "archery.h"
#include "arrow.h"
#include "screen_task.h"
#include "ring_buffer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "ssd1306.h"
#include "bitmap.h"

#include "cmsis_gcc.h"

EventGroupHandle_t archery_event_state = NULL;
TaskHandle_t task_archery_update_handle = NULL;
volatile game_archery_t game_archery;

static void archery_task_handler(void *param)
{
    unused(param);
    EventBits_t uxBits;
    while(true){
        uxBits = xEventGroupWaitBits(archery_event_state, ALL_EVENT, pdTRUE, pdFALSE, portMAX_DELAY);
        if(uxBits & ARCHERY_INIT_BIT){
            if(ring_is_empty(&arrow_buff)){
                portENTER_CRITICAL();
                SSD1306_DrawBitmap(game_archery.x, game_archery.y, game_archery.action_img, ARCHERY_WIDTH, ARCHERY_HEIGH, !game_archery.visible);  // clear old img
                game_archery.action_img = bitmap_archery_II;
                SSD1306_DrawBitmap(game_archery.x, game_archery.y, game_archery.action_img, ARCHERY_WIDTH, ARCHERY_HEIGH, game_archery.visible);
                portEXIT_CRITICAL();
            }
            else{
                portENTER_CRITICAL();
                SSD1306_DrawBitmap(game_archery.x, game_archery.y, game_archery.action_img, ARCHERY_WIDTH, ARCHERY_HEIGH, !game_archery.visible);  // clear old img
                game_archery.action_img = bitmap_archery_I;
                SSD1306_DrawBitmap(game_archery.x, game_archery.y, game_archery.action_img, ARCHERY_WIDTH, ARCHERY_HEIGH, game_archery.visible);
                portEXIT_CRITICAL();
            }
            xSemaphoreGive(semphr_task_arrow_update);
        }
        else if(uxBits & ARCHERY_DOWN_BIT){
            portENTER_CRITICAL();
            SSD1306_DrawBitmap(game_archery.x, game_archery.y, game_archery.action_img, ARCHERY_WIDTH, ARCHERY_HEIGH, !game_archery.visible); // clear old img
            portEXIT_CRITICAL();
            game_archery.y += ARCHERY_Y_STEP;
            if(game_archery.y > ARCHERY_DOWN_THRESHOLD){
                game_archery.y = ARCHERY_DOWN_THRESHOLD;
            }
            xEventGroupSetBits(archery_event_state, ARCHERY_INIT_BIT);
        }
        else{
            portENTER_CRITICAL();
            SSD1306_DrawBitmap(game_archery.x, game_archery.y, game_archery.action_img, ARCHERY_WIDTH, ARCHERY_HEIGH, !game_archery.visible); // clear old img
            portEXIT_CRITICAL();
            game_archery.y -= ARCHERY_Y_STEP;
            if(game_archery.y < ARCHERY_UP_THRESHOLD){
                game_archery.y = ARCHERY_UP_THRESHOLD;
            }
            xEventGroupSetBits(archery_event_state, ARCHERY_INIT_BIT);
        }
    }
}

void archery_task_create(void)
{
    BaseType_t status;

    game_archery.action_img = bitmap_archery_I;
    game_archery.visible = true;
    game_archery.x = ARCHERY_X;
    game_archery.y = ARCHERY_Y_START;

    archery_event_state = xEventGroupCreate();
    if(archery_event_state == NULL){
        __disable_irq();
        while(true);
    }
    xEventGroupSetBits(archery_event_state, ARCHERY_INIT_BIT);

    status = xTaskCreate(archery_task_handler, "task game_archery", 512, NULL, ARCHERY_TASK_PRIORYTY, &task_archery_update_handle);
    if(status == pdFAIL){
        __disable_irq();
        while(true);
    }
}



