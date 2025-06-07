#pragma once

#include "../collision.h"

void Lara_Col_Update(ITEM *item, COLL_INFO *coll);

extern void Lara_Col_Walk(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Run(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Stop(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_FastBack(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Hang(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Back(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_SideStep(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_ClimbLeft(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_ClimbRight(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_ClimbStance(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Climbing(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_ClimbDown(ITEM *item, COLL_INFO *coll);
