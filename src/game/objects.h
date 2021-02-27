#ifndef T1M_GAME_OBJECTS_H
#define T1M_GAME_OBJECTS_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define InitialiseDoor                  ((void          (*)(int16_t item_num))0x0042CA40)
#define DoorControl                     ((void          (*)(int16_t item_num))0x0042CEF0)
#define DrawBridgeFloor                 ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0042D1F0)
#define DrawBridgeCeiling               ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0042D230)
#define DrawBridgeCollision             ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0042D270)
#define CogControl                      ((void          (*)(int16_t item_num))0x0042D420)
#define BridgeFlatFloor                 ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0042D2A0)
#define BridgeFlatCeiling               ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0042D2C0)
#define BridgeTilt1Floor                ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0042D2E0)
#define BridgeTilt1Ceiling              ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0042D330)
#define BridgeTilt2Floor                ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0042D380)
#define BridgeTilt2Ceiling              ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0042D3D0)
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
#define CabinControl                    ((void          (*)(int16_t item_num))0x0042D4A0)
#define EarthQuakeControl               ((void          (*)(int16_t item_num))0x0042D700)
#define InitialisePlayer1               ((void          (*)(int16_t item_num))0x004114F0)
#define ControlCinematicPlayer          ((void          (*)(int16_t item_num))0x004114A0)
#define InitialiseGenPlayer             ((void          (*)(int16_t item_num))0x004115C0)
#define ControlWaterFall                ((void          (*)(int16_t item_num))0x0041A9B0)
#define ControlBodyPart                 ((void          (*)(int16_t item_num))0x0043CAD0)
#define ControlNatlaGun                 ((void          (*)(int16_t item_num))0x0042C910)
#define ControlMissile                  ((void          (*)(int16_t item_num))0x0043C1C0)
#define ControlGunShot                  ((void          (*)(int16_t item_num))0x00430E00)
#define PickUpScionCollision            ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x00433240)
#define PickUpScion4Collision           ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x004333B0)
#define BoatControl                     ((void          (*)(int16_t item_num))0x0042D520)
#define Scion3Control                   ((void          (*)(int16_t item_num))0x0042D580)
#define ControlSplash1                  ((void          (*)(int16_t item_num))0x0041A930)
// clang-format on

#endif
