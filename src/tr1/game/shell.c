#include "game/shell.h"

#include "game/clock.h"
#include "game/console/common.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/level.h"
#include "game/music.h"
#include "game/option.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/s_shell.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/enum_map.h>
#include <libtrx/filesystem.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/game/ui.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TIMESTAMP_SIZE 20

typedef enum {
    M_MOD_UNKNOWN,
    M_MOD_OG,
    M_MOD_UB,
    M_MOD_DEMO_PC,
    M_MOD_CUSTOM_LEVEL,
} M_MOD;

typedef struct {
    M_MOD mod;
    const char *level_to_play;
    int32_t save_to_load;
} SHELL_ARGS;

static struct {
    char *game_flow_path;
    char *game_strings_path;
} m_ModPaths[] = {
    [M_MOD_OG] = {
        .game_flow_path = "cfg/TR1X_gameflow.json5",
        .game_strings_path = "cfg/TR1X_strings.json5",
    },
    [M_MOD_UB] = {
        .game_flow_path = "cfg/TR1X_gameflow_ub.json5",
        .game_strings_path = "cfg/TR1X_strings_ub.json5",
    },
    [M_MOD_DEMO_PC] = {
        .game_flow_path = "cfg/TR1X_gameflow_demo_pc.json5",
        .game_strings_path = "cfg/TR1X_strings_demo_pc.json5",
    },
    [M_MOD_CUSTOM_LEVEL] = {
        .game_flow_path = "cfg/TR1X_gameflow_level.json5",
        .game_strings_path = "cfg/TR1X_strings_level.json5",
    },
};

static SHELL_ARGS m_Args = {
    .mod = M_MOD_UNKNOWN,
    .level_to_play = nullptr,
    .save_to_load = -1,
};

static const char *m_CurrentGameFlowPath;

static void M_ShowHelp(void);
static void M_LoadConfig(void);
static void M_HandleConfigChange(const EVENT *event, void *data);

static void M_ShowHelp(void)
{
    puts("Currently available options:");
    puts("");
    puts("-g/--gold: launch The Unfinished Business expansion pack.");
    puts("   --demo-pc: launch the PC demo level file.");
    puts("-l/--level <PATH>: launch a specific level file.");
    puts("-s/--save <NUM>: launch from a specific save slot (starts at 1).");
}

static void M_HandleConfigChange(const EVENT *const event, void *const data)
{
    const CONFIG *const old = &g_Config;
    const CONFIG *const new = &g_SavedConfig;

#define CHANGED(subject) (old->subject != new->subject)

    if (CHANGED(audio.sound_volume)) {
        Sound_SetMasterVolume(g_Config.audio.sound_volume);
    }
    if (CHANGED(audio.music_volume)) {
        Music_SetVolume(g_Config.audio.music_volume);
    }

    if (CHANGED(gameplay.maximum_save_slots) && Savegame_IsInitialised()) {
        Savegame_Shutdown();
        Savegame_Init();
        Savegame_ScanSavedGames();
        Savegame_HighlightNewestSlot();
    }

    Output_ApplyRenderSettings();
}

static void M_LoadConfig(void)
{
    Config_Read();
    Config_SubscribeChanges(M_HandleConfigChange, nullptr);

    Sound_SetMasterVolume(g_Config.audio.sound_volume);
    Music_SetVolume(g_Config.audio.music_volume);
}

void Shell_Shutdown(void)
{
    Console_Shutdown();
    GameBuf_Shutdown();
    Savegame_Shutdown();

    GameStringTable_Shutdown();
    GF_Shutdown();

    Overlay_Shutdown();
    Output_Shutdown();
    Input_Shutdown();
    Music_Shutdown();
    Sound_Shutdown();
    UI_Shutdown();
    Text_Shutdown();
    Config_Shutdown();
    Log_Shutdown();
}

const char *Shell_GetConfigPath(void)
{
    return "cfg/TR1X.json5";
}

const char *Shell_GetGameFlowPath(void)
{
    return m_ModPaths[m_Args.mod].game_flow_path;
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
            out_args->mod = M_MOD_UB;
        }
        if (!strcmp(args[i], "--demo-pc") || !strcmp(args[i], "-demo_pc")) {
            out_args->mod = M_MOD_DEMO_PC;
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
    GameString_Init();
    EnumMap_Init();
    Config_Init();

    Text_Init();
    UI_Init();

    Input_Init();
    Sound_Init();
    Music_Init();

    M_LoadConfig();

    Clock_Init();

    S_Shell_CreateWindow();
    S_Shell_Init();

    Random_Seed();

    if (!Output_Init()) {
        Shell_ExitSystem("Could not initialise video system");
        return 1;
    }
    Screen_Init();

    GF_Init();
    GF_LoadFromFile(m_ModPaths[m_Args.mod].game_flow_path);
    GameStringTable_Init();
    if (m_Args.mod != M_MOD_OG) {
        GameStringTable_Load(m_ModPaths[M_MOD_OG].game_strings_path, false);
    }
    GameStringTable_Load(m_ModPaths[m_Args.mod].game_strings_path, true);
    GameStringTable_Apply(nullptr);

    Savegame_Init();
    Savegame_ScanSavedGames();
    Savegame_HighlightNewestSlot();
    GameBuf_Init();
    Console_Init();

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
            if (m_Args.level_to_play != nullptr) {
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

    Config_Write();
    EnumMap_Shutdown();
    GameString_Shutdown();

    if (m_Args.level_to_play != nullptr) {
        Memory_FreePointer(&g_GameFlow.level_tables[GFLT_MAIN].levels[0].path);
    }
    return 0;
}

void Shell_ProcessInput(void)
{
    if (g_InputDB.screenshot) {
        Screenshot_Make(g_Config.rendering.screenshot_format);
    }

    if (g_InputDB.toggle_bilinear_filter) {
        g_Config.rendering.texture_filter =
            (g_Config.rendering.texture_filter + 1) % GFX_TF_NUMBER_OF;

        switch (g_Config.rendering.texture_filter) {
        case GFX_TF_NN:
            Console_Log(GS(OSD_TEXTURE_FILTER_SET), GS(OSD_TEXTURE_FILTER_NN));
            break;
        case GFX_TF_BILINEAR:
            Console_Log(
                GS(OSD_TEXTURE_FILTER_SET), GS(OSD_TEXTURE_FILTER_BILINEAR));
            break;
        case GFX_TF_NUMBER_OF:
            break;
        }

        Config_Write();
    }

    if (g_InputDB.toggle_trapezoid_filter) {
        g_Config.rendering.enable_trapezoid_filter ^= true;
        Console_Log(
            g_Config.rendering.enable_trapezoid_filter
                ? GS(OSD_TRAPEZOID_FILTER_ON)
                : GS(OSD_TRAPEZOID_FILTER_OFF));
        Config_Write();
    }

    if (g_InputDB.toggle_fps_counter) {
        g_Config.rendering.enable_fps_counter ^= true;
        Console_Log(
            g_Config.rendering.enable_fps_counter ? GS(OSD_FPS_COUNTER_ON)
                                                  : GS(OSD_FPS_COUNTER_OFF));
        Config_Write();
    }

    if (g_InputDB.toggle_fullscreen) {
        S_Shell_ToggleFullscreen();
    }

    if (g_InputDB.turbo_cheat) {
        Clock_CycleTurboSpeed(!g_Input.slow);
    }
}
