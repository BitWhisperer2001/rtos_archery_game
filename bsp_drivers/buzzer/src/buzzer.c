#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"
#include "buzzer.h"

#define TIM3_CLOCK_HZ   (100000U)
#define WHOLENOTE_MS(bpm)  ((60000 * 4) / (bpm))

static void buzzer_apply_pwm(uint16_t auto_reload, uint16_t compare)
{
    /*
     * ARR and CCR1 both use preload.  A software update event commits them as
     * one atomic PWM configuration and resets the time base immediately.
     *
     * Without UG, the first note after power-on waits for the boot ARR
     * (49999 ticks, roughly 500 ms) to expire.  The short notes in
     * SOUND_GAME_START are then overwritten before ever reaching the output.
     */
    TIM_SetAutoreload(TIM3, auto_reload);
    TIM_SetCompare1(TIM3, compare);
    TIM_SetCounter(TIM3, 0U);
    TIM_GenerateEvent(TIM3, TIM_EventSource_Update);

    /*
     * UG also sets UIF.  This driver does not use the update interrupt, but
     * clearing the flag leaves TIM3 in a deterministic state for diagnostics.
     */
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
}

static void buzzer_io_timer3_channel1_config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Initialize every SPL field before applying the board's PWM pin profile. */
    GPIO_StructInit(&GPIO_InitStructure);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_TIM3);
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void buzzer_timer3_pwm_config(void)
{
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    TIM_OCInitTypeDef OC1_InitStructure;

    /*
     * SPL writes complete register configurations. Defaults prevent reserved or
     * unused members from inheriting indeterminate stack values.
     */
    TIM_TimeBaseStructInit(&TIM_InitStructure);
    TIM_OCStructInit(&OC1_InitStructure);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_DeInit(TIM3);
    TIM_InitStructure.TIM_Prescaler = TIM3_PRESCALER;
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStructure.TIM_Period = TIM3_PERIOD;
    TIM_InitStructure.TIM_RepetitionCounter = 0x00;   // This parameter only valid for TIM1 and TIM8
    TIM_TimeBaseInit(TIM3, &TIM_InitStructure);
    OC1_InitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    OC1_InitStructure.TIM_OutputState = TIM_OutputState_Enable;
    /*
     * Cold-start silently.  Driving 50% duty here produced an audible pop
     * before the buzzer task had selected its first real note.
     */
    OC1_InitStructure.TIM_Pulse = 0U;
    OC1_InitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM3, &OC1_InitStructure);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);

    /*
     * Commit the silent preload before enabling the counter.  The timer output
     * is therefore known-low when PA6 is later connected to the alternate
     * function, avoiding a GPIO-to-PWM transition glitch.
     */
    buzzer_apply_pwm((uint16_t)TIM3_PERIOD, 0U);
    TIM_Cmd(TIM3, ENABLE);
}

void buzzer_start(void)
{
    /*
     * Configure and silence TIM3 while PA6 is still disconnected from its
     * alternate function.  Connect the physical buzzer only after PWM state is
     * deterministic.
     */
    buzzer_timer3_pwm_config();
    buzzer_io_timer3_channel1_config();
}

uint32_t buzzer_play_tones(const Tone_TypeDef tones, uint16_t bpm)
{
    uint32_t wholenote;
    uint32_t noteDuration;

    /*
     * Invalid resource data must fail silently instead of reaching undefined
     * integer division. A zero frequency with a valid duration remains a rest.
     */
    if ((bpm == 0U) || (tones.duration == 0) ||
        (tones.duration == INT16_MIN)) {
        buzzer_stop();
        return 0U;
    }

    wholenote = WHOLENOTE_MS(bpm);
    if (tones.duration > 0) {
        noteDuration = wholenote / (uint16_t)tones.duration;
    } else {
        noteDuration = (wholenote / (uint16_t)(-tones.duration)) * 3U / 2U;
    }

    if(tones.freq == 0){
        buzzer_stop();
        return noteDuration;
    }

    /*
     * Frequencies outside the 16-bit timer's representable range become a
     * silent note instead of overflowing ARR into an unrelated pitch.
     */
    if ((TIM3_CLOCK_HZ / tones.freq) > ((uint32_t)UINT16_MAX + 1U)) {
        buzzer_stop();
        return noteDuration;
    }

    uint32_t arr = ((TIM3_CLOCK_HZ / tones.freq) - 1U);
    uint32_t ccr = ((arr + 1U) / 2U);
    buzzer_apply_pwm((uint16_t)arr, (uint16_t)ccr);
    return noteDuration;
}

void buzzer_stop(void)
{
    /*
     * Commit silence immediately instead of waiting for the next PWM overflow.
     * This also gives every note its intended 10% inter-note quiet interval.
     */
    buzzer_apply_pwm((uint16_t)TIM3->ARR, 0U);
}
