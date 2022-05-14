#pragma once

#include "global/types.h"

#include <stdint.h>

void Bridge_SetupFlat(OBJECT_INFO *obj);
void Bridge_SetupTilt1(OBJECT_INFO *obj);
void Bridge_SetupTilt2(OBJECT_INFO *obj);
void Bridge_SetupDrawBridge(OBJECT_INFO *obj);

void Bridge_DrawBridgeFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void Bridge_DrawBridgeCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void Bridge_DrawBridgeCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void Bridge_DrawBridgeControl(int16_t item_num);

void Bridge_FlatFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void Bridge_FlatCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);

void Bridge_Tilt1Floor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void Bridge_Tilt1Ceiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);

void Bridge_Tilt2Floor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void Bridge_Tilt2Ceiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
