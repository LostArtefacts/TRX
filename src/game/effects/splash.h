#ifndef T1M_GAME_EFFECTS_SPLASH_H
#define T1M_GAME_EFFECTS_SPLASH_H

#include "game/types.h"
#include <stdint.h>

void SetupSplash(OBJECT_INFO *obj);
void Splash(ITEM_INFO *item);
void ControlSplash1(int16_t fx_num);

#endif
