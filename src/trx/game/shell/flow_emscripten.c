#include <trx/core/filesystem.h>
#include <trx/core/webgl_log.h>
#include <trx/game/shell.h>
#include <trx/platform/yield.h>
#include <trx/version.h>

#include <SDL2/SDL.h>
#include <emscripten.h>
#include <stdint.h>
#include <stdio.h>

// clang-format off
EM_JS(void, js_show_start_gate, (void), {
    var gate = document.getElementById('start-gate');
    if (!gate) return;
    document.getElementById('loading').classList.add('hidden');
    gate.classList.remove('hidden');
    Module._startGateDismissed = false;

    function dismiss() {
        if (Module._startGateDismissed) return;
        Module._startGateDismissed = true;
        gate.classList.add('hidden');
        document.getElementById('canvas').focus();
        document.removeEventListener('keydown', dismiss, true);
        document.removeEventListener('mousedown', dismiss, true);
        document.removeEventListener('touchstart', dismiss, true);
    }
    Module._dismissStartGate = dismiss;
    document.addEventListener('keydown', dismiss, true);
    document.addEventListener('mousedown', dismiss, true);
    document.addEventListener('touchstart', dismiss, true);

    // Poll for gamepad button presses.
    (function poll() {
        if (Module._startGateDismissed) return;
        var gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
        for (var i = 0; i < gamepads.length; i++) {
            var gp = gamepads[i];
            if (!gp) continue;
            for (var j = 0; j < gp.buttons.length; j++) {
                if (gp.buttons[j].pressed) { dismiss(); return; }
            }
        }
        requestAnimationFrame(poll);
    })();
})

EM_JS(int, js_is_start_gate_dismissed, (void), {
    return Module._startGateDismissed ? 1 : 0;
})

// --- IDBFS persistence ---

EM_JS(void, js_init_idbfs, (const char *mount_path), {
    var path = UTF8ToString(mount_path);
    try { FS.mkdir('/persist'); } catch(e) {}
    try { FS.mkdir(path); } catch(e) {}
    FS.mount(IDBFS, {}, path);
    try { FS.mkdir(path + '/saves'); } catch(e) {}
    try { FS.mkdir(path + '/cfg'); } catch(e) {}
})

EM_JS(void, js_start_idbfs_sync_from_db, (void), {
    Module._idbfsSyncDone = false;
    FS.syncfs(true, function(err) {
        if (err) console.error('[IDBFS] sync from DB error:', err);
        Module._idbfsSyncDone = true;
    });
})

EM_JS(int, js_is_idbfs_sync_done, (void), {
    return Module._idbfsSyncDone ? 1 : 0;
})

EM_JS(void, js_restore_config, (const char *src, const char *dst), {
    try {
        var data = FS.readFile(UTF8ToString(src));
        FS.writeFile(UTF8ToString(dst), data);
    } catch(e) { /* no persisted config yet */ }
})

EM_JS(void, js_persist_file_and_sync, (const char *src, const char *dst), {
    var d = UTF8ToString(dst);
    try { FS.mkdirTree(d.substring(0, d.lastIndexOf('/'))); } catch(e) {}
    try {
        FS.writeFile(d, FS.readFile(UTF8ToString(src)));
    } catch(e) { console.error('[IDBFS] persist failed:', e); }
    FS.syncfs(false, function(err) {
        if (err) console.error('[IDBFS] sync error:', err);
    });
})

EM_JS(void, js_sync_idbfs_to_db, (void), {
    FS.syncfs(false, function(err) {
        if (err) console.error('[IDBFS] sync error:', err);
    });
})

EM_JS(void, js_set_touch_controls_visible, (int visible), {
    if (Module.setTouchControlsVisible) {
        Module.setTouchControlsVisible(visible);
    }
})

EM_JS(int, js_has_touch_support, (void), {
    return navigator.maxTouchPoints > 0 ? 1 : 0;
})

// clang-format on

// setenv is POSIX but not declared under strict C standard modes.
int setenv(const char *name, const char *value, int overwrite);

void Shell_InitIDBFS(void)
{
    js_init_idbfs("/persist/" TRX_GAME_ID);

    js_start_idbfs_sync_from_db();
    while (!js_is_idbfs_sync_done()) {
        emscripten_sleep(10);
    }
    WEBGL_LOG("[WEBGL] IDBFS ready at /persist/%s", TRX_GAME_ID);

    // Redirect saves directory to IDBFS-backed path.
    char saves_dir[64];
    snprintf(saves_dir, sizeof(saves_dir), "/persist/%s/saves", TRX_GAME_ID);
    setenv("TRX_SAVES_DIR", saves_dir, 1);

    // Restore persisted user config to /cfg/ (before Config_Read runs).
    const int ver = TRX_GAME_ID[2] - '0';
    char persist_cfg[64];
    char cfg_path[64];
    snprintf(
        persist_cfg, sizeof(persist_cfg), "/persist/%s/cfg/TR%dX.json5",
        TRX_GAME_ID, ver);
    snprintf(cfg_path, sizeof(cfg_path), "/cfg/TR%dX.json5", ver);
    js_restore_config(persist_cfg, cfg_path);
}

void Shell_WaitForUserInput(void)
{
    WEBGL_LOG("[WEBGL] Showing start gate, waiting for user interaction...");
    js_show_start_gate();
    while (!js_is_start_gate_dismissed()) {
        emscripten_sleep(50);
    }
    WEBGL_LOG("[WEBGL] User interaction received, proceeding.");
}

void Shell_PersistConfigToIDBFS(void)
{
    const int ver = TRX_GAME_ID[2] - '0';
    char src[64];
    char dst[64];
    snprintf(src, sizeof(src), "/cfg/TR%dX.json5", ver);
    snprintf(dst, sizeof(dst), "/persist/%s/cfg/TR%dX.json5", TRX_GAME_ID, ver);
    js_persist_file_and_sync(src, dst);
}

void Shell_PersistSavesToIDBFS(void)
{
    js_sync_idbfs_to_db();
}

bool Shell_HasTouchSupport(void)
{
    return js_has_touch_support();
}

void Shell_SetTouchControlsVisible(const bool visible)
{
    js_set_touch_controls_visible(visible);
}

uint32_t Shell_GetWindowExtraFlags(void)
{
    return SDL_WINDOW_ALLOW_HIGHDPI;
}

void Shell_SetupGLContextVersion(void)
{
    // WebGL 2.0 = OpenGL ES 3.0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
}
