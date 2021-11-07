#ifndef T1M_GLOBAL_VARS_H
#define T1M_GLOBAL_VARS_H

#include "global/const.h"
#include "global/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern const char *T1MVersion;

extern int32_t PhdWinMaxX;
extern int32_t PhdWinMaxY;
extern int32_t PhdWinCenterX;
extern int32_t PhdWinCenterY;
extern int32_t PhdViewDist;
extern int32_t PhdPersp;
extern int32_t PhdFarZ;
extern int32_t PhdNearZ;
extern int32_t PhdLeft;
extern int32_t PhdBottom;
extern int32_t PhdRight;
extern int32_t PhdTop;
extern int32_t PhdWinWidth;
extern int32_t PhdWinHeight;
extern PHD_VBUF PhdVBuf[1500];
extern PHD_SPRITE PhdSpriteInfo[MAX_SPRITES];
extern PHD_TEXTURE PhdTextureInfo[MAX_TEXTURES];
extern PHD_MATRIX *PhdMatrixPtr;
extern PHD_MATRIX W2VMatrix;

extern int32_t WibbleOffset;
extern int32_t WibbleTable[WIBBLE_SIZE];
extern int32_t ShadeTable[WIBBLE_SIZE];
extern int32_t RandTable[WIBBLE_SIZE];
extern int32_t LsAdder;
extern int32_t LsDivider;
extern SHADOW_INFO ShadowInfo;

extern RGB888 GamePalette[256];
extern bool ModeLock;

extern int32_t NoInputCount;
extern bool IDelay;
extern int32_t IDCount;
extern INPUT_STATE Input;
extern INPUT_STATE InputDB;
extern int32_t OptionSelected;

extern bool SoundIsActive;
extern int16_t SampleLUT[MAX_SAMPLES];
extern SAMPLE_INFO *SampleInfos;
extern int16_t MusicTrack;
extern bool MusicLoop;
extern uint16_t MusicTrackFlags[MAX_CD_TRACKS];
extern int32_t MnSoundMasterVolume;

extern TEXTSTRING *VersionText;
extern TEXTSTRING *AmmoText;
extern TEXTSTRING *FPSText;
extern int32_t FPSCounter;

extern void (*EffectRoutines[])(ITEM_INFO *item);

extern GAMEFLOW GF;
extern LARA_INFO Lara;
extern ITEM_INFO *LaraItem;
extern CAMERA_INFO Camera;
extern SAVEGAME_INFO SaveGame;
extern int32_t SavedGamesCount;
extern int32_t SaveCounter;
extern int32_t CurrentLevel;
extern uint32_t *DemoData;
extern bool LevelComplete;
extern bool ResetFlag;
extern bool ChunkyFlag;
extern int32_t OverlayFlag;
extern int32_t HeightType;

extern ROOM_INFO *RoomInfo;
extern int16_t *FloorData;
extern int16_t *MeshBase;
extern int16_t **Meshes;
extern OBJECT_INFO Objects[O_NUMBER_OF];
extern STATIC_INFO StaticObjects[STATIC_NUMBER_OF];
extern int8_t *TexturePagePtrs[MAX_TEXTPAGES];
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
extern ANIM_STRUCT *Anims;
extern ANIM_CHANGE_STRUCT *AnimChanges;
extern ANIM_RANGE_STRUCT *AnimRanges;
extern int16_t *AnimTextureRanges;
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
extern int16_t RoomsToDraw[MAX_ROOMS_TO_DRAW];
extern int16_t RoomsToDrawCount;
extern bool IsWibbleEffect;
extern bool IsWaterEffect;
extern bool IsShadeEffect;

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

extern int32_t HealthBarTimer;
extern int16_t StoredLaraHealth;

extern int16_t InvMode;
extern int32_t InvExtraData[8];
extern int16_t InvChosen;

extern HWR_Resolution AvailableResolutions[RESOLUTIONS_SIZE];

#endif
