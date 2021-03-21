#ifndef T1M_GAME_TRAPS_THORS_HAMMER_H
#define T1M_GAME_TRAPS_THORS_HAMMER_H

#include "global/types.h"

typedef enum {
    THS_SET = 0,
    THS_TEASE = 1,
    THS_ACTIVE = 2,
    THS_DONE = 3,
} THOR_HAMMER_STATE;

void SetupThorsHandle(OBJECT_INFO *obj);
void SetupThorsHead(OBJECT_INFO *obj);
void InitialiseThorsHandle(int16_t item_num);
void ThorsHandleControl(int16_t item_num);
void ThorsHandleCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void ThorsHeadCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);

#endif
