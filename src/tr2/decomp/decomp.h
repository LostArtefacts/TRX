#pragma once

#include "global/types.h"

#include <stdint.h>

// TODO: This is a placeholder file containing functions that originated from
// the decompilation phase, and they are currently disorganized. Eventually,
// they'll need to be properly modularized. The same applies to all files
// within the decomp/ directory which are scheduled for extensive refactoring.

void Game_SetCutsceneTrack(int32_t track);
void CutscenePlayer_Control(int16_t item_num);
void Lara_Control_Cutscene(int16_t item_num);
void CutscenePlayer1_Initialise(int16_t item_num);
void CutscenePlayerGen_Initialise(int16_t item_num);
int32_t Misc_Move3DPosTo3DPos(
    PHD_3DPOS *src_pos, const PHD_3DPOS *dst_pos, int32_t velocity,
    int16_t ang_add);
void S_InitialisePolyList(bool clear_back_buffer);
void DecreaseScreenSize(void);
void IncreaseScreenSize(void);
void S_UnloadLevelFile(void);
void GetValidLevelsList(REQUEST_INFO *req);
void InitialiseGameFlags(void);
void GetCarriedItems(void);
int32_t DoShift(ITEM *vehicle, const XYZ_32 *pos, const XYZ_32 *old);
int32_t DoDynamics(int32_t height, int32_t fall_speed, int32_t *out_y);
int32_t GetCollisionAnim(const ITEM *vehicle, XYZ_32 *moved);
void InitialiseFinalLevel(void);
