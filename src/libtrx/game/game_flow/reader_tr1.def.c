// NOTE: this is an included file, not a compile unit on its own.
// This is to avoid exposing symbols.

static M_SEQUENCE_EVENT_HANDLER m_SequenceEventHandlers[] = {
    // clang-format off
    // Events without arguments
    { GFS_REMOVE_WEAPONS,   nullptr, nullptr },
    { GFS_REMOVE_SCIONS,    nullptr, nullptr },
    { GFS_REMOVE_AMMO,      nullptr, nullptr },
    { GFS_REMOVE_FLARES,    nullptr, nullptr },
    { GFS_REMOVE_MEDIPACKS, nullptr, nullptr },
    { GFS_EXIT_TO_TITLE,    nullptr, nullptr },
    { GFS_LEVEL_STATS,      nullptr, nullptr },
    { GFS_LEVEL_COMPLETE,   nullptr, nullptr },

    // Events with integer arguments
    { GFS_LOOP_GAME,        M_HandleIntEvent, "level_id" },
    { GFS_PLAY_CUTSCENE,    M_HandleIntEvent, "cutscene_id" },
    { GFS_PLAY_FMV,         M_HandleIntEvent, "fmv_id" },
    { GFS_PLAY_MUSIC,       M_HandleIntEvent, "music_track" },
    { GFS_SETUP_BACON_LARA, M_HandleIntEvent, "anchor_room" },
    { GFS_DISABLE_FLOOR,    M_HandleIntEvent, "height" },

    // Special cases with custom handlers
    { GFS_LOADING_SCREEN,   M_HandlePictureEvent, nullptr },
    { GFS_DISPLAY_PICTURE,  M_HandlePictureEvent, nullptr },
    { GFS_TOTAL_STATS,      M_HandleTotalStatsEvent, nullptr },
    { GFS_ADD_ITEM,         M_HandleAddItemEvent, nullptr },

    // Sentinel to mark the end of the table
    { (GF_SEQUENCE_EVENT_TYPE)-1, nullptr, nullptr },
    // clang-format on
};

static void M_LoadSettings(
    const M_CONTEXT *const ctx, JSON_OBJECT *const obj,
    GF_LEVEL_SETTINGS *const settings)
{
    M_LoadCommonSettings(ctx, obj, settings);
}

static void M_LoadLevelItemDrops(
    const M_CONTEXT *const ctx, JSON_OBJECT *const jlvl_obj,
    GF_LEVEL *const level)
{
    JSON_ARRAY *const drops = JSON_ObjectGetArray(jlvl_obj, "item_drops");
    level->item_drops.count = 0;

    if (drops != nullptr && ctx->gf->enable_tr2_item_drops) {
        LOG_WARNING(
            "TR2 item drops are enabled: gameflow-defined drops for level "
            "%d will be ignored",
            level->num);
        return;
    }
    if (drops == nullptr) {
        return;
    }

    level->item_drops.count = (signed)drops->length;
    level->item_drops.data =
        Memory_Alloc(sizeof(GF_DROP_ITEM_DATA) * (signed)drops->length);

    for (int32_t i = 0; i < level->item_drops.count; i++) {
        GF_DROP_ITEM_DATA *data = &level->item_drops.data[i];
        JSON_OBJECT *jlvl_data = JSON_ArrayGetObject(drops, i);

        data->enemy_num =
            JSON_ObjectGetInt(jlvl_data, "enemy_num", JSON_INVALID_NUMBER);
        if (data->enemy_num == JSON_INVALID_NUMBER) {
            Shell_ExitSystemFmt(
                "%s, level %d, item drop %d: 'enemy_num' must be a number",
                ctx->script_path, level->num, i);
        }

        JSON_ARRAY *object_arr = JSON_ObjectGetArray(jlvl_data, "object_ids");
        if (!object_arr) {
            Shell_ExitSystemFmt(
                "%s, level %d, item drop %d: 'object_ids' must be an array",
                ctx->script_path, level->num, i);
        }

        data->count = (signed)object_arr->length;
        data->object_ids = Memory_Alloc(sizeof(int16_t) * data->count);
        for (int32_t j = 0; j < data->count; j++) {
            const OBJECT_ID id =
                M_GetObjectFromJSONValue(JSON_ArrayGetValue(object_arr, j));
            if (id == NO_OBJECT) {
                Shell_ExitSystemFmt(
                    "%s, level %d, item drop %d, index %d: 'object_id' "
                    "must be a valid object id",
                    ctx->script_path, level->num, i, j);
            }
            data->object_ids[j] = (int16_t)id;
        }
    }
}

static void M_LoadLevelGameSpecifics(
    const M_CONTEXT *const ctx, JSON_OBJECT *const jlvl_obj,
    GF_LEVEL *const level)
{
    M_LoadSettings(ctx, jlvl_obj, &level->settings);
    M_LoadLevelItemDrops(ctx, jlvl_obj, level);
}

static M_SEQUENCE_EVENT_HANDLER *M_GetSequenceEventHandlers(void)
{
    return m_SequenceEventHandlers;
}

static void M_LoadRoot(const M_CONTEXT *const ctx, JSON_OBJECT *const obj)
{
    double tmp_d;
    JSON_ARRAY *tmp_arr;

    M_LoadSettings(ctx, obj, &ctx->gf->settings);

    ctx->gf->enable_tr2_item_drops =
        JSON_ObjectGetBool(obj, "enable_tr2_item_drops", false);
    ctx->gf->convert_dropped_guns =
        JSON_ObjectGetBool(obj, "convert_dropped_guns", false);

    M_LoadGlobalInjections(ctx, obj);
}
