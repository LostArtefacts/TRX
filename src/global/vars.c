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
int32_t FPSCounter = 0;

void (*EffectRoutines[])(ITEM_INFO *item) = {
    Turn180,    DinoStomp, LaraNormal,    LaraBubbles,  FinishLevel,
    EarthQuake, Flood,     RaisingBlock,  Stairs2Slope, DropSand,
    PowerUp,    Explosion, LaraHandsFree, FxFlipMap,    LaraDrawRightGun,
    ChainBlock, Flicker,
};

bool SoundIsActive = true;
int16_t SampleLUT[MAX_SAMPLES] = { 0 };
SAMPLE_INFO *SampleInfos = NULL;
uint16_t MusicTrackFlags[MAX_CD_TRACKS] = { 0 };

int32_t NoInputCount = 0;
bool IDelay = false;
int32_t IDCount = 0;
INPUT_STATE Input = { 0 };
INPUT_STATE InputDB = { 0 };
int32_t OptionSelected = 0;

int32_t PhdPersp = 0;
int32_t PhdLeft = 0;
int32_t PhdBottom = 0;
int32_t PhdRight = 0;
int32_t PhdTop = 0;
PHD_VBUF PhdVBuf[1500] = { 0 };
PHD_SPRITE PhdSpriteInfo[MAX_SPRITES] = { 0 };
PHD_TEXTURE PhdTextureInfo[MAX_TEXTURES] = { 0 };
PHD_MATRIX *PhdMatrixPtr = NULL;
PHD_MATRIX W2VMatrix = { 0 };

int32_t WibbleOffset = 0;
int32_t WibbleTable[WIBBLE_SIZE] = { 0 };
int32_t ShadeTable[WIBBLE_SIZE] = { 0 };
int32_t RandTable[WIBBLE_SIZE] = { 0 };

RGB888 GamePalette[256] = { 0 };
bool ModeLock = false;

GAMEFLOW GF = { 0 };
LARA_INFO Lara = { 0 };
ITEM_INFO *LaraItem = NULL;
CAMERA_INFO Camera = { 0 };
SAVEGAME_INFO SaveGame = { 0 };
int32_t SavedGamesCount = 0;
int32_t SaveCounter = 0;
int32_t CurrentLevel = -1;
uint32_t *DemoData = NULL;
bool LevelComplete = false;
bool ResetFlag = false;
bool ChunkyFlag = false;
int32_t OverlayFlag = 0;
int32_t HeightType = 0;

int32_t HealthBarTimer = 0;
int16_t StoredLaraHealth = 0;

ROOM_INFO *RoomInfo = NULL;
int16_t *FloorData = NULL;
int16_t *MeshBase = NULL;
int16_t **Meshes = NULL;
OBJECT_INFO Objects[O_NUMBER_OF] = { 0 };
STATIC_INFO StaticObjects[STATIC_NUMBER_OF] = { 0 };
int8_t *TexturePagePtrs[MAX_TEXTPAGES] = { NULL };
int16_t RoomCount = 0;
int32_t LevelItemCount = 0;
ITEM_INFO *Items = NULL;
int16_t NextItemFree = -1;
int16_t NextItemActive = -1;
FX_INFO *Effects = NULL;
int16_t NextFxFree = -1;
int16_t NextFxActive = -1;
int32_t NumberBoxes = 0;
BOX_INFO *Boxes = NULL;
uint16_t *Overlap = NULL;
int16_t *GroundZone[2] = { NULL };
int16_t *GroundZone2[2] = { NULL };
int16_t *FlyZone[2] = { NULL };
ANIM_STRUCT *Anims = NULL;
ANIM_CHANGE_STRUCT *AnimChanges = NULL;
ANIM_RANGE_STRUCT *AnimRanges = NULL;
int16_t *AnimTextureRanges = NULL;
int16_t *AnimCommands = NULL;
int32_t *AnimBones = NULL;
int16_t *AnimFrames = NULL;
int16_t *Cine = NULL;
int16_t NumCineFrames = 0;
int16_t CineFrame = -1;
PHD_3DPOS CinePosition = { 0 };
int32_t NumberCameras = 0;
int32_t NumberSoundEffects = 0;
OBJECT_VECTOR *SoundEffectsTable = NULL;
int16_t RoomsToDraw[MAX_ROOMS_TO_DRAW] = { -1 };
int16_t RoomsToDrawCount = 0;
bool IsWibbleEffect = false;
bool IsWaterEffect = false;
bool IsShadeEffect = false;

int16_t *TriggerIndex = NULL;
int32_t FlipTimer = 0;
int32_t FlipEffect = -1;
int32_t FlipStatus = 0;
int32_t FlipMapTable[MAX_FLIP_MAPS] = { 0 };

int16_t InvMode = INV_TITLE_MODE;
int32_t InvExtraData[8] = { 0 };
int16_t InvChosen = -1;

int32_t LsAdder = 0;
int32_t LsDivider = 0;
SHADOW_INFO ShadowInfo = { 0 };

HWR_Resolution AvailableResolutions[RESOLUTIONS_SIZE] = {
    { 320, 200 },   { 512, 384 },   { 640, 480 },   { 800, 600 },
    { 1024, 768 },  { 1280, 720 },  { 1920, 1080 }, { 2560, 1440 },
    { 3840, 2160 }, { 4096, 2160 }, { 7680, 4320 }, { -1, -1 },
};
