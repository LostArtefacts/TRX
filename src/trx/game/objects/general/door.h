#pragma once

#include <trx/game/collision.h>

#include <stddef.h>

typedef enum {
    DOOR_STATE_CLOSED = 0,
    DOOR_STATE_OPEN = 1,
} DOOR_STATE;

void Door_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

void Door_Initialise(int16_t item_num);
void Door_OpenPortals(ITEM *item);
void Door_ShutPortals(ITEM *item);
// Returns the room behind the doorway, which the door is drawn from as in
// the original TR2-TR4, or NO_ROOM to draw the door from its own room too.
int16_t Door_GetDrawRoomNum(const ITEM *item);
size_t Door_GetPrivSize(void);
void Door_LiftActivate(ITEM *item, int32_t count);
int16_t Door_FindNearbyCrowbarDoor(void);
