#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>
#include <stdbool.h>
#include "buzzer.h"
#include "pitches.h"

const Tone_TypeDef tones_cc[] = {
  	{2000, 8},
	{0, 0}
};

static const Tone_TypeDef tones_BUM[] = {
	{3000,3},
	{4500,6},
	{   0,0}
};

static const Tone_TypeDef tones_Lets_go[] = {
	{ 262,100},
	{ 330,100},
	{ 392,100},
	{ 349,100},
	{ 330,100},
	{ 392,100},
	{  44,100},
	{   0,200},
	{   0,200},
	{   0,0}     // <-- tones end
};

static const Tone_TypeDef tones_startup[] = {
	{2000,3},
	{2000,3},
	{   0,4},
	{3000,4},
	{   0,4},
	{4000,4},
	{   0,4},
	{1200,5},
	{  0,7},
	{4500,7},
	{   0,0}     // <-- tones end
};

static const Tone_TypeDef tones_3beep[] = {
	{3000, 2},
	{   0,10},
	{1000, 4},
	{   0,10},
	{3000, 2},
	{   0, 0}
};

const Tone_TypeDef GodFather[] = {
  {REST, 4}, {REST, 8}, {REST, 8}, {REST, 8}, {NOTE_E4, 8}, {NOTE_A4, 8}, {NOTE_C5, 8}, //1
  {NOTE_B4, 8}, {NOTE_A4, 8}, {NOTE_C5, 8}, {NOTE_A4, 8}, {NOTE_B4, 8}, {NOTE_A4, 8}, {NOTE_F4, 8}, {NOTE_G4, 8},
  {NOTE_E4, 2}, {NOTE_E4, 8}, {NOTE_A4, 8}, {NOTE_C5, 8},
  {NOTE_B4, 8}, {NOTE_A4, 8}, {NOTE_C5, 8}, {NOTE_A4, 8}, {NOTE_C5, 8}, {NOTE_A4, 8}, {NOTE_E4, 8}, {NOTE_DS4, 8},

  {NOTE_D4, 2}, {NOTE_D4, 8}, {NOTE_F4, 8}, {NOTE_GS4, 8}, //5
  {NOTE_B4, 2}, {NOTE_D4, 8}, {NOTE_F4, 8}, {NOTE_GS4, 8},
  {NOTE_A4, 2}, {NOTE_C4, 8}, {NOTE_C4, 8}, {NOTE_G4, 8},
  {NOTE_F4, 8}, {NOTE_E4, 8}, {NOTE_G4, 8}, {NOTE_F4, 8}, {NOTE_F4, 8}, {NOTE_E4, 8}, {NOTE_E4, 8}, {NOTE_GS4, 8},

  {NOTE_A4, 2}, {REST, 8}, {NOTE_A4, 8}, {NOTE_A4, 8}, {NOTE_GS4, 8}, //9
  {NOTE_G4, 2}, {NOTE_B4, 8}, {NOTE_A4, 8}, {NOTE_F4, 8},
  {NOTE_E4, 2}, {NOTE_E4, 8}, {NOTE_G4, 8}, {NOTE_E4, 8},
  {NOTE_D4, 2}, {NOTE_D4, 8}, {NOTE_D4, 8}, {NOTE_F4, 8}, {NOTE_DS4, 8},
};


#endif  // SOUND_H


