#ifndef T1M_GLOBAL_VARS_H
#define T1M_GLOBAL_VARS_H

#include "global/const.h"
#include "global/types.h"
#include "inject_util.h"

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

#define ScreenSizer             VAR_I_(0x00453558, double, 1.0)
#define GameSizer               VAR_I_(0x00453560, double, 1.0)
#define FadeValue               VAR_I_(0x00453568, int32_t, 0x100000)
#define FadeLimit               VAR_I_(0x0045356C, int32_t, 0x100000)
#define FadeAdder               VAR_I_(0x00453570, int32_t, 0x8000)
#define LsAdder                 VAR_U_(0x0068F3A0, int32_t)
#define LsYaw                   VAR_U_(0x0068F3B4, int32_t)
#define LsPitch                 VAR_U_(0x006D4DE4, int32_t)
#define LsVectorView            VAR_U_(0x006E8050, PHD_VECTOR)
#define LsDivider               VAR_U_(0x006D7604, int32_t)
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

extern int32_t NoInputCount;
extern int32_t IDelay;
extern int32_t IDCount;
extern int32_t Input;
extern int32_t InputDB;
extern int32_t KeyChange;

#define SoundIsActive           VAR_I_(0x00456688, int32_t, 1)
#define SoundInit1              VAR_U_(0x0045A31C, int32_t)
#define SoundInit2              VAR_U_(0x0045A320, int32_t)
#define SoundsLoaded            VAR_U_(0x0045A214, int32_t)
#define SampleLUT               ARRAY_(0x0045EB60, int16_t, [MAX_SAMPLES])
#define SampleInfos             VAR_U_(0x0045ED60, SAMPLE_INFO*)
#define SampleData              VAR_U_(0x0045B954, SAMPLE_DATA**)
#define NumSampleData           VAR_U_(0x0045B96C, int32_t)
#define NumSampleInfos          VAR_U_(0x0045A210, int32_t)
#define NumSamples              VAR_U_(0x0045E964, int32_t)
#define DecibelLUT              ARRAY_(0x0045F1E0, int32_t, [DECIBEL_LUT_SIZE])
#define CDTrack                 VAR_I_(0x004534DC, int16_t, -1)
#define CDTrackLooped           VAR_I_(0x0045B97C, int16_t, -1)
#define CDLoop                  VAR_U_(0x0045B94C, int32_t)
#define CDNumTracks             VAR_U_(0x0045B964, int32_t)
#define CDFlags                 ARRAY_(0x00462C20, int16_t, [MAX_CD_TRACKS])
#define MnSoundMasterVolume     VAR_U_(0x0045668C, int32_t)
#define MnSoundMasterVolumeDefault VAR_U_(0x00456690, int32_t)
#define SFXPlaying              ARRAY_(0x0045E980, MN_SFX_PLAY_INFO, [MAX_PLAYING_FX])
#define MnAmbientLookup         ARRAY_(0x0045A200, int16_t, [MAX_AMBIENT_FX])
#define MnSoundMasterFadeOn     VAR_U_(0x0045A21C, int32_t)
#define MnSoundFadeCounter      VAR_U_(0x0045A220, int32_t)
#define MnSoundFadeRate         VAR_I_(0x00456694, int32_t, 10)
#define MnSoundsPlaying         VAR_U_(0x0045A218, int32_t)
#define MnAmbientLookupIdx      VAR_U_(0x0045A224, int32_t)

#define IsHardwareRenderer      VAR_U_(0x00463610, int32_t)
#define IConfig                 VAR_U_(0x0045A23C, int32_t)
#define RenderSettings          VAR_U_(0x0045A240, uint32_t)
#define OldRenderSettings       VAR_U_(0x0045E960, uint32_t)
#define Quality                 VAR_I_(0x00453030, int32_t, 0x3000000)
#define ScanCodeNames           ARRAY_(0x00454A40, char*, [])
#define OptionMusicVolume       VAR_I_(0x00456334, int32_t, 255)
#define OptionSoundFXVolume     VAR_I_(0x00456330, int32_t, 165)
#define OptionGammaLevel        VAR_U_(0x0045A0A0, int16_t)

#define InvKeysCurrent          VAR_I_(0x0045A048, int16_t, 0)
#define InvKeysList             ARRAY_(0x00455FA8, INVENTORY_ITEM*, [23])
#define InvKeysObjects          VAR_I_(0x0045A044, int16_t, 1)
#define InvKeysQtys             ARRAY_(0x00455F78, int16_t, [])
#define InvMainCurrent          VAR_U_(0x0045A040, int16_t)
#define InvMainList             ARRAY_(0x00455F18, INVENTORY_ITEM*, [23])
#define InvMainObjects          VAR_I_(0x00455EE0, int16_t, 1)
#define InvMainQtys             ARRAY_(0x00455EE8, int16_t, [])
#define InvOptionCurrent        VAR_U_(0x0045A04C, int16_t)
#define InvOptionList           ARRAY_(0x00456008, INVENTORY_ITEM*, [5])
#define InvOptionObjects        VAR_I_(0x00456004, int16_t, 5)
#define InvColours              ARRAY_(0x0045EE80, int16_t, [IC_NUMBER_OF])
#define InvNFrames              VAR_U_(0x004550A0, int32_t)
#define CompassStatus           VAR_U_(0x0045A0A4, int16_t)
#define CompassSpeed            VAR_U_(0x0045A0A8, int16_t)
#define CompassNeedle           VAR_U_(0x0045A0AC, int16_t)
#define Item_Data               VAR_U_(0x0045A0B8, int16_t)

#define InvItemCompass          VAR_U_(0x00455760, INVENTORY_ITEM)
#define InvItemMedi             VAR_U_(0x004559E0, INVENTORY_ITEM)
#define InvItemBigMedi          VAR_U_(0x00455A20, INVENTORY_ITEM)
#define InvItemLeadBar          VAR_U_(0x00455A60, INVENTORY_ITEM)
#define InvItemPickup1          VAR_U_(0x00455AA0, INVENTORY_ITEM)
#define InvItemPickup2          VAR_U_(0x00455AE0, INVENTORY_ITEM)
#define InvItemScion            VAR_U_(0x00455B20, INVENTORY_ITEM)
#define InvItemPuzzle1          VAR_U_(0x00455B60, INVENTORY_ITEM)
#define InvItemPuzzle2          VAR_U_(0x00455BA0, INVENTORY_ITEM)
#define InvItemPuzzle3          VAR_U_(0x00455BE0, INVENTORY_ITEM)
#define InvItemPuzzle4          VAR_U_(0x00455C20, INVENTORY_ITEM)
#define InvItemKey1             VAR_U_(0x00455C60, INVENTORY_ITEM)
#define InvItemKey2             VAR_U_(0x00455CA0, INVENTORY_ITEM)
#define InvItemKey3             VAR_U_(0x00455CE0, INVENTORY_ITEM)
#define InvItemKey4             VAR_U_(0x00455D20, INVENTORY_ITEM)
#define InvItemPistols          VAR_U_(0x004557A0, INVENTORY_ITEM)
#define InvItemShotgun          VAR_U_(0x004557E0, INVENTORY_ITEM)
#define InvItemMagnum           VAR_U_(0x00455820, INVENTORY_ITEM)
#define InvItemUzi              VAR_U_(0x00455860, INVENTORY_ITEM)
#define InvItemGrenade          VAR_U_(0x004559A0, INVENTORY_ITEM)
#define InvItemPistolAmmo       VAR_U_(0x004558A0, INVENTORY_ITEM)
#define InvItemShotgunAmmo      VAR_U_(0x004558E0, INVENTORY_ITEM)
#define InvItemMagnumAmmo       VAR_U_(0x00455920, INVENTORY_ITEM)
#define InvItemUziAmmo          VAR_U_(0x00455960, INVENTORY_ITEM)
#define InvItemGame             VAR_U_(0x00455D60, INVENTORY_ITEM)
#define InvItemDetails          VAR_U_(0x00455DA0, INVENTORY_ITEM)
#define InvItemSound            VAR_U_(0x00455DE0, INVENTORY_ITEM)
#define InvItemControls         VAR_U_(0x00455E20, INVENTORY_ITEM)
#define InvItemGamma            VAR_U_(0x00455E60, INVENTORY_ITEM)
#define InvItemLarasHome        VAR_U_(0x00455EA0, INVENTORY_ITEM)
#define InvSprGammaLevel        ARRAY_(0x00455218, INVENTORY_SPRITE, [10])
#define InvSprGammaList         ARRAY_(0x004552F8, INVENTORY_SPRITE*, [])

#define ReqBgndMoreUp           VAR_U_(0x004550A8, SG_COL*)
#define ReqBgndMoreDown         VAR_U_(0x004550C8, SG_COL*)
#define ReqBgndGour1            VAR_U_(0x004550E8, SG_COL*)
#define ReqBgndGour2            VAR_U_(0x00455108, SG_COL*)
#define ReqMainGour1            VAR_U_(0x00455120, SG_COL*)
#define ReqMainGour2            VAR_U_(0x00455140, SG_COL*)
#define ReqUnselGour1           VAR_U_(0x00455190, SG_COL*)
#define ReqUnselGour2           VAR_U_(0x00455178, SG_COL*)
// clang-format on

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
extern int16_t RoomsToDraw[MAX_ROOMS_TO_DRAW];
extern int32_t RoomsToDrawNum;
extern int32_t NumberCameras;
extern int32_t NumberSoundEffects;
extern OBJECT_VECTOR *SoundEffectsTable;
extern SHADOW_INFO ShadowInfo;

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

extern int16_t BarOffsetY[6];

extern HWR_Resolution AvailableResolutions[RESOLUTIONS_SIZE];

#endif
