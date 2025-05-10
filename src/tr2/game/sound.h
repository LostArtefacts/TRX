#pragma once

#include "global/types.h"

#include <libtrx/game/sound.h>

#define SOUND_DEFAULT_PITCH 0x10000

void Sound_Init(void);
void Sound_Shutdown(void);

void Sound_UpdateEffects(void);
void Sound_EndScene(void);
