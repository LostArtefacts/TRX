#pragma once

// Collision routines.

#include "global/types.h"

extern void (*g_LaraCollisionRoutines[])(ITEM *item, COLL_INFO *coll);

void Lara_Col_Walk(ITEM *item, COLL_INFO *coll);
void Lara_Col_Run(ITEM *item, COLL_INFO *coll);
void Lara_Col_Stop(ITEM *item, COLL_INFO *coll);
void Lara_Col_ForwardJump(ITEM *item, COLL_INFO *coll);
void Lara_Col_Pose(ITEM *item, COLL_INFO *coll);
void Lara_Col_FastBack(ITEM *item, COLL_INFO *coll);
void Lara_Col_TurnR(ITEM *item, COLL_INFO *coll);
void Lara_Col_TurnL(ITEM *item, COLL_INFO *coll);
void Lara_Col_Death(ITEM *item, COLL_INFO *coll);
void Lara_Col_FastFall(ITEM *item, COLL_INFO *coll);
void Lara_Col_Hang(ITEM *item, COLL_INFO *coll);
void Lara_Col_Reach(ITEM *item, COLL_INFO *coll);
void Lara_Col_Splat(ITEM *item, COLL_INFO *coll);
void Lara_Col_Tread(ITEM *item, COLL_INFO *coll);
void Lara_Col_Land(ITEM *item, COLL_INFO *coll);
void Lara_Col_Compress(ITEM *item, COLL_INFO *coll);
void Lara_Col_Back(ITEM *item, COLL_INFO *coll);
void Lara_Col_Swim(ITEM *item, COLL_INFO *coll);
void Lara_Col_Glide(ITEM *item, COLL_INFO *coll);
void Lara_Col_Null(ITEM *item, COLL_INFO *coll);
void Lara_Col_FastTurn(ITEM *item, COLL_INFO *coll);
void Lara_Col_StepRight(ITEM *item, COLL_INFO *coll);
void Lara_Col_StepLeft(ITEM *item, COLL_INFO *coll);
void Lara_Col_Roll2(ITEM *item, COLL_INFO *coll);
void Lara_Col_Slide(ITEM *item, COLL_INFO *coll);
void Lara_Col_BackJump(ITEM *item, COLL_INFO *coll);
void Lara_Col_RightJump(ITEM *item, COLL_INFO *coll);
void Lara_Col_LeftJump(ITEM *item, COLL_INFO *coll);
void Lara_Col_UpJump(ITEM *item, COLL_INFO *coll);
void Lara_Col_FallBack(ITEM *item, COLL_INFO *coll);
void Lara_Col_HangLeft(ITEM *item, COLL_INFO *coll);
void Lara_Col_HangRight(ITEM *item, COLL_INFO *coll);
void Lara_Col_SlideBack(ITEM *item, COLL_INFO *coll);
void Lara_Col_SurfTread(ITEM *item, COLL_INFO *coll);
void Lara_Col_SurfSwim(ITEM *item, COLL_INFO *coll);
void Lara_Col_Dive(ITEM *item, COLL_INFO *coll);
void Lara_Col_PushBlock(ITEM *item, COLL_INFO *coll);
void Lara_Col_PullBlock(ITEM *item, COLL_INFO *coll);
void Lara_Col_PPReady(ITEM *item, COLL_INFO *coll);
void Lara_Col_Pickup(ITEM *item, COLL_INFO *coll);
void Lara_Col_Controlled(ITEM *item, COLL_INFO *coll);
void Lara_Col_SwitchOn(ITEM *item, COLL_INFO *coll);
void Lara_Col_SwitchOff(ITEM *item, COLL_INFO *coll);
void Lara_Col_UseKey(ITEM *item, COLL_INFO *coll);
void Lara_Col_UsePuzzle(ITEM *item, COLL_INFO *coll);
void Lara_Col_UWDeath(ITEM *item, COLL_INFO *coll);
void Lara_Col_Roll(ITEM *item, COLL_INFO *coll);
void Lara_Col_Special(ITEM *item, COLL_INFO *coll);
void Lara_Col_SurfBack(ITEM *item, COLL_INFO *coll);
void Lara_Col_SurfLeft(ITEM *item, COLL_INFO *coll);
void Lara_Col_SurfRight(ITEM *item, COLL_INFO *coll);
void Lara_Col_UseMidas(ITEM *item, COLL_INFO *coll);
void Lara_Col_DieMidas(ITEM *item, COLL_INFO *coll);
void Lara_Col_SwanDive(ITEM *item, COLL_INFO *coll);
void Lara_Col_FastDive(ITEM *item, COLL_INFO *coll);
void Lara_Col_Gymnast(ITEM *item, COLL_INFO *coll);
void Lara_Col_WaterOut(ITEM *item, COLL_INFO *coll);
void Lara_Col_Twist(ITEM *item, COLL_INFO *coll);
void Lara_Col_UWRoll(ITEM *item, COLL_INFO *coll);
