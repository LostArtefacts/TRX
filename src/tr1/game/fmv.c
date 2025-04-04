#include "game/fmv.h"

#include "game/input.h"
#include "game/music.h"
#include "game/output.h"
#include "game/screen.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/types.h"
#include "specific/s_shell.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/engine/audio.h>
#include <libtrx/engine/video.h>
#include <libtrx/filesystem.h>
#include <libtrx/gfx/context.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

static bool m_Muted = false;
static bool m_IsPlaying = false;
static const char *m_Extensions[] = {
    ".mp4", ".mkv", ".mpeg", ".avi", ".webm", ".rpl", nullptr,
};

static void *M_AllocateSurface(int32_t width, int32_t height, void *user_data);
static void M_DeallocateSurface(void *surface, void *user_data);
static void M_ClearSurface(void *surface, void *user_data);
static void M_RenderBegin(void *surface, void *user_data);
static void M_RenderEnd(void *surface, void *user_data);
static void *M_LockSurface(void *surface, void *user_data);
static void M_UnlockSurface(void *surface, void *user_data);
static void M_UploadSurface(void *surface, void *user_data);
static bool M_Play(const char *file_path);

static void *M_AllocateSurface(
    const int32_t width, const int32_t height, void *const user_data)
{
    GFX_2D_SURFACE_DESC surface_desc = {
        .width = width,
        .height = height,
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

static void M_RenderBegin(void *surface, void *const user_data)
{
    Output_BeginScene();
}

static void M_RenderEnd(void *surface, void *const user_data)
{
    Output_EndScene();
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
    GFX_2D_RENDERER *const renderer_2d = user_data;
    GFX_2D_SURFACE *const surface_ = surface;
    GFX_2D_Renderer_Upload(renderer_2d, &surface_->desc, surface_->buffer);
    GFX_2D_Renderer_Render(renderer_2d);
}

static bool M_Play(const char *const file_path)
{
    VIDEO *video = Video_Open(file_path);
    if (video == nullptr) {
        return false;
    }

    GFX_2D_RENDERER *renderer_2d = GFX_2D_Renderer_Create();
    Audio_Shutdown();

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
        Video_SetSurfaceSize(
            video, Screen_GetResWidth(), Screen_GetResHeight());
        Video_SetSurfacePixelFormat(video, AV_PIX_FMT_BGRA);
        GFX_Context_SetDisplaySize(Screen_GetResWidth(), Screen_GetResHeight());
        Video_PumpEvents(video);
        Shell_ProcessEvents();

        Input_Update();

        if (g_InputDB.menu_confirm || g_InputDB.menu_back
            || Shell_IsExiting()) {
            Video_Stop(video);
        }
    }
    Video_Close(video);

    Audio_Init();
    GFX_2D_Renderer_Destroy(renderer_2d);
    Output_ApplyRenderSettings();

    return true;
}

void FMV_Mute(void)
{
    m_Muted = true;
}

void FMV_Unmute(void)
{
    m_Muted = false;
}

bool FMV_IsPlaying(void)
{
    return m_IsPlaying;
}

bool FMV_Play(const char *path)
{
    Music_Stop();
    Sound_StopAll();

    if (!g_Config.gameplay.enable_fmv) {
        return true;
    }

    m_IsPlaying = true;
    char *final_path = File_GuessExtension(path, m_Extensions);
    const bool result = M_Play(final_path);
    Memory_FreePointer(&final_path);
    m_IsPlaying = false;
    return result;
}
