// NOTE: this is an included file, not a compile unit on its own.
// This is to avoid exposing symbols.

static GF_LEVEL_SETTINGS m_DefaultSettings = {
    .sfx_path = nullptr,
};

static M_SEQUENCE_EVENT_HANDLER m_SequenceEventHandlers[] = {
    // clang-format off
    // Events without arguments
    { GFS_ENABLE_SUNSET,     nullptr, nullptr },
    { GFS_REMOVE_WEAPONS,    nullptr, nullptr },
    { GFS_REMOVE_SCIONS,     nullptr, nullptr },
    { GFS_REMOVE_AMMO,       nullptr, nullptr },
    { GFS_REMOVE_FLARES,     nullptr, nullptr },
    { GFS_REMOVE_MEDIPACKS,  nullptr, nullptr },
    { GFS_LEVEL_COMPLETE,    nullptr, nullptr },
    { GFS_LEVEL_STATS,       nullptr, nullptr },
    { GFS_EXIT_TO_TITLE,     nullptr, nullptr },

    // Events with integer arguments
    { GFS_SET_START_ANIM,    M_HandleIntEvent, "anim" },
    { GFS_LOOP_GAME,         M_HandleIntEvent, "level_id" },
    { GFS_PLAY_CUTSCENE,     M_HandleIntEvent, "cutscene_id" },
    { GFS_PLAY_FMV,          M_HandleIntEvent, "fmv_id" },
    { GFS_PLAY_MUSIC,        M_HandleIntEvent, "music_track" },
    { GFS_SETUP_BACON_LARA,  M_HandleIntEvent, "anchor_room" },
    { GFS_DISABLE_FLOOR,     M_HandleIntEvent, "height" },

    // Special cases with custom handlers
    { GFS_LOADING_SCREEN,    M_HandlePictureEvent, nullptr },
    { GFS_DISPLAY_PICTURE,   M_HandlePictureEvent, nullptr },
    { GFS_TOTAL_STATS,       M_HandleTotalStatsEvent, nullptr },
    { GFS_ADD_ITEM,          M_HandleAddItemEvent, nullptr },
    { GFS_ADD_SECRET_REWARD, M_HandleAddItemEvent, nullptr },

    // Sentinel to mark the end of the table
    { (GF_SEQUENCE_EVENT_TYPE)-1, nullptr, nullptr },
    // clang-format on
};

static void M_LoadSettings(
    const M_CONTEXT *const ctx, JSON_OBJECT *const obj,
    GF_LEVEL_SETTINGS *const settings)
{
    M_LoadCommonSettings(ctx, obj, settings);

    {
        const char *tmp_s =
            JSON_ObjectGetString(obj, "sfx_path", JSON_INVALID_STRING);
        if (tmp_s != JSON_INVALID_STRING) {
            settings->sfx_path = Memory_DupStr(tmp_s);
        }
    }
}

static void M_LoadLevelGameSpecifics(
    const M_CONTEXT *const ctx, JSON_OBJECT *const jlvl_obj,
    GF_LEVEL *const level)
{
    M_LoadSettings(ctx, jlvl_obj, &level->settings);
}

static M_SEQUENCE_EVENT_HANDLER *M_GetSequenceEventHandlers(void)
{
    return m_SequenceEventHandlers;
}

static void M_LoadRoot(const M_CONTEXT *const ctx, JSON_OBJECT *const obj)
{
    ctx->gf->settings = m_DefaultSettings;
    M_LoadSettings(ctx, obj, &ctx->gf->settings);
    M_LoadGlobalInjections(ctx, obj);
}
