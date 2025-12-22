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

void Output_Background_LoadMono(void);

// Scene-only snapshot background composited in SCENE_PASS_BACKGROUND.
void Output_Background_EnableSnapshot(bool enable);
bool Output_Background_IsSnapshotEnabled(void);
void Output_Background_CaptureSnapshotScene(void);
void Output_Background_CaptureSnapshotPresented(void);
void Output_Background_SetOverlayOpacity(float opacity);

// Captures the current geometry FBO into an internal texture and fades it out
// on top of the scene (SCENE_PASS_UI), underneath the UI viewport.
void Output_Background_BeginTransitionFadeOut(double duration);

void Output_RefreshBackgroundScaling(void);
void Output_ClearLastBackgroundPath(void);
