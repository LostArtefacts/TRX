#ifndef TOMB1MAIN_GAME_COLLIDE_H
#define TOMB1MAIN_GAME_COLLIDE_H

#include "types.h"

// clang-format off
#define GetCollisionInfo        ((void          __cdecl(*)(COLL_INFO* coll, int32_t xpos, int32_t ypos, int32_t zpos, int16_t room_number, int32_t objheight))0x00411780)
#define LaraBaddieCollision     ((void          __cdecl(*)(ITEM_INFO *item, COLL_INFO *coll))0x00412700)
#define CreatureCollision       ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x00412910)
#define ObjectCollision         ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x00412990)
#define DoorCollision           ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x004129F0)
#define TrapCollision           ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x00412A70)
// clang-format on

#endif
