#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/level.h"
#include "game/objects/creatures/big_spider.h"
#include "game/objects/creatures/monk.h"
#include "game/objects/creatures/spider.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/render/common.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/viewport.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/enum_map.h>
#include <libtrx/game/game_string_manager.h>
#include <libtrx/game/music.h>
#include <libtrx/game/objects/creatures/bear.h>
#include <libtrx/game/objects/creatures/wolf.h>
#include <libtrx/game/shell.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>

typedef enum {
    M_MOD_UNKNOWN,
    M_MOD_OG,
    M_MOD_GM,
    M_MOD_CUSTOM_LEVEL,
} M_MOD;

typedef struct {
    M_MOD mod;
    const char *level_to_play;
    int32_t save_to_load;
} SHELL_ARGS;

static SDL_Window *m_Window = nullptr;
static const char *const m_CommonStringsPath = "cfg/TRX_common_strings.json5";

static struct {
    char *game_flow_path;
    char *game_strings_path;
} m_ModPaths[] = {
    [M_MOD_OG] = {
        .game_flow_path = "cfg/TR2X_gameflow.json5",
        .game_strings_path = "cfg/TR2X_strings.json5",
    },
    [M_MOD_GM] = {
        .game_flow_path = "cfg/TR2X_gameflow_gm.json5",
        .game_strings_path = "cfg/TR2X_strings_gm.json5",
    },
    [M_MOD_CUSTOM_LEVEL] = {
        .game_flow_path = "cfg/TR2X_gameflow_level.json5",
        .game_strings_path = "cfg/TR2X_strings_level.json5",
    },
};

static SHELL_ARGS m_Args = {
    .mod = M_MOD_UNKNOWN,
    .level_to_play = nullptr,
    .save_to_load = -1,
};

static bool M_CreateGameWindow(void);
static void M_ShowHelp(void);
static void M_ShowWindow(void);

static struct {
    bool is_fullscreen;
    bool is_maximized;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} m_LastWindowState = {};

static bool M_CreateGameWindow(void)
{
    int32_t result = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    if (result < 0) {
        Shell_ExitSystemFmt(
            "Error while calling SDL_Init: 0x%lx, %s", result, SDL_GetError());
        return false;
    }

    LOG_DEBUG(
        "%d,%d -> %dx%d", g_Config.window.x, g_Config.window.y,
        g_Config.window.width, g_Config.window.height);
    m_Window = SDL_CreateWindow(
        "TR2X", g_Config.window.x, g_Config.window.y, g_Config.window.width,
        g_Config.window.height,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    if (m_Window == nullptr) {
        Shell_ExitSystemFmt("Failed to create SDL window: %s", SDL_GetError());
        return false;
    }

    return true;
}

static void M_ShowHelp(void)
{
    puts("Currently available options:");
    puts("");
    puts("-g/--gold: launch The Golden Mask expansion pack.");
    puts("-l/--level <PATH>: launch a specific level file.");
    puts("-s/--save <NUM>: launch from a specific save slot (starts at 1).");
}

void Shell_HandleConfigChange(const CONFIG *const old, const CONFIG *const new)
{
    Shell_HandleCommonConfigChange(old, new);

#define L_CHANGED(subject) (old->subject != new->subject)

    if (L_CHANGED(rendering.render_mode)) {
        Render_Reset(RENDER_RESET_ALL);
    } else if (
        L_CHANGED(rendering.enable_zbuffer)
        || L_CHANGED(rendering.enable_perspective_filter)
        || L_CHANGED(rendering.upscaling_filter)
        || L_CHANGED(rendering.enable_wireframe)
        || L_CHANGED(rendering.wireframe_width)
        || L_CHANGED(rendering.texture_filter)
        || L_CHANGED(rendering.lighting_contrast)) {
        Render_Reset(RENDER_RESET_PARAMS);
    }

    if (L_CHANGED(visuals.fov) || L_CHANGED(visuals.use_psx_fov)) {
        if (Viewport_GetSystemFOV() == -1) {
            Viewport_AlterFOV(-1);
        }
    }

    if (L_CHANGED(visuals.fog_start) || L_CHANGED(visuals.fog_end)
        || L_CHANGED(visuals.water_color.g) || L_CHANGED(visuals.water_color.b)
        || L_CHANGED(visuals.water_color.r)) {
        Output_ApplyLevelSettings();
    }
#undef L_CHANGED
}

static void M_ShowWindow(void)
{
    Render_Init();
    Shell_SyncToWindow();

    SDL_ShowWindow(m_Window);
    SDL_RaiseWindow(m_Window);
    Shell_RefreshRendererViewport();

    Viewport_AlterFOV(-1);
    Viewport_Reset();
    Render_Reset(RENDER_RESET_PARAMS);
}

bool Shell_ParseArgs(const int32_t arg_count, const char **args)
{
    SHELL_ARGS *const out_args = &m_Args;
    out_args->mod = M_MOD_OG;

    for (int32_t i = 0; i < arg_count; i++) {
        if (!strcmp(args[i], "-h") || !strcmp(args[i], "--help")) {
            M_ShowHelp();
            return false;
        }
        if (!strcmp(args[i], "-g") || !strcmp(args[i], "--gold")
            || !strcmp(args[i], "-gold")) {
            out_args->mod = M_MOD_GM;
        }
        if ((!strcmp(args[i], "-l") || !strcmp(args[i], "--level"))
            && i + 1 < arg_count) {
            out_args->level_to_play = args[i + 1];
            out_args->mod = M_MOD_CUSTOM_LEVEL;
        }
        if ((!strcmp(args[i], "-s") || !strcmp(args[i], "--save"))
            && i + 1 < arg_count) {
            if (String_ParseInteger(args[i + 1], &out_args->save_to_load)) {
                out_args->save_to_load--;
            }
        }
    }
    return true;
}

int32_t Shell_Main(void)
{
    LOG_INFO("Game directory: %s", File_GetGameDirectory());

    if (m_Args.mod == M_MOD_GM) {
        Object_Get(O_MONK_3)->setup_func = Monk3_Setup;
        Object_Get(O_BEAR)->setup_func = Bear_Setup;
        Object_Get(O_WOLF)->setup_func = Wolf_Setup;
    } else {
        Object_Get(O_MONK_1)->setup_func = Monk1_Setup;
        Object_Get(O_SPIDER)->setup_func = Spider_Setup;
        Object_Get(O_BIG_SPIDER)->setup_func = BigSpider_Setup;
    }

    Shell_InitCommonModules();
    Shell_LoadConfig();

    if (!M_CreateGameWindow()) {
        Shell_ExitSystem("Failed to create game window");
        return 1;
    }
    Output_Init();
    M_ShowWindow();

    GF_Init();
    GF_LoadFromFile(m_ModPaths[m_Args.mod].game_flow_path);

    GameStringManager_ClearSourceFiles();
    GameStringManager_AddSourceFile(m_CommonStringsPath, false);
    GameStringManager_AddSourceFile(
        m_ModPaths[M_MOD_OG].game_strings_path, false);
    GameStringManager_AddSourceFile(
        m_ModPaths[m_Args.mod].game_strings_path, true);
    GameStringManager_DiscoverLanguages();
    GameStringManager_ReloadLanguage(g_Config.language);

    Level_Init();

    Savegame_Init();
    Savegame_InitCurrentInfo();
    Savegame_ScanSavedGames();
    Savegame_HighlightNewestSlot();

    if (m_Args.level_to_play != nullptr) {
        Memory_Free(g_GameFlow.level_tables[GFLT_MAIN].levels[0].path);
        g_GameFlow.level_tables[GFLT_MAIN].levels[0].path =
            Memory_DupStr(m_Args.level_to_play);
    }

    GF_COMMAND gf_cmd = m_Args.save_to_load != -1
        ? (GF_COMMAND) { .action = GF_START_SAVED_GAME,
                         .param = m_Args.save_to_load }
        : m_Args.level_to_play != nullptr
        ? (GF_COMMAND) { .action = GF_START_GAME, .param = 0 }
        : GF_DoFrontendSequence();

    bool loop_continue = !Shell_IsExiting();
    while (loop_continue) {
        LOG_INFO(
            "action=%s param=%d", ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action),
            gf_cmd.param);

        switch (gf_cmd.action) {
        case GF_START_GAME:
        case GF_SELECT_GAME: {
            const int32_t level_num = g_GameFlow.single_level >= 0
                ? g_GameFlow.single_level
                : gf_cmd.param;
            const GF_SEQUENCE_CONTEXT seq_ctx =
                gf_cmd.action == GF_SELECT_GAME ? GFSC_SELECT : GFSC_NORMAL;
            const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, level_num);
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
            if (g_GameFlow.title_level == nullptr) {
                gf_cmd = g_GameFlow.cmd_title;
                if (gf_cmd.action == GF_NOOP
                    || gf_cmd.action == GF_EXIT_TO_TITLE) {
                    Shell_ExitSystem("Title disabled & no replacement");
                    return 1;
                }
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

    Config_Write();
    if (m_Args.level_to_play != nullptr) {
        Memory_FreePointer(&g_GameFlow.level_tables[GFLT_MAIN].levels[0].path);
    }
    return 0;
}

void Shell_Shutdown(void)
{
    Render_Shutdown();
    Shell_ShutdownCommonModules();
}

const char *Shell_GetConfigPath(void)
{
    return "cfg/TR2X.json5";
}

const char *Shell_GetGameFlowPath(void)
{
    return m_ModPaths[m_Args.mod].game_flow_path;
}

SDL_Window *Shell_GetWindow(void)
{
    return m_Window;
}
