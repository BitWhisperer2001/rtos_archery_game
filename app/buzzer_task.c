#include <stdint.h>
#include <stdbool.h>
#include "system_init.h"
#include "buzzer.h"
#include "buzzer_task.h"
#include "pitches.h"
#include "sound.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "cmsis_gcc.h"

TaskHandle_t task_buzzer_handle = NULL;
EventGroupHandle_t buzzer_event_state = NULL;

#define ALL_BIT                 (ARROW_SHOT_BIT | ARCHERY_NO_ARROW_BIT | METEOROID_DESTROY | MUSIC_BACKGROUND)

static void buzzer_play_tone_cc(void)
{
    for (uint16_t i = 0; i < 2; i++) {
        uint32_t duration = buzzer_play_tones(tones_cc[i], 1100);
        vTaskDelay(pdMS_TO_TICKS(duration * 0.9));
        buzzer_stop();
        vTaskDelay(pdMS_TO_TICKS(duration * 0.1));
    }
}

static void buzzer_play_tone_3bip(void)
{
    for (uint16_t i = 0; i < 6; i++) {
        uint32_t duration = buzzer_play_tones(tones_3beep[i], 800);
        vTaskDelay(pdMS_TO_TICKS(duration * 0.9));
        buzzer_stop();
        vTaskDelay(pdMS_TO_TICKS(duration * 0.1));
    }
}

void buzzer_play_tone_game_over(void)
{
    for (uint16_t i = 0; i < 6; i++) {
        uint32_t duration = buzzer_play_tones(tones_3beep[i], 850);
        sys_delay(duration * 0.9);
        buzzer_stop();
        sys_delay(duration * 0.1);
    }
}

void buzzer_play_tone_game_begin(void)
{
    buzzer_start();
    sys_delay(10);
    for (uint16_t i = 0; i < 11; i++) {
        uint32_t duration = buzzer_play_tones(tones_startup[i], 800);
        sys_delay(duration * 0.9);
        buzzer_stop();
        sys_delay(duration * 0.1);
    }
}

static void buzzer_play_tone_BUM(void)
{
    for (uint16_t i = 0; i < 3; i++) {
        uint32_t duration = buzzer_play_tones(tones_BUM[i], 950);
        vTaskDelay(pdMS_TO_TICKS(duration * 0.9));
        buzzer_stop();
        vTaskDelay(pdMS_TO_TICKS(duration * 0.1));
    }
}

static void task_buzzer_handler(void *param)
{
    unused(param);
    EventBits_t uBits;
    while(true){
        uBits = xEventGroupWaitBits(buzzer_event_state, ALL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        if(uBits & ARROW_SHOT_BIT){
            buzzer_play_tone_cc();
        }
        else if(uBits & ARCHERY_NO_ARROW_BIT){
            buzzer_play_tone_3bip();
        }
        else{   // if(uBits & METEOROID_DESTROY)
            buzzer_play_tone_BUM();
        }
    }
}

void buzzer_task_create(void)
{
    BaseType_t status;
    status = xTaskCreate(task_buzzer_handler, "task play buzzer", 512, NULL, TASK_BUZZER_PRIORITY, &task_buzzer_handle);
    if(status == pdFAIL){
        __disable_irq();
        while(1);
    }

    buzzer_event_state = xEventGroupCreate();
    if(buzzer_event_state == NULL){
        __disable_irq();
        while(1);
    }
}
