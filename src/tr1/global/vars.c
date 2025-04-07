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
GAME_INFO g_GameInfo = { .select_save_slot = -1 };
bool g_LevelComplete = false;
int32_t g_OverlayFlag = 0;

INVENTORY_MODE g_InvMode;

#ifndef MESON_BUILD
const char *g_TRXVersion = "TR1X (non-Docker build)";
#endif
