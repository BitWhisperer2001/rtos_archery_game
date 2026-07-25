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

#define ALL_BIT \
    (ARROW_SHOT_BIT | ARCHERY_NO_ARROW_BIT | METEOROID_DESTROY | \
     MUSIC_BACKGROUND | GAME_OVER_SOUND_BIT | GAME_START_SOUND_BIT)

static void buzzer_delay_runtime(uint32_t duration)
{
    /* Integer ratios avoid pulling double-precision helpers into firmware. */
    vTaskDelay(pdMS_TO_TICKS((duration * 9U) / 10U));
    buzzer_stop();
    vTaskDelay(pdMS_TO_TICKS(duration / 10U));
}

static void buzzer_play_sequence_runtime(const tone_sequence_t *sequence,
                                         uint16_t bpm)
{
    for (uint16_t i = 0U; i < sequence->length; i++) {
        uint32_t duration = buzzer_play_tones(sequence->tones[i], bpm);
        buzzer_delay_runtime(duration);
    }
}

static void task_buzzer_handler(void *param)
{
    unused(param);
    EventBits_t uBits;
    while(true){
        uBits = xEventGroupWaitBits(buzzer_event_state, ALL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        if(uBits & GAME_OVER_SOUND_BIT){
            buzzer_play_sequence_runtime(&SOUND_GAME_OVER, 850U);
        }

        else if(uBits & GAME_START_SOUND_BIT){
            /*
             * The screen owner publishes this only after the start prompt has
             * reached the OLED. Playback remains in task context and never
             * blocks rendering, boot initialization or button sampling.
             */
            buzzer_play_sequence_runtime(&SOUND_GAME_START, 800U);
        }

        else if(uBits & ARROW_SHOT_BIT){
            buzzer_play_sequence_runtime(&SOUND_ARROW_SHOT, 1100U);
        }

        else if(uBits & ARCHERY_NO_ARROW_BIT){
            buzzer_play_sequence_runtime(&SOUND_NO_ARROW, 800U);
        }

        else if(uBits & METEOROID_DESTROY){
            buzzer_play_sequence_runtime(&SOUND_METEOROID_DESTROY, 950U);
        }
    }
}

void buzzer_task_create(void)
{
    BaseType_t status;

    /*
     * Configure PWM once during boot.  All later sounds only update timer
     * registers from the dedicated buzzer owner task.
     */
    buzzer_start();

    /*
     * Publish the task's synchronization object before its task handle. This
     * remains race-free even if module creation later moves after scheduler start.
     */
    buzzer_event_state = xEventGroupCreate();
    configASSERT(buzzer_event_state != NULL);

    status = xTaskCreate(task_buzzer_handler, "task play buzzer", 512, NULL, TASK_BUZZER_PRIORITY, &task_buzzer_handle);
    configASSERT(status == pdPASS);
}
