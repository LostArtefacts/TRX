#ifndef TR1MAIN_GAME_COLLIDE_H
#define TR1MAIN_GAME_COLLIDE_H

#include "types.h"

// clang-format off
#define GetCollisionInfo        ((void          __cdecl(*)(COLL_INFO* coll, int32_t xpos, int32_t ypos, int32_t zpos, int16_t room_number, int32_t objheight))0x00411780)
#define LaraBaddieCollision     ((void          __cdecl(*)(ITEM_INFO *item, COLL_INFO *coll))0x00412700)
// clang-format on

#endif
