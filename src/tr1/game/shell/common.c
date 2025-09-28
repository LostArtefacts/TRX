#include "game/game_flow.h"
#include "game/savegame.h"
#include "game/shell.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/enum_map.h>
#include <libtrx/game/fmv.h>
#include <libtrx/game/game_string_manager.h>
#include <libtrx/game/lua/common.h>
#include <libtrx/game/output.h>
#include <libtrx/gfx/context.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>

static SDL_Window *m_Window = nullptr;

static void M_CreateGameWindow(void)
{
    m_Window = SDL_CreateWindow(
        "TR1X", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE
            | SDL_WINDOW_OPENGL);

    if (m_Window == nullptr) {
        Shell_ExitSystem("System Error: cannot create window");
    }
}

static void M_CreateGLContext(void)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    if (!GFX_Context_Attach(m_Window)) {
        Shell_ExitSystem("System Error: cannot attach opengl context");
    }
}

static void M_ShowWindow(void)
{
    Shell_SyncToWindow();
    SDL_ShowWindow(m_Window);
    SDL_RaiseWindow(m_Window);
    Shell_RefreshRendererViewport();
}

void Shell_HandleConfigChange(const CONFIG *const old, const CONFIG *const new)
{
    Shell_HandleCommonConfigChange(old, new);

#define L_CHANGED(subject) (old->subject != new->subject)

    if (L_CHANGED(rendering.upscaling_filter)
        || L_CHANGED(rendering.enable_wireframe)
        || L_CHANGED(rendering.wireframe_width)
        || L_CHANGED(rendering.enable_vsync)
        || L_CHANGED(rendering.anisotropy_filter)) {
        Output_ApplyRenderSettings();
    }

    if (L_CHANGED(gameplay.maximum_save_slots) && Savegame_IsInitialised()) {
        Savegame_Shutdown();
        Savegame_Init();
        Savegame_ScanSavedGames();
        Savegame_HighlightNewestSlot();
    }
#undef L_CHANGED
}

SDL_Window *Shell_GetWindow(void)
{
    return m_Window;
}

int32_t Shell_Main(const SHELL_ARGS *args)
{
    ASSERT(args != nullptr);
    LOG_INFO("Game directory: %s", File_GetGameDirectory());

    Shell_InitCommonModules();
    args = Shell_CommonInit(args);
    M_CreateGameWindow();
    M_CreateGLContext();
    Output_Init();
    if (!args->headless) {
        M_ShowWindow();
    }

    GF_Init();
    GF_LoadFromFile(Shell_GetGameFlowPath(args->mod));

    GameStringManager_ClearSourceFiles();
    GameStringManager_AddSourceFile(Shell_GetCommonStringsPath(), false);
    GameStringManager_AddSourceFile(Shell_GetBaseGameStringsPath(), false);
    GameStringManager_AddSourceFile(Shell_GetGameStringsPath(args->mod), true);
    GameStringManager_DiscoverLanguages();
    GameStringManager_ReloadLanguage(g_Config.language);

    Savegame_Init();
    Savegame_ScanSavedGames();
    Savegame_HighlightNewestSlot();

    // Execute global Lua script if provided
    if (g_GameFlow.main_script_path != nullptr) {
        LUA_RESULT res = Lua_EvalFile(g_GameFlow.main_script_path);
        if (res.code != LUA_OK) {
            LOG_ERROR("Lua main script error: %s", res.message);
        }
        Lua_FreeResult(&res);
    }

    GF_COMMAND gf_cmd = GF_DoFrontendSequence();

    bool loop_continue = !Shell_IsExiting();
    while (loop_continue) {
        LOG_INFO(
            "action=%s param=%d", ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action),
            gf_cmd.param);

        switch (gf_cmd.action) {
        case GF_START_GAME:
        case GF_SELECT_GAME: {
            const int32_t level_num = gf_cmd.param;
            const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, level_num);
            const GF_SEQUENCE_CONTEXT seq_ctx =
                gf_cmd.action == GF_SELECT_GAME ? GFSC_SELECT : GFSC_NORMAL;
            if (level != nullptr) {
                gf_cmd = GF_DoLevelSequence(level, seq_ctx);
            }
            break;
        }

        case GF_START_SAVED_GAME: {
            const int16_t slot_num = gf_cmd.param;
            const int16_t level_num = Savegame_GetLevelNumber(slot_num);
            if (level_num < 0) {
                LOG_ERROR("Corrupt save file!");
                gf_cmd = (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
            } else {
                Savegame_BindSlot(slot_num);
                const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, level_num);
                gf_cmd = GF_DoLevelSequence(level, GFSC_SAVED);
            }
            break;
        }

        case GF_RESTART_GAME: {
            const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, gf_cmd.param);
            gf_cmd = GF_InterpretSequence(level, GFSC_RESTART, nullptr);
            break;
        }

        case GF_STORY_SO_FAR:
            gf_cmd = GF_PlayAvailableStory(gf_cmd.param);
            break;

        case GF_START_CINE:
            gf_cmd = GF_DoCutsceneSequence(gf_cmd.param);
            break;

        case GF_START_DEMO:
            gf_cmd = GF_DoDemoSequence(gf_cmd.param);
            break;

        case GF_NOOP:
        case GF_LEVEL_COMPLETE:
            gf_cmd = (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
            break;

        case GF_EXIT_TO_TITLE:
            if (args->level_to_play != nullptr) {
                gf_cmd = (GF_COMMAND) { .action = GF_EXIT_GAME };
            } else if (g_GameFlow.title_level == nullptr) {
                Shell_ExitSystem("Title disabled");
            } else {
                gf_cmd = GF_RunTitle();
            }
            break;

        case GF_EXIT_GAME:
            loop_continue = false;
            break;

        default:
            ASSERT_FAIL_FMT(
                "invalid action (action=%s, param=%d)",
                ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action), gf_cmd.param);
        }
    }

    if (args->level_to_play != nullptr) {
        Memory_FreePointer(&g_GameFlow.level_tables[GFLT_MAIN].levels[0].path);
    }
    return 0;
}

void Shell_Shutdown(void)
{
    Shell_ShutdownCommonModules();
}
