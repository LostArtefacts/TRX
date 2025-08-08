#pragma once

#include "../game/viewport.h"
#include "./2d/2d_renderer.h"
#include "./3d/3d_renderer.h"
#include "./common.h"
#include "./renderer.h"

#include <stdint.h>

bool GFX_Context_Attach(void *window_handle);
void GFX_Context_Detach(void);

void GFX_Context_SetDisplayFilter(GFX_TEXTURE_FILTER filter);
bool GFX_Context_GetWireframeMode(void);
void GFX_Context_SetWireframeMode(bool enable);
void GFX_Context_SetLineWidth(int32_t line_width);
void GFX_Context_SetVSync(bool vsync);

void *GFX_Context_GetWindowHandle(void);

void GFX_Context_Clear(void);
void GFX_Context_SwapBuffers(void);
void GFX_Context_SetRendered(void);

void GFX_Context_SwitchToViewport(VIEWPORT_SPACE space);

void GFX_Context_ScheduleScreenshot(const char *path);
const char *GFX_Context_GetScheduledScreenshotPath(void);
void GFX_Context_ClearScheduledScreenshotPath(void);

GFX_CONFIG *GFX_Context_GetConfig(void);
