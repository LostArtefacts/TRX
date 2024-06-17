#include "gfx/renderers/legacy_renderer.h"

#include "gfx/context.h"
#include "gfx/gl/utils.h"
#include "gfx/screenshot.h"

#include <SDL2/SDL_video.h>
#include <assert.h>

static void GFX_Renderer_Legacy_SwapBuffers(GFX_Renderer *renderer);

static void GFX_Renderer_Legacy_SwapBuffers(GFX_Renderer *renderer)
{
    assert(renderer != NULL);

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

GFX_Renderer g_GFX_Renderer_Legacy = {
    .priv = NULL,
    .swap_buffers = &GFX_Renderer_Legacy_SwapBuffers,
    .init = NULL,
    .reset = NULL,
    .shutdown = NULL,
};
