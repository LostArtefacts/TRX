#include "global/vars.h"

#include "game/effect_routines/bubbles.h"
#include "game/effect_routines/chain_block.h"
#include "game/effect_routines/dino_stomp.h"
#include "game/effect_routines/earthquake.h"
#include "game/effect_routines/explosion.h"
#include "game/effect_routines/finish_level.h"
#include "game/effect_routines/flicker.h"
#include "game/effect_routines/flipmap.h"
#include "game/effect_routines/flood.h"
#include "game/effect_routines/lara_effects.h"
#include "game/effect_routines/powerup.h"
#include "game/effect_routines/raising_block.h"
#include "game/effect_routines/sand.h"
#include "game/effect_routines/stairs2slope.h"
#include "game/effect_routines/turn_180.h"

#include <stddef.h>

int32_t g_FPSCounter = 0;

void (*g_EffectRoutines[])(ITEM_INFO *item) = {
    FX_Turn180,       FX_DinoStomp,    FX_LaraNormal,
    FX_Bubbles,       FX_FinishLevel,  FX_Earthquake,
    FX_Flood,         FX_RaisingBlock, FX_Stairs2Slope,
    FX_DropSand,      FX_PowerUp,      FX_Explosion,
    FX_LaraHandsFree, FX_FlipMap,      FX_LaraDrawRightGun,
    FX_ChainBlock,    FX_Flicker,
};

int16_t g_SampleLUT[MAX_SAMPLES] = { 0 };
SAMPLE_INFO *g_SampleInfos = NULL;
uint16_t g_MusicTrackFlags[MAX_CD_TRACKS] = { 0 };

bool g_IDelay = false;
int32_t g_IDCount = 0;
int32_t g_OptionSelected = 0;

int32_t g_PhdPersp = 0;
int32_t g_PhdLeft = 0;
int32_t g_PhdBottom = 0;
int32_t g_PhdRight = 0;
int32_t g_PhdTop = 0;
PHD_SPRITE g_PhdSpriteInfo[MAX_SPRITES] = { 0 };
PHD_TEXTURE g_PhdTextureInfo[MAX_TEXTURES] = { 0 };
MATRIX *g_MatrixPtr = NULL;
MATRIX g_W2VMatrix = { 0 };

LARA_INFO g_Lara = { 0 };
ITEM_INFO *g_LaraItem = NULL;
CAMERA_INFO g_Camera = { 0 };
GAME_INFO g_GameInfo = {
    .override_gf_command = { .command = GF_PHASE_CONTINUE }, 0
};
int32_t g_SavedGamesCount = 0;
int32_t g_SaveCounter = 0;
int16_t g_CurrentLevel = -1;
uint32_t *g_DemoData = NULL;
bool g_LevelComplete = false;
bool g_ChunkyFlag = false;
int32_t g_OverlayFlag = 0;
int32_t g_HeightType = 0;

ROOM_INFO *g_RoomInfo = NULL;
int16_t *g_MeshBase = NULL;
int16_t **g_Meshes = NULL;
OBJECT_INFO g_Objects[O_NUMBER_OF] = { 0 };
STATIC_INFO g_StaticObjects[STATIC_NUMBER_OF] = { 0 };
uint8_t *g_TexturePagePtrs[MAX_TEXTPAGES] = { NULL };
int16_t g_RoomCount = 0;
int32_t g_LevelItemCount = 0;
int32_t g_NumberBoxes = 0;
BOX_INFO *g_Boxes = NULL;
uint16_t *g_Overlap = NULL;
int16_t *g_GroundZone[2] = { NULL };
int16_t *g_GroundZone2[2] = { NULL };
int16_t *g_FlyZone[2] = { NULL };
ANIM_STRUCT *g_Anims = NULL;
ANIM_CHANGE_STRUCT *g_AnimChanges = NULL;
ANIM_RANGE_STRUCT *g_AnimRanges = NULL;
TEXTURE_RANGE *g_AnimTextureRanges = NULL;
int16_t *g_AnimCommands = NULL;
int32_t *g_AnimBones = NULL;
FRAME_INFO *g_AnimFrames = NULL;
int32_t *g_AnimFrameMeshRots = NULL;
int16_t g_NumCineFrames = 0;
int16_t g_CineFrame = -1;
struct CINE_CAMERA *g_CineCamera = NULL;
struct CINE_POSITION g_CinePosition = { 0 };
int32_t g_NumberCameras = 0;
int32_t g_NumberSoundEffects = 0;
OBJECT_VECTOR *g_SoundEffectsTable = NULL;
int16_t g_RoomsToDraw[MAX_ROOMS_TO_DRAW] = { -1 };
int16_t g_RoomsToDrawCount = 0;

int16_t g_InvMode = INV_TITLE_MODE;

int32_t g_LsAdder = 0;
int32_t g_LsDivider = 0;
SHADOW_INFO g_ShadowInfo = { 0 };

#ifndef MESON_BUILD
const char *g_TR1XVersion = "TR1X (non-Docker build)";
#endif
