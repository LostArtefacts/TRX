#ifndef T1M_GAME_CONTROL_H
#define T1M_GAME_CONTROL_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define CheckCheatMode          ((void         (*)())0x00438920)
#define GetFloor                ((FLOOR_INFO*  (*)(int32_t x, int32_t y, int32_t z, int16_t* room_number))0x00413A80)
#define GetCeiling              ((int16_t      (*)(FLOOR_INFO* floor, int32_t x, int32_t y, int32_t z))0x00414880)
#define GetHeight               ((int16_t      (*)(FLOOR_INFO* floor, int32_t x, int32_t y, int32_t z))0x00413D60)
#define GetWaterHeight          ((int16_t      (*)(int32_t x, int32_t y, int32_t z, int16_t room_number))0x00413C60)
#define TestTriggers            ((void         (*)(int16_t* data, int heavy))0x00414080)
#define GetChange               ((int32_t      (*)(ITEM_INFO* item, ANIM_STRUCT* anim))0x00413960)
#define LOS                     ((int32_t      (*)(GAME_VECTOR* start, GAME_VECTOR* target))0x00414B30)
// clang-format on

int32_t ControlPhase(int32_t nframes, int demo_mode);
void AnimateItem(ITEM_INFO* item);
void TranslateItem(ITEM_INFO* item, int32_t x, int32_t y, int32_t z);

void T1MInjectGameControl();

#endif
