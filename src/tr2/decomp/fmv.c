#include "decomp/fmv.h"

#include "config.h"
#include "decomp/decomp.h"
#include "game/input.h"
#include "game/music.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/funcs.h"
#include "global/vars.h"
#include "lib/ddraw.h"

#include <libtrx/engine/video.h>
#include <libtrx/log.h>

#include <assert.h>
#include <string.h>

static bool m_Muted = false;
static LPDIRECTDRAWPALETTE m_DDrawPalette = NULL;
static LPDDS m_PrimaryBufferSurface = NULL;
static LPDDS m_BackBufferSurface = NULL;
static DDPIXELFORMAT m_PixelFormat;

static void M_Play(const char *file_name);
static bool M_CreateScreenBuffers(void);
static void M_ReleaseScreenBuffers(void);

static void *M_AllocateSurface(int32_t width, int32_t height, void *user_data);
static void M_DeallocateSurface(void *surface, void *user_data);
static void M_ClearSurface(void *surface, void *user_data);
static void M_RenderBegin(void *surface, void *user_data);
static void M_RenderEnd(void *surface, void *user_data);
static void *M_LockSurface(void *surface, void *user_data);
static void M_UnlockSurface(void *surface, void *user_data);
static void M_UploadSurface(void *surface, void *user_data);

static bool M_CreateScreenBuffers(void)
{
    m_PrimaryBufferSurface = NULL;
    m_BackBufferSurface = NULL;
    m_DDrawPalette = NULL;

    if (g_SavedAppSettings.fullscreen) {
        {
            DDSDESC dsp = {
                .dwSize = sizeof(DDSDESC),
                .dwFlags = DDSD_BACKBUFFERCOUNT | DDSD_CAPS,
                .dwBackBufferCount = 1,
                .ddsCaps.dwCaps =
                    DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX,
            };

            HRESULT rc = DDrawSurfaceCreate(&dsp, &m_PrimaryBufferSurface);
            if (FAILED(rc)) {
                LOG_ERROR("Failed to create primary screen buffer: %x", rc);
                return false;
            }
        }

        {
            DDSCAPS caps = {
                .dwCaps = DDSCAPS_BACKBUFFER,
            };
            const HRESULT rc =
                m_PrimaryBufferSurface->lpVtbl->GetAttachedSurface(
                    m_PrimaryBufferSurface, &caps, &m_BackBufferSurface);
            if (FAILED(rc)) {
                LOG_ERROR("Failed to create back screen buffer: %x", rc);
                return false;
            }
        }

        if (g_GameVid_IsVga) {
            PALETTEENTRY palette[256];

            // Populate the palette with a palette corresponding to
            // AV_PIX_FMT_RGB8
            for (int32_t i = 0; i < 256; i++) {
                PALETTEENTRY *col = &palette[i];

                col->peRed = (i >> 5) * 36;
                col->peGreen = ((i >> 2) & 7) * 36;
                col->peBlue = (i & 3) * 85;
            }

            HRESULT rc = IDirectDraw_CreatePalette(
                g_DDraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_INITIALIZE,
                palette, &m_DDrawPalette, 0);
            if (FAILED(rc)) {
                LOG_ERROR(
                    "Failed to set primary screen buffer palette: %x", rc);
                return false;
            }

            rc = m_PrimaryBufferSurface->lpVtbl->SetPalette(
                m_PrimaryBufferSurface, m_DDrawPalette);
            if (FAILED(rc)) {
                LOG_ERROR(
                    "Failed to attach palette to the primary screen buffer: %x",
                    rc);
                return false;
            }
        }

    } else {
        DDSDESC dsp = {
            .dwSize = sizeof(DDSDESC),
            .dwFlags = DDSD_CAPS,
            .ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE,
        };

        const HRESULT rc = DDrawSurfaceCreate(&dsp, &m_PrimaryBufferSurface);
        if (FAILED(rc)) {
            LOG_ERROR("Failed to create primary screen buffer: %x", rc);
            return false;
        }
    }

    memset(&m_PixelFormat, 0, sizeof(m_PixelFormat));
    m_PixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    const HRESULT rc = IDirectDrawSurface_GetPixelFormat(
        m_PrimaryBufferSurface, &m_PixelFormat);
    if (FAILED(rc)) {
        LOG_ERROR(
            "Failed to get pixel format of the primary screen buffer: %x", rc);
    }

    return true;
}

static void M_ReleaseScreenBuffers(void)
{
    if (m_PrimaryBufferSurface != NULL) {
        m_PrimaryBufferSurface->lpVtbl->Release(m_PrimaryBufferSurface);
        m_PrimaryBufferSurface = NULL;
    }

    if (m_DDrawPalette != NULL) {
        m_DDrawPalette->lpVtbl->Release(m_DDrawPalette);
    }
}

static void *M_AllocateSurface(
    const int32_t width, const int32_t height, void *const user_data)
{
    VIDEO *const video = user_data;

    LPDDS surface;
    DDSDESC dsp = {
        .dwSize = sizeof(DDSDESC),
        .dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS,
        .dwWidth = width,
        .dwHeight = height,
        .ddsCaps = {
            .dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
        },
    };
    const HRESULT rc = DDrawSurfaceCreate(&dsp, &surface);
    if (FAILED(rc)) {
        LOG_ERROR("Failed to create render buffer: %x", rc);
    }

    // Set pixel format
    if (m_PixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
        Video_SetSurfacePixelFormat(video, AV_PIX_FMT_RGB8);
    } else if (m_PixelFormat.dwRGBBitCount == 16) {
        if (m_PixelFormat.dwRBitMask == 0xF800) {
            Video_SetSurfacePixelFormat(video, AV_PIX_FMT_RGB565);
        } else {
            Video_SetSurfacePixelFormat(video, AV_PIX_FMT_BGR565);
        }
    } else if (m_PixelFormat.dwRGBBitCount == 24) {
        if (m_PixelFormat.dwRBitMask == 255) {
            Video_SetSurfacePixelFormat(video, AV_PIX_FMT_RGB24);
        } else {
            Video_SetSurfacePixelFormat(video, AV_PIX_FMT_BGR24);
        }
    } else if (m_PixelFormat.dwRGBBitCount == 32) {
        if (m_PixelFormat.dwRBitMask == 255) {
            Video_SetSurfacePixelFormat(video, AV_PIX_FMT_RGB0);
        } else {
            Video_SetSurfacePixelFormat(video, AV_PIX_FMT_BGR0);
        }
    }

    // Set pitch
    surface->lpVtbl->GetSurfaceDesc(surface, &dsp);
    Video_SetSurfaceStride(video, dsp.lPitch);

    return surface;
}

static void M_DeallocateSurface(void *const surface_, void *const user_data)
{
    LPDDS surface = surface_;
    const HRESULT rc = surface->lpVtbl->Release(surface);
    if (FAILED(rc)) {
        LOG_ERROR("Failed to release render buffer: %x", rc);
    }
}

static void M_ClearSurface(void *const surface_, void *const user_data)
{
    LPDDS surface = surface_;
    WinVidClearBuffer(surface, NULL, 0);
}

static void M_RenderBegin(void *const surface, void *const user_data)
{
}

static void M_RenderEnd(void *const surface_, void *const user_data)
{
    LPDDS surface = surface_;

    if (g_SavedAppSettings.fullscreen) {
        LPRECT rect = NULL;
        HRESULT rc = m_BackBufferSurface->lpVtbl->Blt(
            m_BackBufferSurface, rect, surface, rect, DDBLT_WAIT, NULL);
        if (FAILED(rc)) {
            LOG_ERROR(
                "Failed to copy pixels to the primary screen buffer: %x", rc);
        }

        rc = m_PrimaryBufferSurface->lpVtbl->Flip(
            m_PrimaryBufferSurface, NULL, DDFLIP_WAIT);
        if (FAILED(rc)) {
            LOG_ERROR("Failed to flip the primary screen buffer: %x", rc);
        }
    } else {
        LPRECT rect = &g_PhdWinRect;
        RECT dst_rect = {
            .left = g_GameWindowPositionX + rect->left,
            .top = g_GameWindowPositionY + rect->top,
            .bottom = g_GameWindowPositionY + rect->bottom,
            .right = g_GameWindowPositionX + rect->right,
        };
        const HRESULT rc = m_PrimaryBufferSurface->lpVtbl->Blt(
            m_PrimaryBufferSurface, &dst_rect, surface, rect, DDBLT_WAIT, NULL);
        if (FAILED(rc)) {
            LOG_ERROR(
                "Failed to copy pixels to the primary screen buffer: %x", rc);
        }
    }
}

static void *M_LockSurface(void *const surface_, void *const user_data)
{
    LPDDS surface = surface_;
    LPDDSURFACEDESC desc = user_data;
    assert(desc != NULL);

    HRESULT rc;
    while (true) {
        rc = surface->lpVtbl->Lock(surface, 0, desc, 0, 0);
        if (rc != DDERR_WASSTILLDRAWING) {
            break;
        }
    }

    if (rc == DDERR_SURFACELOST) {
        surface->lpVtbl->Restore(surface);
    }

    if (FAILED(rc)) {
        return NULL;
    }

    return desc->lpSurface;
}

static void M_UnlockSurface(void *const surface_, void *const user_data)
{
    LPDDS surface = surface_;
    LPDDSURFACEDESC desc = user_data;
    surface->lpVtbl->Unlock(surface, desc);
}

static void M_UploadSurface(void *const surface, void *const user_data)
{
}

static bool M_EnterFMVMode(void)
{
    ShowCursor(false);
    Music_Stop();

    RenderFinish(false);
    if (!M_CreateScreenBuffers()) {
        return false;
    }

    return true;
}

static void M_ExitFMVMode(void)
{
    M_ReleaseScreenBuffers();

    if (!g_IsGameToExit) {
        RenderStart(true);
    }
    ShowCursor(true);
}

static void M_Play(const char *const file_name)
{
    g_IsFMVPlaying = true;
    const char *const full_path = GetFullPath(file_name);
    WinPlayFMV(full_path, true);
    g_IsFMVPlaying = false;
}

bool __cdecl PlayFMV(const char *const file_name)
{
    if (M_EnterFMVMode()) {
        M_Play(file_name);
    }
    M_ExitFMVMode();
    return g_IsGameToExit;
}

bool __cdecl IntroFMV(
    const char *const file_name_1, const char *const file_name_2)
{
    if (M_EnterFMVMode()) {
        M_Play(file_name_1);
        M_Play(file_name_2);
    }
    M_ExitFMVMode();
    return g_IsGameToExit;
}

void __cdecl WinPlayFMV(const char *const file_name, const bool is_playback)
{
    DDSURFACEDESC surface_desc = { .dwSize = sizeof(DDSURFACEDESC), 0 };

    VIDEO *video = Video_Open(file_name);
    if (video == NULL) {
        return;
    }

    Video_SetSurfaceAllocatorFunc(video, M_AllocateSurface, video);
    Video_SetSurfaceDeallocatorFunc(video, M_DeallocateSurface, NULL);
    Video_SetSurfaceClearFunc(video, M_ClearSurface, NULL);
    Video_SetRenderBeginFunc(video, M_RenderBegin, NULL);
    Video_SetRenderEndFunc(video, M_RenderEnd, NULL);
    Video_SetSurfaceLockFunc(video, M_LockSurface, &surface_desc);
    Video_SetSurfaceUnlockFunc(video, M_UnlockSurface, &surface_desc);
    Video_SetSurfaceUploadFunc(video, M_UploadSurface, NULL);

    Video_Start(video);
    while (video->is_playing) {
        Video_SetVolume(
            video,
            m_Muted ? 0 : g_OptionSoundVolume / (float)Sound_GetMaxVolume());

        Video_SetSurfaceSize(video, g_PhdWinWidth, g_PhdWinHeight);

        Video_PumpEvents(video);

        WinVidSpinMessageLoop(false);

        if (Input_Update()) {
            Video_Stop(video);
            break;
        }
        if ((g_InputDB & IN_OPTION) != 0) {
            Video_Stop(video);
            break;
        }
    }
    Video_Close(video);
}

void __cdecl WinStopFMV(bool is_playback)
{
}

bool __cdecl S_PlayFMV(const char *const file_name)
{
    return PlayFMV(file_name);
}

bool __cdecl S_IntroFMV(
    const char *const file_name_1, const char *const file_name_2)
{
    return IntroFMV(file_name_1, file_name_2);
}
