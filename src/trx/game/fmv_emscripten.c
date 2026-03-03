#include <trx/av/audio.h>
#include <trx/config.h>
#include <trx/core/log.h>
#include <trx/game/console.h>
#include <trx/game/fmv.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/music.h>
#include <trx/game/output.h>
#include <trx/game/output/quad.h>
#include <trx/game/shell.h>
#include <trx/game/sound.h>
#include <trx/game/ui.h>
#include <trx/game/viewport.h>
#include <trx/gl/gl_webgl_compat.h>

#include <emscripten.h>

static bool m_IsPlaying = false;

// ---------------------------------------------------------------------------
// JavaScript interop for HTML5 <video> playback.
//
// The browser's native video decoder handles all container / codec work
// (hardware-accelerated where available).  Each frame is uploaded to a
// WebGL texture via texImage2D(video), then rendered through the existing
// Output_Quad pipeline.  Audio is played by the <video> element.
// ---------------------------------------------------------------------------

// clang-format off

EM_JS(int, js_fmv_open, (const char *path_ptr), {
    var path = UTF8ToString(path_ptr);

    var video = document.createElement('video');
    video.playsInline = true;
    video.preload     = 'auto';
    video.crossOrigin = 'anonymous';
    video.style.display = 'none';
    document.body.appendChild(video);

    Module._fmv = {
        video : video,
        ready : false,
        ended : false,
        error : false,
    };

    video.addEventListener('canplay', function() {
        Module._fmv.ready = true;
    });
    video.addEventListener('ended', function() {
        Module._fmv.ended = true;
    });
    video.addEventListener('error', function() {
        Module._fmv.error = true;
    });

    // Check for user-uploaded FMV stored as a blob in IndexedDB (via the
    // game data manager).  Falls back to streaming via HTTP.
    // Blob keys are lowercased by gamedata.js; FMV paths from the gameflow
    // may be uppercase (e.g. "fmv/LOGO.mp4"), so normalise before lookup.
    var blobKey = path.toLowerCase();
    if (Module._fmvBlobs && Module._fmvBlobs[blobKey]) {
        var blobUrl = URL.createObjectURL(Module._fmvBlobs[blobKey]);
        Module._fmv.blobUrl = blobUrl;
        video.src = blobUrl;
    } else {
        video.src = path;
    }

    var p = video.play();
    if (p !== undefined) {
        p.catch(function() { Module._fmv.error = true; });
    }
    return 1;
})

EM_JS(void, js_fmv_destroy, (void), {
    if (!Module._fmv) return;
    var fmv = Module._fmv;
    // Revoke blob URL if one was created for this playback.
    if (fmv.blobUrl) {
        URL.revokeObjectURL(fmv.blobUrl);
    }
    fmv.video.pause();
    fmv.video.removeAttribute('src');
    fmv.video.load();
    if (fmv.video.parentNode) {
        fmv.video.parentNode.removeChild(fmv.video);
    }
    Module._fmv = null;
});

// Returns: 0 = still loading, 1 = ready, -1 = error.
EM_JS(int, js_fmv_poll, (void), {
    if (!Module._fmv)        return -1;
    if (Module._fmv.error)   return -1;
    if (Module._fmv.ready)   return  1;
    return 0;
})

EM_JS(int, js_fmv_is_ended, (void), {
    return (Module._fmv && (Module._fmv.ended || Module._fmv.error)) ? 1 : 0;
})

EM_JS(int, js_fmv_get_width, (void), {
    return Module._fmv ? Module._fmv.video.videoWidth : 0;
})

EM_JS(int, js_fmv_get_height, (void), {
    return Module._fmv ? Module._fmv.video.videoHeight : 0;
})

EM_JS(void, js_fmv_set_volume, (float vol), {
    if (Module._fmv) {
        Module._fmv.video.volume = Math.max(0, Math.min(1, vol));
    }
});

// Upload the current video frame into the given GL texture.
EM_JS(void, js_fmv_upload_frame, (int tex_id), {
    if (!Module._fmv || !Module._fmv.ready) return;
    var video = Module._fmv.video;
    if (video.readyState < 2) return;

    var gl = GLctx;
    gl.bindTexture(gl.TEXTURE_2D, GL.textures[tex_id]);
    gl.texImage2D(
        gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, video);
})

// clang-format on

// ---------------------------------------------------------------------------
// FMV playback implementation
// ---------------------------------------------------------------------------

static bool M_Play(const char *const file_name)
{
    if (file_name == nullptr) {
        return false;
    }

    if (!js_fmv_open(file_name)) {
        LOG_WARNING("FMV: could not read file: %s", file_name);
        return false;
    }

    // Wait for the browser to load enough of the video to begin playback.
    for (int32_t waited = 0; waited < 5000; waited += 10) {
        const int32_t status = js_fmv_poll();
        if (status != 0) {
            break;
        }
        emscripten_sleep(10);
    }

    if (js_fmv_poll() != 1) {
        LOG_WARNING(
            "FMV: video failed to load or unsupported format: %s", file_name);
        js_fmv_destroy();
        return false;
    }

    const int32_t vid_w = js_fmv_get_width();
    const int32_t vid_h = js_fmv_get_height();
    if (vid_w <= 0 || vid_h <= 0) {
        LOG_WARNING("FMV: invalid video dimensions %dx%d", vid_w, vid_h);
        js_fmv_destroy();
        return false;
    }

    LOG_INFO("FMV: playing %s (%dx%d)", file_name, vid_w, vid_h);

    // Create a GL texture that will receive video frames from the browser.
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    OUTPUT_QUAD *renderer_2d = Output_Quad_Create();
    // Video frames are stored top-to-bottom but GL textures are bottom-to-top;
    // flip_y corrects for this via UV coordinates.
    Output_Quad_SetExternalTexture(renderer_2d, tex, vid_w, vid_h, true);
    Output_Quad_SetFit(
        renderer_2d, OUTPUT_QUAD_FIT_LETTERBOX, (float)vid_w, (float)vid_h);

    g_OldInputDB = g_Input;

    while (!js_fmv_is_ended()) {
        Shell_ProcessEvents();

        // Sync volume with game settings.
        const float vol = Audio_IsMuted()
            ? 0.0f
            : g_Config.audio.master_volume * g_Config.audio.fmv_volume;
        js_fmv_set_volume(vol);

        // Upload the current video frame to the GL texture.
        js_fmv_upload_frame((int)tex);

        // Render the video full-screen with letterboxing.
        Output_BeginScene();
        Output_Quad_Render(renderer_2d);

        // Overlay the in-game console / UI.
        Output_SwitchViewport(VIEWPORT_UI);
        UI_BeginScene();
        Console_Draw();
        Console_Control();
        Console_Control();
        UI_EndScene();
        UI_Draw();

        Output_EndScene();
        Output_FlipScreen();

        // Handle input — allow the player to skip the FMV.
        Input_Update();
        Shell_ProcessInput();
        if (g_InputDB.menu_back || g_InputDB.menu_confirm
            || GF_GetOverrideCommand().action != GF_NOOP || Shell_IsExiting()) {
            break;
        }

        emscripten_sleep(0);
    }

    js_fmv_destroy();
    glDeleteTextures(1, &tex);
    Output_Quad_Destroy(renderer_2d);
    Output_ApplyRenderSettings();
    return true;
}

bool FMV_Play(const char *const file_name)
{
    Music_Stop();
    Sound_StopAll();

    if (!g_Config.gameplay.enable_fmv) {
        return false;
    }

    // The path is already a fully-formed URL (e.g. "fmv/cafe.mp4"),
    // constructed at gameflow load time — no VFS extension guessing needed.
    m_IsPlaying = true;
    const bool result = M_Play(file_name);
    m_IsPlaying = false;
    return result;
}

bool FMV_IsPlaying(void)
{
    return m_IsPlaying;
}
