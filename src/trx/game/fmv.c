#include <trx/game/fmv.h>

#include <trx/av/audio.h>
#include <trx/av/video.h>
#include <trx/config.h>
#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/console.h>
#include <trx/game/fader.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/input.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/ui.h>
#include <trx/game/music.h>
#include <trx/game/output.h>
#include <trx/game/output/overlay.h>
#include <trx/game/output/quad.h>
#include <trx/game/overlay.h>
#include <trx/game/shell.h>
#include <trx/game/sound.h>
#include <trx/game/ui.h>
#include <trx/game/ui/touch_overlay.h>
#include <trx/game/viewport.h>

#include <string.h>

#define M_FADE_TIME 0.4f
#define M_PAUSE_OVERLAY_OPACITY 0.8f

typedef struct {
    OUTPUT_QUAD_SURFACE_DESC desc;
    uint8_t *buffer;
} M_SURFACE;

typedef struct {
    OUTPUT_QUAD *renderer_2d;
    bool show_pause_overlay;
    FADER pause_fader;
} M_RENDER_CONTEXT;

static bool m_IsPlaying = false;

static const char *const m_FallbackExts[] = {
    ".mp4", ".mpeg", ".webm", ".avi", ".fmv", ".rpl", nullptr,
};

static OUTPUT_QUAD_SURFACE_DESC M_MakeSurfaceDesc(
    const int32_t width, const int32_t height)
{
    return (OUTPUT_QUAD_SURFACE_DESC) {
        .width = width,
        .height = height,
        .bit_count = 32,
        .tex_format = GL_BGRA,
        .tex_type = GL_UNSIGNED_INT_8_8_8_8_REV,
        .uv = {
            { .u = 0.0f, .v = 0.0f },
            { .u = 1.0f, .v = 0.0f },
            { .u = 1.0f, .v = 1.0f },
            { .u = 0.0f, .v = 1.0f },
        },
        .pitch = width * 4,
    };
}

static int32_t M_OpenAudioStream(const char *const file_name)
{
    int32_t audio_id = AUDIO_NO_SOUND;
    if (IGNORE(Audio_Stream_CreateFromFile(file_name, &audio_id))) {
        return audio_id;
    }

    // The video file may lack an audio stream (e.g. remastered .ogv).
    // Try other FMV extensions to find a file that contains audio.
    const char *const dot = strrchr(file_name, '.');
    if (dot == nullptr) {
        return AUDIO_NO_SOUND;
    }

    const size_t base_len = (size_t)(dot - file_name);
    for (const char *const *ext = m_FallbackExts; *ext != nullptr; ext++) {
        char *const candidate =
            String_Format("%.*s%s", (int)base_len, file_name, *ext);
        if (FS_Exists(candidate)) {
            const bool opened =
                IGNORE(Audio_Stream_CreateFromFile(candidate, &audio_id));
            Memory_Free(candidate);
            if (opened) {
                return audio_id;
            }
        } else {
            Memory_Free(candidate);
        }
    }
    return AUDIO_NO_SOUND;
}

static float M_GetAudioVolume(void)
{
    return Audio_IsMuted()
        ? 0.0f
        : g_Config.audio.master_volume * g_Config.audio.fmv_volume;
}

static void *M_AllocateSurface(
    const int32_t width, const int32_t height, void *const user_data)
{
    M_SURFACE *const surface = Memory_Alloc(sizeof(M_SURFACE));
    surface->desc = M_MakeSurfaceDesc(width, height);
    surface->buffer = Memory_Alloc(surface->desc.pitch * surface->desc.height);
    return surface;
}

static void M_DeallocateSurface(void *const surface, void *const user_data)
{
    M_SURFACE *const surface_ = surface;
    Memory_Free(surface_->buffer);
    Memory_Free(surface_);
}

static void M_ClearSurface(void *const surface, void *const user_data)
{
    ASSERT(surface != nullptr);
    M_SURFACE *const surface_ = surface;
    memset(surface_->buffer, 0, surface_->desc.pitch * surface_->desc.height);
}

static void M_RenderBegin(void *const surface, void *const user_data)
{
    Output_BeginScene();
}

static void M_DrawUI(void)
{
    UI_BeginScene();
    LUA_UI_DrawRegions();
    Overlay_DrawUI();
    Console_Draw();
    Console_Control();
    Console_Control();
    UI_EndScene();
    UI_Draw();
}

static float M_GetPauseOverlayOpacity(const M_RENDER_CONTEXT *const ctx)
{
    if (!g_Config.ui.pause_fade_effects) {
        return ctx->show_pause_overlay ? M_PAUSE_OVERLAY_OPACITY : 0.0f;
    }
    return Fader_GetCurrentValue(&ctx->pause_fader) * M_PAUSE_OVERLAY_OPACITY;
}

static bool M_ShouldShowPauseText(const M_RENDER_CONTEXT *const ctx)
{
    if (!ctx->show_pause_overlay) {
        return false;
    }
    if (!g_Config.ui.pause_fade_effects) {
        return true;
    }
    return !Fader_IsActive(&ctx->pause_fader)
        && Fader_GetCurrentValue(&ctx->pause_fader) >= 1.0f;
}

static void M_RenderEnd(void *const surface, void *const user_data)
{
    Output_EndScene();
    Output_FlipScreen();
}

static void *M_LockSurface(void *const surface, void *const user_data)
{
    ASSERT(surface != nullptr);
    M_SURFACE *const surface_ = surface;
    return surface_->buffer;
}

static void M_UnlockSurface(void *const surface, void *const user_data)
{
}

static void M_UploadSurface(void *const surface, void *const user_data)
{
    M_RENDER_CONTEXT *const ctx = user_data;
    M_SURFACE *const surface_ = surface;
    const float overlay_opacity = M_GetPauseOverlayOpacity(ctx);
    Output_Quad_Upload(ctx->renderer_2d, &surface_->desc, surface_->buffer);
    Output_Quad_SetFit(
        ctx->renderer_2d, OUTPUT_QUAD_FIT_LETTERBOX, surface_->desc.width,
        surface_->desc.height);
    Output_Quad_SetFilter(ctx->renderer_2d, g_Config.rendering.fmv_filter);

    Output_SwitchViewport(VIEWPORT_GAME);
    Output_Quad_Render(ctx->renderer_2d);
    if (overlay_opacity > 0.0f) {
        Output_Overlay_DrawBlackRectangle(overlay_opacity, false);
    }

    Output_SwitchViewport(VIEWPORT_UI);
    M_DrawUI();
}

static void M_SetPauseText(const bool show)
{
    if (show) {
        Overlay_SetBottomText((OVERLAY_TEXT) {
            .kind = OVERLAY_TEXT_GS_KEY,
            .gs_key = GS_ID("general/pause/paused"),
        });
    } else {
        Overlay_SetBottomText((OVERLAY_TEXT) {});
    }
}

static void M_RedrawFrame(M_RENDER_CONTEXT *const ctx)
{
    const float overlay_opacity = M_GetPauseOverlayOpacity(ctx);
    Output_BeginScene();
    Output_SwitchViewport(VIEWPORT_GAME);
    Output_Quad_Render(ctx->renderer_2d);
    if (overlay_opacity > 0.0f) {
        Output_Overlay_DrawBlackRectangle(overlay_opacity, false);
    }

    Output_SwitchViewport(VIEWPORT_UI);
    M_DrawUI();

    Output_EndScene();
    Output_FlipScreen();
}

static RESULT M_Play(const char *const file_name)
{
    VIDEO *video = nullptr;
    MUST(Video_Open(file_name, &video));

    M_RENDER_CONTEXT render_ctx = {};
    MUST(Output_Quad_Create(&render_ctx.renderer_2d));

    Video_SetSurfaceAllocatorFunc(video, M_AllocateSurface, nullptr);
    Video_SetSurfaceDeallocatorFunc(video, M_DeallocateSurface, nullptr);
    Video_SetSurfaceClearFunc(video, M_ClearSurface, nullptr);
    Video_SetRenderBeginFunc(video, M_RenderBegin, nullptr);
    Video_SetRenderEndFunc(video, M_RenderEnd, nullptr);
    Video_SetSurfaceLockFunc(video, M_LockSurface, nullptr);
    Video_SetSurfaceUnlockFunc(video, M_UnlockSurface, nullptr);
    Video_SetSurfaceUploadFunc(video, M_UploadSurface, &render_ctx);
    Video_SetSurfacePixelFormat(video, AV_PIX_FMT_BGRA);
    Video_SetAudioEnabled(video, false);

    const int32_t audio_id = M_OpenAudioStream(file_name);
    bool input_paused = false;
    bool paused = false;

    g_OldInputDB = g_Input;
    Fader_InitTo(&render_ctx.pause_fader, 0.0f, 0.0f, 0.0f);
    M_SetPauseText(false);
    Video_Start(video);

    IGNORE(Audio_Stream_SetVolume(audio_id, M_GetAudioVolume()));
    IGNORE(Audio_Stream_Unpause(audio_id));

    while (video->is_playing) {
        Shell_ProcessEvents();

        const bool focus_paused = Shell_ShouldPauseForFocusLoss();
        Input_Update();
        Shell_ProcessInput();

        render_ctx.show_pause_overlay = input_paused;
        M_SetPauseText(M_ShouldShowPauseText(&render_ctx));
        Overlay_Control();
        LUA_FireEvent(LUA_EVENT_TICK);

        const bool should_pause = focus_paused || input_paused;
        if (should_pause != paused) {
            Video_SetPaused(video, should_pause);
            IGNORE(Audio_Stream_SetPaused(audio_id, should_pause));
            paused = should_pause;
        }

        IGNORE(Audio_Stream_SetVolume(audio_id, M_GetAudioVolume()));
        const double audio_ts = Audio_Stream_GetTimestamp(audio_id);
        if (audio_ts >= 0.0) {
            Video_SetExternalAudioClock(video, audio_ts);
        }

        Video_PumpEvents(video);

        if (paused) {
            M_RedrawFrame(&render_ctx);
        }

        if ((g_InputDB.pause || (input_paused && g_InputDB.menu_back))
            && !focus_paused) {
            input_paused = !input_paused;
            if (g_Config.ui.pause_fade_effects) {
                Fader_InitFromCurrent(
                    &render_ctx.pause_fader, input_paused ? 1.0f : 0.0f,
                    M_FADE_TIME);
            }
        } else if (
            (!paused
             && (g_InputDB.menu_back || g_InputDB.menu_confirm
                 || TouchOverlay_HasAnyFingerDown()))
            || GF_GetOverrideCommand().action != GF_NOOP || Shell_IsExiting()) {
            Video_Stop(video);
            break;
        }
    }

    M_SetPauseText(false);
    IGNORE(Audio_Stream_Close(audio_id));
    Video_Close(video);

    Output_Quad_Destroy(render_ctx.renderer_2d);
    Output_ApplyRenderSettings();
    return OK;
}

RESULT FMV_Play(const char *const file_path)
{
    Music_Stop();
    Sound_StopAll();

    if (!g_Config.gameplay.enable_fmv) {
        return OK;
    }

    m_IsPlaying = true;
    Output_SetSupersamplingEnabled(false);
    const RESULT result = M_Play(file_path);
    Output_SetSupersamplingEnabled(true);
    m_IsPlaying = false;
    return result;
}

bool FMV_IsPlaying(void)
{
    return m_IsPlaying;
}
