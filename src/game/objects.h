#ifndef T1M_GAME_OBJECTS_H
#define T1M_GAME_OBJECTS_H

#include "game/types.h"
#include <stdint.h>

void ShutThatDoor(DOORPOS_DATA* d);
void OpenThatDoor(DOORPOS_DATA* d);
void InitialiseDoor(int16_t item_num);
void DoorControl(int16_t item_num);

int32_t OnDrawBridge(ITEM_INFO* item, int32_t x, int32_t y);
void DrawBridgeFloor(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
void DrawBridgeCeiling(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
void DrawBridgeCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void BridgeFlatFloor(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
void BridgeFlatCeiling(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
int32_t GetOffset(ITEM_INFO* item, int32_t x, int32_t z);
void BridgeTilt1Floor(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
void BridgeTilt1Ceiling(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
void BridgeTilt2Floor(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
void BridgeTilt2Ceiling(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);

void CogControl(int16_t item_num);
void CabinControl(int16_t item_num);
void BoatControl(int16_t item_num);
void ScionControl(int16_t item_num);
void Scion3Control(int16_t item_num);
void EarthQuakeControl(int16_t item_num);

void T1MInjectGameObjects();

#endif
