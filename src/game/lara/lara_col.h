#pragma once

// Collision routines.

#include "global/types.h"

extern void (*g_LaraCollisionRoutines[])(ITEM_INFO *item, COLL_INFO *coll);

void Lara_Col_Walk(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Run(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Stop(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_ForwardJump(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Pose(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_FastBack(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_TurnR(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_TurnL(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Death(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_FastFall(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Hang(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Reach(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Splat(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Tread(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Land(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Compress(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Back(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Swim(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Glide(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Null(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_FastTurn(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_StepRight(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_StepLeft(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Roll2(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Slide(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_BackJump(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_RightJump(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_LeftJump(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_UpJump(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_FallBack(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_HangLeft(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_HangRight(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_SlideBack(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_SurfTread(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_SurfSwim(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Dive(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_PushBlock(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_PullBlock(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_PPReady(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Pickup(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Controlled(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_SwitchOn(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_SwitchOff(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_UseKey(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_UsePuzzle(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_UWDeath(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Roll(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Special(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_SurfBack(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_SurfLeft(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_SurfRight(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_UseMidas(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_DieMidas(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_SwanDive(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_FastDive(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Gymnast(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_WaterOut(ITEM_INFO *item, COLL_INFO *coll);
void Lara_Col_Twist(ITEM_INFO *item, COLL_INFO *coll);
