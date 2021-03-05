#ifndef T1M_GAME_OBJECTS_H
#define T1M_GAME_OBJECTS_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define SwitchControl                   ((void          (*)(int16_t item_num))0x00433DE0)
#define SwitchCollision                 ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x004336F0)
#define SwitchCollision2                ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x00433810)
#define KeyHoleCollision                ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x00433900)
#define TrapDoorControl                 ((void          (*)(int16_t item_num))0x0043A670)
#define TrapDoorFloor                   ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0043A6D0)
#define TrapDoorCeiling                 ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0043A720)
#define PickUpCollision                 ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x00433080)
#define InitialiseSaveGameItem          ((void          (*)(int16_t item_num))0x00433F30)
#define PuzzleHoleCollision             ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x00433B40)
#define InitialisePlayer1               ((void          (*)(int16_t item_num))0x004114F0)
#define ControlCinematicPlayer          ((void          (*)(int16_t item_num))0x004114A0)
#define InitialiseGenPlayer             ((void          (*)(int16_t item_num))0x004115C0)
#define ControlBodyPart                 ((void          (*)(int16_t item_num))0x0043CAD0)
#define ControlMissile                  ((void          (*)(int16_t item_num))0x0043C1C0)
#define ControlGunShot                  ((void          (*)(int16_t item_num))0x00430E00)
#define PickUpScionCollision            ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x00433240)
#define PickUpScion4Collision           ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x004333B0)
// clang-format on

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
