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
#include <libtrx/game/ui/common.h>
#include <libtrx/memory.h>

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
} M_MOD;

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
};
static M_MOD m_ActiveMod = M_MOD_UNKNOWN;

static const char *m_CurrentGameFlowPath;

static void M_LoadConfig(void);
static void M_HandleConfigChange(const EVENT *event, void *data);

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

void Shell_Init(
    const char *const game_flow_path, const char *const game_strings_path)
{
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
        return;
    }
    Screen_Init();

    GF_Init();
    GF_Load(game_flow_path);
    GameStringTable_LoadFromFile(game_strings_path);
    GameStringTable_Apply(nullptr);

    Savegame_Init();
    Savegame_ScanSavedGames();
    Savegame_HighlightNewestSlot();
    GameBuf_Init();
    Console_Init();
}

void Shell_Shutdown(void)
{
    Console_Shutdown();
    GameBuf_Shutdown();
    Savegame_Shutdown();
    GF_Shutdown();

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
    return m_ModPaths[m_ActiveMod].game_flow_path;
}

void Shell_Main(void)
{
    m_ActiveMod = M_MOD_OG;

    char **args = nullptr;
    int32_t arg_count = 0;
    S_Shell_GetCommandLine(&arg_count, &args);
    for (int32_t i = 0; i < arg_count; i++) {
        if (!strcmp(args[i], "-gold")) {
            m_ActiveMod = M_MOD_UB;
        }
        if (!strcmp(args[i], "-demo_pc")) {
            m_ActiveMod = M_MOD_DEMO_PC;
        }
    }
    for (int i = 0; i < arg_count; i++) {
        Memory_FreePointer(&args[i]);
    }
    Memory_FreePointer(&args);

    GameString_Init();
    EnumMap_Init();
    Config_Init();

    Shell_Init(
        m_ModPaths[m_ActiveMod].game_flow_path,
        m_ModPaths[m_ActiveMod].game_strings_path);

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
            if (g_GameFlow.title_level == nullptr) {
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

    if (g_InputDB.toggle_perspective_filter) {
        g_Config.rendering.enable_perspective_filter ^= true;
        Console_Log(
            g_Config.rendering.enable_perspective_filter
                ? GS(OSD_PERSPECTIVE_FILTER_ON)
                : GS(OSD_PERSPECTIVE_FILTER_OFF));
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
