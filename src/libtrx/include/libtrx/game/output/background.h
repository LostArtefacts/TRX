#pragma once

#include "../../engine/image.h"
#include "./types.h"

void Output_InitBackground(void);
void Output_ShutdownBackground(void);
bool Output_LoadBackgroundFromFile(const char *path);
void Output_ReloadBackgroundImage(void);

BACKGROUND_TYPE Output_GetBackgroundType(void);
bool Output_LoadBackgroundFromImage(const IMAGE *image);
void Output_LoadBackgroundFromObject(void);
void Output_UnloadBackground(void);

void Output_DrawBackground(void);

// TODO: make these functions private once output module is consolidated
char *Output_GetLastBackgroundPath(void);
void Output_ClearLastBackgroundPath(void);
