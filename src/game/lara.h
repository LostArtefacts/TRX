#ifndef TOMB1MAIN_GAME_LARA_H
#define TOMB1MAIN_GAME_LARA_H

#include "game/types.h"
#include "util.h"

// clang-format off
#define InitialiseLaraInventory ((void          __cdecl(*)(int level_id))0x00428170)
#define LaraControl             ((void          __cdecl(*)(int16_t item_num))0x00427850)
#define UpdateLaraRoom          ((void          __cdecl(*)(ITEM_INFO* item, int height))0x004126A0)
#define ShiftItem               ((void          __cdecl(*)(ITEM_INFO* item, COLL_INFO *coll))0x00412660)
#define LaraGun                 ((void          __cdecl(*)())0x00426BD0)
#define LaraTargetInfo          ((void          __cdecl(*)(WEAPON_INFO* winfo))0x00426F20)
#define LaraGetNewTarget        ((void          __cdecl(*)(WEAPON_INFO* winfo))0x004270C0)
#define AimWeapon               ((void          __cdecl(*)(WEAPON_INFO* winfo, LARA_ARM* arm))0x00427360)
#define FireWeapon              ((int32_t       __cdecl(*)(int32_t weapon_type, ITEM_INFO *target, ITEM_INFO *src, PHD_ANGLE *angles))0x00427430)
// clang-format on

void __cdecl LaraSwapMeshExtra();
void __cdecl AnimateLara(ITEM_INFO* item);
void __cdecl UseItem(int16_t object_num);
void __cdecl ControlLaraExtra(int16_t item_num);
void __cdecl InitialiseLaraLoad(int16_t item_num);
void __cdecl InitialiseLara();

void __cdecl LaraAboveWater(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraUnderWater(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraSurface(ITEM_INFO* item, COLL_INFO* coll);

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
void __cdecl LaraAsTread(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsLand(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsCompress(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSwim(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsGlide(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsNull(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFastTurn(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsStepRight(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsStepLeft(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsRoll2(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSlide(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsBackJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsRightJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsLeftJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUpJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFallBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsHangLeft(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsHangRight(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSlideBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSurfTread(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSurfSwim(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsDive(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPushBlock(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPullBlock(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPPReady(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPickup(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSwitchOn(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSwitchOff(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUseKey(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUsePuzzle(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUWDeath(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsRoll(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSpecial(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSurfBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSurfLeft(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSurfRight(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUseMidas(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsDieMidas(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSwanDive(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFastDive(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsGymnast(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsWaterOut(ITEM_INFO* item, COLL_INFO* coll);

void __cdecl LaraColWalk(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColRun(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColStop(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColForwardJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColPose(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColFastBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColTurnR(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColTurnL(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColDeath(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColFastFall(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColHang(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColReach(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSplat(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColTread(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColLand(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColCompress(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSwim(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColGlide(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColNull(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColFastTurn(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColStepRight(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColStepLeft(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColRoll2(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSlide(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColBackJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColRightJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColLeftJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColUpJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColFallBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColHangLeft(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColHangRight(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSlideBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSurfTread(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSurfSwim(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColDive(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColPushBlock(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColPullBlock(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColPPReady(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColPickup(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSwitchOn(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSwitchOff(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColUseKey(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColUsePuzzle(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColUWDeath(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColRoll(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSpecial(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSurfBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSurfLeft(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSurfRight(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColUseMidas(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColDieMidas(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSwanDive(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColFastDive(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColGymnast(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColWaterOut(ITEM_INFO* item, COLL_INFO* coll);

void __cdecl LaraColDefault(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColJumper(ITEM_INFO* item, COLL_INFO* coll);

void __cdecl GetLaraCollisionInfo(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraSlideSlope(ITEM_INFO* item, COLL_INFO* coll);
int32_t __cdecl LaraHitCeiling(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraHangTest(ITEM_INFO* item, COLL_INFO* coll);
int32_t __cdecl LaraDeflectEdge(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraDeflectEdgeJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraSlideEdgeJump(ITEM_INFO* item, COLL_INFO* coll);
int32_t __cdecl TestLaraVault(ITEM_INFO* item, COLL_INFO* coll);
int32_t __cdecl LaraTestHangJump(ITEM_INFO* item, COLL_INFO* coll);
int32_t __cdecl LaraTestHangJumpUp(ITEM_INFO* item, COLL_INFO* coll);
int32_t __cdecl TestHangSwingIn(ITEM_INFO* item, PHD_ANGLE angle);
int32_t __cdecl TestLaraSlide(ITEM_INFO* item, COLL_INFO* coll);
int16_t __cdecl LaraFloorFront(ITEM_INFO* item, PHD_ANGLE ang, int32_t dist);
int32_t __cdecl LaraLandedBad(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraSwimCollision(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraWaterCurrent(COLL_INFO* coll);
void __cdecl LaraSurfaceCollision(ITEM_INFO* item, COLL_INFO* coll);
int32_t __cdecl LaraTestWaterClimbOut(ITEM_INFO* item, COLL_INFO* coll);

extern void (*LaraControlRoutines[])(ITEM_INFO* item, COLL_INFO* coll);
extern void (*LaraCollisionRoutines[])(ITEM_INFO* item, COLL_INFO* coll);

void __cdecl draw_shotgun();
void __cdecl draw_shotgun_meshes();
void __cdecl ready_shotgun();
void __cdecl RifleHandler(int32_t weapon_type);
void __cdecl AnimateShotgun();
void __cdecl FireShotgun();

void Tomb1MLookLeftRight();
void Tomb1MLookUpDown();
void Tomb1MResetLook();

void Tomb1MInjectGameLara();
void Tomb1MInjectGameLaraMisc();
void Tomb1MInjectGameLaraSurf();
void Tomb1MInjectGameLaraSwim();
void Tomb1MInjectGameLaraGun1();

#endif
