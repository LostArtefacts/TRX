#include "global/vars.h"

int32_t g_FPSCounter = 0;

int32_t g_OptionSelected = 0;

int32_t g_PhdPersp = 0;
int32_t g_PhdLeft = 0;
int32_t g_PhdBottom = 0;
int32_t g_PhdRight = 0;
int32_t g_PhdTop = 0;
float g_FltResZ;
float g_FltResZBuf;

LARA_INFO g_Lara = {};
ITEM *g_LaraItem = nullptr;
GAME_INFO g_GameInfo = {};
int32_t g_SavedGamesCount = 0;
int32_t g_SaveCounter = 0;
bool g_LevelComplete = false;
int32_t g_OverlayFlag = 0;
int32_t g_HeightType = 0;

int32_t g_NumberBoxes = 0;
BOX_INFO *g_Boxes = nullptr;
uint16_t *g_Overlap = nullptr;
int16_t *g_GroundZone[2] = { nullptr };
int16_t *g_GroundZone2[2] = { nullptr };
int16_t *g_FlyZone[2] = { nullptr };

INVENTORY_MODE g_InvMode;

#ifndef MESON_BUILD
const char *g_TRXVersion = "TR1X (non-Docker build)";
#endif
