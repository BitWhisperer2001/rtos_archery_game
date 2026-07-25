#include "sound.h"

#include "pitches.h"

#define ARRAY_LENGTH(array_) ((uint16_t)(sizeof(array_) / sizeof((array_)[0])))

/* Resource data lives in one translation unit, preventing duplicate symbols. */
static const Tone_TypeDef arrow_shot_tones[] = {
    {2000U, 8}
};

static const Tone_TypeDef no_arrow_tones[] = {
    {3000U, 2},
    {REST, 10},
    {1000U, 4},
    {REST, 10},
    {3000U, 2}
};

static const Tone_TypeDef meteoroid_destroy_tones[] = {
    {3000U, 3},
    {REST, 10},
    {4500U, 6}
};

static const Tone_TypeDef game_start_tones[] = {
    {2000, 7},
	{REST, 7},
	{3000, 7},
	{REST, 7},
	{4000, 7},
	{REST, 7},
	{1200, 8},
	{REST, 10},
	{4500, 10},
};

/*
 * Keep a separate game-over object even though it currently shares the melody.
 * The semantic name lets the sound evolve without changing buzzer-task logic.
 */
static const Tone_TypeDef game_over_tones[] = {
    {3000U, 3},
    {REST, 10},
    {1000U, 3},
    {REST, 10},
    {3000U, 3}
};

const tone_sequence_t SOUND_ARROW_SHOT = {
    arrow_shot_tones,
    ARRAY_LENGTH(arrow_shot_tones)
};

const tone_sequence_t SOUND_NO_ARROW = {
    no_arrow_tones,
    ARRAY_LENGTH(no_arrow_tones)
};

const tone_sequence_t SOUND_METEOROID_DESTROY = {
    meteoroid_destroy_tones,
    ARRAY_LENGTH(meteoroid_destroy_tones)
};

const tone_sequence_t SOUND_GAME_START = {
    game_start_tones,
    ARRAY_LENGTH(game_start_tones)
};

const tone_sequence_t SOUND_GAME_OVER = {
    game_over_tones,
    ARRAY_LENGTH(game_over_tones)
};
