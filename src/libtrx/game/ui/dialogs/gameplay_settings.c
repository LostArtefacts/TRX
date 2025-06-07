#include "game/ui/dialogs/gameplay_settings.h"

#include "config.h"
#include "game/lara/const.h"

#if TR_VERSION == 1
static const UI_SETTINGS_ENUM_ENTRY m_StatDetailModeEnumEntries[] = {
    { SDM_MINIMAL, GS_ID(GAMEPLAY_SETTINGS_STAT_DETAIL_MODE_MINIMAL) },
    { SDM_DETAILED, GS_ID(GAMEPLAY_SETTINGS_STAT_DETAIL_MODE_DETAILED) },
    { SDM_FULL, GS_ID(GAMEPLAY_SETTINGS_STAT_DETAIL_MODE_FULL) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_TargetModeEnumEntries[] = {
    { TLM_FULL, GS_ID(GAMEPLAY_SETTINGS_TARGET_LOCK_MODE_FULL) },
    { TLM_SEMI, GS_ID(GAMEPLAY_SETTINGS_TARGET_LOCK_MODE_SEMI) },
    { TLM_NONE, GS_ID(GAMEPLAY_SETTINGS_TARGET_LOCK_MODE_NONE) },
    { -1, nullptr },
};
#endif

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

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_demo,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_DEMO),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.enable_cutscenes,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_CUTSCENES),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_loading_screens,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_LOADING_SCREENS),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.enable_game_modes,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_GAME_MODES),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_save_crystals,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_SAVE_CRYSTALS),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.enable_auto_item_selection,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_AUTO_ITEM_SELECTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_item_examining,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_ITEM_EXAMINING),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.stat_detail_mode,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_STAT_DETAIL_MODE),
        .option_type = COT_ENUM,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_StatDetailModeEnumEntries,
    },

    {
        .target = &g_Config.gameplay.enable_compass_stats,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_COMPASS_STATS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_total_stats,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TOTAL_STATS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_timer_in_inventory,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TIMER_IN_INVENTORY),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_deaths_counter,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_DEATHS_COUNTER),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.maximum_save_slots,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_MAXIMUM_SAVE_SLOTS),
        .option_type = COT_INT32,
        .min_value = 1,
        .max_value = 1000,
        .delta_fast = 10,
        .delta_slow = 1,
    },

    {
        .target = &g_Config.gameplay.enable_enhanced_saves,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_ENHANCED_SAVES),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.revert_to_pistols,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_REMEMBER_GUNS_BETWEEN_LEVELS),
        .option_type = COT_INVERTED_BOOL,
    },

    {
        .target = &g_Config.gameplay.restore_ps1_enemies,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_RESTORE_PS1_ENEMIES),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.change_pierre_spawn,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_CHANGE_PIERRE_SPAWN),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.disable_trex_collision,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_TREX_COLLISION),
        .option_type = COT_BOOL,
    },
#endif

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

#if TR_VERSION == 1
    {
        .target = &g_Config.input.enable_numeric_keys,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_NUMERIC_KEYS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_walk_to_items,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_WALK_TO_ITEMS),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.input.enable_tr3_sidesteps,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR3_SIDESTEPS),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_enhanced_look,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_ENHANCED_LOOK),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_tr2_jumping,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR2_JUMPING),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_jump_twists,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_JUMP_TWISTS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_lean_jumping,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_LEAN_JUMPING),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_swing_cancel,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_SWING_CANCEL),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_uw_roll,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_UW_ROLL),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_tr2_swimming,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR2_SWIMMING),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_tr2_swim_cancel,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR2_SWIM_CANCEL),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_wading,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_WADING),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.target_mode,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_TARGET_MODE),
        .option_type = COT_ENUM,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_TargetModeEnumEntries,
    },

    {
        .target = &g_Config.gameplay.enable_target_change,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TARGET_CHANGE),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_inverted_look,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_INVERTED_LOOK),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.camera_speed,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_CAMERA_SPEED),
        .option_type = COT_INT32,
        .min_value = 1,
        .max_value = 10,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.input.enable_buffering,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_BUFFERING),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_shotgun_targeting,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_SHOTGUN_TARGETING),
        .option_type = COT_BOOL,
    },
#endif

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

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.start_lara_hitpoints,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_START_LARA_HITPOINTS),
        .option_type = COT_INT32,
        .min_value = 1,
        .max_value = LARA_MAX_HITPOINTS,
        .delta_slow = 10,
        .delta_fast = 100,
    },

    {
        .target = &g_Config.gameplay.disable_medpacks,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_MEDPACKS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.disable_healing_between_levels,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_HEALING_BETWEEN_LEVELS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.disable_shotgun,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_SHOTGUN),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.disable_magnums,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_MAGNUMS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.disable_uzis,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_UZIS),
        .option_type = COT_BOOL,
    },
#endif

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

#if TR_VERSION == 1
    {
        .target = &g_Config.audio.load_music_triggers,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_LOAD_MUSIC_TRIGGERS),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.fix_animated_sprites,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_ANIMATED_SPRITES),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_bridge_collision,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_BRIDGE_COLLISION),
        .option_type = COT_BOOL,
    },
#elif TR_VERSION == 2
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
#endif

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.fix_descending_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_DESCENDING_GLITCH),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_wall_jump_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_WALL_JUMP_GLITCH),
        .option_type = COT_BOOL,
    },
#endif

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

#if TR_VERSION == 2
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
#endif

#if TR__VERSION == 1
    {
        .target = &g_Config.gameplay.fix_alligator_ai,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_ALLIGATOR_AI),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.fix_bear_ai,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_BEAR_AI),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.audio.fix_tihocan_secret_sound,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_TIHOCAN_SECRET_SOUND),
        .option_type = COT_BOOL,
    },
#endif

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
