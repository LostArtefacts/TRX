#include "game/effects/bubble.h"
#include "game/effects/chain_block.h"
#include "game/effects/dino_stomp.h"
#include "game/effects/earthquake.h"
#include "game/effects/explosion.h"
#include "game/effects/finish_level.h"
#include "game/effects/flicker.h"
#include "game/effects/flipmap.h"
#include "game/effects/flood.h"
#include "game/effects/lara_effects.h"
#include "game/effects/powerup.h"
#include "game/effects/raising_block.h"
#include "game/effects/sand.h"
#include "game/effects/stairs2slope.h"
#include "game/effects/turn_180.h"
#include "global/vars.h"

char *GameMemoryPointer = NULL;
int32_t g_FPSCounter = 0;

void (*g_EffectRoutines[])(ITEM_INFO *item) = {
    Turn180,    DinoStomp, LaraNormal,    LaraBubbles,  FinishLevel,
    EarthQuake, Flood,     RaisingBlock,  Stairs2Slope, DropSand,
    PowerUp,    Explosion, LaraHandsFree, FxFlipMap,    LaraDrawRightGun,
    ChainBlock, Flicker,
};

int16_t g_SampleLUT[MAX_SAMPLES] = { 0 };
SAMPLE_INFO *g_SampleInfos = NULL;
uint16_t g_MusicTrackFlags[MAX_CD_TRACKS] = { 0 };

int32_t g_NoInputCount = 0;
bool g_IDelay = false;
int32_t g_IDCount = 0;
int32_t g_OptionSelected = 0;

int32_t g_PhdPersp = 0;
int32_t g_PhdLeft = 0;
int32_t g_PhdBottom = 0;
int32_t g_PhdRight = 0;
int32_t g_PhdTop = 0;
PHD_VBUF g_PhdVBuf[1500] = { 0 };
PHD_SPRITE g_PhdSpriteInfo[MAX_SPRITES] = { 0 };
PHD_TEXTURE g_PhdTextureInfo[MAX_TEXTURES] = { 0 };
PHD_MATRIX *g_PhdMatrixPtr = NULL;
PHD_MATRIX g_W2VMatrix = { 0 };

int32_t g_WibbleOffset = 0;
int32_t g_WibbleTable[WIBBLE_SIZE] = { 0 };
int32_t g_ShadeTable[WIBBLE_SIZE] = { 0 };
int32_t g_RandTable[WIBBLE_SIZE] = { 0 };

RGB888 g_GamePalette[256] = { 0 };
bool g_ModeLock = false;

LARA_INFO g_Lara = { 0 };
ITEM_INFO *g_LaraItem = NULL;
CAMERA_INFO g_Camera = { 0 };
SAVEGAME_INFO g_SaveGame = { 0 };
int32_t g_SavedGamesCount = 0;
int32_t g_SaveCounter = 0;
int32_t g_CurrentLevel = -1;
uint32_t *g_DemoData = NULL;
bool g_LevelComplete = false;
bool g_ResetFlag = false;
bool g_ChunkyFlag = false;
int32_t g_OverlayFlag = 0;
int32_t g_HeightType = 0;

int32_t g_HealthBarTimer = 0;
int16_t g_StoredLaraHealth = 0;

ROOM_INFO *g_RoomInfo = NULL;
int16_t *g_FloorData = NULL;
int16_t *g_MeshBase = NULL;
int16_t **g_Meshes = NULL;
OBJECT_INFO g_Objects[O_NUMBER_OF] = { 0 };
STATIC_INFO g_StaticObjects[STATIC_NUMBER_OF] = { 0 };
int8_t *g_TexturePagePtrs[MAX_TEXTPAGES] = { NULL };
int16_t g_RoomCount = 0;
int32_t g_LevelItemCount = 0;
ITEM_INFO *g_Items = NULL;
int16_t g_NextItemFree = -1;
int16_t g_NextItemActive = -1;
FX_INFO *g_Effects = NULL;
int16_t g_NextFxFree = -1;
int16_t g_NextFxActive = -1;
int32_t g_NumberBoxes = 0;
BOX_INFO *g_Boxes = NULL;
uint16_t *g_Overlap = NULL;
int16_t *g_GroundZone[2] = { NULL };
int16_t *g_GroundZone2[2] = { NULL };
int16_t *g_FlyZone[2] = { NULL };
ANIM_STRUCT *g_Anims = NULL;
ANIM_CHANGE_STRUCT *g_AnimChanges = NULL;
ANIM_RANGE_STRUCT *g_AnimRanges = NULL;
int16_t *g_AnimTextureRanges = NULL;
int16_t *g_AnimCommands = NULL;
int32_t *g_AnimBones = NULL;
int16_t *g_AnimFrames = NULL;
int16_t *g_Cine = NULL;
int16_t g_NumCineFrames = 0;
int16_t g_CineFrame = -1;
PHD_3DPOS g_CinePosition = { 0 };
int32_t g_NumberCameras = 0;
int32_t g_NumberSoundEffects = 0;
OBJECT_VECTOR *g_SoundEffectsTable = NULL;
int16_t g_RoomsToDraw[MAX_ROOMS_TO_DRAW] = { -1 };
int16_t g_RoomsToDrawCount = 0;
bool g_IsWibbleEffect = false;
bool g_IsWaterEffect = false;
bool g_IsShadeEffect = false;

int16_t *g_TriggerIndex = NULL;
int32_t g_FlipTimer = 0;
int32_t g_FlipEffect = -1;
int32_t g_FlipStatus = 0;
int32_t g_FlipMapTable[MAX_FLIP_MAPS] = { 0 };

int16_t g_InvMode = INV_TITLE_MODE;
int32_t g_InvExtraData[8] = { 0 };
int16_t g_InvChosen = -1;

int32_t g_LsAdder = 0;
int32_t g_LsDivider = 0;
SHADOW_INFO g_ShadowInfo = { 0 };

HWR_Resolution g_AvailableResolutions[RESOLUTIONS_SIZE] = {
    { 320, 200 },   { 512, 384 },   { 640, 480 },   { 800, 600 },
    { 1024, 768 },  { 1280, 720 },  { 1920, 1080 }, { 2560, 1440 },
    { 3840, 2160 }, { 4096, 2160 }, { 7680, 4320 }, { -1, -1 },
};
