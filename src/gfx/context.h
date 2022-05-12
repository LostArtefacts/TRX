#pragma once

#include "gfx/2d/2d_renderer.h"
#include "gfx/3d/3d_renderer.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct GFX_Context GFX_Context;

void GFX_Context_Attach(HWND hwnd);
void GFX_Context_Detach(void);
void GFX_Context_SetVSync(bool vsync);
bool GFX_Context_IsFullscreen(void);
void GFX_Context_SetFullscreen(bool fullscreen);
void GFX_Context_SetWindowSize(int32_t width, int32_t height);
void GFX_Context_SetDisplaySize(int32_t width, int32_t height);
int32_t GFX_Context_GetDisplayWidth(void);
int32_t GFX_Context_GetDisplayHeight(void);
int32_t GFX_Context_GetWindowWidth(void);
int32_t GFX_Context_GetWindowHeight(void);
int32_t GFX_Context_GetScreenWidth(void);
int32_t GFX_Context_GetScreenHeight(void);
void GFX_Context_SetupViewport(void);
void GFX_Context_SwapBuffers(void);
void GFX_Context_SetRendered(void);
bool GFX_Context_IsRendered(void);
HWND GFX_Context_GetHWnd(void);
void GFX_Context_ScheduleScreenshot(const char *path);
GFX_2D_Renderer *GFX_Context_GetRenderer2D(void);
GFX_3D_Renderer *GFX_Context_GetRenderer3D(void);
