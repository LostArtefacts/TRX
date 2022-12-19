#pragma once

#include "global/const.h"
#include "global/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern const char *g_T1MVersion;

extern int32_t g_PhdPersp;
extern int32_t g_PhdLeft;
extern int32_t g_PhdBottom;
extern int32_t g_PhdRight;
extern int32_t g_PhdTop;
extern PHD_SPRITE g_PhdSpriteInfo[MAX_SPRITES];
extern PHD_TEXTURE g_PhdTextureInfo[MAX_TEXTURES];
extern MATRIX *g_MatrixPtr;
extern MATRIX g_W2VMatrix;

extern int32_t g_LsAdder;
extern int32_t g_LsDivider;
extern SHADOW_INFO g_ShadowInfo;

extern bool g_IDelay;
extern int32_t g_IDCount;
extern int32_t g_OptionSelected;

extern int16_t g_SampleLUT[MAX_SAMPLES];
extern SAMPLE_INFO *g_SampleInfos;
extern uint16_t g_MusicTrackFlags[MAX_CD_TRACKS];
extern int32_t g_Sound_MasterVolume;

extern int32_t g_FPSCounter;

extern void (*g_EffectRoutines[])(ITEM_INFO *item);

extern LARA_INFO g_Lara;
extern ITEM_INFO *g_LaraItem;
extern int16_t g_OldLaraHitPoints;
extern CAMERA_INFO g_Camera;
extern GAME_INFO g_GameInfo;
extern int32_t g_SavedGamesCount;
extern int32_t g_SaveCounter;
extern int16_t g_CurrentLevel;
extern uint32_t *g_DemoData;
extern bool g_LevelComplete;
extern bool g_ChunkyFlag;
extern int32_t g_OverlayFlag;
extern int32_t g_HeightType;

extern ROOM_INFO *g_RoomInfo;
extern int16_t *g_FloorData;
extern int16_t *g_MeshBase;
extern int16_t **g_Meshes;
extern OBJECT_INFO g_Objects[O_NUMBER_OF];
extern STATIC_INFO g_StaticObjects[STATIC_NUMBER_OF];
extern uint8_t *g_TexturePagePtrs[MAX_TEXTPAGES];
extern int16_t g_RoomCount;
extern int32_t g_LevelItemCount;
extern int32_t g_NumberBoxes;
extern BOX_INFO *g_Boxes;
extern uint16_t *g_Overlap;
extern int16_t *g_GroundZone[2];
extern int16_t *g_GroundZone2[2];
extern int16_t *g_FlyZone[2];
extern ANIM_STRUCT *g_Anims;
extern ANIM_CHANGE_STRUCT *g_AnimChanges;
extern ANIM_RANGE_STRUCT *g_AnimRanges;
extern int16_t *g_AnimTextureRanges;
extern int16_t *g_AnimCommands;
extern int32_t *g_AnimBones;
extern int16_t *g_AnimFrames;
extern int16_t *g_Cine;
extern int16_t g_NumCineFrames;
extern int16_t g_CineFrame;
extern PHD_3DPOS g_CinePosition;
extern int32_t g_NumberCameras;
extern int32_t g_NumberSoundEffects;
extern OBJECT_VECTOR *g_SoundEffectsTable;
extern int16_t g_RoomsToDraw[MAX_ROOMS_TO_DRAW];
extern int16_t g_RoomsToDrawCount;

extern REQUEST_INFO g_SavegameRequester;

extern int16_t g_InvMode;
extern int32_t g_InvExtraData[8];
extern int16_t g_InvChosen;

extern RESOLUTION g_AvailableResolutions[RESOLUTIONS_SIZE];
extern float g_Difficulty_Presets[MAX_DIFFICULTY_PRESETS];
