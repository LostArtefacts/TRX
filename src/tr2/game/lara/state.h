#pragma once

// Lara state routines.

#include "global/types.h"

// TODO: make static
void __cdecl Lara_SwimTurn(ITEM *item);

void __cdecl Lara_State_Walk(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Run(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Stop(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_ForwardJump(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_FastBack(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_TurnRight(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_TurnLeft(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Death(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_FastFall(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Hang(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Reach(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Splat(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Compress(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Back(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Null(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_FastTurn(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_StepRight(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_StepLeft(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Slide(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_BackJump(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_RightJump(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_LeftJump(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_UpJump(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Fallback(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_HangLeft(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_HangRight(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_SlideBack(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_PushBlock(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_PPReady(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Pickup(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_PickupFlare(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_SwitchOn(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_UseKey(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Special(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_SwanDive(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_FastDive(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_WaterOut(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Wade(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_DeathSlide(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Extra_Breath(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Extra_YetiKill(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Extra_SharkKill(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Extra_Airlock(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Extra_GongBong(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Extra_DinoKill(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Extra_PullDagger(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Extra_StartAnim(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Extra_StartHouse(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Extra_FinalAnim(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_ClimbLeft(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_ClimbRight(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_ClimbStance(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Climbing(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_ClimbEnd(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_ClimbDown(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_SurfSwim(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_SurfBack(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_SurfLeft(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_SurfRight(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_SurfTread(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Swim(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Glide(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Tread(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_Dive(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_UWDeath(ITEM *item, COLL_INFO *coll);
void __cdecl Lara_State_UWTwist(ITEM *item, COLL_INFO *coll);
