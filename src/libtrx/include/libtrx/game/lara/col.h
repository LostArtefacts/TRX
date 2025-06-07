#pragma once

#include "../collision.h"

void Lara_Col_Update(ITEM *item, COLL_INFO *coll);

extern void Lara_Col_Walk(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Run(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Stop(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_ForwardJump(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_FastBack(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Hang(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Back(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Swim(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_SideStep(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Roll2(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_BackJump(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_RightJump(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_LeftJump(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_UpJump(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_SurfTread(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_SurfSwim(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_UWDeath(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Roll(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_SurfBack(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_SurfLeft(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_SurfRight(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_SwanDive(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_FastDive(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Wade(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Jumper(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_ClimbLeft(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_ClimbRight(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_ClimbStance(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Climbing(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_ClimbDown(ITEM *item, COLL_INFO *coll);
