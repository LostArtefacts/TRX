#ifndef T1M_GAME_OBJECTS_DOOR_H
#define T1M_GAME_OBJECTS_DOOR_H

#include "global/types.h"

void SetupDoor(OBJECT_INFO *obj);
void ShutThatDoor(DOORPOS_DATA *d);
void OpenThatDoor(DOORPOS_DATA *d);
void InitialiseDoor(int16_t item_num);
void DoorControl(int16_t item_num);
void OpenNearestDoors(ITEM_INFO *lara_item);

#endif
