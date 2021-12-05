#pragma once

#ifdef __cplusplus
    #include <cstdbool>
    #include <cstdint>
extern "C" {
#else
    #include <stdbool.h>
    #include <stdint.h>
#endif
#include <windows.h>

#include "gfx/2d/2d_renderer.h"

typedef struct GFX_Context GFX_Context;

void GFX_Context_Attach(HWND hwnd);
void GFX_Context_Detach();
bool GFX_Context_IsFullscreen();
void GFX_Context_SetFullscreen(bool fullscreen);
void GFX_Context_SetWindowSize(int32_t width, int32_t height);
void GFX_Context_SetDisplaySize(int32_t width, int32_t height);
int32_t GFX_Context_GetDisplayWidth();
int32_t GFX_Context_GetDisplayHeight();
int32_t GFX_Context_GetWindowWidth();
int32_t GFX_Context_GetWindowHeight();
int32_t GFX_Context_GetScreenWidth();
int32_t GFX_Context_GetScreenHeight();
void GFX_Context_SetupViewport();
void GFX_Context_SwapBuffers();
void GFX_Context_SetRendered();
bool GFX_Context_IsRendered();
HWND GFX_Context_GetHWnd();
void GFX_Context_ScheduleScreenshot(const char *path);
GFX_2D_Renderer *GFX_Context_GetRenderer2D();

#ifdef __cplusplus
}
#endif
