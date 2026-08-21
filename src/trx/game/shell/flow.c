#include <trx/config.h>
#include <trx/config/registry.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/result.h>
#include <trx/core/strings.h>
#include <trx/core/subsystem.h>
#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/clock.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_strings/manager.h>
#include <trx/game/lua.h>
#include <trx/game/output.h>
#include <trx/game/replay/test_recorder.h>
#include <trx/game/replay/test_replay.h>
#include <trx/game/savegame.h>
#include <trx/game/shell.h>
#include <trx/game/shell/platform.h>
#include <trx/game/shell/session.h>
#include <trx/game/shell/state.h>
#include <trx/game/stats.h>
#include <trx/gl/context.h>
#include <trx/version.h>

#include <SDL2/SDL.h>
#include <stdio.h>

static SHELL_SESSION *m_Session = nullptr;
static SDL_Window *m_Window = nullptr;
static char *m_PendingMod = nullptr;

// Flags preserved across mod switches (needed to rebuild args in main()).
static bool m_PrevHeadless = false;
static bool m_PrevQuiet = false;

// Given back before the config module goes down, so a mod switch does not
// leave a copy behind.
static int32_t m_ConfigListener = -1;

static RESULT M_CreateGameWindow(void)
{
    if (m_Window != nullptr) {
        return OK; // Window persists across mod switches
    }
    m_Window = SDL_CreateWindow(
        "TRX", g_Config.window.x, g_Config.window.y, g_Config.window.width,
        g_Config.window.height,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    FAIL_IF(
        m_Window == nullptr, "the game window could not be created: %s",
        SDL_GetError());
    Shell_EnableThemeSupport(m_Window);
    return OK;
}

static RESULT M_FailUnsupportedGraphics(const RESULT cause)
{
    char *driver = TRX_GL_Context_DescribeDriver(m_Window);

#ifdef _WIN32
    const char *const hint =
        " Where the card is too old for that, installing "
        "Mesa3D lets TRX draw the game without it.";
#else
    const char *const hint = "";
#endif

    char *message = String_Format(
        "TRX needs OpenGL 3.3 to draw the game, and the graphics driver on "
        "this computer does not offer it.\n"
        "\n"
        "Graphics driver: %s\n"
        "\n"
        "Installing the latest drivers for the graphics card usually helps.%s",
        driver != nullptr ? driver : "unknown", hint);

    const RESULT result = Result_Prefix(cause, "%s", message);

    Memory_FreePointer(&message);
    Memory_FreePointer(&driver);
    return result;
}

static RESULT M_CreateGLContext(void)
{
    if (TRX_GL_Context_GetWindowHandle() != nullptr) {
        return OK; // GL context persists across mod switches
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    const RESULT attached = TRX_GL_Context_Attach(m_Window);
    if (!IS_OK(attached)) {
        return M_FailUnsupportedGraphics(attached);
    }
    return OK;
}

static void M_ShowWindow(void)
{
    Shell_SyncToWindow();
    SDL_ShowWindow(m_Window);
    SDL_RaiseWindow(m_Window);
    Shell_RefreshRendererViewport();
}

static void M_HandleConfigChange(const EVENT *const event, void *const data)
{
    Shell_HandleConfigChange(event->data);
}

static RESULT M_SetupSDL(void)
{
    SDL_version compiled;
    SDL_VERSION(&compiled);
    LOG_INFO(
        "SDL version: %d.%d.%d", compiled.major, compiled.minor,
        compiled.patch);
    FAIL_IF(
        SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0,
        "SDL could not be brought up: %s", SDL_GetError());
    return OK;
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

static RESULT M_LoadCatalog(
    const CATALOG_CONTEXT context, const char *const filename,
    const bool allow_duplicates)
{
    const char *path = nullptr;
    MUST(GamePath_Resolve(GAME_DYNAMIC_PATH_CATALOG, filename, &path));
    return Catalog_Load(context, path, allow_duplicates);
}

static RESULT M_InitModules(void)
{
    Shell_SetupHiDPI();
    Shell_SetupLibAV();
    MUST(M_SetupSDL());
    M_SetupGL();

    // Some of the subsystems read the clock or the video state as they come
    // up, so the platform stands first.
    Subsystem_InitAll();

    MUST(LUA_Init(), "the Lua runtime could not be brought up");

    const SHELL_ARGS *const args = Shell_GetArgs();
    if (args != nullptr && args->startup.dump_lua_api) {
        LUA_DumpAPI();
        exit(0);
    }
    return OK;
}

static void M_ShutdownModules(void)
{
    if (m_ConfigListener >= 0) {
        Config_UnsubscribeChanges(m_ConfigListener);
        m_ConfigListener = -1;
    }

    if (TestReplay_IsOpened()) {
        TestReplay_Close();
    }
    if (TestRecorder_IsOpened()) {
        TestRecorder_Close();
    }

    // The Lua bridges are subscribed to the modules they wrap and unsubscribe
    // here, so the modules have to still be standing.
    LUA_Shutdown();

    Subsystem_ShutdownAll();
}

static RESULT M_PrepareSystem(void)
{
    SHELL_SESSION *const s = m_Session;
    ASSERT(s != nullptr);
    const char *const test_replay_path = s->args->test_replay_path;

    FAIL_IF(
        s->args->test_record_path != nullptr
            && s->args->test_replay_path != nullptr,
        "--test-record and --test-replay cannot both be given");

    if (test_replay_path != nullptr) {
        // Allow inferring engine version from outer args for replays lacking
        // embedded info (created with the old directory layout).
        g_TRVersion = s->args->startup.engine_version;
        SHELL_ARGS *const tmp_args = TestReplay_Open(test_replay_path);
        FAIL_IF(
            tmp_args == nullptr, "the replay file '%s' could not be read",
            test_replay_path);
        tmp_args->headless = s->args->headless;
        tmp_args->debug_render_performance = s->args->debug_render_performance;
        ShellSession_UseArgs(s, tmp_args);
    } else if (s->args->headless) {
        return FAIL("--headless can only be given with --test-replay");
    }

    g_TRVersion = s->args->startup.engine_version;
    LOG_INFO("Engine version: %d", g_TRVersion);
    LOG_INFO(
        "Mod: %s",
        s->args->startup.mod != nullptr ? s->args->startup.mod->name : nullptr);
    const SHELL_MOD *const requested_mod = s->args->startup.mod;
    if (s->args->startup.mod_explicit
        && (requested_mod == nullptr || !requested_mod->is_valid)) {
        const char *const name = requested_mod != nullptr
            ? requested_mod->name
            : s->args->startup.mod_request;
        const char *const reason = Shell_GetModRejection(name);
        if (reason != nullptr) {
            return FAIL("%s cannot be played.\n\n%s", name, reason);
        }
        if (requested_mod == nullptr) {
            return FAIL("there is no game called %s.", name);
        }
        return FAIL("%s cannot be played.", name);
    }

    if (s->args->startup.engine_version <= 0
        || s->args->startup.mod == nullptr) {
        const char *const rejections = Shell_GetModRejections();
        if (rejections != nullptr) {
            return FAIL(
                "There is no game to play.\n\nThe following were passed "
                "over:\n%s",
                rejections);
        }
        return FAIL("There is no game to play.");
    }
    if (s->args->startup.mod->mod_type != MOD_DIRECT_LEVEL
        && test_replay_path == nullptr) {
        ShellState_RememberLastPlayedMod(s->args->startup.mod->name);
    }

    Config_RegisterBuiltInOptions();

    GamePath_Init(s->args);

    // The catalogs name the objects, samples and music the subsystem loads
    // look themselves up in.
    MUST(M_LoadCatalog(CATALOG_OBJECTS, "catalog_objects.csv", false));
    MUST(M_LoadCatalog(CATALOG_MUSIC, "catalog_music.csv", false));
    MUST(M_LoadCatalog(CATALOG_SAMPLES, "catalog_samples.csv", true));
    MUST(M_LoadCatalog(CATALOG_LARA_STATES, "catalog_lara_states.csv", false));
    MUST(M_LoadCatalog(CATALOG_LARA_ANIMS, "catalog_lara_anims.csv", false));
    MUST(
        M_LoadCatalog(CATALOG_ITEM_ACTIONS, "catalog_item_actions.csv", false));
    MUST(Subsystem_LoadAll());

    if (test_replay_path != nullptr) {
        TestReplay_Start();
    } else {
        char *engine_config_path =
            GamePath_ExpandVars("%config_dir%/TR%tr_version%X.json5");
        FAIL_IF(
            engine_config_path == nullptr,
            "the settings file path could not be worked out");
        SHOULD(
            Config_Read(
                engine_config_path,
                Shell_GetGameFlowPath(s->args->startup.mod)),
            "The settings start from their defaults");
        Memory_FreePointer(&engine_config_path);

        if (s->args->test_record_path != nullptr) {
            TestRecorder_Open(
                s->args->test_record_path, s->args->original_args);
        }
    }
    m_ConfigListener = Config_SubscribeChanges(M_HandleConfigChange, nullptr);

    Subsystem_ApplyConfigAll();
    return OK;
}

void Shell_RequestModSwitch(const char *const mod_name)
{
    Memory_FreePointer(&m_PendingMod);
    m_PendingMod = Memory_DupStr(mod_name);
}

const char *Shell_GetPendingMod(void)
{
    return m_PendingMod;
}

void Shell_ClearPendingMod(void)
{
    Memory_FreePointer(&m_PendingMod);
}

bool Shell_GetPrevHeadless(void)
{
    return m_PrevHeadless;
}

bool Shell_GetPrevQuiet(void)
{
    return m_PrevQuiet;
}

const SHELL_ARGS *Shell_GetArgs(void)
{
    return m_Session != nullptr ? m_Session->args : nullptr;
}

void Shell_SetHeadless(const bool headless)
{
    ASSERT(m_Session != nullptr);
    SHELL_ARGS *const args = (SHELL_ARGS *)m_Session->args;
    if (args->headless == headless) {
        return;
    }

    args->headless = headless;
    // The clock counts frames either way; only the pacing changes here.
    if (headless) {
        Clock_DisableWait();
    } else {
        Clock_EnableWait();
        Clock_SyncTick();
    }
}

SDL_Window *Shell_GetWindow(void)
{
    return m_Window;
}

int32_t Shell_Main(const SHELL_ARGS *const args)
{
    ASSERT(m_Session == nullptr);
    m_Session = ShellSession_Create();

    SHELL_SESSION *const s = m_Session;
    ShellSession_UseArgs(s, args);

    LOG_INFO("Game directory: %s", GamePath_Get(GAME_PATH_TRX_DIR));

    EXIT_ON_FAIL(M_InitModules(), "TRX could not start");
    EXIT_ON_FAIL(M_PrepareSystem(), "TRX could not start");
    if (s->args->startup.mod == nullptr) {
        Shell_ExitSystem("No --mod specified.");
        return 1;
    }
    GamePath_Init(s->args);
    EXIT_ON_FAIL(M_CreateGameWindow(), "TRX could not open a window");
    EXIT_ON_FAIL(M_CreateGLContext(), "TRX cannot draw the game");
    EXIT_ON_FAIL(Output_Init(), "TRX cannot draw the game");
    if (s->args->headless) {
        Shell_RefreshRendererViewport();
    } else {
        M_ShowWindow();
    }

    GF_Init();
    EXIT_ON_FAIL(
        GF_LoadFromFile(Shell_GetGameFlowPath(s->args->startup.mod)),
        "Failed to load the game flow");

    EXIT_ON_FAIL(
        GameStringManager_LoadForMod(s->args->startup.mod),
        "Failed to load the game strings for mod '%s'",
        s->args->startup.mod->name);

    Savegame_Init();
    SG_Manager_ScanSavedGames();

    LUA_RunGameScript();

    // The settings a recording carries are the ones that exist by now, the
    // game's own among them.
    if (TestReplay_IsOpened()) {
        TestReplay_ApplyDeferredConfig();
    }
    if (TestRecorder_IsOpened()) {
        TestRecorder_WriteConfig();
    }

    Stats_CalculateMaxStats();
    GF_COMMAND gf_cmd;
    EXIT_ON_FAIL(
        GF_DoFrontendSequence(&gf_cmd), "Cannot work out where to start");
    EXIT_ON_FAIL(GF_RunUntilExit(gf_cmd), "The game flow could not carry on");

    if (m_PendingMod != nullptr) {
        if (TestReplay_IsOpened()) {
            TestReplay_Close();
        }
        if (TestRecorder_IsOpened()) {
            TestRecorder_Close();
        }
        // Save flags needed to rebuild args in main() before freeing the
        // session (which owns and will free the args struct).
        m_PrevHeadless = s->args->headless;
        m_PrevQuiet = s->args->quiet;
        M_ShutdownModules();
        ShellSession_Free(m_Session);
        m_Session = nullptr;
        return 0;
    }

    const int32_t replay_exit_code = TestReplay_GetExitCodeOverride();
    return replay_exit_code >= 0 ? replay_exit_code : 0;
}

void Shell_Shutdown(void)
{
    M_ShutdownModules();
    TRX_GL_Context_Detach();
    Log_Shutdown();
    if (m_Session != nullptr) {
        ShellSession_Free(m_Session);
        m_Session = nullptr;
    }
}
