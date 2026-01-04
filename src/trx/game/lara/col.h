#pragma once

#include <trx/game/collision.h>
#include <trx/game/lara/enum.h>

void Lara_Col_Register(
    LARA_TRX_STATE state, void (*handle_func)(ITEM *item, COLL_INFO *coll));
void Lara_Col_Update(ITEM *item, COLL_INFO *coll);
void Lara_Col_GetInfo(const ITEM *item, COLL_INFO *coll);
void Lara_Col_Shift(COLL_INFO *coll);
bool Lara_Col_TestVault(ITEM *item, COLL_INFO *coll);
bool Lara_Col_TestLadderHang(ITEM *item, const COLL_INFO *coll);
void Lara_Col_DeflectEdgeJump(ITEM *item, COLL_INFO *coll);
bool Lara_Col_LandedBad(ITEM *item);
void Lara_Col_MonkeySwingSnap(ITEM *item);
