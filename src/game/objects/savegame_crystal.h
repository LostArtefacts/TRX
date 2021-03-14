#ifndef T1M_GAME_OBJECTS_SAVEGAME_CRYSTAL_H
#define T1M_GAME_OBJECTS_SAVEGAME_CRYSTAL_H

#include "game/types.h"

#include <stdint.h>

void SetupSaveGameCrystal(OBJECT_INFO *obj);
void InitialiseSaveGameItem(int16_t item_num);
void ControlSaveGameItem(int16_t item_num);
void PickUpSaveGameCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);

#endif
