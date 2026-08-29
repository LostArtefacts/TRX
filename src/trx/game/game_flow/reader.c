#include <trx/game/game_flow/reader.h>

#include <trx/core/enum_map.h>
#include <trx/core/filesystem.h>
#include <trx/core/json/util/file.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/value.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/game_flow/types.h>
#include <trx/game/game_flow/vars.h>
#include <trx/game/inventory_ring/types.h>
#include <trx/game/lara/skin/storage.h>
#include <trx/game/level/settings.h>
#include <trx/game/objects/names.h>
#include <trx/game/output/sky.h>
#include <trx/game/shell.h>
#include <trx/version.h>

#include <string.h>

#define M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(name)                            \
    int32_t name(                                                              \
        const M_CONTEXT *ctx, GF_SEQUENCE_EVENT *event, void *extra_data,      \
        void *user_arg)

typedef struct {
    GAME_FLOW *gf;
    const char *script_path;
    JSON_READ_IO *io;
    bool validation_mode;
} M_CONTEXT;

typedef int32_t (*M_SEQUENCE_EVENT_HANDLER_FUNC)(
    const M_CONTEXT *ctx, GF_SEQUENCE_EVENT *event, void *extra_data,
    void *user_arg);

typedef struct {
    GF_SEQUENCE_EVENT_TYPE event_type;
    M_SEQUENCE_EVENT_HANDLER_FUNC handler_func;
    void *handler_func_arg;
} M_SEQUENCE_EVENT_HANDLER;

typedef struct {
    const char *key;
    LEVEL_SETTING setting;
    TRX_VALUE_TYPE json_type;
    const void *json_param;
} M_SETTING_KEY;

typedef RESULT (*M_LOAD_ARRAY_FUNC)(
    const M_CONTEXT *ctx, void *target_elem, size_t target_elem_idx,
    void *user_arg);

static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleIntEvent);
static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandlePictureEvent);
static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleTotalStatsEvent);
static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleAddItemEvent);
static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleGlobeSelectEvent);
static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleSetupHorizonEvent);
static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleSetupLensFlareEvent);

static M_SEQUENCE_EVENT_HANDLER m_SequenceEventHandlers[] = {
    // clang-format off
    // Events without arguments
    { GFS_ENABLE_SUNSET,     nullptr, nullptr },
    { GFS_ENABLE_LIGHTNING,  nullptr, nullptr },
    { GFS_REMOVE_WEAPONS,    nullptr, nullptr },
    { GFS_REMOVE_SCIONS,     nullptr, nullptr },
    { GFS_REMOVE_AMMO,       nullptr, nullptr },
    { GFS_REMOVE_FLARES,     nullptr, nullptr },
    { GFS_REMOVE_MEDIPACKS,  nullptr, nullptr },
    { GFS_REMOVE_BINOCULARS,  nullptr, nullptr },
    { GFS_LEVEL_COMPLETE,    nullptr, nullptr },
    { GFS_LEVEL_STATS,       nullptr, nullptr },
    { GFS_EXIT_TO_TITLE,     nullptr, nullptr },
    { GFS_GLOBE_SELECT,      M_HandleGlobeSelectEvent, nullptr },

    // Events with integer arguments
    { GFS_SET_START_ANIM,    M_HandleIntEvent, "anim" },
    { GFS_LOOP_GAME,         M_HandleIntEvent, "level_id" },
    { GFS_PLAY_CUTSCENE,     M_HandleIntEvent, "cutscene_id" },
    { GFS_PLAY_FMV,          M_HandleIntEvent, "fmv_id" },
    { GFS_PLAY_MUSIC,        M_HandleIntEvent, "music_track" },
    { GFS_DISABLE_FLOOR,     M_HandleIntEvent, "height" },
    { GFS_SETUP_UV_ROTATE,   M_HandleIntEvent, "speed" },

    // Special cases with custom handlers
    { GFS_LOADING_SCREEN,    M_HandlePictureEvent, nullptr },
    { GFS_DISPLAY_PICTURE,   M_HandlePictureEvent, nullptr },
    { GFS_TOTAL_STATS,       M_HandleTotalStatsEvent, nullptr },
    { GFS_ADD_ITEM,          M_HandleAddItemEvent, nullptr },
    { GFS_ADD_SECRET_REWARD, M_HandleAddItemEvent, nullptr },
    { GFS_SETUP_HORIZON,     M_HandleSetupHorizonEvent, nullptr },
    { GFS_SETUP_LENS_FLARE,  M_HandleSetupLensFlareEvent, nullptr },

    // Sentinel to mark the end of the table
    { (GF_SEQUENCE_EVENT_TYPE)-1, nullptr, nullptr },
    // clang-format on
};

// clang-format off
static const M_SETTING_KEY m_SettingKeys[] = {
#define X_SETTING(name_, field_, type_, config_)                               \
    { .key = #field_,                                                          \
      .setting = LEVEL_SETTING_##name_,                                        \
      .json_type = Value_TypeOf(((GF_LEVEL_SETTINGS *)nullptr)->field_.value) },
#define X_SETTING_ENUM(name_, field_, enum_, fallback_)                        \
    { .key = #field_,                                                          \
      .setting = LEVEL_SETTING_##name_,                                        \
      .json_type = TVT_ENUM,                                                   \
      .json_param = ENUM_MAP_NAME(enum_) },
#include <trx/game/level/settings.def>
#undef X_SETTING
#undef X_SETTING_ENUM
};
// clang-format on

static RESULT M_ReadObjectID(
    const M_CONTEXT *const ctx, OBJECT_ID *const object_id_out)
{
    int32_t game_id;
    if (Result_Absorb(JSON_READ_CURRENT(ctx->io, &game_id))) {
        *object_id_out = Object_SlotToID(game_id);
    } else {
        const char *object_key;
        MUST(JSON_READ_CURRENT(ctx->io, &object_key));
        *object_id_out = Object_IdFromKey(object_key);
    }
    if (!ctx->validation_mode && *object_id_out == NO_OBJECT) {
        return JSON_ReadIO_Fail(
            ctx->io, "'object_id' must be a valid object id");
    }
    return OK;
}

static M_SEQUENCE_EVENT_HANDLER *M_GetSequenceEventHandlers(void)
{
    return m_SequenceEventHandlers;
}

// What a script keys its own data by. The case is lowered here rather than in
// FS_GetStem, which is a path split and holds no case policy.
static char *M_MakeLevelKey(const char *const path)
{
    char *const key = FS_GetStem(path);
    for (char *c = key; c != nullptr && *c != '\0'; c++) {
        *c = (*c >= 'A' && *c <= 'Z') ? *c + ('a' - 'A') : *c;
    }
    return key;
}

// Read a "path" value that may be either a plain string or an array of
// candidate strings tried in order.
// Pass optional=true to allow the key to be absent (out_path is set to nullptr
// in that case).
// Pass path_type=(GAME_DYNAMIC_PATH)-1 to skip resolution and return the first
// candidate as-is.
static RESULT M_ReadPath(
    JSON_READ_IO *const io, const char *const key, const bool optional,
    const GAME_DYNAMIC_PATH path_type, char **const out_path,
    const bool suppress_errors)
{
    ASSERT(key != nullptr);
    *out_path = nullptr;
    const bool resolve = path_type != (GAME_DYNAMIC_PATH)-1;

    if (!JSON_ReadIO_HasKey(io, key)) {
        if (suppress_errors || optional) {
            return OK;
        }
        return JSON_ReadIO_Fail(io, "there is no '%s' here", key);
    }
    MUST(JSON_PUSH(io, key));

    // Every way out below pops first, so that a failure here leaves the
    // reader where it started rather than partway into the file.
    RESULT result = OK;
    const char *first_candidate = nullptr;
    JSON_ARRAY *const path_array =
        JSON_ValueAsArray(JSON_ReadIO_GetCurrentValue(io));
    const int32_t count = path_array != nullptr ? JSON_ARRAY_LEN(io) : 1;
    if (count <= 0) {
        result = JSON_ReadIO_Fail(io, "'%s' names no file at all", key);
    } else {
        for (int32_t i = 0; i < count; i++) {
            const char *path = nullptr;
            const RESULT read = path_array != nullptr
                ? JSON_READ_A(io, i, &path)
                : JSON_READ_CURRENT(io, &path);
            if (!IS_OK(read)) {
                result = read;
                break;
            }
            if (first_candidate == nullptr) {
                first_candidate = path;
            }
            if (!resolve) {
                *out_path = Memory_DupStr(path);
                break;
            }
            const char *const resolved = GamePath_PeekResolve(path_type, path);
            if (resolved != nullptr) {
                *out_path = Memory_DupStr(resolved);
                break;
            }
        }
        if (IS_OK(result) && *out_path == nullptr && resolve) {
            result = JSON_ReadIO_Fail(
                io, "no file named by '%s' is installed, starting with '%s'",
                key, first_candidate != nullptr ? first_candidate : "");
        }
    }

    // The sizing pass and an optional path both do without one.
    if (!IS_OK(result) && (suppress_errors || optional)) {
        IGNORE(result);
        result = OK;
    }

    const RESULT popped = JSON_ReadIO_Pop(io);
    if (!IS_OK(result)) {
        IGNORE(popped);
        return result;
    }
    return popped;
}

static RESULT M_LoadSettings(
    const M_CONTEXT *const ctx, GF_LEVEL_SETTINGS *const settings)
{
    JSON_READ_IO *const io = ctx->io;

    for (size_t i = 0; i < ARRAY_SIZE(m_SettingKeys); i++) {
        const M_SETTING_KEY *const key = &m_SettingKeys[i];
        if (!JSON_ReadIO_HasKey(io, key->key)) {
            continue;
        }
        TRX_VALUE value;
        RESULT result = JSONValue_Read(
            JSON_ReadIO_GetCurrentObject(io), key->key, key->json_type,
            key->json_param, &value);
        if (IS_OK(result) && key->json_type == TVT_ENUM) {
            value.type = TVT_S32;
        }
        if (IS_OK(result)) {
            result = Level_SetDeclaredSetting(settings, key->setting, &value);
        }
        if (!IS_OK(result)) {
            JSON_ReadIO_SetError(io, "Invalid %s value", key->key);
            return result;
        }
    }

    if (JSON_ReadIO_HasKey(io, "sfx_path")) {
        MUST(JSON_PUSH(io, "sfx_path"));
        MUST(JSON_POP(io));
        SHOULD(M_ReadPath(
            io, "sfx_path", false, GAME_DYNAMIC_PATH_SFX_FILE,
            &settings->sfx_path, false));
    }
    return OK;
}

static RESULT M_LoadLevelItemDrops(
    const M_CONTEXT *const ctx, GF_LEVEL *const level)
{
    JSON_READ_IO *const io = ctx->io;
    level->item_drops.count = 0;

    if (!JSON_ReadIO_HasKey(io, "item_drops")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "item_drops"));

    if (ctx->gf->enable_tr2_item_drops) {
        LOG_WARNING(
            "TR2 item drops are enabled: gameflow-defined drops for level "
            "%d will be ignored",
            level->num);
        MUST(JSON_POP(io));
        return OK;
    }

    level->item_drops.count = JSON_ARRAY_LEN(io);
    if (level->item_drops.count < 0) {
        return JSON_ReadIO_Fail(io, "a list was expected");
    }
    level->item_drops.data = Memory_Alloc(
        sizeof(GF_DROP_ITEM_DATA) * (size_t)level->item_drops.count);

    for (int32_t i = 0; i < level->item_drops.count; i++) {
        MUST(JSON_PUSH_INDEX(io, i));
        GF_DROP_ITEM_DATA *data = &level->item_drops.data[i];

        MUST(JSON_READ(io, "enemy_num", &data->enemy_num));

        MUST(JSON_PUSH(io, "object_ids"));
        const int32_t object_count = JSON_ARRAY_LEN(io);
        if (object_count < 0) {
            return JSON_ReadIO_Fail(io, "a list was expected");
        }
        data->count = object_count;
        data->object_ids = Memory_Alloc(sizeof(int16_t) * data->count);
        for (int32_t j = 0; j < data->count; j++) {
            MUST(JSON_PUSH_INDEX(io, j));
            OBJECT_ID id = NO_OBJECT;
            MUST(M_ReadObjectID(ctx, &id));
            data->object_ids[j] = (int16_t)id;
            MUST(JSON_POP(io));
        }
        MUST(JSON_POP(io));
        MUST(JSON_POP(io));
    }
    MUST(JSON_POP(io));
    return OK;
}

// Copies only the root sound path into the level settings. Other root settings
// remain fallback values.
static void M_InheritRootSfxPath(
    GF_LEVEL_SETTINGS *const dst, const GF_LEVEL_SETTINGS *const src)
{
    dst->sfx_path =
        src->sfx_path != nullptr ? Memory_DupStr(src->sfx_path) : nullptr;
}

static RESULT M_ReadModMeta(JSON_READ_IO *const io, GF_MOD_META *const meta)
{
    meta->name = nullptr;
    meta->engine = 0;
    meta->extends = nullptr;

    const char *tmp_s = nullptr;
    MUST(JSON_READ_OPT(io, "name", &tmp_s));
    if (tmp_s != nullptr) {
        meta->name = Memory_DupStr(tmp_s);
    }

    MUST(JSON_READ_OPT(io, "engine", &meta->engine));

    tmp_s = nullptr;
    MUST(JSON_READ_OPT(io, "extends", &tmp_s));
    if (tmp_s != nullptr) {
        meta->extends = Memory_DupStr(tmp_s);
    }
    return OK;
}

static RESULT M_LoadRoot(const M_CONTEXT *const ctx)
{
    JSON_READ_IO *const io = ctx->io;
    const char *tmp_s = nullptr;

    MUST(M_ReadModMeta(io, &ctx->gf->meta));

    // A title that names no picture shows its own level behind the menu. One
    // that names a picture and cannot find it is a broken install, not that.
    tmp_s = nullptr;
    MUST(JSON_READ_OPT(io, "main_menu_picture", &tmp_s));
    if (tmp_s != nullptr) {
        ctx->gf->main_menu_background_path = Memory_DupStr(
            GamePath_TryResolve(GAME_DYNAMIC_PATH_IMAGE_FILE, tmp_s));
    }
    ctx->gf->main_menu_use_live_scene = tmp_s == nullptr;

    MUST(JSON_READ(io, "savegame_file_fmt", &tmp_s));
    ctx->gf->savegame_file_fmt = Memory_DupStr(tmp_s);

    if (JSON_ReadIO_HasKey(io, "ambient_tracks")) {

        MUST(JSON_PUSH(io, "ambient_tracks"));
        const int32_t count = JSON_ARRAY_LEN(io);
        if (count < 0) {
            return JSON_ReadIO_Fail(io, "a list was expected");
        }
        if (count > 0) {
            ctx->gf->ambient_tracks.is_present = true;
            ctx->gf->ambient_tracks.count = count;
            ctx->gf->ambient_tracks.ids =
                Memory_Alloc(sizeof(MUSIC_SLOT) * (size_t)count);
            for (int32_t i = 0; i < count; i++) {
                int32_t track = MX_INACTIVE;
                SHOULD(JSON_READ_A(io, i, &track));
                ctx->gf->ambient_tracks.ids[i] = track;
            }
        }
        MUST(JSON_POP(io));
    }

    ctx->gf->enable_tr2_item_drops = g_TRVersion > 1;
    MUST(JSON_READ_D(
        io, "enable_tr2_item_drops", &ctx->gf->enable_tr2_item_drops,
        g_TRVersion > 1));
    MUST(JSON_READ_D(
        io, "convert_dropped_guns", &ctx->gf->convert_dropped_guns,
        g_TRVersion > 1));
    return OK;
}

static RESULT M_LoadGlobeEntry(
    const M_CONTEXT *const ctx, void *const target_elem, size_t idx,
    void *const user_arg)
{
    GF_GLOBE_ENTRY *const e = target_elem;
    ASSERT(user_arg == nullptr);
    JSON_READ_IO *const io = ctx->io;
    MUST(JSON_READ(io, "rot", &e->rot));

    MUST(JSON_READ(io, "start_level_ordinal", &e->start_level_ordinal));
    MUST(JSON_READ(
        io, "completion_level_ordinal", &e->completion_level_ordinal));

    MUST(JSON_PUSH(io, "prereq_zones"));
    const int32_t prereq_count = JSON_ARRAY_LEN(io);
    if (prereq_count < 0) {
        return JSON_ReadIO_Fail(io, "a list was expected");
    }
    uint32_t computed_prereq_mask = 0;
    for (int32_t i = 0; i < prereq_count; i++) {
        int32_t zone = JSON_INVALID_NUMBER;
        MUST(JSON_READ_A(io, i, &zone));
        if (zone < 0 || zone >= MAX_GLOBE_ZONES) {
            return JSON_ReadIO_Fail(
                io, "'prereq_zones' entries must be in range 0..%d",
                MAX_GLOBE_ZONES - 1);
        }
        computed_prereq_mask |= 1u << zone;
    }
    e->prereq_mask = computed_prereq_mask;
    MUST(JSON_POP(io));

    int32_t mesh_idx = JSON_INVALID_NUMBER;
    MUST(JSON_READ(io, "mesh_idx", &mesh_idx));
    e->mesh_idx = (uint8_t)mesh_idx;
    return OK;
}

static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleIntEvent)
{
    JSON_READ_IO *const io = ctx->io;
    if (event != nullptr) {
        int32_t value;
        SHOULD(JSON_READ_D(io, user_arg, &value, -1));
        event->data = (void *)(intptr_t)value;
    }
    return 0;
}

static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandlePictureEvent)
{
    JSON_READ_IO *const io = ctx->io;
    char *expanded_path = nullptr;
    SHOULD(M_ReadPath(
        io, "path", false, GAME_DYNAMIC_PATH_IMAGE_FILE, &expanded_path,
        event == nullptr));

    if (event != nullptr) {
        GF_DISPLAY_PICTURE_DATA *const event_data = extra_data;
        SHOULD(JSON_READ_D(io, "legal", &event_data->is_legal, false));
        SHOULD(JSON_READ_D(io, "credit", &event_data->is_credit, false));
        SHOULD(
            JSON_READ_D(io, "display_time", &event_data->display_time, 5.0f));
        SHOULD(
            JSON_READ_D(io, "fade_in_time", &event_data->fade_in_time, 1.0f));
        SHOULD(JSON_READ_D(
            io, "fade_out_time", &event_data->fade_out_time, 1.0f / 3.0f));
        if (expanded_path != nullptr) {
            event_data->path =
                (char *)extra_data + sizeof(GF_DISPLAY_PICTURE_DATA);
            strcpy(event_data->path, expanded_path);
        } else {
            event_data->path = nullptr;
        }
        event->data = event_data;
    }

    const int32_t out_size = sizeof(GF_DISPLAY_PICTURE_DATA)
        + (expanded_path == nullptr ? 0 : strlen(expanded_path) + 1);
    Memory_FreePointer(&expanded_path);
    return out_size;

fail:
    Memory_FreePointer(&expanded_path);
    return 0;
}

static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleTotalStatsEvent)
{
    JSON_READ_IO *const io = ctx->io;
    char *expanded_path = nullptr;
    SHOULD(M_ReadPath(
        io, "background_path", false, GAME_DYNAMIC_PATH_IMAGE_FILE,
        &expanded_path, event == nullptr));
    if (expanded_path == nullptr) {
        if (event != nullptr) {
            event->data = nullptr;
        }
        return 0;
    }
    if (event != nullptr) {
        char *const event_data = extra_data;
        strcpy(event_data, expanded_path);
        event->data = event_data;
    }
    const int32_t out_size = strlen(expanded_path) + 1;
    Memory_FreePointer(&expanded_path);
    return out_size;
}

static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleAddItemEvent)
{
    JSON_READ_IO *const io = ctx->io;
    OBJECT_ID obj_id = NO_OBJECT;
    if (!SHOULD(JSON_PUSH(io, "object_id"))) {
        return -1;
    }
    if (!Result_Absorb(M_ReadObjectID(ctx, &obj_id))) {
        return -1;
    }
    if (!Result_Absorb(JSON_POP(io))) {
        return -1;
    }
    if (event != nullptr) {
        GF_ADD_ITEM_DATA *const event_data = extra_data;
        event_data->object_id = obj_id;
        SHOULD(JSON_READ_D(io, "quantity", &event_data->quantity, 1));
        event_data->inv_type =
            event->type == GFS_ADD_ITEM ? GF_INV_REGULAR : GF_INV_SECRET;
        event->data = event_data;
    }
    return sizeof(GF_ADD_ITEM_DATA);
fail:
    return -1;
}

static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleSetupHorizonEvent)
{
    JSON_READ_IO *const io = ctx->io;
    RGB_888 color = {};
    if (!Result_Absorb(JSON_READ(io, "color", &color))) {
        return -1;
    }
    if (event != nullptr) {
        GF_SETUP_HORIZON_DATA *const event_data = extra_data;
        event_data->color = color;
        SHOULD(JSON_READ_D(io, "layer", &event_data->layer, 0));
        if (event_data->layer < 0
            || event_data->layer >= OUTPUT_SKY_LAYER_COUNT) {
            IGNORE(JSON_ReadIO_Fail(io, "'layer' must be 0 or 1"));
            return -1;
        }
        SHOULD(JSON_READ_D(io, "speed", &event_data->speed, 0));
        SHOULD(JSON_READ_D(io, "color_add", &event_data->color_add, false));
        SHOULD(
            JSON_READ_D(io, "fog_gradient", &event_data->fog_gradient, false));
        event->data = event_data;
    }
    return sizeof(GF_SETUP_HORIZON_DATA);
fail:
    return -1;
}

static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleSetupLensFlareEvent)
{
    JSON_READ_IO *const io = ctx->io;
    XYZ_32 pos = {};
    RGB_888 color = {};
    if (!Result_Absorb(JSON_READ(io, "x", &pos.x))) {
        return -1;
    }
    if (!Result_Absorb(JSON_READ(io, "y", &pos.y))) {
        return -1;
    }
    if (!Result_Absorb(JSON_READ(io, "z", &pos.z))) {
        return -1;
    }
    if (!Result_Absorb(JSON_READ(io, "color", &color))) {
        return -1;
    }
    if (event != nullptr) {
        GF_SETUP_LENS_FLARE_DATA *const event_data = extra_data;
        event_data->pos = pos;
        event_data->color = color;
        event->data = event_data;
    }
    return sizeof(GF_SETUP_LENS_FLARE_DATA);
fail:
    return -1;
}

static M_DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleGlobeSelectEvent)
{
    JSON_READ_IO *const io = ctx->io;
    const char *image;
    SHOULD(JSON_READ_D(io, "image", &image, nullptr));
    char *expanded_image =
        Memory_DupStr(GamePath_TryResolve(GAME_DYNAMIC_PATH_IMAGE_FILE, image));
    if (expanded_image == nullptr) {
        if (event != nullptr) {
            event->data = nullptr;
        }
        return 0;
    }

    if (event != nullptr) {
        GF_GLOBE_SELECT_DATA *const event_data = extra_data;
        event_data->image_path =
            (char *)extra_data + sizeof(GF_GLOBE_SELECT_DATA);
        strcpy(event_data->image_path, expanded_image);
        event->data = event_data;
    }

    const int32_t out_size =
        sizeof(GF_GLOBE_SELECT_DATA) + strlen(expanded_image) + 1;
    Memory_FreePointer(&expanded_image);
    return out_size;
}

static RESULT M_LoadArray(
    const M_CONTEXT *const ctx, const char *const key, int32_t *const count,
    void **const elements, const size_t element_size,
    const M_LOAD_ARRAY_FUNC load_func, void *const load_func_arg)
{
    if (!JSON_ReadIO_HasKey(ctx->io, key)) {
        return OK;
    }
    MUST(JSON_PUSH(ctx->io, key));
    const int32_t elem_count = JSON_ARRAY_LEN(ctx->io);
    if (elem_count < 0) {
        return JSON_ReadIO_Fail(ctx->io, "a list was expected");
    }
    *count = elem_count;
    *elements = Memory_Alloc(element_size * (size_t)(*count));

    for (size_t i = 0; i < (size_t)elem_count; i++) {
        void *const element = (char *)*elements + i * element_size;
        MUST(JSON_PUSH_INDEX(ctx->io, i));
        MUST(load_func(ctx, element, i, load_func_arg));
        MUST(JSON_POP(ctx->io));
    }
    MUST(JSON_POP(ctx->io));
    return OK;
}

static RESULT M_LoadGlobeSelectEntries(const M_CONTEXT *const ctx)
{
    ctx->gf->globe.count = 0;
    ctx->gf->globe.entries = nullptr;
    return M_LoadArray(
        ctx, "globe_select_entries", &ctx->gf->globe.count,
        (void **)&ctx->gf->globe.entries, sizeof(GF_GLOBE_ENTRY),
        M_LoadGlobeEntry, nullptr);
}

static int32_t M_LoadSequenceEvent(
    const M_CONTEXT *const ctx, GF_SEQUENCE_EVENT *const event,
    void *const extra_data)
{
    const char *type_str = nullptr;
    if (!Result_Absorb(JSON_READ(ctx->io, "type", &type_str))) {
        return -1;
    }
    const GF_SEQUENCE_EVENT_TYPE type =
        ENUM_MAP_GET(GF_SEQUENCE_EVENT_TYPE, type_str, -1);
    if (type == (GF_SEQUENCE_EVENT_TYPE)-1) {
        IGNORE(JSON_ReadIO_Fail(
            ctx->io, "unknown game flow sequence event type: '%s'", type_str));
        return -1;
    }

    const M_SEQUENCE_EVENT_HANDLER *handler = M_GetSequenceEventHandlers();
    while (handler->event_type != (GF_SEQUENCE_EVENT_TYPE)-1
           && handler->event_type != type) {
        handler++;
    }

    int32_t extra_data_size = 0;
    if (handler->handler_func != nullptr) {
        extra_data_size = handler->handler_func(
            ctx, nullptr, nullptr, handler->handler_func_arg);
    }
    if (extra_data_size >= 0 && event != nullptr) {
        event->type = handler->event_type;
        if (handler->handler_func != nullptr) {
            handler->handler_func(
                ctx, event, extra_data, handler->handler_func_arg);
        } else {
            event->data = nullptr;
        }
    }
    return extra_data_size;
fail:
    return -1;
}

static RESULT M_LoadSequence(
    const M_CONTEXT *const ctx, GF_SEQUENCE *const sequence)
{
    JSON_READ_IO *const io = ctx->io;
    sequence->length = 0;
    const int32_t seq_count = JSON_ARRAY_LEN(io);
    if (seq_count < 0) {
        return JSON_ReadIO_Fail(io, "a list was expected");
    }

    const size_t event_base_size = sizeof(GF_SEQUENCE_EVENT);
    size_t total_data_size = 0;
    for (int32_t i = 0; i < seq_count; i++) {
        MUST(JSON_PUSH_INDEX(io, i));
        const int32_t event_extra_size =
            M_LoadSequenceEvent(ctx, nullptr, nullptr);
        MUST(JSON_POP(io));
        if (event_extra_size < 0) {
            return JSON_ReadIO_Fail(io, "a list was expected");
        }
        sequence->length++;
        total_data_size += Memory_Align((size_t)event_extra_size);
    }

    const size_t events_block_size =
        Memory_Align((size_t)sequence->length * event_base_size);
    total_data_size += events_block_size;

    char *const data = Memory_Alloc(total_data_size);
    char *extra_data_ptr = data + events_block_size;
    sequence->events = (GF_SEQUENCE_EVENT *)data;

    int32_t j = 0;
    for (int32_t i = 0; i < seq_count; i++) {
        MUST(JSON_PUSH_INDEX(io, i));
        const int32_t event_extra_size =
            M_LoadSequenceEvent(ctx, &sequence->events[j], extra_data_ptr);
        MUST(JSON_POP(io));
        if (event_extra_size < 0) {
            // Parsing this event failed - discard it
            continue;
        }
        extra_data_ptr += Memory_Align((size_t)event_extra_size);
        j++;
    }
    return OK;
}

static RESULT M_LoadLevelInjections(
    const M_CONTEXT *const ctx, GF_LEVEL *const level)
{
    JSON_READ_IO *const io = ctx->io;
    bool inherit;
    MUST(JSON_READ_D(io, "inherit_injections", &inherit, true));
    int32_t local_count = 0;
    if (JSON_ReadIO_HasKey(io, "injections")) {
        MUST(JSON_PUSH(io, "injections"));
        local_count = JSON_ARRAY_LEN(io);
        if (local_count < 0) {
            return JSON_ReadIO_Fail(io, "a list was expected");
        }
        MUST(JSON_POP(io));
    }

    level->injections.count = 0;
    if (local_count == 0 && !inherit) {
        return OK;
    }

    if (inherit) {
        level->injections.count += ctx->gf->injections.count;
    }
    level->injections.count += local_count;

    level->injections.data_paths =
        Memory_Alloc(sizeof(char *) * level->injections.count);

    int32_t base_index = 0;
    if (inherit) {
        for (int32_t i = 0; i < ctx->gf->injections.count; i++) {
            level->injections.data_paths[i] =
                Memory_DupStr(ctx->gf->injections.data_paths[i]);
        }
        base_index = ctx->gf->injections.count;
    }

    if (local_count == 0) {
        return OK;
    }

    MUST(JSON_PUSH(io, "injections"));
    for (int32_t i = 0; i < local_count; i++) {
        const char *str = nullptr;
        MUST(JSON_READ_A(io, i, &str));
        level->injections.data_paths[base_index + i] = Memory_DupStr(
            GamePath_TryResolve(GAME_DYNAMIC_PATH_INJECTION_FILE, str));
    }
    MUST(JSON_POP(io));
    return OK;
}

static RESULT M_LoadLevelSequence(
    const M_CONTEXT *const ctx, GF_LEVEL *const level)
{
    MUST(JSON_PUSH(ctx->io, "sequence"));
    MUST(M_LoadSequence(ctx, &level->sequence));
    MUST(JSON_POP(ctx->io));

    for (int32_t i = 0; i < level->sequence.length; i++) {
        GF_SEQUENCE_EVENT *const event = &level->sequence.events[i];
        if (event->type == GFS_LOOP_GAME) {
            event->data = (void *)(intptr_t)level->num;
        }
    }
    return OK;
}

static RESULT M_LoadLevel(
    const M_CONTEXT *const ctx, void *const target_elem, const size_t idx,
    void *const user_arg)
{
    GF_LEVEL *const level = target_elem;
    JSON_READ_IO *const io = ctx->io;
    level->num = idx;

    {
        level->type = (GF_LEVEL_TYPE)(intptr_t)user_arg;
        if (JSON_ReadIO_HasKey(io, "type")) {
            MUST(JSON_PUSH(io, "type"));
            const char *tmp = nullptr;
            MUST(JSON_READ_CURRENT(io, &tmp));
            MUST(JSON_POP(io));
            const GF_LEVEL_TYPE user_type =
                ENUM_MAP_GET(GF_LEVEL_TYPE, tmp, -1);
            if (user_type == (GF_LEVEL_TYPE)-1) {
                return JSON_ReadIO_Fail(io, "unrecognized type '%s'", tmp);
            }

            if (level->type != GFL_NORMAL
                && GF_GetLevelTableType(user_type) != GFLT_MAIN) {
                return JSON_ReadIO_Fail(
                    io, "cannot override level type=%s to %s",
                    ENUM_MAP_TO_STRING(GF_LEVEL_TYPE, level->type),
                    ENUM_MAP_TO_STRING(GF_LEVEL_TYPE, user_type));
            }
            level->type = user_type;
        }
    }

    if (level->type == GFL_DUMMY) {
        return OK;
    }

    {
        const GAME_DYNAMIC_PATH path_type =
            (level->type == GFL_DUMMY || level->type == GFL_CURRENT)
            ? (GAME_DYNAMIC_PATH)-1
            : (level->type == GFL_TITLE || level->type == GFL_GYM)
            ? GAME_DYNAMIC_PATH_SHARED_LEVEL_FILE
            : GAME_DYNAMIC_PATH_LEVEL_FILE;
        MUST(M_ReadPath(io, "path", false, path_type, &level->path, false));
        level->key = M_MakeLevelKey(level->path);
    }
    // A level's script is named after the level: wall.tr2 runs
    // scripts/wall.lua where the game ships one. Nothing declares it, and a
    // level without one is the common case, so a miss is silent.
    if (level->key != nullptr) {
        // The resolver spells its answer into the same rotating buffers
        // String_FormatStatic hands out, so what it is asked for is owned here.
        char *rel = String_Format("%s.lua", level->key);
        level->script_path = Memory_DupStr(
            GamePath_PeekResolve(GAME_DYNAMIC_PATH_LEVEL_SCRIPT_FILE, rel));
        Memory_FreePointer(&rel);
    }

    {
        level->music_track = MX_INACTIVE;
        if (JSON_ReadIO_HasKey(io, "music_track")) {
            MUST(JSON_PUSH(io, "music_track"));
            MUST(JSON_READ_CURRENT(io, &level->music_track));
            MUST(JSON_POP(io));
        }
    }

    {
        const char *tmp = nullptr;
        MUST(JSON_READ_OPT(io, "lara_outfit", &tmp));
        if (tmp != nullptr) {
            if (!ctx->validation_mode
                && !Lara_Skin_IsOutfitDefined(
                    Lara_Skin_FindOutfitByName(tmp))) {
                LOG_WARNING(
                    "%s: level '%s' asks for outfit '%s', which this game does "
                    "not offer; Lara wears the default one",
                    ctx->script_path, level->path, tmp);
            }
            level->lara_outfit = Memory_DupStr(tmp);
        }
    }

    level->weather_type = WEATHER_NONE;
    {
        if (JSON_ReadIO_HasKey(io, "weather_type")) {
            MUST(JSON_PUSH(io, "weather_type"));
            const char *tmp = nullptr;
            MUST(JSON_READ_CURRENT(io, &tmp));
            MUST(JSON_POP(io));
            level->weather_type = ENUM_MAP_GET(WEATHER_TYPE, tmp, WEATHER_NONE);
        }
    }
    MUST(JSON_READ_D(io, "water_particles", &level->water_particles, false));

    MUST(JSON_READ_D(
        io, "unobtainable_pickups", &level->unobtainable.pickups, 0));
    MUST(JSON_READ_D(io, "unobtainable_kills", &level->unobtainable.kills, 0));
    MUST(JSON_READ_D(
        io, "unobtainable_ally_kills", &level->unobtainable.ally_kills, 0));
    MUST(JSON_READ_D(
        io, "unobtainable_secrets", &level->unobtainable.secrets, 0));

    M_InheritRootSfxPath(&level->settings, &ctx->gf->settings);

    MUST(M_LoadSettings(ctx, &level->settings));
    MUST(M_LoadLevelItemDrops(ctx, level));
    MUST(M_LoadLevelSequence(ctx, level));
    MUST(M_LoadLevelInjections(ctx, level));
    return OK;
}

static RESULT M_LoadLevelTable(
    const M_CONTEXT *const ctx, const char *const key,
    GF_LEVEL_TABLE *const level_table, const GF_LEVEL_TYPE default_level_type)
{
    MUST(M_LoadArray(
        ctx, key, &level_table->count, (void **)&level_table->levels,
        sizeof(GF_LEVEL), M_LoadLevel, (void *)(intptr_t)default_level_type));
    return OK;
}

static RESULT M_LoadLevels(const M_CONTEXT *const ctx)
{
    MUST(JSON_PUSH(ctx->io, "levels"));
    MUST(JSON_POP(ctx->io));
    MUST(M_LoadLevelTable(
        ctx, "levels", &ctx->gf->level_tables[GFLT_MAIN], GFL_NORMAL));
    return OK;
}

static RESULT M_LoadCutscenes(const M_CONTEXT *const ctx)
{
    return M_LoadLevelTable(
        ctx, "cutscenes", &ctx->gf->level_tables[GFLT_CUTSCENES], GFL_CUTSCENE);
}

static RESULT M_LoadDemos(const M_CONTEXT *const ctx)
{
    return M_LoadLevelTable(
        ctx, "demos", &ctx->gf->level_tables[GFLT_DEMOS], GFL_DEMO);
}

static RESULT M_LoadTitleLevel(const M_CONTEXT *const ctx)
{
    if (!JSON_ReadIO_HasKey(ctx->io, "title")) {
        return OK;
    }
    MUST(JSON_PUSH(ctx->io, "title"));
    ctx->gf->title_level = Memory_Alloc(sizeof(GF_LEVEL));
    MUST(
        M_LoadLevel(ctx, ctx->gf->title_level, 0, (void *)(intptr_t)GFL_TITLE));
    MUST(JSON_POP(ctx->io));
    return OK;
}

static RESULT M_LoadFMV(
    const M_CONTEXT *const ctx, void *const target_elem, size_t idx,
    void *const user_arg)
{
    GF_FMV *const fmv = target_elem;
    ASSERT(user_arg == nullptr);
    JSON_READ_IO *const io = ctx->io;
    char *path = nullptr;
    SHOULD(M_ReadPath(
        io, "path", false, GAME_DYNAMIC_PATH_FMV_FILE, &path, false));
    fmv->path = path;
    MUST(JSON_READ_D(io, "legal", &fmv->is_legal, false));
    MUST(JSON_READ_D(io, "credit", &fmv->is_credit, false));
    MUST(JSON_READ_D(io, "intro", &fmv->is_intro, false));
    return OK;
}

static RESULT M_LoadFMVs(const M_CONTEXT *const ctx)
{
    MUST(M_LoadArray(
        ctx, "fmvs", &ctx->gf->fmv_count, (void **)&ctx->gf->fmvs,
        sizeof(GF_FMV), M_LoadFMV, nullptr));
    return OK;
}

static RESULT M_LoadGlobalInjections(const M_CONTEXT *const ctx)
{
    JSON_READ_IO *const io = ctx->io;
    ctx->gf->injections.count = 0;
    if (!JSON_ReadIO_HasKey(io, "injections")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "injections"));
    ctx->gf->injections.count = JSON_ARRAY_LEN(io);
    if (ctx->gf->injections.count < 0) {
        return JSON_ReadIO_Fail(io, "a list was expected");
    }
    ctx->gf->injections.data_paths =
        Memory_Alloc(sizeof(char *) * (size_t)ctx->gf->injections.count);
    for (int32_t i = 0; i < ctx->gf->injections.count; i++) {
        const char *str = nullptr;
        MUST(JSON_READ_A(io, i, &str));
        ctx->gf->injections.data_paths[i] = Memory_DupStr(
            GamePath_TryResolve(GAME_DYNAMIC_PATH_INJECTION_FILE, str));
    }
    MUST(JSON_POP(io));
    return OK;
}

static RESULT M_LoadSections(const M_CONTEXT *const ctx)
{
    MUST(M_LoadRoot(ctx));
    MUST(M_LoadSettings(ctx, &ctx->gf->settings));
    MUST(M_LoadGlobeSelectEntries(ctx));
    MUST(M_LoadGlobalInjections(ctx));
    MUST(M_LoadLevels(ctx));
    MUST(M_LoadCutscenes(ctx));
    MUST(M_LoadDemos(ctx));
    MUST(M_LoadFMVs(ctx));
    MUST(M_LoadTitleLevel(ctx));
    return OK;
}

static RESULT M_LoadGameFlowDoc(
    JSON_VALUE *const doc, const char *const path, const bool validation_mode,
    char **const error_out)
{
    GF_Shutdown();

    M_CONTEXT ctx = { .gf = &g_GameFlow, .validation_mode = validation_mode };
    ctx.gf->path = Memory_DupStr(path);
    ctx.script_path = g_GameFlow.path;
    ctx.io = nullptr;

    ctx.io = JSON_ReadIO_Create(doc, 0, path);

    const RESULT result = M_LoadSections(&ctx);

    JSON_ReadIO_Destroy(ctx.io);
    JSON_ValueFree(doc);
    if (!IS_OK(result)) {
        // The failure already says what stopped the read and where; the mod
        // list shows that to the player.
        if (error_out != nullptr && result.msg != nullptr) {
            *error_out = Memory_DupStr(result.msg);
        }
        GF_Shutdown();
    }
    return result;
}

static RESULT M_LoadGameFlowEx(
    const char *const path, const bool validation_mode, char **const error_out)
{
    FAIL_IF(path == nullptr, "the game flow file is not where TRX looked");
    JSON_VALUE *doc = nullptr;
    const RESULT read = JSONFile_ReadRequired(path, &doc);
    if (!IS_OK(read)) {
        if (error_out != nullptr && read.msg != nullptr) {
            *error_out = Memory_DupStr(read.msg);
        }
        return read;
    }
    return M_LoadGameFlowDoc(doc, path, validation_mode, error_out);
}

RESULT GF_LoadFromFile(const char *const path)
{
    return M_LoadGameFlowEx(path, false, nullptr);
}

bool GF_TryLoadFromFile(const char *const path)
{
    return Result_Absorb(M_LoadGameFlowEx(path, false, nullptr));
}

bool GF_ValidateMod(
    const char *const mod_name, const char *const path, char **const error_out)
{
    if (!Result_Absorb(M_LoadGameFlowEx(path, true, error_out))) {
        LOG_WARNING("Mod '%s' has invalid gameflow data", mod_name);
        return false;
    }
    GF_Shutdown();
    return true;
}

bool GF_ReadModMeta(
    const char *const path, GF_MOD_META *const meta, char **const error_out)
{
    ASSERT(meta != nullptr);

    JSON_VALUE *doc = nullptr;
    const RESULT read = JSONFile_ReadRequired(path, &doc);
    if (!IS_OK(read)) {
        if (error_out != nullptr && read.msg != nullptr) {
            *error_out = Memory_DupStr(read.msg);
        }
        IGNORE(read);
        return false;
    }

    JSON_READ_IO *const io = JSON_ReadIO_Create(doc, 0, path);
    const bool ok = Result_Absorb(M_ReadModMeta(io, meta));
    JSON_ReadIO_Destroy(io);
    JSON_ValueFree(doc);
    return ok;
}
