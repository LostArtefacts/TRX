#ifndef T1M_GLOBAL_VARS_H
#define T1M_GLOBAL_VARS_H

#include "dynarray.h"
#include "global/const.h"
#include "global/types.h"
#include "inject_util.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// clang-format off
// TODO: decompile me!

#define FltWinTop               VAR_U_(0x0068F3B8, float)
#define FltWinLeft              VAR_U_(0x00695180, float)
#define FltWinBottom            VAR_U_(0x0068F3A4, float)
#define FltWinRight             VAR_U_(0x006E8040, float)
#define PhdWinMaxX              VAR_U_(0x006CAD00, int32_t)
#define PhdWinMaxY              VAR_U_(0x006CAD10, int32_t)
#define PhdWinCenterX           VAR_U_(0x0068F3AC, int32_t)
#define PhdWinCenterY           VAR_U_(0x0068F3B0, int32_t)
#define PhdViewDist             VAR_U_(0x006D4DE0, int32_t)
#define PhdScrWidth             VAR_U_(0x006D7600, int32_t)
#define PhdScrHeight            VAR_U_(0x006CAD08, int32_t)
#define PhdCenterX              VAR_U_(0x0068F3AC, int32_t)
#define PhdCenterY              VAR_U_(0x0068F3B0, int32_t)
#define PhdPersp                VAR_U_(0x0069518C, int32_t)
#define PhdFarZ                 VAR_U_(0x00695184, int32_t)
#define PhdNearZ                VAR_U_(0x006CAD04, int32_t)
#define PhdVBuf                 ARRAY_(0x006D7620, PHD_VBUF, [1500])
#define PhdWet                  VAR_U_(0x0045A300, int32_t)
#define PhdLeft                 VAR_U_(0x00695190, int32_t)
#define PhdBottom               VAR_U_(0x00695188, int32_t)
#define PhdRight                VAR_U_(0x006E80E0, int32_t)
#define PhdTop                  VAR_U_(0x0068D380, int32_t)
#define PhdWinWidth             VAR_U_(0x006CADD4, int32_t)
#define PhdWinHeight            VAR_U_(0x0068F3A8, int32_t)
#define PhdSpriteInfo           ARRAY_(0x0068D3A0, PHD_SPRITE, [MAX_SPRITES_PAGE * MAX_SPRITES])
#define PhdTextureInfo          ARRAY_(0x006CADE0, PHD_TEXTURE, [MAX_TEXTURES])
#define PhdMatrixPtr            VAR_U_(0x006CAD14, PHD_MATRIX*)
#define MatrixStack             ARRAY_(0x006D4E80, PHD_MATRIX, [MAX_MATRICES])
#define W2VMatrix               VAR_U_(0x006CADA0, PHD_MATRIX)
#define IMRate                  VAR_U_(0x00462BE0, int32_t)
#define IMFrac                  VAR_U_(0x00462570, int32_t)
#define IMMatrixPtr             VAR_U_(0x00462BE4, PHD_MATRIX*)
#define IMMatrixStack           ARRAY_(0x00461F40, PHD_MATRIX, [MAX_NESTED_MATRICES])

#define WibbleTable             ARRAY_(0x006E8060, int32_t, [WIBBLE_SIZE])
#define ShadeTable              ARRAY_(0x006D4E00, int32_t, [WIBBLE_SIZE])
#define RandTable               ARRAY_(0x006CAD20, int32_t, [WIBBLE_SIZE])
#define DepthQTable             ARRAY_(0x006D5600, uint8_t, [32][256])
#define GouraudTable            ARRAY_(0x0068B380, uint8_t, [256][32])
#define AnimTextureRanges       VAR_U_(0x0045E640, int16_t*)
#define GamePalette             ARRAY_(0x0045E660, RGB888, [256])
#define WaterPalette            ARRAY_(0x0045E33D, RGB888, [256])
#define WibbleOffset            VAR_U_(0x00459F08, int32_t)
#define IsWibbleEffect          VAR_U_(0x00459F0C, int32_t)
#define IsWaterEffect           VAR_U_(0x00459F10, int32_t)
#define IsShadeEffect           VAR_U_(0x00459F14, int32_t)
#define TexturePagePtrs         ARRAY_(0x006E7FC0, int8_t*, [MAX_TEXTPAGES])
#define ModeLock                VAR_I_(0x00459F70, int32_t, 0)

#define StringToShow            ARRAY_(0x00456AD0, char, [128])
#define RoomInfo                VAR_U_(0x00462BE8, ROOM_INFO*)
#define GameAllocMemPointer     VAR_U_(0x0045E32C, char*)
#define GameAllocMemUsed        VAR_U_(0x0045E330, uint32_t)
#define GameAllocMemFree        VAR_U_(0x0045E334, uint32_t)
#define GameMemoryPointer       VAR_U_(0x0045A034, char*)
#define GameMemorySize          VAR_U_(0x0045EEF8, uint32_t)
#define Rand1                   VAR_I_(0x00453CC0, int32_t, 0xD371F947)
#define Rand2                   VAR_I_(0x00453CC4, int32_t, 0xD371F947)
// clang-format on

extern int32_t NoInputCount;
extern int32_t IDelay;
extern int32_t IDCount;
extern INPUT_STATE Input;
extern INPUT_STATE InputDB;
extern int32_t KeyChange;
extern int32_t OptionSelected;

extern bool SoundIsActive;
extern int16_t SampleLUT[MAX_SAMPLES];
extern SAMPLE_INFO *SampleInfos;
extern SAMPLE_DATA **SampleData;
extern int32_t NumSampleData;
extern int16_t MusicTrack;
extern int16_t MusicTrackLooped;
extern bool MusicLoop;
extern uint16_t MusicTrackFlags[MAX_CD_TRACKS];
extern int32_t MnSoundMasterVolume;

extern const char *ATIUserSettingsPath;
extern const char *T1MUserSettingsPath;
extern const char *T1MGlobalSettingsPath;
extern const char *T1MGameflowPath;
extern const char *T1MGameflowGoldPath;

extern int8_t IsGameWindowActive;
extern double UITextScale;
extern double UIBarScale;
extern TEXTSTRING *AmmoText;
extern TEXTSTRING *FPSText;
extern int32_t FPSCounter;

extern void (*EffectRoutines[])(ITEM_INFO *item);

extern GAMEFLOW GF;
extern LARA_INFO Lara;
extern ITEM_INFO *LaraItem;
extern CAMERA_INFO Camera;
extern int32_t CameraUnderwater;
extern SAVEGAME_INFO SaveGame;
extern int32_t SavedGamesCount;
extern int32_t SaveCounter;
extern int32_t CurrentLevel;
extern int32_t DemoLevel;
extern uint32_t *DemoPtr;
extern int32_t DemoCount;
extern int32_t TitleLoaded;
extern int32_t LevelComplete;
extern int32_t ResetFlag;
extern int32_t OverlayFlag;
extern int32_t ChunkyFlag;
extern int32_t HeightType;

extern int16_t *FloorData;
extern int16_t *MeshBase;
extern int16_t **Meshes;
extern OBJECT_INFO Objects[O_NUMBER_OF];
extern STATIC_INFO StaticObjects[STATIC_NUMBER_OF];
extern int16_t RoomCount;
extern int32_t LevelItemCount;
extern ITEM_INFO *Items;
extern int16_t NextItemFree;
extern int16_t NextItemActive;
extern FX_INFO *Effects;
extern int16_t NextFxFree;
extern int16_t NextFxActive;
extern int32_t NumberBoxes;
extern BOX_INFO *Boxes;
extern uint16_t *Overlap;
extern int16_t *GroundZone[2];
extern int16_t *GroundZone2[2];
extern int16_t *FlyZone[2];
extern int32_t SlotsUsed;
extern CREATURE_INFO *BaddieSlots;
extern ANIM_STRUCT *Anims;
extern ANIM_CHANGE_STRUCT *AnimChanges;
extern ANIM_RANGE_STRUCT *AnimRanges;
extern int16_t *AnimCommands;
extern int32_t *AnimBones;
extern int16_t *AnimFrames;
extern int16_t *Cine;
extern int16_t NumCineFrames;
extern int16_t CineFrame;
extern PHD_3DPOS CinePosition;
extern int32_t NumberCameras;
extern int32_t NumberSoundEffects;
extern OBJECT_VECTOR *SoundEffectsTable;
extern DYNARRAY *RoomsToDraw; // array of int16_t

extern int16_t *TriggerIndex;
extern int32_t FlipTimer;
extern int32_t FlipEffect;
extern int32_t FlipStatus;
extern int32_t FlipMapTable[MAX_FLIP_MAPS];

extern int32_t MeshCount;
extern int32_t MeshPtrCount;
extern int32_t AnimCount;
extern int32_t AnimChangeCount;
extern int32_t AnimRangeCount;
extern int32_t AnimCommandCount;
extern int32_t AnimBoneCount;
extern int32_t AnimFrameCount;
extern int32_t ObjectCount;
extern int32_t StaticCount;
extern int32_t TextureCount;
extern int32_t FloorDataSize;
extern int32_t TexturePageCount;
extern int32_t AnimTextureRangeCount;
extern int32_t SpriteInfoCount;
extern int32_t SpriteCount;
extern int32_t OverlapCount;

extern REQUEST_INFO LoadSaveGameRequester;
extern REQUEST_INFO NewGameRequester;

extern int32_t HealthBarTimer;
extern int16_t StoredLaraHealth;

extern int16_t InvMode;
extern int32_t InvExtraData[];
extern int16_t InvChosen;
extern int8_t InvDisableResolutionSwitch;

extern int32_t LsAdder;
extern int32_t LsDivider;
extern SHADOW_INFO ShadowInfo;
extern int16_t BarOffsetY[6];

extern HWR_Resolution AvailableResolutions[RESOLUTIONS_SIZE];

#endif
