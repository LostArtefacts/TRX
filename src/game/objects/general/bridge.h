#pragma once

#include "global/types.h"

#include <stdint.h>

void Bridge_SetupFlat(OBJECT_INFO *obj);
void Bridge_SetupTilt1(OBJECT_INFO *obj);
void Bridge_SetupTilt2(OBJECT_INFO *obj);
void Bridge_SetupDrawBridge(OBJECT_INFO *obj);

void Bridge_Initialise(int16_t item_num);

int16_t Bridge_GetDrawBridgeFloorHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);
int16_t Bridge_GetDrawBridgeCeilingHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);
void Bridge_DrawBridgeCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void Bridge_DrawBridgeControl(int16_t item_num);

int16_t Bridge_GetFlatFloorHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);
int16_t Bridge_GetFlatCeilingHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);

int16_t Bridge_GetTilt1FloorHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);
int16_t Bridge_GetTilt1CeilingHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);

int16_t Bridge_GetTilt2FloorHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);
int16_t Bridge_GetTilt2CeilingHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);
