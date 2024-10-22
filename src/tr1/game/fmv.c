#include "game/fmv.h"

#include "config.h"
#include "game/input.h"
#include "game/music.h"
#include "game/screen.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/types.h"
#include "specific/s_output.h"
#include "specific/s_shell.h"

#include <libtrx/engine/audio.h>
#include <libtrx/engine/video.h>
#include <libtrx/filesystem.h>
#include <libtrx/gfx/context.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <assert.h>
#include <stddef.h>

static bool m_Muted = false;
static bool m_IsPlaying = false;
static const char *m_Extensions[] = {
    ".mp4", ".mkv", ".mpeg", ".avi", ".webm", ".rpl", NULL,
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
    GFX_2D_SURFACE_DESC surface_desc = { 0 };
    bool result = GFX_2D_Surface_Lock(surface, &surface_desc);
    if (!result) {
        return;
    }
    memset(surface_desc.pixels, 0, surface_desc.pitch * surface_desc.height);
    GFX_2D_Surface_Unlock(surface);
}

static void M_RenderBegin(void *surface, void *const user_data)
{
    S_Output_RenderBegin();
}

static void M_RenderEnd(void *surface, void *const user_data)
{
    S_Output_RenderEnd();
    S_Output_FlipScreen();
}

static void *M_LockSurface(void *const surface, void *const user_data)
{
    GFX_2D_SURFACE_DESC *surface_desc = user_data;
    bool result = GFX_2D_Surface_Lock(surface, surface_desc);
    assert(result);
    return surface_desc->pixels;
}

static void M_UnlockSurface(void *const surface, void *const user_data)
{
    GFX_2D_Surface_Unlock(surface);
}

static void M_UploadSurface(void *const surface, void *const user_data)
{
    GFX_2D_RENDERER *renderer_2d = user_data;
    GFX_2D_SURFACE *surface_ = surface;
    GFX_2D_Renderer_Upload(renderer_2d, &surface_->desc, surface_->buffer);
    GFX_2D_Renderer_Render(renderer_2d);
}

static bool M_Play(const char *const file_path)
{
    Audio_Shutdown();
    GFX_2D_RENDERER *renderer_2d = GFX_Context_GetRenderer2D();
    GFX_2D_SURFACE_DESC surface_desc = { 0 };

    VIDEO *video = Video_Open(file_path);
    if (video == NULL) {
        return false;
    }

    Video_SetSurfaceAllocatorFunc(video, M_AllocateSurface, NULL);
    Video_SetSurfaceDeallocatorFunc(video, M_DeallocateSurface, NULL);
    Video_SetSurfaceClearFunc(video, M_ClearSurface, NULL);
    Video_SetRenderBeginFunc(video, M_RenderBegin, NULL);
    Video_SetRenderEndFunc(video, M_RenderEnd, NULL);
    Video_SetSurfaceLockFunc(video, M_LockSurface, &surface_desc);
    Video_SetSurfaceUnlockFunc(video, M_UnlockSurface, NULL);
    Video_SetSurfaceUploadFunc(video, M_UploadSurface, renderer_2d);

    Video_Start(video);
    while (video->is_playing) {
        Video_SetVolume(
            video,
            m_Muted ? 0 : g_Config.sound_volume / (float)Sound_GetMaxVolume());
        Video_SetSurfaceSize(
            video, Screen_GetResWidth(), Screen_GetResHeight());
        Video_SetSurfacePixelFormat(video, AV_PIX_FMT_BGRA);
        GFX_Context_SetDisplaySize(Screen_GetResWidth(), Screen_GetResHeight());
        Video_PumpEvents(video);
        S_Shell_SpinMessageLoop();

        Input_Update();

        if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
            Video_Stop(video);
        }
    }
    Video_Close(video);

    Audio_Init();
    S_Output_ApplyRenderSettings();

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
    Sound_StopAllSamples();

    if (!g_Config.enable_fmv) {
        return true;
    }

    m_IsPlaying = true;
    char *final_path = File_GuessExtension(path, m_Extensions);
    bool ret = M_Play(final_path);
    Memory_FreePointer(&final_path);
    m_IsPlaying = false;
    return ret;
}
