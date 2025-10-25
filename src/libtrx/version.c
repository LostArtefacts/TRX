#include "version.h"

#ifndef MESON_BUILD
const char *g_TRXVersion = "TR1X (non-Docker build)";
#endif

int32_t g_TRVersion = TR_VERSION; // overriden at runtime when loading a level
