#include <trx/game/fmv.h>

#include <trx/av/audio.h>
#include <trx/av/video.h>
#include <trx/config.h>
#include <trx/debug.h>
#include <trx/filesystem.h>
#include <trx/game/console.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/music.h>
#include <trx/game/output.h>
#include <trx/game/shell.h>
#include <trx/game/sound.h>
#include <trx/game/ui.h>
#include <trx/game/viewport.h>
#include <trx/gl/context.h>
#include <trx/log.h>
#include <trx/strings.h>

#include <string.h>

static bool m_IsPlaying = false;

static void *M_AllocateSurface(
    const int32_t width, const int32_t height, void *const user_data)
{
    TRX_GL_2D_SURFACE_DESC surface_desc = {
        .width = width,
        .height = height,
        .tex_format = GL_BGRA,
        .tex_type = GL_UNSIGNED_INT_8_8_8_8_REV,
    };
    return TRX_GL_2D_Surface_Create(&surface_desc);
}

static void M_DeallocateSurface(void *const surface, void *const user_data)
{
    TRX_GL_2D_Surface_Free(surface);
}

static void M_ClearSurface(void *const surface, void *const user_data)
{
    ASSERT(surface != nullptr);
    TRX_GL_2D_SURFACE *const surface_ = surface;
    memset(surface_->buffer, 0, surface_->desc.pitch * surface_->desc.height);
}

static void M_RenderBegin(void *const surface, void *const user_data)
{
    Output_BeginScene();
}

static void M_RenderEnd(void *const surface, void *const user_data)
{
    Output_EndScene();
    Output_FlipScreen();
}

static void *M_LockSurface(void *const surface, void *const user_data)
{
    ASSERT(surface != nullptr);
    TRX_GL_2D_SURFACE *const surface_ = surface;
    return surface_->buffer;
}

static void M_UnlockSurface(void *const surface, void *const user_data)
{
}

static void M_UploadSurface(void *const surface, void *const user_data)
{
    TRX_GL_2D_RENDERER *const renderer_2d = user_data;
    TRX_GL_2D_SURFACE *const surface_ = surface;
    TRX_GL_2D_Renderer_Upload(renderer_2d, &surface_->desc, surface_->buffer);
    TRX_GL_2D_Renderer_Render(renderer_2d);

    Output_SwitchViewport(VIEWPORT_UI);
    UI_BeginScene();
    Console_Draw();
    Console_Control();
    Console_Control();
    UI_EndScene();
    UI_Draw();
}

static bool M_Play(const char *const file_name)
{
    if (file_name == nullptr || String_IsEmpty(file_name)) {
        LOG_ERROR("Cannot play FMV: empty file path");
        return false;
    }

    VIDEO *const video = Video_Open(file_name);
    if (video == nullptr) {
        return false;
    }

    TRX_GL_2D_RENDERER *renderer_2d = TRX_GL_2D_Renderer_Create();

    Video_SetSurfaceAllocatorFunc(video, M_AllocateSurface, nullptr);
    Video_SetSurfaceDeallocatorFunc(video, M_DeallocateSurface, nullptr);
    Video_SetSurfaceClearFunc(video, M_ClearSurface, nullptr);
    Video_SetRenderBeginFunc(video, M_RenderBegin, nullptr);
    Video_SetRenderEndFunc(video, M_RenderEnd, nullptr);
    Video_SetSurfaceLockFunc(video, M_LockSurface, nullptr);
    Video_SetSurfaceUnlockFunc(video, M_UnlockSurface, nullptr);
    Video_SetSurfaceUploadFunc(video, M_UploadSurface, renderer_2d);

    g_OldInputDB = g_Input;
    Video_Start(video);
    while (video->is_playing) {
        Shell_ProcessEvents();
        Video_SetVolume(
            video,
            Audio_IsMuted()
                ? 0.0f
                : g_Config.audio.master_volume * g_Config.audio.fmv_volume);
        Video_SetSurfaceSize(
            video, Viewport_GetWidth(VIEWPORT_GAME),
            Viewport_GetHeight(VIEWPORT_GAME));
        Video_SetSurfacePixelFormat(video, AV_PIX_FMT_BGRA);

        Video_PumpEvents(video);

        Input_Update();
        Shell_ProcessInput();
        if (g_InputDB.menu_back || g_InputDB.menu_confirm
            || GF_GetOverrideCommand().action != GF_NOOP || Shell_IsExiting()) {
            Video_Stop(video);
            break;
        }
    }
    Video_Close(video);

    TRX_GL_2D_Renderer_Destroy(renderer_2d);
    Output_ApplyRenderSettings();
    return true;
}

bool FMV_Play(const char *const file_path)
{
    Music_Stop();
    Sound_StopAll();

    if (!g_Config.gameplay.enable_fmv) {
        return false;
    }

    m_IsPlaying = true;
    const bool result = M_Play(file_path);
    m_IsPlaying = false;
    return result;
}

bool FMV_IsPlaying(void)
{
    return m_IsPlaying;
}
