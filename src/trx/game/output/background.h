#pragma once

#include <trx/config/types.h>
#include <trx/engine/image.h>
#include <trx/game/output/types.h>

void Output_InitBackground(void);
void Output_ShutdownBackground(void);
bool Output_LoadBackgroundFromFile(const char *path);
void Output_ReloadBackgroundImage(void);

BACKGROUND_TYPE Output_GetBackgroundType(void);
bool Output_LoadBackgroundFromImage(const IMAGE *image);
void Output_LoadBackgroundFromObject(bool wave);
void Output_UnloadBackground(void);

void Output_DrawBackground(void);

// Scene-only snapshot background composited in SCENE_PASS_BACKGROUND.
void Output_Background_EnableSnapshot(bool enable);
bool Output_Background_IsSnapshotEnabled(void);
void Output_Background_CaptureSnapshotScene(void);
void Output_Background_SetOverlayOpacity(float opacity);

void Output_RefreshBackgroundScaling(void);
void Output_ClearLastBackgroundPath(void);
