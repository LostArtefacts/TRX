// NOTE: this is an included file, not a compile unit on its own.
// This is to avoid exposing symbols.

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

static void M_LoadRoot(const M_CONTEXT *const ctx, JSON_OBJECT *const obj)
{
    M_LoadSettings(ctx, obj, &ctx->gf->settings);

    ctx->gf->enable_tr2_item_drops =
        JSON_ObjectGetBool(obj, "enable_tr2_item_drops", false);
    ctx->gf->convert_dropped_guns =
        JSON_ObjectGetBool(obj, "convert_dropped_guns", false);

    M_LoadGlobalInjections(ctx, obj);
}
