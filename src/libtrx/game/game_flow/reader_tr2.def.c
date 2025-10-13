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
    { GFS_REMOVE_AMMO,       nullptr, nullptr },
    { GFS_REMOVE_FLARES,     nullptr, nullptr },
    { GFS_REMOVE_MEDIPACKS,  nullptr, nullptr },
    { GFS_LEVEL_COMPLETE,    nullptr, nullptr },
    { GFS_LEVEL_STATS,       nullptr, nullptr },
    { GFS_EXIT_TO_TITLE,     nullptr, nullptr },

    // Events with integer arguments
    { GFS_SET_CAMERA_ANGLE,  M_HandleIntEvent, "angle" },
    { GFS_SET_START_ANIM,    M_HandleIntEvent, "anim" },
    { GFS_LOOP_GAME,         M_HandleIntEvent, "level_id" },
    { GFS_PLAY_CUTSCENE,     M_HandleIntEvent, "cutscene_id" },
    { GFS_PLAY_FMV,          M_HandleIntEvent, "fmv_id" },
    { GFS_PLAY_MUSIC,        M_HandleIntEvent, "music_track" },
    { GFS_SETUP_BACON_LARA,  M_HandleIntEvent, "anchor_room" },
    { GFS_DISABLE_FLOOR,     M_HandleIntEvent, "height" },

    // Special cases with custom handlers
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

static GF_COMMAND M_LoadCommand(
    const M_CONTEXT *const ctx, JSON_OBJECT *const jcmd,
    const GF_COMMAND fallback)
{
    if (jcmd == nullptr) {
        return fallback;
    }

    const char *const action_str =
        JSON_ObjectGetString(jcmd, "action", JSON_INVALID_STRING);
    const int32_t param = JSON_ObjectGetInt(jcmd, "param", -1);
    if (action_str == JSON_INVALID_STRING) {
        Shell_ExitSystemFmt(
            "%s: Unknown game flow action: %s", ctx->script_path, action_str);
        return fallback;
    }

    const GF_ACTION action =
        ENUM_MAP_GET(GF_ACTION, action_str, (GF_ACTION)-1234);
    if (action == (GF_ACTION)-1234) {
        Shell_ExitSystemFmt(
            "%s: Unknown game flow action: %s", ctx->script_path, action_str);
        return fallback;
    }

    return (GF_COMMAND) { .action = action, .param = param };
}

static void M_LoadRoot(const M_CONTEXT *const ctx, JSON_OBJECT *const obj)
{
    ctx->gf->cmd_init = M_LoadCommand(
        ctx, JSON_ObjectGetObject(obj, "cmd_init"),
        (GF_COMMAND) { .action = GF_EXIT_TO_TITLE });
    ctx->gf->cmd_title = M_LoadCommand(
        ctx, JSON_ObjectGetObject(obj, "cmd_title"),
        (GF_COMMAND) { .action = GF_NOOP });
    ctx->gf->cmd_death_demo_mode = M_LoadCommand(
        ctx, JSON_ObjectGetObject(obj, "cmd_death_demo_mode"),
        (GF_COMMAND) { .action = GF_EXIT_TO_TITLE });
    ctx->gf->cmd_death_in_game = M_LoadCommand(
        ctx, JSON_ObjectGetObject(obj, "cmd_death_in_game"),
        (GF_COMMAND) { .action = GF_NOOP });
    ctx->gf->cmd_demo_interrupt = M_LoadCommand(
        ctx, JSON_ObjectGetObject(obj, "cmd_demo_interrupt"),
        (GF_COMMAND) { .action = GF_EXIT_TO_TITLE });
    ctx->gf->cmd_demo_end = M_LoadCommand(
        ctx, JSON_ObjectGetObject(obj, "cmd_demo_end"),
        (GF_COMMAND) { .action = GF_EXIT_TO_TITLE });

    ctx->gf->is_demo_version = JSON_ObjectGetBool(obj, "demo_version", false);

    ctx->gf->settings = m_DefaultSettings;
    M_LoadSettings(ctx, obj, &ctx->gf->settings);

    // clang-format off
    ctx->gf->demo_delay = JSON_ObjectGetInt(obj, "demo_delay", 30);
    ctx->gf->load_save_disabled = JSON_ObjectGetBool(obj, "load_save_disabled", false);
    ctx->gf->cheat_keys = JSON_ObjectGetBool(obj, "cheat_keys", true);
    ctx->gf->lockout_option_ring = JSON_ObjectGetBool(obj, "lockout_option_ring", true);
    ctx->gf->play_any_level = JSON_ObjectGetBool(obj, "play_any_level", false);
    ctx->gf->single_level = JSON_ObjectGetInt(obj, "single_level", -1);
    // clang-format on

    ctx->gf->secret_track = JSON_ObjectGetInt(obj, "secret_track", MX_INACTIVE);

    M_LoadGlobalInjections(ctx, obj);
}
