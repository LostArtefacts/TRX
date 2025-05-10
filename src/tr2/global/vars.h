#pragma once

#include "game/game_flow/types.h"
#include "global/types.h"

#include <libtrx/game/camera/vars.h>
#include <libtrx/gfx/context.h>

#include <SDL2/SDL.h>

extern const float g_RhwFactor;

extern SDL_Window *g_SDLWindow;

extern uint32_t g_PerspectiveDistance;
extern int32_t g_OverlayStatus;
extern int32_t g_MidSort;
extern int32_t g_PhdWinTop;
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
extern PHD_VBUF *g_PhdVBuf;
extern float g_FltWinRight;
extern int32_t g_PhdWinRight;
extern int32_t g_SurfaceCount;
extern SORT_ITEM *g_Sort3DPtr;
extern uint16_t g_SoundOptionLine;
extern LARA_INFO g_Lara;
extern ITEM *g_LaraItem;
extern CREATURE *g_BaddieSlots;
extern WEAPON_INFO g_Weapons[];
extern int16_t g_FinalBossActive;
extern uint16_t g_FinalLevelCount;
extern int16_t g_FinalBossCount;
extern int16_t g_FinalBossItem[5];

extern bool g_GF_RemoveAmmo;
extern bool g_GF_RemoveWeapons;
extern int32_t g_GF_LaraStartAnim;

extern XYZ_32 g_InteractPosition;
extern bool g_DetonateAllMines;
