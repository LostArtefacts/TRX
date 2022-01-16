#pragma once

#include "global/types.h"

void SetupDoor(OBJECT_INFO *obj);
void InitialiseDoor(int16_t item_num);
void DoorControl(int16_t item_num);
void OpenNearestDoors(ITEM_INFO *lara_item);
void SetLaraDoorCollision(bool is_lara_collide);
bool CheckLaraDoorCollision();
