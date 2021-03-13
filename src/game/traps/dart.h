#ifndef T1M_GAME_TRAPS_DART_H
#define T1M_GAME_TRAPS_DART_H

#include "game/types.h"
#include <stdint.h>

void SetupDart(OBJECT_INFO *obj);
void SetupDartEffect(OBJECT_INFO *obj);
void DartsControl(int16_t item_num);
void DartEffectControl(int16_t fx_num);

#endif
