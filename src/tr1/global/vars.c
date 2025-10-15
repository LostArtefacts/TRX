#include "global/vars.h"

#ifndef MESON_BUILD
const char *g_TRXVersion = "TR1X (non-Docker build)";
#endif

int32_t g_TRVersion = TR_VERSION; // overriden at runtime when loading a level

int32_t g_OverlayFlag = 0;

int32_t g_PhdPersp = 0;
int32_t g_PhdLeft = 0;
int32_t g_PhdBottom = 0;
int32_t g_PhdRight = 0;
int32_t g_PhdTop = 0;

GAME_INFO g_GameInfo = { .select_save_slot = -1, .select_level_num = -1 };
