#include <stdbool.h>
#include <stdint.h>

#include "buzzer_task.h"

#include "app_settings.h"
#include "buzzer.h"
#include "sound.h"
#include "system_init.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define AUDIO_HIGH_QUEUE_LENGTH      (4U)
#define AUDIO_NORMAL_QUEUE_LENGTH    (8U)
#define AUDIO_POLL_SLICE_MS          (10U)

typedef enum {
    AUDIO_PRIORITY_GAMEPLAY = 0,
    AUDIO_PRIORITY_LIFECYCLE
} audio_priority_t;

typedef struct {
    const tone_sequence_t *sequence;
    uint16_t bpm;
    audio_priority_t priority;
} audio_command_t;

static TaskHandle_t task_buzzer_handle = NULL;
static QueueHandle_t audio_high_queue = NULL;
static QueueHandle_t audio_normal_queue = NULL;

static bool audio_command_from_cue(audio_cue_t cue, audio_command_t *command)
{
    switch (cue) {
        case AUDIO_CUE_ARROW_SHOT:
            command->sequence = &SOUND_ARROW_SHOT;
            command->bpm = 1100U;
            command->priority = AUDIO_PRIORITY_GAMEPLAY;
            return true;

        case AUDIO_CUE_NO_ARROW:
            command->sequence = &SOUND_NO_ARROW;
            command->bpm = 800U;
            command->priority = AUDIO_PRIORITY_GAMEPLAY;
            return true;

        case AUDIO_CUE_METEOR_DESTROYED:
            command->sequence = &SOUND_METEOROID_DESTROY;
            command->bpm = 950U;
            command->priority = AUDIO_PRIORITY_GAMEPLAY;
            return true;

        case AUDIO_CUE_GAME_OVER:
            command->sequence = &SOUND_GAME_OVER;
            command->bpm = 850U;
            command->priority = AUDIO_PRIORITY_LIFECYCLE;
            return true;

        case AUDIO_CUE_GAME_START:
            command->sequence = &SOUND_GAME_START;
            command->bpm = 800U;
            command->priority = AUDIO_PRIORITY_LIFECYCLE;
            return true;

        default:
            return false;
    }
}

static bool buzzer_wait_interruptible(uint32_t duration_ms,
                                      audio_priority_t current_priority,
                                      audio_command_t *preempting)
{
    uint32_t remaining = duration_ms;

    while (remaining > 0U) {
        /*
         * Lifecycle sounds can preempt gameplay effects within at most 10 ms.
         * Equal-priority commands remain FIFO and play after the current cue.
         */
        if ((current_priority == AUDIO_PRIORITY_GAMEPLAY) &&
            (xQueueReceive(audio_high_queue, preempting, 0U) == pdPASS)) {
            buzzer_stop();
            return true;
        }

        uint32_t slice =
            (remaining > AUDIO_POLL_SLICE_MS) ? AUDIO_POLL_SLICE_MS :
                                                remaining;
        vTaskDelay(pdMS_TO_TICKS(slice));
        remaining -= slice;
    }

    return false;
}

static bool buzzer_play_command(const audio_command_t *command,
                                audio_command_t *preempting)
{
    for (uint16_t i = 0U; i < command->sequence->length; i++) {
        uint32_t duration =
            buzzer_play_tones(command->sequence->tones[i], command->bpm);
        uint32_t active_ms = (duration * 9U) / 10U;
        uint32_t quiet_ms = duration - active_ms;

        if (buzzer_wait_interruptible(active_ms, command->priority,
                                      preempting)) {
            return true;
        }

        buzzer_stop();
        if (buzzer_wait_interruptible(quiet_ms, command->priority,
                                      preempting)) {
            return true;
        }
    }

    return false;
}

static bool buzzer_receive_next(audio_command_t *command)
{
    for (;;) {
        /*
         * Queue sets do not provide member priority and become inconsistent
         * when a member is drained directly.  A task notification is used only
         * as the wake-up doorbell, while these two queues remain the source of
         * truth and preserve strict lifecycle-before-gameplay selection.
         */
        if (xQueueReceive(audio_high_queue, command, 0U) == pdPASS) {
            return true;
        }
        if (xQueueReceive(audio_normal_queue, command, 0U) == pdPASS) {
            return true;
        }

        /*
         * Clear accumulated doorbells before checking the queues again.  If a
         * producer races with this call, FreeRTOS keeps its notification
         * pending and the task cannot lose the wake-up.
         */
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

static void task_buzzer_handler(void *param)
{
    audio_command_t current;

    (void)param;

    for (;;) {
        if (!buzzer_receive_next(&current)) {
            continue;
        }

        /*
         * A preempting lifecycle cue becomes current immediately.  No dynamic
         * allocation or recursive playback is needed.
         */
        audio_command_t preempting;
        while (buzzer_play_command(&current, &preempting)) {
            current = preempting;
        }
    }
}

bool buzzer_request_cue(audio_cue_t cue)
{
    audio_command_t command;
    app_settings_t settings = app_settings_get();

    if ((!settings.sound_enabled) ||
        (!audio_command_from_cue(cue, &command))) {
        return false;
    }

    QueueHandle_t destination =
        (command.priority == AUDIO_PRIORITY_LIFECYCLE)
            ? audio_high_queue
            : audio_normal_queue;

    if ((destination == NULL) || (task_buzzer_handle == NULL) ||
        (xQueueSendToBack(destination, &command, 0U) != pdPASS)) {
        return false;
    }

    /* Notify only after enqueue succeeds so every doorbell represents work. */
    xTaskNotifyGive(task_buzzer_handle);
    return true;
}

void buzzer_flush_gameplay_cues(void)
{
    if (audio_normal_queue != NULL) {
        audio_command_t discarded;

        /*
         * Session reset discards queued effects from the old game.  A cue
         * already playing will be preempted by the high-priority game-over cue.
         * Drain through the queue API so queue-set readiness tokens stay valid.
         */
        while (xQueueReceive(audio_normal_queue, &discarded, 0U) == pdPASS) {
            /* Intentionally empty. */
        }
    }
}

void buzzer_task_create(void)
{
    BaseType_t status;

    /* The BSP establishes a silent, glitch-free TIM3 state before scheduling. */
    buzzer_start();

    audio_high_queue =
        xQueueCreate(AUDIO_HIGH_QUEUE_LENGTH, sizeof(audio_command_t));
    audio_normal_queue =
        xQueueCreate(AUDIO_NORMAL_QUEUE_LENGTH, sizeof(audio_command_t));
    configASSERT(audio_high_queue != NULL);
    configASSERT(audio_normal_queue != NULL);

    status = xTaskCreate(task_buzzer_handler, "audio owner", 512U, NULL,
                         TASK_BUZZER_PRIORITY, &task_buzzer_handle);
    configASSERT(status == pdPASS);
}
