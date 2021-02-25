#ifndef T1M_GAME_CONTROL_H
#define T1M_GAME_CONTROL_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define CheckCheatMode          ((void         (*)())0x00438920)
#define GetCeiling              ((int16_t      (*)(FLOOR_INFO* floor, int32_t x, int32_t y, int32_t z))0x00414880)
#define TestTriggers            ((void         (*)(int16_t* data, int heavy))0x00414080)
#define LOS                     ((int32_t      (*)(GAME_VECTOR* start, GAME_VECTOR* target))0x00414B30)
// clang-format on

int32_t ControlPhase(int32_t nframes, int demo_mode);
void AnimateItem(ITEM_INFO* item);
int32_t GetChange(ITEM_INFO* item, ANIM_STRUCT* anim);
void TranslateItem(ITEM_INFO* item, int32_t x, int32_t y, int32_t z);
FLOOR_INFO* GetFloor(int32_t x, int32_t y, int32_t z, int16_t* room_num);
int16_t GetWaterHeight(int32_t x, int32_t y, int32_t z, int16_t room_num);
int16_t GetHeight(FLOOR_INFO* floor, int32_t x, int32_t y, int32_t z);
int16_t GetDoor(FLOOR_INFO* floor);

void T1MInjectGameControl();

#endif
