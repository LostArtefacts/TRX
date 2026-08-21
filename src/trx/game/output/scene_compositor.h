#pragma once

#include <trx/game/output/scene_source.h>
#include <trx/gl/enum.h>

void SceneCompositor_Init(void);
void SceneCompositor_Shutdown(void);
void SceneCompositor_AddSource(const SCENE_SOURCE *source);
void SceneCompositor_BeginScene(void);
void SceneCompositor_EndScene(void);
void SceneCompositor_Flush(void);

// Renders the scene even where the frame would otherwise be skipped, so a
// capture reads back real content instead of an empty frame. A headless run
// draws nothing on its own.
void SceneCompositor_BeginCapture(void);
void SceneCompositor_EndCapture(void);

void SceneCompositor_AnimateTextures(void);
void SceneCompositor_SetSamplerFilter(TEXTURE_FILTER filter);
