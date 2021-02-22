#ifndef TOMB1MAIN_GAME_BOX_H
#define TOMB1MAIN_GAME_BOX_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define CreatureMood            ((void          __cdecl(*)(ITEM_INFO* item, AI_INFO* info, int32_t violent))0x0040E040)
#define CalculateTarget         ((int32_t       __cdecl(*)(PHD_VECTOR* target, ITEM_INFO* item, LOT_INFO* LOT))0x0040E850)
#define CreatureAnimation       ((int32_t       __cdecl(*)(int16_t item_num, PHD_ANGLE angle, int16_t tilt))0x0040EEE0)
#define CreatureTurn            ((PHD_ANGLE     __cdecl(*)(ITEM_INFO *item, PHD_ANGLE maximum_turn))0x0040F750)
#define CreatureHead            ((void          __cdecl(*)(ITEM_INFO* item, int16_t required))0x0040F870)
#define CreatureEffect          ((int16_t       __cdecl(*)(ITEM_INFO* item, BITE_INFO* bite, int16_t __cdecl(*generate)(int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE yrot, int16_t room_number)))0x0040F8C0)
// clang-format on

void __cdecl InitialiseCreature(int16_t item_num);
void __cdecl CreatureAIInfo(ITEM_INFO* item, AI_INFO* info);

void Tomb1MInjectGameBox();

#endif
