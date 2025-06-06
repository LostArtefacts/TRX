#include "game/ui/dialogs/gameplay_settings.h"

#include <libtrx/config.h>

static const UI_SETTINGS_OPTION m_GeneralOptions[] = {
    {
        .target = &g_Config.gameplay.enable_legal,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_LEGAL),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_fmv,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_FMV),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_cutscenes,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_CUTSCENES),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_game_modes,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_GAME_MODES),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_auto_item_selection,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_AUTO_ITEM_SELECTION),
        .option_type = COT_BOOL,
    },

    {
        .target = nullptr,
    },
};

static const UI_SETTINGS_OPTION m_ControlOptions[] = {
    {
        .target = &g_Config.input.enable_responsive_passport,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_RESPONSIVE_PASSPORT),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.input.enable_tr3_sidesteps,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR3_SIDESTEPS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.camera_speed,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_CAMERA_SPEED),
        .option_type = COT_INT32,
        .min_value = 1,
        .max_value = 10,
    },

    {
        .target = nullptr,
    },
};

static const UI_SETTINGS_OPTION m_GameplayModOptions[] = {
    {
        .target = &g_Config.gameplay.enable_cheats,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_CHEATS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_console,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_CONSOLE),
        .option_type = COT_BOOL,
    },

    {
        .target = nullptr,
    },
};

static const UI_SETTINGS_OPTION m_GameplayFixOptions[] = {
    {
        .target = &g_Config.gameplay.fix_floor_data_issues,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_FLOOR_DATA_ISSUES),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.fix_texture_issues,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_TEXTURE_ISSUES),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.fix_item_rots,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_ITEM_ROTS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_bridge_collision,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_BRIDGE_COLLISION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.fix_glide_cameras,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_GLIDE_CAMERAS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_walk_run_jump,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_WALK_RUN_JUMP),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_flare_throw_priority,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_FLARE_THROW_PRIORITY),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_m16_accuracy,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_M16_ACCURACY),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_bear_ai,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_BEAR_AI),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_qwop_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_QWOP_GLITCH),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_item_duplication_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_ITEM_DUPLICATION_GLITCH),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_step_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_STEP_GLITCH),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_free_flare_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_FREE_FLARE_GLITCH),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_pickup_drift_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_PICKUP_DRIFT_GLITCH),
        .option_type = COT_BOOL,
    },

    {
        .target = nullptr,
    },
};

static const UI_SETTINGS_TAB m_Tabs[] = {
    { GS_ID(GAMEPLAY_SETTINGS_GENERAL_TAB), m_GeneralOptions },
    { GS_ID(GAMEPLAY_SETTINGS_CONTROLS_TAB), m_ControlOptions },
    { GS_ID(GAMEPLAY_SETTINGS_MODS_TAB), m_GameplayModOptions },
    { GS_ID(GAMEPLAY_SETTINGS_FIXES_TAB), m_GameplayFixOptions },
};

void UI_GameplaySettings_Init(UI_GAMEPLAY_SETTINGS_STATE *const s)
{
    UI_Settings_InitWithTabs(
        s, GS_ID(GAMEPLAY_SETTINGS_TITLE), sizeof(m_Tabs) / sizeof(m_Tabs[0]),
        m_Tabs);
}

void UI_GameplaySettings_Free(UI_GAMEPLAY_SETTINGS_STATE *const s)
{
    UI_Settings_Free(s);
}

bool UI_GameplaySettings_Control(UI_GAMEPLAY_SETTINGS_STATE *const s)
{
    return UI_Settings_Control(s);
}

void UI_GameplaySettings(UI_GAMEPLAY_SETTINGS_STATE *const s)
{
    UI_Settings(s);
}
