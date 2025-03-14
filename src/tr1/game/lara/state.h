#pragma once

// Lara state routines.

#include "global/types.h"

void Lara_State_Initialise(void);

void Lara_State_Empty(ITEM *item, COLL_INFO *coll);
void Lara_State_Walk(ITEM *item, COLL_INFO *coll);
void Lara_State_Run(ITEM *item, COLL_INFO *coll);
void Lara_State_Stop(ITEM *item, COLL_INFO *coll);
void Lara_State_ForwardJump(ITEM *item, COLL_INFO *coll);
void Lara_State_FastBack(ITEM *item, COLL_INFO *coll);
void Lara_State_TurnR(ITEM *item, COLL_INFO *coll);
void Lara_State_TurnL(ITEM *item, COLL_INFO *coll);
void Lara_State_Death(ITEM *item, COLL_INFO *coll);
void Lara_State_FastFall(ITEM *item, COLL_INFO *coll);
void Lara_State_Hang(ITEM *item, COLL_INFO *coll);
void Lara_State_Reach(ITEM *item, COLL_INFO *coll);
void Lara_State_Tread(ITEM *item, COLL_INFO *coll);
void Lara_State_Compress(ITEM *item, COLL_INFO *coll);
void Lara_State_Back(ITEM *item, COLL_INFO *coll);
void Lara_State_Swim(ITEM *item, COLL_INFO *coll);
void Lara_State_Glide(ITEM *item, COLL_INFO *coll);
void Lara_State_Null(ITEM *item, COLL_INFO *coll);
void Lara_State_FastTurn(ITEM *item, COLL_INFO *coll);
void Lara_State_StepRight(ITEM *item, COLL_INFO *coll);
void Lara_State_StepLeft(ITEM *item, COLL_INFO *coll);
void Lara_State_Slide(ITEM *item, COLL_INFO *coll);
void Lara_State_BackJump(ITEM *item, COLL_INFO *coll);
void Lara_State_RightJump(ITEM *item, COLL_INFO *coll);
void Lara_State_LeftJump(ITEM *item, COLL_INFO *coll);
void Lara_State_UpJump(ITEM *item, COLL_INFO *coll);
void Lara_State_FallBack(ITEM *item, COLL_INFO *coll);
void Lara_State_HangLeft(ITEM *item, COLL_INFO *coll);
void Lara_State_HangRight(ITEM *item, COLL_INFO *coll);
void Lara_State_SlideBack(ITEM *item, COLL_INFO *coll);
void Lara_State_SurfTread(ITEM *item, COLL_INFO *coll);
void Lara_State_SurfSwim(ITEM *item, COLL_INFO *coll);
void Lara_State_Dive(ITEM *item, COLL_INFO *coll);
void Lara_State_PushBlock(ITEM *item, COLL_INFO *coll);
void Lara_State_PullBlock(ITEM *item, COLL_INFO *coll);
void Lara_State_PPReady(ITEM *item, COLL_INFO *coll);
void Lara_State_Pickup(ITEM *item, COLL_INFO *coll);
void Lara_State_Controlled(ITEM *item, COLL_INFO *coll);
void Lara_State_SwitchOn(ITEM *item, COLL_INFO *coll);
void Lara_State_SwitchOff(ITEM *item, COLL_INFO *coll);
void Lara_State_UseKey(ITEM *item, COLL_INFO *coll);
void Lara_State_UsePuzzle(ITEM *item, COLL_INFO *coll);
void Lara_State_UWDeath(ITEM *item, COLL_INFO *coll);
void Lara_State_Special(ITEM *item, COLL_INFO *coll);
void Lara_State_SurfBack(ITEM *item, COLL_INFO *coll);
void Lara_State_SurfLeft(ITEM *item, COLL_INFO *coll);
void Lara_State_SurfRight(ITEM *item, COLL_INFO *coll);
void Lara_State_UseMidas(ITEM *item, COLL_INFO *coll);
void Lara_State_DieMidas(ITEM *item, COLL_INFO *coll);
void Lara_State_SwanDive(ITEM *item, COLL_INFO *coll);
void Lara_State_FastDive(ITEM *item, COLL_INFO *coll);
void Lara_State_WaterOut(ITEM *item, COLL_INFO *coll);
void Lara_State_UWRoll(ITEM *item, COLL_INFO *coll);
void Lara_State_Wade(ITEM *item, COLL_INFO *coll);
