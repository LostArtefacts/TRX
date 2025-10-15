#include "global/vars.h"

#ifndef MESON_BUILD
const char *g_TRXVersion = "TR2X (non-Docker build)";
#endif

int32_t g_TRVersion = TR_VERSION; // overriden at runtime when loading a level

int32_t g_OverlayFlag = 1;

int32_t g_PhdPersp = 0;
int32_t g_PhdTop = 0;
int32_t g_PhdLeft = 0;
int32_t g_PhdRight = 0;
int32_t g_PhdBottom = 0;
