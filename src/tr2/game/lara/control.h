#pragma once

#include "global/types.h"

void __cdecl Lara_HandleAboveWater(ITEM *item, COLL_INFO *coll);

void __cdecl Lara_HandleSurface(ITEM *item, COLL_INFO *coll);

void __cdecl Lara_HandleUnderwater(ITEM *item, COLL_INFO *coll);

void __cdecl Lara_Control(int16_t item_num);
void __cdecl Lara_ControlExtra(int16_t item_num);

void __cdecl Lara_Animate(ITEM *item);

void __cdecl Lara_UseItem(GAME_OBJECT_ID object_id);

void __cdecl Lara_InitialiseLoad(int16_t item_num);

void __cdecl Lara_Initialise(GAMEFLOW_LEVEL_TYPE type);

void __cdecl Lara_InitialiseInventory(int32_t level_num);

void __cdecl Lara_InitialiseMeshes(int32_t level_num);

void Lara_GetOffVehicle(void);
void Lara_SwapSingleMesh(LARA_MESH mesh, GAME_OBJECT_ID);

int16_t Lara_GetNearestEnemy(void);
void Lara_TakeDamage(int16_t damage, bool hit_status);
