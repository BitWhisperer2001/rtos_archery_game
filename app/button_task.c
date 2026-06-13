#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "timers.h"
#include "event_groups.h"
#include "button.h"
#include "system_log.h"
#include "system_init.h"
#include "buzzer_task.h"
#include "button_task.h"
#include "archery.h"
#include "arrow.h"
#include "meteoroid.h"
#include "bitmap.h"
#include "ring_buffer.h"
#include "ssd1306.h"
#include "app.h"
#include "cmsis_gcc.h"

TimerHandle_t bt_polling_timer_handle = NULL;
TimerHandle_t bt_process_timer_handle = NULL;

static bool isBtDownPressed = false;
static bool isBtUpPressed = false;
static bool isBtOkPressed = false;
static bool isTimerProcessStarted = false;

static void button_check_handler(TimerHandle_t xTimer)
{
    unused(xTimer);
    if(button_read(BT_DOWN) == false){  // button down pressed
        isBtDownPressed = true;
        if(isTimerProcessStarted == false){
            if(xTimerStart(bt_process_timer_handle, 0) == pdPASS){
                isTimerProcessStarted = true;
            }
        }
    }
    if(button_read(BT_UP) == false){   // button up pressed
        isBtUpPressed = true;
        if(isTimerProcessStarted == false){
            if(xTimerStart(bt_process_timer_handle, 0) == pdPASS){
                isTimerProcessStarted = true;
            }
        }
    }
    if(button_read(BT_OK) == false){   // button OK pressed
        isBtOkPressed = true;
        if(isTimerProcessStarted == false){
            if(xTimerStart(bt_process_timer_handle, 0) == pdPASS){
                isTimerProcessStarted = true;
            }
        }
    }
}

static void button_process(TimerHandle_t xTimer)
{
    unused(xTimer);
    if(isBtDownPressed == true){
        if(button_read(BT_DOWN) == true){  // button down release
            isBtDownPressed = false;
            if(isGameOver == true){
                // task_delete_all();
                // game_restart();
                NVIC_SystemReset();
            }
            else{
                xEventGroupSetBits(archery_event_state, ARCHERY_DOWN_BIT);
            }
            if(xTimerStop(bt_process_timer_handle, 0) == pdPASS){
                isTimerProcessStarted = false;
            }
        }
    }
    if(isBtUpPressed == true){
        if(button_read(BT_UP) == true){  // button up release
            isBtUpPressed = false;
            if(isGameOver == true){
                // task_delete_all();
                // game_restart();
                NVIC_SystemReset();
            }
            else{
                xEventGroupSetBits(archery_event_state, ARCHERY_UP_BIT);
            }
            if(xTimerStop(bt_process_timer_handle, 0) == pdPASS){
                isTimerProcessStarted = false;
            }
        }
    }
    if(isBtOkPressed == true) {
        if(button_read(BT_OK) == true){  // button OK release
            isBtOkPressed = false;
            if(isGameOver == true){
                // task_delete_all();
                // game_restart();
                NVIC_SystemReset();
            }
            else {
                if(!ring_is_empty(&arrow_buff)){
                    arrow_num_index = (uint8_t)ring_buff_get(&arrow_buff);
                    game_arrow[arrow_num_index].x = game_archery.x;
                    game_arrow[arrow_num_index].y = game_archery.y + 5;  // offset game_archery center
                    game_arrow[arrow_num_index].visible = true;
                    xEventGroupSetBits(buzzer_event_state, ARROW_SHOT_BIT);
                }
                else{
                    xEventGroupSetBits(buzzer_event_state, ARCHERY_NO_ARROW_BIT);
                }
            }
            if(xTimerStop(bt_process_timer_handle, 0) == pdPASS){
                isTimerProcessStarted = false;
            }
        }
    }
}

void button_task_create(void)
{
    button_setup(BT_DOWN | BT_UP | BT_OK);
    bt_polling_timer_handle = xTimerCreate("button polling timer", pdMS_TO_TICKS(20), pdTRUE, NULL, button_check_handler);
    if(bt_polling_timer_handle == NULL){
        // DBG_LOG("Init timer bt_polling_timer_handle fail");
        __disable_irq();
        while(true);
    }
    bt_process_timer_handle = xTimerCreate("button process timer", pdMS_TO_TICKS(50), pdTRUE, NULL, button_process);
    if(bt_process_timer_handle == NULL){
        // DBG_LOG("Init timer bt_process_timer_handle fail");
        __disable_irq();
        while(true);
    }
    xTimerStart(bt_polling_timer_handle, pdMS_TO_TICKS(0));
}




