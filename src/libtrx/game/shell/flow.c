#include "config.h"
#include "debug.h"
#include "enum_map.h"
#include "game/catalog.h"
#include "game/clock.h"
#include "game/console.h"
#include "game/events.h"
#include "game/fmv.h"
#include "game/game_buf.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/game_string_manager.h"
#include "game/lara/pose.h"
#include "game/lua.h"
#include "game/music.h"
#include "game/option.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/shell/platform.h"
#include "game/sound.h"
#include "game/test_recorder.h"
#include "game/test_replay.h"
#include "log.h"
#include "utils.h"

#include <SDL2/SDL.h>
#include <stdio.h>

static const SHELL_ARGS *m_ShellArgs = nullptr;

static void M_HandleConfigChange(const EVENT *const event, void *const data)
{
    const CONFIG *const old = &g_Config;
    const CONFIG *const new = &g_SavedConfig;
    Shell_HandleConfigChange(old, new);
}

static void M_SetupSDL(void)
{
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0) {
        Shell_ExitSystemFmt("Cannot initialize SDL: %s", SDL_GetError());
    }
}

static void M_SetupGL(void)
{
    // Setup minimum properties of GL context
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

static void M_LoadCatalog(
    const CATALOG_CONTEXT context, const char *const filename)
{
    const char *const path =
        String_FormatStatic("%s/%s", Shell_GetConfigDir(), filename);
    if (!Catalog_Load(context, path)) {
        Shell_ExitSystemFmt("Failed to load catalogs from %s", path);
    }
}

const char *Shell_GetConfigDir(void)
{
    return "cfg";
}

const SHELL_ARGS *Shell_GetArgs(void)
{
    return m_ShellArgs;
}

void Shell_InitCommonModules(void)
{
    Shell_SetupHiDPI();
    Shell_SetupLibAV();
    M_SetupSDL();
    M_SetupGL();

    GameString_Init();
    GameStringManager_Init();
    EnumMap_Init();
    Config_Init();
    UI_Init();
    Console_Init();
    Overlay_Init();
    GameEvent_Init();

    Input_Init();

    GameBuf_Init();
    Random_Seed();
    Lara_Pose_Init();

    Clock_Init();
    LUA_Init();
}

void Shell_ShutdownCommonModules(void)
{
    if (TestReplay_IsOpened()) {
        TestReplay_Close();
    }
    if (TestRecorder_IsOpened()) {
        TestRecorder_Close();
    }

    Lara_Pose_Shutdown();

    Console_Shutdown();
    Savegame_Shutdown();

    GF_Shutdown();
    LUA_Shutdown();
    Overlay_Shutdown();
    Option_Shutdown();
    Output_Shutdown();

    Input_Shutdown();
    Music_Shutdown();
    Sound_Shutdown();
    UI_Shutdown();
    GameEvent_Shutdown();

    GameStringManager_Shutdown();
    GameString_Shutdown();
    GameBuf_Shutdown();
    Catalog_Shutdown();

    Config_Shutdown();
    EnumMap_Shutdown();
    Log_Shutdown();
}

const SHELL_ARGS *Shell_CommonInit(const SHELL_ARGS *const args)
{
    const SHELL_ARGS *new_args = args;

    if (args->test_record_path != nullptr
        && args->test_replay_path != nullptr) {
        Shell_ExitSystem("Cannot use both --test-record and --test-replay");
    }
    if (args->headless && args->test_replay_path == nullptr) {
        Shell_ExitSystem("--headless can only be used with --test-replay");
    }

    M_LoadCatalog(CATALOG_OBJECTS, "catalog_objects.csv");
    M_LoadCatalog(CATALOG_MUSIC, "catalog_music.csv");
    M_LoadCatalog(CATALOG_SAMPLES, "catalog_samples.csv");
    M_LoadCatalog(CATALOG_LARA_STATES, "catalog_lara_states.csv");
    M_LoadCatalog(CATALOG_LARA_ANIMS, "catalog_lara_anims.csv");

    if (args->test_replay_path != nullptr) {
        SHELL_ARGS *tmp_args = TestReplay_Open(args->test_replay_path);
        if (tmp_args != nullptr) {
            // original args not needed, free immediately.
            if (tmp_args->original_args != nullptr) {
                Vector_Free(tmp_args->original_args);
                tmp_args->original_args = nullptr;
            }
            tmp_args->headless = args->headless;
            tmp_args->debug_render_performance = args->debug_render_performance;
            new_args = tmp_args;
        }
    } else {
        Config_Read(
            String_FormatStatic(
                "%s/%s.json5", Shell_GetConfigDir(), PROJECT_NAME),
            Shell_GetGameFlowPath(args->mod));

        if (args->test_record_path != nullptr) {
            TestRecorder_Open(args->test_record_path, args->original_args);
        }
    }
    Config_SubscribeChanges(M_HandleConfigChange, nullptr);

    Clock_SetSimSpeed(Clock_GetSpeedMultiplier());
    if (!new_args->headless) {
        Sound_Init();
        Music_Init();
        Sound_SetMasterVolume(g_Config.audio.sound_volume);
        Music_SetVolume(g_Config.audio.music_volume);
    } else {
        Clock_DisableWait();
        const int32_t fps = new_args->headless_fps > 0 ? new_args->headless_fps
                                                       : Clock_GetCurrentFPS();
        Clock_EnableHeadlessFixedFPS(fps);
    }

    m_ShellArgs = new_args;
    return new_args;
}
