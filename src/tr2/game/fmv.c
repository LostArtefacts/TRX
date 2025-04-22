#include "game/fmv.h"

#include "game/input.h"
#include "game/music.h"
#include "game/render/common.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/engine/video.h>
#include <libtrx/filesystem.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <string.h>

static bool m_Muted = false;
static bool m_IsFMVPlaying = false;

static void *M_AllocateSurface(int32_t width, int32_t height, void *user_data);
static void M_DeallocateSurface(void *surface, void *user_data);
static void M_ClearSurface(void *surface, void *user_data);
static void M_RenderBegin(void *surface, void *user_data);
static void M_RenderEnd(void *surface, void *user_data);
static void *M_LockSurface(void *surface, void *user_data);
static void M_UnlockSurface(void *surface, void *user_data);
static void M_UploadSurface(void *surface, void *user_data);

static bool M_Play(const char *file_name);

static void *M_AllocateSurface(
    const int32_t width, const int32_t height, void *const user_data)
{
    GFX_2D_SURFACE_DESC surface_desc = {
        .width = width,
        .height = height,
        .tex_format =
            g_Config.rendering.render_mode == RM_SOFTWARE ? GL_RED : GL_BGRA,
        .tex_type = g_Config.rendering.render_mode == RM_SOFTWARE
            ? GL_UNSIGNED_BYTE
            : GL_UNSIGNED_INT_8_8_8_8_REV,
    };
    return GFX_2D_Surface_Create(&surface_desc);
}

static void M_DeallocateSurface(void *const surface, void *const user_data)
{
    GFX_2D_Surface_Free(surface);
}

static void M_ClearSurface(void *const surface, void *const user_data)
{
    ASSERT(surface != nullptr);
    GFX_2D_SURFACE *const surface_ = surface;
    memset(surface_->buffer, 0, surface_->desc.pitch * surface_->desc.height);
}

static void M_RenderBegin(void *const surface, void *const user_data)
{
    GFX_Context_Clear();
}

static void M_RenderEnd(void *const surface, void *const user_data)
{
    GFX_Context_SwapBuffers();
}

static void *M_LockSurface(void *const surface, void *const user_data)
{
    ASSERT(surface != nullptr);
    GFX_2D_SURFACE *const surface_ = surface;
    return surface_->buffer;
}

static void M_UnlockSurface(void *const surface, void *const user_data)
{
}

static void M_UploadSurface(void *const surface, void *const user_data)
{
    GFX_2D_RENDERER *renderer_2d = user_data;
    GFX_2D_SURFACE *surface_ = surface;
    GFX_2D_Renderer_Upload(renderer_2d, &surface_->desc, surface_->buffer);
    GFX_2D_Renderer_Render(renderer_2d);
}

static bool M_Play(const char *const file_name)
{
    VIDEO *const video = Video_Open(file_name);
    if (video == nullptr) {
        return true;
    }

    m_IsFMVPlaying = true;
    GFX_2D_RENDERER *const renderer_2d = GFX_2D_Renderer_Create();

    // Populate the palette with a palette corresponding to
    // AV_PIX_FMT_RGB8
    GFX_COLOR palette[256];
    for (int32_t i = 0; i < 256; i++) {
        GFX_COLOR *const col = &palette[i];
        col->r = 0x24 * (i >> 5);
        col->g = 0x24 * ((i >> 2) & 7);
        col->b = 0x55 * (i & 3);
    }

    Video_SetSurfaceAllocatorFunc(video, M_AllocateSurface, nullptr);
    Video_SetSurfaceDeallocatorFunc(video, M_DeallocateSurface, nullptr);
    Video_SetSurfaceClearFunc(video, M_ClearSurface, nullptr);
    Video_SetRenderBeginFunc(video, M_RenderBegin, nullptr);
    Video_SetRenderEndFunc(video, M_RenderEnd, nullptr);
    Video_SetSurfaceLockFunc(video, M_LockSurface, nullptr);
    Video_SetSurfaceUnlockFunc(video, M_UnlockSurface, nullptr);
    Video_SetSurfaceUploadFunc(video, M_UploadSurface, renderer_2d);

    Video_Start(video);
    while (video->is_playing) {
        Video_SetVolume(
            video,
            m_Muted
                ? 0
                : g_Config.audio.sound_volume / (float)Sound_GetMaxVolume());

        const SHELL_SIZE display_size = Shell_GetCurrentDisplaySize();
        Video_SetSurfaceSize(video, display_size.w, display_size.h);
        if (g_Config.rendering.render_mode == RM_SOFTWARE) {
            Video_SetSurfacePixelFormat(video, AV_PIX_FMT_RGB8);
            GFX_2D_Renderer_SetPalette(renderer_2d, palette);
        } else {
            Video_SetSurfacePixelFormat(video, AV_PIX_FMT_BGRA);
            GFX_2D_Renderer_SetPalette(renderer_2d, nullptr);
        }

        Video_PumpEvents(video);
        Shell_ProcessEvents();

        Input_Update();
        Shell_ProcessInput();
        if (g_InputDB.menu_back || g_InputDB.menu_confirm
            || Shell_IsExiting()) {
            Video_Stop(video);
            break;
        }
    }
    Video_Close(video);

    GFX_2D_Renderer_Destroy(renderer_2d);
    m_IsFMVPlaying = false;
    return true;
}

bool FMV_Play(const char *const file_name)
{
    Music_Stop();
    if (!g_Config.gameplay.enable_fmv) {
        return false;
    }

    const bool result = M_Play(file_name);
    if (!Shell_IsExiting()) {
        Render_Reset(RENDER_RESET_ALL);
    }
    return result;
}

bool FMV_IsPlaying(void)
{
    return m_IsFMVPlaying;
}
