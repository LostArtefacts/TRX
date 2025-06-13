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

static const UI_SETTINGS_ENUM_ENTRY m_WallGlitchEnumEntries[] = {
    { WALL_GLITCH_FIXED, GS_ID(GAMEPLAY_SETTINGS_WALL_GLITCH_FIXED) },
    { WALL_GLITCH_TR1, GS_ID(GAMEPLAY_SETTINGS_WALL_GLITCH_TR1) },
    { WALL_GLITCH_TR2, GS_ID(GAMEPLAY_SETTINGS_WALL_GLITCH_TR2) },
    { -1, nullptr },
};

static const UI_SETTINGS_OPTION m_GeneralOptions[] = {
    {
        .target = &g_Config.gameplay.enable_legal,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_LEGAL),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_LEGAL_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_fmv,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_FMV),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_FMV_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_demo,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_DEMO),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_DEMO_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.enable_cutscenes,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_CUTSCENES),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_CUTSCENES_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_loading_screens,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_LOADING_SCREENS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_LOADING_SCREENS_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.enable_game_modes,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_GAME_MODES),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_GAME_MODES_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_save_crystals,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_SAVE_CRYSTALS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_SAVE_CRYSTALS_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.enable_auto_item_selection,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_AUTO_ITEM_SELECTION),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_AUTO_ITEM_SELECTION_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_item_examining,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_ITEM_EXAMINING),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_ITEM_EXAMINING_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.stat_detail_mode,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_STAT_DETAIL_MODE),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_STAT_DETAIL_MODE_DESCRIPTION),
        .option_type = COT_ENUM,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_StatDetailModeEnumEntries,
    },

    {
        .target = &g_Config.gameplay.enable_compass_stats,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_COMPASS_STATS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_COMPASS_STATS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_total_stats,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TOTAL_STATS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_TOTAL_STATS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_timer_in_inventory,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TIMER_IN_INVENTORY),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_TIMER_IN_INVENTORY_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_deaths_counter,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_DEATHS_COUNTER),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_DEATHS_COUNTER_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.maximum_save_slots,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_MAXIMUM_SAVE_SLOTS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_MAXIMUM_SAVE_SLOTS_DESCRIPTION),
        .option_type = COT_INT32,
        .min_value = 1,
        .max_value = 1000,
        .delta_fast = 10,
        .delta_slow = 1,
    },
#endif

    {
        .target = &g_Config.gameplay.enable_enhanced_saves,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_ENHANCED_SAVES),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_ENHANCED_SAVES_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.revert_to_pistols,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_REMEMBER_GUNS_BETWEEN_LEVELS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_REMEMBER_GUNS_BETWEEN_LEVELS_DESCRIPTION),
        .option_type = COT_INVERTED_BOOL,
    },

    {
        .target = &g_Config.gameplay.restore_ps1_enemies,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_RESTORE_PS1_ENEMIES),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_RESTORE_PS1_ENEMIES_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.change_pierre_spawn,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_CHANGE_PIERRE_SPAWN),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_CHANGE_PIERRE_SPAWN_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.disable_trex_collision,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_TREX_COLLISION),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_DISABLE_TREX_COLLISION_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.enable_enemy_rotation,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_ENEMY_ROTATION),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_ENEMY_ROTATION_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 2
    {
        .target = &g_Config.gameplay.enable_ally_targeting,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_ALLY_TARGETING),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_ALLY_TARGETING_DESCRIPTION),
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
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_RESPONSIVE_PASSPORT_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.input.enable_numeric_keys,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_NUMERIC_KEYS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_NUMERIC_KEYS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_walk_to_items,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_WALK_TO_ITEMS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_WALK_TO_ITEMS_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.input.enable_tr3_sidesteps,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR3_SIDESTEPS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR3_SIDESTEPS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_enhanced_look,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_ENHANCED_LOOK),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_ENHANCED_LOOK_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_tr2_jumping,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR2_JUMPING),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR2_JUMPING_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_jump_twists,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_JUMP_TWISTS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_JUMP_TWISTS_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.enable_lean_jumping,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_LEAN_JUMPING),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_LEAN_JUMPING_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_swing_cancel,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_SWING_CANCEL),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_SWING_CANCEL_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_smooth_wall_deflect,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_SMOOTH_WALL_DEFLECT),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_SMOOTH_WALL_DEFLECT_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_step_roll_boost,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_STEP_ROLL_BOOST),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_STEP_ROLL_BOOST_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_uw_roll,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_UW_ROLL),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_UW_ROLL_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_tr2_swimming,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR2_SWIMMING),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR2_SWIMMING_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_tr2_swim_cancel,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR2_SWIM_CANCEL),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_TR2_SWIM_CANCEL_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_wading,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_WADING),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_WADING_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.target_mode,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_TARGET_MODE),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_TARGET_MODE_DESCRIPTION),
        .option_type = COT_ENUM,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_TargetModeEnumEntries,
    },

    {
        .target = &g_Config.gameplay.enable_target_change,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_TARGET_CHANGE),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_TARGET_CHANGE_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_inverted_look,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_INVERTED_LOOK),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_ENABLE_INVERTED_LOOK_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.camera_speed,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_CAMERA_SPEED),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_CAMERA_SPEED_DESCRIPTION),
        .option_type = COT_INT32,
        .min_value = 1,
        .max_value = 10,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.input.enable_buffering,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_BUFFERING),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_BUFFERING_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_shotgun_targeting,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_SHOTGUN_TARGETING),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_SHOTGUN_TARGETING_DESCRIPTION),
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
        .description_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_CHEATS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.enable_console,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_CONSOLE),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_ENABLE_CONSOLE_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.start_lara_hitpoints,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_START_LARA_HITPOINTS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_START_LARA_HITPOINTS_DESCRIPTION),
        .option_type = COT_INT32,
        .min_value = 1,
        .max_value = LARA_MAX_HITPOINTS,
        .delta_slow = 10,
        .delta_fast = 100,
    },

    {
        .target = &g_Config.gameplay.disable_medpacks,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_MEDPACKS),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_MEDPACKS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.disable_healing_between_levels,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_HEALING_BETWEEN_LEVELS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_DISABLE_HEALING_BETWEEN_LEVELS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.disable_shotgun,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_SHOTGUN),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_SHOTGUN_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.disable_magnums,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_MAGNUMS),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_MAGNUMS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.disable_uzis,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_UZIS),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_DISABLE_UZIS_DESCRIPTION),
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
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_FLOOR_DATA_ISSUES_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.fix_texture_issues,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_TEXTURE_ISSUES),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_TEXTURE_ISSUES_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.fix_item_rots,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_ITEM_ROTS),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_FIX_ITEM_ROTS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.audio.load_music_triggers,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_LOAD_MUSIC_TRIGGERS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_LOAD_MUSIC_TRIGGERS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.fix_animated_sprites,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_ANIMATED_SPRITES),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_ANIMATED_SPRITES_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.fix_bridge_collision,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_BRIDGE_COLLISION),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_BRIDGE_COLLISION_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 2
    {
        .target = &g_Config.visuals.fix_glide_cameras,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_GLIDE_CAMERAS),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_GLIDE_CAMERAS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_walk_run_jump,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_WALK_RUN_JUMP),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_WALK_RUN_JUMP_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_flare_throw_priority,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_FLARE_THROW_PRIORITY),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_FLARE_THROW_PRIORITY_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_m16_accuracy,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_M16_ACCURACY),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_FIX_M16_ACCURACY_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.fix_descending_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_DESCENDING_GLITCH),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_DESCENDING_GLITCH_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.wall_glitch_mode,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_WALL_GLITCH_MODE),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_WALL_GLITCH_MODE_DESCRIPTION),
        .option_type = COT_ENUM,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_WallGlitchEnumEntries,
    },

    {
        .target = &g_Config.gameplay.fix_wade_wall_hit,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_WADE_WALL_HIT),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_WADE_WALL_HIT_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_water_exit,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_WATER_EXIT),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_FIX_WATER_EXIT_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_qwop_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_QWOP_GLITCH),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_FIX_QWOP_GLITCH_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_step_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_STEP_GLITCH),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_FIX_STEP_GLITCH_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_item_duplication_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_ITEM_DUPLICATION_GLITCH),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_ITEM_DUPLICATION_GLITCH_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 2
    {
        .target = &g_Config.gameplay.fix_free_flare_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_FREE_FLARE_GLITCH),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_FREE_FLARE_GLITCH_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.gameplay.fix_pickup_drift_glitch,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_PICKUP_DRIFT_GLITCH),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_PICKUP_DRIFT_GLITCH_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.fix_alligator_ai,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_ALLIGATOR_AI),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_FIX_ALLIGATOR_AI_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.gameplay.fix_bear_ai,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_BEAR_AI),
        .description_id = GS_ID(GAMEPLAY_SETTINGS_FIX_BEAR_AI_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.audio.fix_tihocan_secret_sound,
        .label_id = GS_ID(GAMEPLAY_SETTINGS_FIX_TIHOCAN_SECRET_SOUND),
        .description_id =
            GS_ID(GAMEPLAY_SETTINGS_FIX_TIHOCAN_SECRET_SOUND_DESCRIPTION),
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
