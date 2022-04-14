#pragma once

// Lara state routines.

#include "global/types.h"

extern void (*g_LaraStateRoutines[])(ITEM_INFO *item, COLL_INFO *coll);

void Lara_State_Walk(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Run(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Stop(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_ForwardJump(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Pose(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_FastBack(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_TurnR(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_TurnL(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Death(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_FastFall(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Hang(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Reach(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Splat(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Tread(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Land(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Compress(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Back(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Swim(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Glide(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Null(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_FastTurn(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_StepRight(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_StepLeft(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Roll2(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Slide(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_BackJump(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_RightJump(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_LeftJump(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_UpJump(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_FallBack(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_HangLeft(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_HangRight(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_SlideBack(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_SurfTread(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_SurfSwim(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Dive(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_PushBlock(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_PullBlock(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_PPReady(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Pickup(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Controlled(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_SwitchOn(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_SwitchOff(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_UseKey(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_UsePuzzle(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_UWDeath(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Roll(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Special(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_SurfBack(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_SurfLeft(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_SurfRight(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_UseMidas(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_DieMidas(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_SwanDive(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_FastDive(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_Gymnast(ITEM_INFO *item, COLL_INFO *coll);
void Lara_State_WaterOut(ITEM_INFO *item, COLL_INFO *coll);
