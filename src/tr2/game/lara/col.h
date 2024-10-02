#pragma once

#include "global/types.h"

// TODO: make static
void __cdecl Lara_CollideStop(ITEM *item, const COLL_INFO *coll);
bool __cdecl Lara_Fallen(ITEM *item, const COLL_INFO *coll);
bool __cdecl Lara_TestWaterClimbOut(ITEM *item, const COLL_INFO *coll);
bool __cdecl Lara_TestWaterStepOut(ITEM *item, const COLL_INFO *coll);
void __cdecl Lara_SurfaceCollision(ITEM *item, COLL_INFO *coll);

void __cdecl Lara_Col_Walk(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Run(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Stop(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_ForwardJump(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_FastBack(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_TurnRight(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_TurnLeft(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Death(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_FastFall(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Hang(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Reach(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Splat(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Land(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Compress(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Back(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_StepRight(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_StepLeft(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Slide(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_BackJump(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_RightJump(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_LeftJump(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_UpJump(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Fallback(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_HangLeft(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_HangRight(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_SlideBack(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Null(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Roll(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Roll2(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_SwanDive(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_FastDive(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Wade(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Default(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Jumper(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_ClimbLeft(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_ClimbRight(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_ClimbStance(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Climbing(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_ClimbDown(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_SurfSwim(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_SurfBack(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_SurfLeft(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_SurfRight(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_SurfTread(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_Swim(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_Col_UWDeath(ITEM *item, COLL_INFO *coll);
