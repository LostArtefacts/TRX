#ifndef T1M_GAME_COLLIDE_H
#define T1M_GAME_COLLIDE_H

#include "types.h"
#include <stdint.h>

// clang-format off
#define GetCollisionInfo        ((void         (*)(COLL_INFO* coll, int32_t xpos, int32_t ypos, int32_t zpos, int16_t room_number, int32_t objheight))0x00411780)
#define LaraBaddieCollision     ((void         (*)(ITEM_INFO *item, COLL_INFO *coll))0x00412700)
#define CreatureCollision       ((void         (*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x00412910)
#define ObjectCollision         ((void         (*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x00412990)
#define DoorCollision           ((void         (*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x004129F0)
#define TrapCollision           ((void         (*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x00412A70)
// clang-format on

#endif
