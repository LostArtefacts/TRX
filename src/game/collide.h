#ifndef T1M_GAME_COLLIDE_H
#define T1M_GAME_COLLIDE_H

#include "types.h"
#include <stdint.h>

// clang-format off
#define LaraBaddieCollision     ((void          (*)(ITEM_INFO *item, COLL_INFO *coll))0x00412700)
#define CreatureCollision       ((void          (*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x00412910)
#define ObjectCollision         ((void          (*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x00412990)
#define DoorCollision           ((void          (*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x004129F0)
#define TrapCollision           ((void          (*)(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll))0x00412A70)
// clang-format on

void GetCollisionInfo(
    COLL_INFO* coll, int32_t xpos, int32_t ypos, int32_t zpos, int16_t room_num,
    int32_t objheight);
int32_t FindGridShift(int32_t src, int32_t dst);
int32_t CollideStaticObjects(
    COLL_INFO* coll, int32_t x, int32_t y, int32_t z, int16_t room_number,
    int32_t hite);
void GetNearByRooms(
    int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, int16_t room_num);
void GetNewRoom(int32_t x, int32_t y, int32_t z, int16_t room_num);
void ShiftItem(ITEM_INFO* item, COLL_INFO* coll);
int16_t GetTiltType(FLOOR_INFO* floor, int32_t x, int32_t y, int32_t z);

void T1MInjectGameCollide();

#endif
