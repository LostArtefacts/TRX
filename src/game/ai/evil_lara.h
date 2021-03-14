#ifndef T1M_GAME_AI_EVIL_LARA_H
#define T1M_GAME_AI_EVIL_LARA_H

#include "game/types.h"

#include <stdint.h>

void SetupEvilLara(OBJECT_INFO *obj);
void InitialiseEvilLara(int16_t item_num);
void ControlEvilLara(int16_t item_num);
void DrawEvilLara(ITEM_INFO *item);

#endif
