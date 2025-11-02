// NOTE: this is an included file, not a compile unit on its own.
// This is to avoid exposing symbols.

static void M_LoadLevelGameSpecifics(
    const M_CONTEXT *const ctx, JSON_OBJECT *const jlvl_obj,
    GF_LEVEL *const level)
{
    M_LoadSettings(ctx, jlvl_obj, &level->settings);
}

static void M_LoadRoot(const M_CONTEXT *const ctx, JSON_OBJECT *const obj)
{
    M_LoadSettings(ctx, obj, &ctx->gf->settings);
    M_LoadGlobalInjections(ctx, obj);
}
