#include "game/game_flow/reader.h"

#include "debug.h"
#include "enum_map.h"
#include "filesystem.h"
#include "game/game_flow/common.h"
#include "game/game_flow/types.h"
#include "game/game_flow/vars.h"
#include "game/objects/common.h"
#include "game/objects/names.h"
#include "game/shell.h"
#include "json_file.h"
#include "log.h"
#include "memory.h"
#include "strings.h"

#include <string.h>

typedef struct {
    GAME_FLOW *gf;
    const char *script_path;
} M_CONTEXT;

#define DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(name)                              \
    int32_t name(                                                              \
        const M_CONTEXT *ctx, JSON_OBJECT *event_obj,                          \
        GF_SEQUENCE_EVENT *event, void *extra_data, void *user_arg)

typedef int32_t (*M_SEQUENCE_EVENT_HANDLER_FUNC)(
    const M_CONTEXT *ctx, JSON_OBJECT *event_obj, GF_SEQUENCE_EVENT *event,
    void *extra_data, void *user_arg);

typedef struct {
    GF_SEQUENCE_EVENT_TYPE event_type;
    M_SEQUENCE_EVENT_HANDLER_FUNC handler_func;
    void *handler_func_arg;
} M_SEQUENCE_EVENT_HANDLER;

typedef void (*M_LOAD_ARRAY_FUNC)(
    const M_CONTEXT *ctx, JSON_OBJECT *source_elem, void *target_elem,
    size_t target_elem_idx, void *user_arg);

static OBJECT_ID M_GetObjectFromJSONValue(const JSON_VALUE *value);

static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleIntEvent);
static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandlePictureEvent);
static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleTotalStatsEvent);
static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleAddItemEvent);

static void M_LoadGlobalInjections(const M_CONTEXT *ctx, JSON_OBJECT *obj);
static void M_LoadCommonSettings(
    const M_CONTEXT *ctx, JSON_OBJECT *obj, GF_LEVEL_SETTINGS *settings);

#if TR_VERSION == 1
    #include "./reader_tr1.def.c"
#elif TR_VERSION == 2
    #include "./reader_tr2.def.c"
#endif

static void M_CopyRootSettingsIntoLevel(
    const M_CONTEXT *const ctx, GF_LEVEL_SETTINGS *const dst,
    const GF_LEVEL_SETTINGS *const src)
{
    *dst = *src;
#if TR_VERSION == 2
    dst->sfx_path = nullptr;
#endif
    if (src->ambient_tracks.is_present) {
        const GF_AMBIENT_DATA *const root = &src->ambient_tracks;
        GF_AMBIENT_DATA *const lvl = &dst->ambient_tracks;
        lvl->ids = Memory_Alloc(sizeof(*lvl->ids) * root->count);
        memcpy(lvl->ids, root->ids, sizeof(*lvl->ids) * root->count);
    }
}

static bool M_ParseRGB888(JSON_VALUE *const value, RGB_888 *const target)
{
    if (value != nullptr && value->type == JSON_TYPE_ARRAY) {
        const JSON_ARRAY *const tmp_arr = JSON_ValueAsArray(value);
        const RGB_F color = {
            JSON_ArrayGetDouble(tmp_arr, 0, -1.0),
            JSON_ArrayGetDouble(tmp_arr, 1, -1.0),
            JSON_ArrayGetDouble(tmp_arr, 2, -1.0),
        };
        if (color.r >= 0.0 && color.g >= 0.0 && color.b >= 0.0) {
            *target = (RGB_888) {
                color.r * 255.0f,
                color.g * 255.0f,
                color.b * 255.0f,
            };
            return true;
        }
    } else if (value != nullptr && value->type == JSON_TYPE_STRING) {
        const char *tmp_str = JSON_ValueGetString(value, JSON_INVALID_STRING);
        ASSERT(tmp_str != JSON_INVALID_STRING);
        RGB_888 tmp_color;
        if (String_ParseRGB888(tmp_str, &tmp_color)) {
            *target = tmp_color;
            return true;
        }
    }
    return false;
}

static void M_LoadCommonSettings(
    const M_CONTEXT *const ctx, JSON_OBJECT *const obj,
    GF_LEVEL_SETTINGS *const settings)
{
    {
        const double value =
            JSON_ObjectGetDouble(obj, "fog_start", JSON_INVALID_NUMBER);
        if (value != JSON_INVALID_NUMBER) {
            settings->fog_start.is_present = true;
            settings->fog_start.value = value;
        }
    }

    {
        const double value =
            JSON_ObjectGetDouble(obj, "fog_end", JSON_INVALID_NUMBER);
        if (value != JSON_INVALID_NUMBER) {
            settings->fog_end.is_present = true;
            settings->fog_end.value = value;
        }
    }

    {
        const int value =
            JSON_ObjectGetBool(obj, "fog_transparency", JSON_INVALID_BOOL);
        if (value != JSON_INVALID_BOOL) {
            settings->fog_transparency.is_present = true;
            settings->fog_transparency.value = value;
        }
    }

    {
        JSON_VALUE *const tmp_value = JSON_ObjectGetValue(obj, "fog_color");
        if (M_ParseRGB888(tmp_value, &settings->fog_color.value)) {
            settings->fog_color.is_present = true;
        }
    }

    {
        JSON_VALUE *const tmp_value = JSON_ObjectGetValue(obj, "water_color");
        if (M_ParseRGB888(tmp_value, &settings->water_color.value)) {
            settings->water_color.is_present = true;
        }
    }

    {
        const JSON_ARRAY *const tmp_arr =
            JSON_ObjectGetArray(obj, "ambient_tracks");
        if (tmp_arr != nullptr && tmp_arr->length != 0) {
            settings->ambient_tracks.is_present = true;
            settings->ambient_tracks.count = tmp_arr->length;
            settings->ambient_tracks.ids =
                Memory_Alloc(sizeof(MUSIC_ID) * tmp_arr->length);
            for (size_t i = 0; i < tmp_arr->length; i++) {
                settings->ambient_tracks.ids[i] =
                    JSON_ArrayGetInt(tmp_arr, i, MX_INACTIVE);
            }
        }
    }

    {
        const int32_t value =
            JSON_ObjectGetBool(obj, "cold_water", JSON_INVALID_BOOL);
        if (value != JSON_INVALID_BOOL) {
            settings->cold_water.is_present = true;
            settings->cold_water.value = value;
        }
    }
}

static void M_LoadCommonRoot(const M_CONTEXT *const ctx, JSON_OBJECT *const obj)
{
    const char *tmp_s =
        JSON_ObjectGetString(obj, "main_menu_picture", JSON_INVALID_STRING);
    if (tmp_s == JSON_INVALID_STRING) {
        Shell_ExitSystemFmt(
            "%s: 'main_menu_picture' must be a string", ctx->script_path);
    }
    ctx->gf->main_menu_background_path = Memory_DupStr(tmp_s);

    tmp_s =
        JSON_ObjectGetString(obj, "savegame_fmt_legacy", JSON_INVALID_STRING);
    if (tmp_s == JSON_INVALID_STRING) {
        Shell_ExitSystemFmt(
            "%s: 'savegame_fmt_legacy' must be a string", ctx->script_path);
    }
    ctx->gf->savegame_fmt_legacy = Memory_DupStr(tmp_s);

    tmp_s = JSON_ObjectGetString(obj, "savegame_fmt_bson", JSON_INVALID_STRING);
    if (tmp_s == JSON_INVALID_STRING) {
        Shell_ExitSystemFmt(
            "%s: 'savegame_fmt_bson' must be a string", ctx->script_path);
    }
    ctx->gf->savegame_fmt_bson = Memory_DupStr(tmp_s);

    ctx->gf->enable_killer_pushblocks =
        JSON_ObjectGetBool(obj, "enable_killer_pushblocks", true);
    {
        const char *tmp_s =
            JSON_ObjectGetString(obj, "main_script", JSON_INVALID_STRING);
        if (tmp_s != JSON_INVALID_STRING) {
            ctx->gf->main_script_path = Memory_DupStr(tmp_s);
        }
    }
}

static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleIntEvent)
{
    if (event != nullptr) {
        event->data =
            (void *)(intptr_t)JSON_ObjectGetInt(event_obj, user_arg, -1);
    }
    return 0;
}

static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandlePictureEvent)
{
    const char *const path = JSON_ObjectGetString(event_obj, "path", nullptr);
    if (event != nullptr) {
        GF_DISPLAY_PICTURE_DATA *const event_data = extra_data;
        event_data->path = (char *)extra_data + sizeof(GF_DISPLAY_PICTURE_DATA);
        event_data->is_legal = JSON_ObjectGetBool(event_obj, "legal", false);
        event_data->is_credit = JSON_ObjectGetBool(event_obj, "credit", false);
        event_data->display_time =
            JSON_ObjectGetDouble(event_obj, "display_time", 5.0);
        event_data->fade_in_time =
            JSON_ObjectGetDouble(event_obj, "fade_in_time", 1.0);
        event_data->fade_out_time =
            JSON_ObjectGetDouble(event_obj, "fade_out_time", 1.0 / 3.0);
        if (path != nullptr) {
            strcpy(event_data->path, path);
        }
        event->data = event_data;
    }
    return sizeof(GF_DISPLAY_PICTURE_DATA)
        + (path == nullptr ? 0 : strlen(path) + 1);
}

static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleTotalStatsEvent)
{
    const char *const path =
        JSON_ObjectGetString(event_obj, "background_path", nullptr);
    if (path == nullptr) {
        if (event != nullptr) {
            event->data = nullptr;
        }
        return 0;
    }
    if (event != nullptr) {
        char *const event_data = extra_data;
        strcpy(event_data, path);
        event->data = event_data;
    }
    return strlen(path) + 1;
}

static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleAddItemEvent)
{
    const OBJECT_ID obj_id =
        M_GetObjectFromJSONValue(JSON_ObjectGetValue(event_obj, "object_id"));
    if (obj_id == NO_OBJECT) {
        Shell_ExitSystemFmt(
            "%s: Invalid item in add item event", ctx->script_path);
        return -1;
    }
    if (event != nullptr) {
        GF_ADD_ITEM_DATA *const event_data = extra_data;
        event_data->object_id = obj_id;
        event_data->quantity = JSON_ObjectGetInt(event_obj, "quantity", 1);
        event_data->inv_type =
            event->type == GFS_ADD_ITEM ? GF_INV_REGULAR : GF_INV_SECRET;
        event->data = event_data;
    }
    return sizeof(GF_ADD_ITEM_DATA);
}

static OBJECT_ID M_GetObjectFromJSONValue(const JSON_VALUE *const value)
{
    OBJECT_ID object_id;
    int32_t game_id = JSON_ValueGetInt(value, JSON_INVALID_NUMBER);
    if (game_id != JSON_INVALID_NUMBER) {
        object_id = Object_FromGameID(game_id);
    } else {
        const char *const object_key =
            JSON_ValueGetString(value, JSON_INVALID_STRING);
        if (object_key == JSON_INVALID_STRING) {
            return NO_OBJECT;
        }
        object_id = Object_IdFromKey(object_key);
    }
    return object_id;
}

static void M_LoadArray(
    const M_CONTEXT *const ctx, JSON_OBJECT *const obj, const char *const key,
    int32_t *const count, void **const elements, const size_t element_size,
    const M_LOAD_ARRAY_FUNC load_func, void *const load_func_arg)
{
    if (!JSON_ObjectContainsKey(obj, key)) {
        return;
    }

    JSON_ARRAY *const elem_arr = JSON_ObjectGetArray(obj, key);
    if (elem_arr == nullptr) {
        Shell_ExitSystemFmt("%s: '%s' must be a list", ctx->script_path, key);
    }

    *count = elem_arr->length;
    *elements = Memory_Alloc(element_size * (*count));

    JSON_ARRAY_ELEMENT *elem = elem_arr->start;
    for (size_t i = 0; i < elem_arr->length; i++, elem = elem->next) {
        void *const element = (char *)*elements + i * element_size;

        JSON_OBJECT *const elem_obj = JSON_ValueAsObject(elem->value);
        if (elem_obj == nullptr) {
            Shell_ExitSystemFmt(
                "%s: '%s' elements must be dictionaries", ctx->script_path,
                key);
        }

        load_func(ctx, elem_obj, element, i, load_func_arg);
    }
}

static size_t M_LoadSequenceEvent(
    const M_CONTEXT *const ctx, JSON_OBJECT *const event_obj,
    GF_SEQUENCE_EVENT *const event, void *const extra_data)
{
    const char *const type_str = JSON_ObjectGetString(event_obj, "type", "");
    const GF_SEQUENCE_EVENT_TYPE type =
        ENUM_MAP_GET(GF_SEQUENCE_EVENT_TYPE, type_str, -1);
    if (type == (GF_SEQUENCE_EVENT_TYPE)-1) {
        Shell_ExitSystemFmt(
            "%s: Unknown game flow sequence event type: '%s'", ctx->script_path,
            type_str);
    }

    const M_SEQUENCE_EVENT_HANDLER *handler = M_GetSequenceEventHandlers();
    while (handler->event_type != (GF_SEQUENCE_EVENT_TYPE)-1
           && handler->event_type != type) {
        handler++;
    }

    int32_t extra_data_size = 0;
    if (handler->handler_func != nullptr) {
        extra_data_size = handler->handler_func(
            ctx, event_obj, nullptr, nullptr, handler->handler_func_arg);
    }
    if (extra_data_size >= 0 && event != nullptr) {
        event->type = handler->event_type;
        if (handler->handler_func != nullptr) {
            handler->handler_func(
                ctx, event_obj, event, extra_data, handler->handler_func_arg);
        } else {
            event->data = nullptr;
        }
    }
    return extra_data_size;
}

static void M_LoadSequence(
    const M_CONTEXT *const ctx, JSON_ARRAY *const jseq_arr,
    GF_SEQUENCE *const sequence)
{
    sequence->length = 0;
    if (jseq_arr == nullptr) {
        return;
    }
    size_t event_base_size = sizeof(GF_SEQUENCE_EVENT);
    size_t total_data_size = 0;
    for (size_t i = 0; i < jseq_arr->length; i++) {
        JSON_OBJECT *jevent = JSON_ArrayGetObject(jseq_arr, i);
        const int32_t event_extra_size =
            M_LoadSequenceEvent(ctx, jevent, nullptr, nullptr);
        if (event_extra_size < 0) {
            // Parsing this event failed - discard it
            continue;
        }
        total_data_size += event_base_size;
        total_data_size += event_extra_size;
        sequence->length++;
    }

    char *const data = Memory_Alloc(total_data_size);
    char *extra_data_ptr = data + event_base_size * sequence->length;
    sequence->events = (GF_SEQUENCE_EVENT *)data;

    int32_t j = 0;
    for (int32_t i = 0; i < sequence->length; i++) {
        JSON_OBJECT *const jevent = JSON_ArrayGetObject(jseq_arr, i);
        const int32_t event_extra_size = M_LoadSequenceEvent(
            ctx, jevent, &sequence->events[j++], extra_data_ptr);
        if (event_extra_size < 0) {
            // Parsing this event failed - discard it
            continue;
        }
        extra_data_ptr += event_extra_size;
    }
}

static void M_LoadLevelInjections(
    const M_CONTEXT *const ctx, JSON_OBJECT *const jlvl_obj,
    GF_LEVEL *const level)
{
    const bool inherit =
        JSON_ObjectGetBool(jlvl_obj, "inherit_injections", true);
    JSON_ARRAY *const injections = JSON_ObjectGetArray(jlvl_obj, "injections");

    level->injections.count = 0;
    if (injections == nullptr && !inherit) {
        return;
    }

    if (inherit) {
        level->injections.count += ctx->gf->injections.count;
    }
    if (injections != nullptr) {
        level->injections.count += injections->length;
    }

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

    if (injections == nullptr) {
        return;
    }

    for (size_t i = 0; i < injections->length; i++) {
        const char *const str = JSON_ArrayGetString(injections, i, nullptr);
        level->injections.data_paths[base_index + i] = Memory_DupStr(str);
    }
}

static void M_LoadLevelSequence(
    const M_CONTEXT *const ctx, JSON_OBJECT *const jlvl_obj,
    GF_LEVEL *const level)
{
    JSON_ARRAY *const jseq_arr = JSON_ObjectGetArray(jlvl_obj, "sequence");
    if (jseq_arr == nullptr) {
        Shell_ExitSystemFmt(
            "%s, level %d: 'sequence' must be a list", ctx->script_path,
            level->num);
    }
    M_LoadSequence(ctx, jseq_arr, &level->sequence);

    for (int32_t i = 0; i < level->sequence.length; i++) {
        GF_SEQUENCE_EVENT *const event = &level->sequence.events[i];
        if (event->type == GFS_LOOP_GAME) {
            event->data = (void *)(intptr_t)level->num;
        }
    }
}

static void M_LoadLevel(
    const M_CONTEXT *const ctx, JSON_OBJECT *const jlvl_obj,
    GF_LEVEL *const level, const size_t idx, void *const user_arg)
{
    level->num = idx;

    {
        level->type = (GF_LEVEL_TYPE)(intptr_t)user_arg;
        const JSON_VALUE *const tmp_v = JSON_ObjectGetValue(jlvl_obj, "type");
        if (tmp_v != nullptr) {
            const char *const tmp =
                JSON_ValueGetString(tmp_v, JSON_INVALID_STRING);
            if (tmp == JSON_INVALID_STRING) {
                Shell_ExitSystemFmt(
                    "%s, level %d: 'type' must be a string", ctx->script_path,
                    level->num);
            }
            const GF_LEVEL_TYPE user_type =
                ENUM_MAP_GET(GF_LEVEL_TYPE, tmp, -1);
            if (user_type == (GF_LEVEL_TYPE)-1) {
                Shell_ExitSystemFmt(
                    "%s, level %d: unrecognized type '%s'", ctx->script_path,
                    level->num, tmp);
            }

            if (level->type != GFL_NORMAL
                && GF_GetLevelTableType(user_type) != GFLT_MAIN) {
                Shell_ExitSystemFmt(
                    "%s, level %d: cannot override level type=%s to %s",
                    ctx->script_path, level->num,
                    ENUM_MAP_TO_STRING(GF_LEVEL_TYPE, level->type),
                    ENUM_MAP_TO_STRING(GF_LEVEL_TYPE, user_type));
            }
            level->type = user_type;
        }
    }

    if (level->type == GFL_DUMMY) {
        return;
    }

    {
        const char *const tmp =
            JSON_ObjectGetString(jlvl_obj, "path", JSON_INVALID_STRING);
        if (tmp == JSON_INVALID_STRING) {
            Shell_ExitSystemFmt(
                "%s, level %d: 'file' must be a string", ctx->script_path,
                level->num);
        }
        level->path = Memory_DupStr(tmp);
    }
    {
        const char *tmp_script =
            JSON_ObjectGetString(jlvl_obj, "script", JSON_INVALID_STRING);
        if (tmp_script != JSON_INVALID_STRING) {
            level->script_path = Memory_DupStr(tmp_script);
        } else {
            level->script_path = nullptr;
        }
    }

    {
        const JSON_VALUE *const tmp_v =
            JSON_ObjectGetValue(jlvl_obj, "music_track");
        if (tmp_v != nullptr) {
            const int32_t tmp = JSON_ValueGetInt(tmp_v, JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                Shell_ExitSystemFmt(
                    "%s, level %d: 'music_track' must be a number",
                    ctx->script_path, level->num);
            }
            level->music_track = tmp;
        } else {
            level->music_track = MX_INACTIVE;
        }
    }

    M_CopyRootSettingsIntoLevel(ctx, &level->settings, &ctx->gf->settings);
    M_LoadLevelGameSpecifics(ctx, jlvl_obj, level);

    M_LoadLevelSequence(ctx, jlvl_obj, level);
    M_LoadLevelInjections(ctx, jlvl_obj, level);
}

static void M_LoadLevelTable(
    const M_CONTEXT *const ctx, JSON_OBJECT *const obj, const char *const key,
    GF_LEVEL_TABLE *const level_table, const GF_LEVEL_TYPE default_level_type)
{
    M_LoadArray(
        ctx, obj, key, &level_table->count, (void **)&level_table->levels,
        sizeof(GF_LEVEL), (M_LOAD_ARRAY_FUNC)M_LoadLevel,
        (void *)(intptr_t)default_level_type);
}

static void M_LoadLevels(const M_CONTEXT *const ctx, JSON_OBJECT *const obj)
{
    JSON_ARRAY *const jlvl_arr = JSON_ObjectGetArray(obj, "levels");
    if (!jlvl_arr) {
        Shell_ExitSystemFmt("%s: 'levels' must be a list", ctx->script_path);
    }
    M_LoadLevelTable(
        ctx, obj, "levels", &ctx->gf->level_tables[GFLT_MAIN], GFL_NORMAL);
}

static void M_LoadCutscenes(const M_CONTEXT *const ctx, JSON_OBJECT *const obj)
{
    M_LoadLevelTable(
        ctx, obj, "cutscenes", &ctx->gf->level_tables[GFLT_CUTSCENES],
        GFL_CUTSCENE);
}

static void M_LoadDemos(const M_CONTEXT *const ctx, JSON_OBJECT *const obj)
{
    M_LoadLevelTable(
        ctx, obj, "demos", &ctx->gf->level_tables[GFLT_DEMOS], GFL_DEMO);
}

static void M_LoadTitleLevel(const M_CONTEXT *const ctx, JSON_OBJECT *obj)
{
    JSON_OBJECT *title_obj = JSON_ObjectGetObject(obj, "title");
    if (title_obj != nullptr) {
        ctx->gf->title_level = Memory_Alloc(sizeof(GF_LEVEL));
        M_LoadLevel(
            ctx, title_obj, ctx->gf->title_level, 0,
            (void *)(intptr_t)GFL_TITLE);
    }
}

static void M_LoadFMV(
    const M_CONTEXT *const ctx, JSON_OBJECT *const obj, GF_FMV *const fmv,
    size_t idx, void *const user_arg)
{
    const char *const path = JSON_ObjectGetString(obj, "path", nullptr);
    if (path == nullptr) {
        Shell_ExitSystemFmt("%s: Missing FMV path", ctx->script_path);
    }
    fmv->path = Memory_DupStr(path);
    fmv->is_legal = JSON_ObjectGetBool(obj, "legal", false);
    fmv->is_credit = JSON_ObjectGetBool(obj, "credit", false);
}

static void M_LoadFMVs(const M_CONTEXT *const ctx, JSON_OBJECT *const obj)
{
    M_LoadArray(
        ctx, obj, "fmvs", &ctx->gf->fmv_count, (void **)&ctx->gf->fmvs,
        sizeof(GF_FMV), (M_LOAD_ARRAY_FUNC)M_LoadFMV, nullptr);
}

static void M_LoadGlobalInjections(
    const M_CONTEXT *const ctx, JSON_OBJECT *const obj)
{
    ctx->gf->injections.count = 0;
    JSON_ARRAY *const injections = JSON_ObjectGetArray(obj, "injections");
    if (injections == nullptr) {
        return;
    }

    ctx->gf->injections.count = injections->length;
    ctx->gf->injections.data_paths =
        Memory_Alloc(sizeof(char *) * injections->length);
    for (size_t i = 0; i < injections->length; i++) {
        const char *const str = JSON_ArrayGetString(injections, i, nullptr);
        ctx->gf->injections.data_paths[i] = Memory_DupStr(str);
    }
}

void GF_LoadFromFile(const char *const path)
{
    char *script_data = nullptr;
    if (!File_Load(path, &script_data, nullptr)) {
        Shell_ExitSystemFmt("Failed to open script file %s", path);
    }

    GF_LoadFromString(script_data, path);
    Memory_FreePointer(&script_data);
}

void GF_LoadFromString(
    const char *const script_data, const char *const script_path)
{
    GF_Shutdown();

    M_CONTEXT ctx = { .gf = &g_GameFlow };
    ctx.gf->main_script_path = nullptr;
    ctx.gf->path = Memory_DupStr(script_path);
    ctx.script_path = g_GameFlow.path;

    JSON_VALUE *const doc = JSONFile_ReadEx(
        script_path, (JSON_FILE_OPTIONS) { .exit_on_error = true });
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(doc);

    M_LoadCommonRoot(&ctx, root_obj);
    M_LoadRoot(&ctx, root_obj);
    M_LoadLevels(&ctx, root_obj);
    M_LoadCutscenes(&ctx, root_obj);
    M_LoadDemos(&ctx, root_obj);
    M_LoadFMVs(&ctx, root_obj);
    M_LoadTitleLevel(&ctx, root_obj);

    JSON_ValueFree(doc);
}
