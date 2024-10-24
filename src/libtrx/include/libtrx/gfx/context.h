#pragma once

#include "2d/2d_renderer.h"
#include "3d/3d_renderer.h"
#include "common.h"
#include "renderers/fbo_renderer.h"

#include <stdbool.h>
#include <stdint.h>

void GFX_Context_SetupEnvironment(void);
void GFX_Context_Attach(void *window_handle);
void GFX_Context_Detach(void);

void GFX_Context_SetDisplayFilter(GFX_TEXTURE_FILTER filter);
void GFX_Context_SetWireframeMode(bool enable);
void GFX_Context_SetLineWidth(int32_t line_width);
void GFX_Context_SetAnisotropyFilter(float value);
void GFX_Context_SetVSync(bool vsync);
void GFX_Context_SetWindowSize(int32_t width, int32_t height);
void GFX_Context_SetDisplaySize(int32_t width, int32_t height);
void GFX_Context_SetRenderingMode(GFX_RENDER_MODE target_mode);

void *GFX_Context_GetWindowHandle(void);
int32_t GFX_Context_GetDisplayWidth(void);
int32_t GFX_Context_GetDisplayHeight(void);

void GFX_Context_Clear(void);
void GFX_Context_SwapBuffers(void);
void GFX_Context_SetRendered(void);

void GFX_Context_SwitchToWindowViewport(void);
void GFX_Context_SwitchToWindowViewportAR(void);
void GFX_Context_SwitchToDisplayViewport(void);

void GFX_Context_ScheduleScreenshot(const char *path);
const char *GFX_Context_GetScheduledScreenshotPath(void);
void GFX_Context_ClearScheduledScreenshotPath(void);

GFX_2D_RENDERER *GFX_Context_GetRenderer2D(void);
GFX_3D_RENDERER *GFX_Context_GetRenderer3D(void);
