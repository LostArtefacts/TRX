#ifndef T1M_GAME_LARA_H
#define T1M_GAME_LARA_H

#include "global/types.h"

#include <stdint.h>

extern WEAPON_INFO Weapons[NUM_WEAPONS];

void LaraControl(int16_t item_num);
void LaraSwapMeshExtra();
void AnimateLara(ITEM_INFO *item);
void AnimateLaraUntil(ITEM_INFO *lara_item, int32_t goal);
void UseItem(int16_t object_num);
void ControlLaraExtra(int16_t item_num);
void InitialiseLaraLoad(int16_t item_num);
void InitialiseLara();
void InitialiseLaraInventory(int32_t level_num);
void LaraInitialiseMeshes(int32_t level_num);

void LaraAboveWater(ITEM_INFO *item, COLL_INFO *coll);
void LaraUnderWater(ITEM_INFO *item, COLL_INFO *coll);
void LaraSurface(ITEM_INFO *item, COLL_INFO *coll);

void LaraAsWalk(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsRun(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsStop(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsForwardJump(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsPose(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsFastBack(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsTurnR(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsTurnL(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsDeath(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsFastFall(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsHang(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsReach(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSplat(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsTread(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsLand(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsCompress(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsBack(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSwim(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsGlide(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsNull(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsFastTurn(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsStepRight(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsStepLeft(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsRoll2(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSlide(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsBackJump(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsRightJump(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsLeftJump(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsUpJump(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsFallBack(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsHangLeft(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsHangRight(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSlideBack(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSurfTread(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSurfSwim(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsDive(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsPushBlock(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsPullBlock(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsPPReady(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsPickup(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSwitchOn(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSwitchOff(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsUseKey(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsUsePuzzle(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsUWDeath(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsRoll(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSpecial(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSurfBack(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSurfLeft(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSurfRight(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsUseMidas(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsDieMidas(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsSwanDive(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsFastDive(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsGymnast(ITEM_INFO *item, COLL_INFO *coll);
void LaraAsWaterOut(ITEM_INFO *item, COLL_INFO *coll);

void LaraColWalk(ITEM_INFO *item, COLL_INFO *coll);
void LaraColRun(ITEM_INFO *item, COLL_INFO *coll);
void LaraColStop(ITEM_INFO *item, COLL_INFO *coll);
void LaraColForwardJump(ITEM_INFO *item, COLL_INFO *coll);
void LaraColPose(ITEM_INFO *item, COLL_INFO *coll);
void LaraColFastBack(ITEM_INFO *item, COLL_INFO *coll);
void LaraColTurnR(ITEM_INFO *item, COLL_INFO *coll);
void LaraColTurnL(ITEM_INFO *item, COLL_INFO *coll);
void LaraColDeath(ITEM_INFO *item, COLL_INFO *coll);
void LaraColFastFall(ITEM_INFO *item, COLL_INFO *coll);
void LaraColHang(ITEM_INFO *item, COLL_INFO *coll);
void LaraColReach(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSplat(ITEM_INFO *item, COLL_INFO *coll);
void LaraColTread(ITEM_INFO *item, COLL_INFO *coll);
void LaraColLand(ITEM_INFO *item, COLL_INFO *coll);
void LaraColCompress(ITEM_INFO *item, COLL_INFO *coll);
void LaraColBack(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSwim(ITEM_INFO *item, COLL_INFO *coll);
void LaraColGlide(ITEM_INFO *item, COLL_INFO *coll);
void LaraColNull(ITEM_INFO *item, COLL_INFO *coll);
void LaraColFastTurn(ITEM_INFO *item, COLL_INFO *coll);
void LaraColStepRight(ITEM_INFO *item, COLL_INFO *coll);
void LaraColStepLeft(ITEM_INFO *item, COLL_INFO *coll);
void LaraColRoll2(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSlide(ITEM_INFO *item, COLL_INFO *coll);
void LaraColBackJump(ITEM_INFO *item, COLL_INFO *coll);
void LaraColRightJump(ITEM_INFO *item, COLL_INFO *coll);
void LaraColLeftJump(ITEM_INFO *item, COLL_INFO *coll);
void LaraColUpJump(ITEM_INFO *item, COLL_INFO *coll);
void LaraColFallBack(ITEM_INFO *item, COLL_INFO *coll);
void LaraColHangLeft(ITEM_INFO *item, COLL_INFO *coll);
void LaraColHangRight(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSlideBack(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSurfTread(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSurfSwim(ITEM_INFO *item, COLL_INFO *coll);
void LaraColDive(ITEM_INFO *item, COLL_INFO *coll);
void LaraColPushBlock(ITEM_INFO *item, COLL_INFO *coll);
void LaraColPullBlock(ITEM_INFO *item, COLL_INFO *coll);
void LaraColPPReady(ITEM_INFO *item, COLL_INFO *coll);
void LaraColPickup(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSwitchOn(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSwitchOff(ITEM_INFO *item, COLL_INFO *coll);
void LaraColUseKey(ITEM_INFO *item, COLL_INFO *coll);
void LaraColUsePuzzle(ITEM_INFO *item, COLL_INFO *coll);
void LaraColUWDeath(ITEM_INFO *item, COLL_INFO *coll);
void LaraColRoll(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSpecial(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSurfBack(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSurfLeft(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSurfRight(ITEM_INFO *item, COLL_INFO *coll);
void LaraColUseMidas(ITEM_INFO *item, COLL_INFO *coll);
void LaraColDieMidas(ITEM_INFO *item, COLL_INFO *coll);
void LaraColSwanDive(ITEM_INFO *item, COLL_INFO *coll);
void LaraColFastDive(ITEM_INFO *item, COLL_INFO *coll);
void LaraColGymnast(ITEM_INFO *item, COLL_INFO *coll);
void LaraColWaterOut(ITEM_INFO *item, COLL_INFO *coll);

void LaraColDefault(ITEM_INFO *item, COLL_INFO *coll);
void LaraColJumper(ITEM_INFO *item, COLL_INFO *coll);

void GetLaraCollisionInfo(ITEM_INFO *item, COLL_INFO *coll);
void LaraSlideSlope(ITEM_INFO *item, COLL_INFO *coll);
int32_t LaraHitCeiling(ITEM_INFO *item, COLL_INFO *coll);
void LaraHangTest(ITEM_INFO *item, COLL_INFO *coll);
int32_t LaraDeflectEdge(ITEM_INFO *item, COLL_INFO *coll);
void LaraDeflectEdgeJump(ITEM_INFO *item, COLL_INFO *coll);
void LaraSlideEdgeJump(ITEM_INFO *item, COLL_INFO *coll);
int32_t TestLaraVault(ITEM_INFO *item, COLL_INFO *coll);
int32_t LaraTestHangJump(ITEM_INFO *item, COLL_INFO *coll);
int32_t LaraTestHangJumpUp(ITEM_INFO *item, COLL_INFO *coll);
int32_t TestHangSwingIn(ITEM_INFO *item, PHD_ANGLE angle);
int32_t TestLaraSlide(ITEM_INFO *item, COLL_INFO *coll);
int16_t LaraFloorFront(ITEM_INFO *item, PHD_ANGLE ang, int32_t dist);
int32_t LaraLandedBad(ITEM_INFO *item, COLL_INFO *coll);
void LaraSwimCollision(ITEM_INFO *item, COLL_INFO *coll);
void LaraWaterCurrent(COLL_INFO *coll);
void LaraSurfaceCollision(ITEM_INFO *item, COLL_INFO *coll);
int32_t LaraTestWaterClimbOut(ITEM_INFO *item, COLL_INFO *coll);

extern void (*LaraControlRoutines[])(ITEM_INFO *item, COLL_INFO *coll);
extern void (*LaraCollisionRoutines[])(ITEM_INFO *item, COLL_INFO *coll);

void LaraGun();
void InitialiseNewWeapon();
void LaraTargetInfo(WEAPON_INFO *winfo);
void LaraGetNewTarget(WEAPON_INFO *winfo);
void find_target_point(ITEM_INFO *item, GAME_VECTOR *target);
void AimWeapon(WEAPON_INFO *winfo, LARA_ARM *arm);
int32_t FireWeapon(
    int32_t weapon_type, ITEM_INFO *target, ITEM_INFO *src, PHD_ANGLE *angles);
void HitTarget(ITEM_INFO *item, GAME_VECTOR *hitpos, int32_t damage);

void DrawShotgun();
void UndrawShotgun();
void DrawShotgunMeshes();
void UndrawShotgunMeshes();
void ReadyShotgun();
void RifleHandler(int32_t weapon_type);
void AnimateShotgun();
void FireShotgun();

void DrawPistols(int32_t weapon_type);
void UndrawPistols(int32_t weapon_type);
void ReadyPistols();
void DrawPistolMeshes(int32_t weapon_type);
void UndrawPistolMeshLeft(int32_t weapon_type);
void UndrawPistolMeshRight(int32_t weapon_type);
void PistolHandler(int32_t weapon_type);
void AnimatePistols(int32_t weapon_type);

void LookLeftRight();
void LookUpDown();
void ResetLook();

void LaraCheatGetStuff();

void T1MInjectGameLara();
void T1MInjectGameLaraMisc();
void T1MInjectGameLaraFire();
void T1MInjectGameLaraGun1();
void T1MInjectGameLaraGun2();

#endif
