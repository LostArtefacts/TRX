#ifndef T1M_GAME_EFFECTS_RICOCHET_H
#define T1M_GAME_EFFECTS_RICOCHET_H

#include "game/types.h"

#include <stdint.h>

void SetupRicochet(OBJECT_INFO *obj);
void Ricochet(GAME_VECTOR *pos);
void ControlRicochet1(int16_t fx_num);

#endif
