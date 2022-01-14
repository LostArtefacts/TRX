#pragma once

#include "global/types.h"

void SetupBridgeFlat(OBJECT_INFO *obj);
void SetupBridgeTilt1(OBJECT_INFO *obj);
void SetupBridgeTilt2(OBJECT_INFO *obj);
void SetupDrawBridge(OBJECT_INFO *obj);
int32_t OnDrawBridge(ITEM_INFO *item, int32_t x, int32_t y);
void DrawBridgeFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void DrawBridgeCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void DrawBridgeCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void BridgeFlatFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void BridgeFlatCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
int32_t GetOffset(ITEM_INFO *item, int32_t x, int32_t y, int32_t z);
void BridgeTilt1Floor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void BridgeTilt1Ceiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void BridgeTilt2Floor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void BridgeTilt2Ceiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
