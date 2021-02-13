#ifndef TR1MAIN_GAME_LARA_H
#define TR1MAIN_GAME_LARA_H

#include "game/types.h"
#include "util.h"

// clang-format off
#define InitialiseLaraInventory ((void          __cdecl(*)(int level_id))0x00428170)
#define LaraControl             ((void          __cdecl(*)(int16_t item_num))0x00427850)
// clang-format on

void __cdecl InitialiseLara();
void __cdecl LaraAsWalk(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsRun(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsStop(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsForwardJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPose(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFastBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsTurnR(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsTurnL(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsDeath(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFastFall(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsHang(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsReach(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSplat(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsLand(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsCompress(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFastTurn(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsStepRight(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsStepLeft(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSlide(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsBackJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsRightJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsLeftJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUpJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFallBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsHangLeft(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsHangRight(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSlideBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPushBlock(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPullBlock(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPPReady(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPickup(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSwitchOn(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSwitchOff(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUseKey(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUsePuzzle(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsRoll(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsRoll2(ITEM_INFO* item, COLL_INFO* coll);
int16_t __cdecl LaraFloorFront(ITEM_INFO* item, PHD_ANGLE ang, int32_t dist);
void __cdecl UseItem(__int16 object_num);

void TR1MInjectLara();
void TR1MInjectLaraMisc();

#endif
