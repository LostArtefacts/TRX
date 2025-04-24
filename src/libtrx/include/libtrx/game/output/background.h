#pragma once

#include "../../engine/image.h"

bool Output_LoadBackgroundFromFile(const char *path);
void Output_ReloadBackgroundImage(void);

extern bool Output_LoadBackgroundFromImage(const IMAGE *image);
extern void Output_LoadBackgroundFromObject(void);
extern void Output_UnloadBackground(void);

extern void Output_DrawBackground(void);

// TODO: make these functions private once output module is consolidated
char *Output_GetLastBackgroundPath(void);
void Output_ClearLastBackgroundPath(void);
