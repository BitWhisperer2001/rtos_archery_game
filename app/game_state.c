#include "game_state.h"

#include "FreeRTOS.h"
#include "semphr.h"

static SemaphoreHandle_t game_state_mutex = NULL;

void game_state_init(void)
{
    /*
     * A real mutex provides priority inheritance.  A binary semaphore would
     * protect the data but could introduce unbounded priority inversion.
     */
    game_state_mutex = xSemaphoreCreateMutex();
    configASSERT(game_state_mutex != NULL);
}

void game_state_lock(void)
{
    /*
     * Only normal tasks call this API; timer callbacks and ISRs publish work
     * instead.  Therefore a blocking mutex take is valid and keeps callers
     * simple.
     */
    configASSERT(game_state_mutex != NULL);
    configASSERT(xSemaphoreTake(game_state_mutex, portMAX_DELAY) == pdTRUE);
}

void game_state_unlock(void)
{
    configASSERT(game_state_mutex != NULL);
    configASSERT(xSemaphoreGive(game_state_mutex) == pdTRUE);
}
