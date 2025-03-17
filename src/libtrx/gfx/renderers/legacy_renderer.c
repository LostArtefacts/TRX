#include "gfx/renderers/legacy_renderer.h"

#include "debug.h"
#include "gfx/context.h"
#include "gfx/gl/utils.h"
#include "gfx/screenshot.h"

#include <SDL2/SDL_video.h>

static void M_SwapBuffers(GFX_RENDERER *renderer);
static void M_GetScale(
    const GFX_RENDERER *renderer, float *out_scale_x, float *out_scale_y);

static void M_SwapBuffers(GFX_RENDERER *renderer)
{
    ASSERT(renderer != nullptr);

    GFX_Context_SwitchToWindowViewportAR();
    if (GFX_Context_GetScheduledScreenshotPath()) {
        GFX_Screenshot_CaptureToFile(GFX_Context_GetScheduledScreenshotPath());
        GFX_Context_ClearScheduledScreenshotPath();
    }

    SDL_GL_SwapWindow(GFX_Context_GetWindowHandle());

    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GFX_GL_CheckError();
}

static void M_GetScale(
    const GFX_RENDERER *renderer, float *const out_scale_x,
    float *const out_scale_y)
{
    const int32_t window_width = GFX_Context_GetWindowWidth();
    const int32_t window_height = GFX_Context_GetWindowHeight();
    *out_scale_x = window_width / (float)GFX_Context_GetDisplayWidth();
    *out_scale_y = window_height / (float)GFX_Context_GetDisplayHeight();
}

GFX_RENDERER g_GFX_Renderer_Legacy = {
    .priv = nullptr,
    .swap_buffers = &M_SwapBuffers,
    .init = nullptr,
    .reset = nullptr,
    .shutdown = nullptr,
    .get_scale = M_GetScale,
};
