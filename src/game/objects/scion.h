#pragma once

#include "global/types.h"

#include <stdint.h>

extern PHD_VECTOR g_PickUpScionPosition;
extern PHD_VECTOR g_PickUpScion4Position;
extern int16_t g_PickUpScionBounds[12];
extern int16_t g_PickUpScion4Bounds[12];

void SetupScion1(OBJECT_INFO *obj);
void SetupScion2(OBJECT_INFO *obj);
void SetupScion3(OBJECT_INFO *obj);
void SetupScion4(OBJECT_INFO *obj);
void SetupScionHolder(OBJECT_INFO *obj);
void ScionControl(int16_t item_num);
void Scion3Control(int16_t item_num);
void PickUpScionCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void PickUpScion4Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
