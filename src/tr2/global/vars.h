#pragma once

#include "game/game_flow/types.h"
#include "global/types.h"

#include <libtrx/game/camera/vars.h>
#include <libtrx/gfx/context.h>

#include <SDL2/SDL.h>

extern const float g_RhwFactor;

extern SDL_Window *g_SDLWindow;

extern uint32_t g_PerspectiveDistance;
extern uint32_t g_AssaultBestTime;
extern int32_t g_OverlayStatus;
extern bool g_GymInvOpenEnabled;
extern int32_t g_MidSort;
extern int32_t g_PhdWinTop;
extern int32_t g_LsAdder;
extern float g_FltWinBottom;
extern float g_FltResZBuf;
extern float g_FltResZ;
extern int32_t g_PhdWinHeight;
extern int32_t g_PhdWinCenterX;
extern int32_t g_PhdWinCenterY;
extern float g_FltWinTop;
extern SORT_ITEM g_SortBuffer[];
extern float g_FltWinLeft;
extern int32_t g_PhdFarZ;
extern float g_FltRhwOPersp;
extern int32_t g_PhdWinBottom;
extern int32_t g_PhdPersp;
extern int32_t g_PhdWinLeft;
extern int16_t g_Info3DBuffer[];
extern int32_t g_PhdWinMaxX;
extern int32_t g_PhdNearZ;
extern float g_FltResZORhw;
extern float g_FltFarZ;
extern float g_FltWinCenterX;
extern float g_FltWinCenterY;
extern float g_FltPerspONearZ;
extern float g_FltRhwONearZ;
extern int32_t g_PhdWinMaxY;
extern float g_FltNearZ;
extern float g_FltPersp;
extern int16_t *g_Info3DPtr;
extern int32_t g_PhdWinWidth;
extern int32_t g_PhdViewDistance;
extern int32_t g_LsDivider;
extern PHD_VBUF g_PhdVBuf[];
extern float g_FltWinRight;
extern XYZ_32 g_LsVectorView;
extern int32_t g_PhdWinRight;
extern int32_t g_SurfaceCount;
extern SORT_ITEM *g_Sort3DPtr;
extern int32_t g_WibbleOffset;
extern bool g_IsWibbleEffect;
extern bool g_IsWaterEffect;
extern bool g_IsShadeEffect;
extern int32_t g_FlipTimer;
extern bool g_IsDemoLoaded;
extern bool g_IsAssaultTimerDisplay;
extern bool g_IsAssaultTimerActive;
extern bool g_IsMonkAngry;
extern uint16_t g_SoundOptionLine;
extern ASSAULT_STATS g_Assault;
extern int32_t g_HealthBarTimer;
extern int32_t g_LevelComplete;
extern SAVEGAME_INFO g_SaveGame;
extern LARA_INFO g_Lara;
extern ITEM *g_LaraItem;
extern int32_t g_FlipStatus;
extern BOX_INFO *g_Boxes;
extern int16_t *g_FlyZone[2];
extern int16_t *g_GroundZone[][2];
extern uint16_t *g_Overlap;
extern CREATURE *g_BaddieSlots;
extern int32_t g_HeightType;
extern int32_t g_FlipMaps[MAX_FLIP_MAPS];
extern bool g_CameraUnderwater;
extern int32_t g_BoxCount;
extern char g_LevelFileName[256];
extern uint16_t g_MusicTrackFlags[64];
extern WEAPON_INFO g_Weapons[];
extern int16_t g_FinalBossActive;
extern int16_t g_FinalLevelCount;
extern int16_t g_FinalBossCount;
extern int16_t g_FinalBossItem[5];
extern REQUEST_INFO g_LoadGameRequester;
extern REQUEST_INFO g_SaveGameRequester;

extern bool g_GF_SunsetEnabled;
extern bool g_GF_RemoveAmmo;
extern bool g_GF_RemoveWeapons;
extern int16_t g_GF_NoFloor;
extern int16_t g_GF_NumSecrets;
extern int32_t g_GF_LaraStartAnim;

extern int32_t g_SavedGames;
extern TEXTSTRING *g_PasswordText1;
extern char g_ValidLevelStrings1[MAX_LEVELS][50];
extern char g_ValidLevelStrings2[MAX_LEVELS][50];
extern uint32_t g_RequesterFlags1[MAX_REQUESTER_ITEMS];
extern uint32_t g_RequesterFlags2[MAX_REQUESTER_ITEMS];
extern int32_t g_SaveCounter;
extern int16_t g_SavedLevels[MAX_LEVELS];
extern XYZ_32 g_InteractPosition;
extern bool g_DetonateAllMines;
extern int32_t g_SunsetTimer;
