#include "game/game_flow/reader.h"

#include "debug.h"
#include "enum_map.h"
#include "filesystem.h"
#include "game/game_flow/common.h"
#include "game/game_flow/types.h"
#include "game/game_flow/vars.h"
#include "game/objects/names.h"
#include "game/shell.h"
#include "json.h"
#include "log.h"
#include "memory.h"

#include <string.h>

#define DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(name)                              \
    int32_t name(                                                              \
        JSON_OBJECT *event_obj, GF_SEQUENCE_EVENT *event, void *extra_data,    \
        void *user_arg)
typedef int32_t (*M_SEQUENCE_EVENT_HANDLER_FUNC)(
    JSON_OBJECT *event_obj, GF_SEQUENCE_EVENT *event, void *extra_data,
    void *user_arg);
typedef struct {
    GF_SEQUENCE_EVENT_TYPE event_type;
    M_SEQUENCE_EVENT_HANDLER_FUNC handler_func;
    void *handler_func_arg;
} M_SEQUENCE_EVENT_HANDLER;

static M_SEQUENCE_EVENT_HANDLER *M_GetSequenceEventHandlers(void);

typedef void (*M_LOAD_ARRAY_FUNC)(
    JSON_OBJECT *source_elem, const GAME_FLOW *gf, void *target_elem,
    size_t target_elem_idx, void *user_arg);

static GAME_OBJECT_ID M_GetObjectFromJSONValue(const JSON_VALUE *value);

static void M_LoadArray(
    JSON_OBJECT *obj, const GAME_FLOW *gf, const char *key, int32_t *count,
    void **elements, size_t element_size, M_LOAD_ARRAY_FUNC load_func,
    void *load_func_arg);

static void M_LoadSettings(JSON_OBJECT *obj, GF_LEVEL_SETTINGS *settings);

static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleIntEvent);
static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandlePictureEvent);
static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleTotalStatsEvent);
static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleAddItemEvent);
static size_t M_LoadSequenceEvent(
    JSON_OBJECT *event_obj, GF_SEQUENCE_EVENT *event, void *extra_data);
static void M_LoadSequence(JSON_ARRAY *jseq_arr, GF_SEQUENCE *sequence);

static void M_LoadLevelInjections(
    JSON_OBJECT *obj, const GAME_FLOW *gf, GF_LEVEL *level);
static void M_LoadLevelGameSpecifics(
    JSON_OBJECT *obj, const GAME_FLOW *gf, GF_LEVEL *level);
static void M_LoadLevelSequence(JSON_OBJECT *obj, GF_LEVEL *level);
static void M_LoadLevel(
    JSON_OBJECT *jlvl_obj, const GAME_FLOW *gf, GF_LEVEL *level, size_t idx,
    void *user_arg);
static void M_LoadLevelTable(
    JSON_OBJECT *obj, const GAME_FLOW *gf, const char *key,
    GF_LEVEL_TABLE *level_table, GF_LEVEL_TYPE default_level_type);

static void M_LoadLevels(JSON_OBJECT *obj, GAME_FLOW *gf);
static void M_LoadCutscenes(JSON_OBJECT *obj, GAME_FLOW *gf);
static void M_LoadDemos(JSON_OBJECT *obj, GAME_FLOW *gf);
static void M_LoadTitleLevel(JSON_OBJECT *obj, GAME_FLOW *gf);
static void M_LoadFMV(
    JSON_OBJECT *obj, const GAME_FLOW *gf, GF_FMV *level, size_t idx,
    void *user_arg);
static void M_LoadFMVs(JSON_OBJECT *obj, GAME_FLOW *gf);
static void M_LoadGlobalInjections(JSON_OBJECT *obj, GAME_FLOW *gf);
static void M_LoadRoot(JSON_OBJECT *obj, GAME_FLOW *gf);

#if TR_VERSION == 1
    #include "./reader_tr1.def.c"
#elif TR_VERSION == 2
    #include "./reader_tr2.def.c"
#endif

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
    if (path == nullptr) {
        Shell_ExitSystem("Missing picture path");
        return -1;
    }
    if (event != nullptr) {
        GF_DISPLAY_PICTURE_DATA *const event_data = extra_data;
        event_data->path = (char *)extra_data + sizeof(GF_DISPLAY_PICTURE_DATA);
        event_data->display_time =
            JSON_ObjectGetDouble(event_obj, "display_time", 5.0);
        event_data->fade_in_time =
            JSON_ObjectGetDouble(event_obj, "fade_in_time", 1.0);
        event_data->fade_out_time =
            JSON_ObjectGetDouble(event_obj, "fade_out_time", 1.0 / 3.0);
        strcpy(event_data->path, path);
        event->data = event_data;
    }
    return sizeof(GF_DISPLAY_PICTURE_DATA) + strlen(path) + 1;
}

static DECLARE_SEQUENCE_EVENT_HANDLER_FUNC(M_HandleTotalStatsEvent)
{
    const char *const path =
        JSON_ObjectGetString(event_obj, "background_path", nullptr);
    if (path == nullptr) {
        Shell_ExitSystem("Missing picture path");
        return -1;
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
    const GAME_OBJECT_ID obj_id =
        M_GetObjectFromJSONValue(JSON_ObjectGetValue(event_obj, "object_id"));
    if (obj_id == NO_OBJECT) {
        Shell_ExitSystem("Invalid item");
        return -1;
    }
    if (event != nullptr) {
        GF_ADD_ITEM_DATA *const event_data = extra_data;
        event_data->object_id = obj_id;
        event_data->quantity = JSON_ObjectGetInt(event_obj, "quantity", 1);
#if TR_VERSION == 2
        event_data->inv_type =
            event->type == GFS_ADD_ITEM ? GF_INV_REGULAR : GF_INV_SECRET;
#endif
        event->data = event_data;
    }
    return sizeof(GF_ADD_ITEM_DATA);
}

static GAME_OBJECT_ID M_GetObjectFromJSONValue(const JSON_VALUE *const value)
{
    int32_t object_id = JSON_ValueGetInt(value, JSON_INVALID_NUMBER);
    if (object_id == JSON_INVALID_NUMBER) {
        const char *const object_key =
            JSON_ValueGetString(value, JSON_INVALID_STRING);
        if (object_key == JSON_INVALID_STRING) {
            return NO_OBJECT;
        }
        object_id = Object_IdFromKey(object_key);
    }
    if (object_id < 0 || object_id >= O_NUMBER_OF) {
        return NO_OBJECT;
    }
    return object_id;
}

static void M_LoadArray(
    JSON_OBJECT *const obj, const GAME_FLOW *const gf, const char *const key,
    int32_t *const count, void **const elements, const size_t element_size,
    const M_LOAD_ARRAY_FUNC load_func, void *const load_func_arg)
{
    if (!JSON_ObjectContainsKey(obj, key)) {
        return;
    }

    JSON_ARRAY *const elem_arr = JSON_ObjectGetArray(obj, key);
    if (elem_arr == nullptr) {
        Shell_ExitSystemFmt("'%s' must be a list", key);
    }

    *count = elem_arr->length;
    *elements = Memory_Alloc(element_size * (*count));

    JSON_ARRAY_ELEMENT *elem = elem_arr->start;
    for (size_t i = 0; i < elem_arr->length; i++, elem = elem->next) {
        void *const element = (char *)*elements + i * element_size;

        JSON_OBJECT *const elem_obj = JSON_ValueAsObject(elem->value);
        if (elem_obj == nullptr) {
            Shell_ExitSystemFmt("'%s' elements must be dictionaries", key);
        }

        load_func(elem_obj, gf, element, i, load_func_arg);
    }
}

static size_t M_LoadSequenceEvent(
    JSON_OBJECT *const event_obj, GF_SEQUENCE_EVENT *const event,
    void *const extra_data)
{
    const char *const type_str = JSON_ObjectGetString(event_obj, "type", "");
    const GF_SEQUENCE_EVENT_TYPE type =
        ENUM_MAP_GET(GF_SEQUENCE_EVENT_TYPE, type_str, -1);

    const M_SEQUENCE_EVENT_HANDLER *handler = M_GetSequenceEventHandlers();
    while (handler->event_type != (GF_SEQUENCE_EVENT_TYPE)-1
           && handler->event_type != type) {
        handler++;
    }

    if (handler->event_type != type) {
        Shell_ExitSystemFmt(
            "Unknown game flow sequence event type: '%s'", type);
    }

    int32_t extra_data_size = 0;
    if (handler->handler_func != nullptr) {
        extra_data_size = handler->handler_func(
            event_obj, nullptr, nullptr, handler->handler_func_arg);
    }
    if (extra_data_size >= 0 && event != nullptr) {
        event->type = handler->event_type;
        if (handler->handler_func != nullptr) {
            handler->handler_func(
                event_obj, event, extra_data, handler->handler_func_arg);
        } else {
            event->data = nullptr;
        }
    }
    return extra_data_size;
}

static void M_LoadSequence(
    JSON_ARRAY *const jseq_arr, GF_SEQUENCE *const sequence)
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
            M_LoadSequenceEvent(jevent, nullptr, nullptr);
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
        const int32_t event_extra_size =
            M_LoadSequenceEvent(jevent, &sequence->events[j++], extra_data_ptr);
        if (event_extra_size < 0) {
            // Parsing this event failed - discard it
            continue;
        }
        extra_data_ptr += event_extra_size;
    }
}

static void M_LoadLevelInjections(
    JSON_OBJECT *const jlvl_obj, const GAME_FLOW *const gf,
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
        level->injections.count += gf->injections.count;
    }
    if (injections != nullptr) {
        level->injections.count += injections->length;
    }

    level->injections.data_paths =
        Memory_Alloc(sizeof(char *) * level->injections.count);

    int32_t base_index = 0;
    if (inherit) {
        for (int32_t i = 0; i < gf->injections.count; i++) {
            level->injections.data_paths[i] =
                Memory_DupStr(gf->injections.data_paths[i]);
        }
        base_index = gf->injections.count;
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
    JSON_OBJECT *const jlvl_obj, GF_LEVEL *const level)
{
    JSON_ARRAY *const jseq_arr = JSON_ObjectGetArray(jlvl_obj, "sequence");
    if (jseq_arr == nullptr) {
        Shell_ExitSystemFmt("level %d: 'sequence' must be a list", level->num);
    }
    M_LoadSequence(jseq_arr, &level->sequence);

    for (int32_t i = 0; i < level->sequence.length; i++) {
        GF_SEQUENCE_EVENT *const event = &level->sequence.events[i];
        if (event->type == GFS_LOOP_GAME) {
            event->data = (void *)(intptr_t)level->num;
        }
    }
}

static void M_LoadLevel(
    JSON_OBJECT *const jlvl_obj, const GAME_FLOW *const gf,
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
                    "level %d: 'type' must be a string", level->num);
            }
            const GF_LEVEL_TYPE user_type =
                ENUM_MAP_GET(GF_LEVEL_TYPE, tmp, -1);
            if (user_type == (GF_LEVEL_TYPE)-1) {
                Shell_ExitSystemFmt("unrecognized type '%s'", tmp);
            }

            if (level->type != GFL_NORMAL
                && GF_GetLevelTableType(user_type) != GFLT_MAIN) {
                Shell_ExitSystemFmt(
                    "cannot override level type=%s to %s",
                    ENUM_MAP_TO_STRING(GF_LEVEL_TYPE, level->type),
                    ENUM_MAP_TO_STRING(GF_LEVEL_TYPE, user_type));
            }
            level->type = user_type;
        }
    }

#if TR_VERSION == 1
    if (level->type == GFL_DUMMY) {
        return;
    }
#endif

    {
        const char *const tmp =
            JSON_ObjectGetString(jlvl_obj, "path", JSON_INVALID_STRING);
        if (tmp == JSON_INVALID_STRING) {
            Shell_ExitSystemFmt(
                "level %d: 'file' must be a string", level->num);
        }
        level->path = Memory_DupStr(tmp);
    }

    {
        const JSON_VALUE *const tmp_v =
            JSON_ObjectGetValue(jlvl_obj, "music_track");
        if (tmp_v != nullptr) {
            const int32_t tmp = JSON_ValueGetInt(tmp_v, JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                Shell_ExitSystemFmt(
                    "level %d: 'music_track' must be a number", level->num);
            }
            level->music_track = tmp;
        } else {
            level->music_track = MX_INACTIVE;
        }
    }

    M_LoadLevelGameSpecifics(jlvl_obj, gf, level);

    M_LoadLevelSequence(jlvl_obj, level);
    M_LoadLevelInjections(jlvl_obj, gf, level);
}

static void M_LoadLevelTable(
    JSON_OBJECT *const obj, const GAME_FLOW *const gf, const char *const key,
    GF_LEVEL_TABLE *const level_table, const GF_LEVEL_TYPE default_level_type)
{
    M_LoadArray(
        obj, gf, key, &level_table->count, (void **)&level_table->levels,
        sizeof(GF_LEVEL), (M_LOAD_ARRAY_FUNC)M_LoadLevel,
        (void *)(intptr_t)default_level_type);
}

static void M_LoadLevels(JSON_OBJECT *const obj, GAME_FLOW *const gf)
{
    JSON_ARRAY *const jlvl_arr = JSON_ObjectGetArray(obj, "levels");
    if (!jlvl_arr) {
        Shell_ExitSystem("'levels' must be a list");
    }
    M_LoadLevelTable(
        obj, gf, "levels", &gf->level_tables[GFLT_MAIN], GFL_NORMAL);
}

static void M_LoadCutscenes(JSON_OBJECT *const obj, GAME_FLOW *const gf)
{
    M_LoadLevelTable(
        obj, gf, "cutscenes", &gf->level_tables[GFLT_CUTSCENES], GFL_CUTSCENE);
}

static void M_LoadDemos(JSON_OBJECT *const obj, GAME_FLOW *const gf)
{
    M_LoadLevelTable(obj, gf, "demos", &gf->level_tables[GFLT_DEMOS], GFL_DEMO);
}

static void M_LoadTitleLevel(JSON_OBJECT *obj, GAME_FLOW *const gf)
{
    JSON_OBJECT *title_obj = JSON_ObjectGetObject(obj, "title");
    if (title_obj != nullptr) {
        gf->title_level = Memory_Alloc(sizeof(GF_LEVEL));
        M_LoadLevel(title_obj, gf, gf->title_level, 0, GFL_TITLE);
    }
}

static void M_LoadFMV(
    JSON_OBJECT *const obj, const GAME_FLOW *const gf, GF_FMV *const fmv,
    size_t idx, void *const user_arg)
{
    const char *const path = JSON_ObjectGetString(obj, "path", nullptr);
    if (path == nullptr) {
        Shell_ExitSystemFmt("Missing FMV path");
    }
    fmv->path = Memory_DupStr(path);
}

static void M_LoadFMVs(JSON_OBJECT *const obj, GAME_FLOW *const gf)
{
    M_LoadArray(
        obj, gf, "fmvs", &gf->fmv_count, (void **)&gf->fmvs, sizeof(GF_FMV),
        (M_LOAD_ARRAY_FUNC)M_LoadFMV, nullptr);
}

static void M_LoadGlobalInjections(JSON_OBJECT *const obj, GAME_FLOW *const gf)
{
    gf->injections.count = 0;
    JSON_ARRAY *const injections = JSON_ObjectGetArray(obj, "injections");
    if (injections == nullptr) {
        return;
    }

    gf->injections.count = injections->length;
    gf->injections.data_paths =
        Memory_Alloc(sizeof(char *) * injections->length);
    for (size_t i = 0; i < injections->length; i++) {
        const char *const str = JSON_ArrayGetString(injections, i, nullptr);
        gf->injections.data_paths[i] = Memory_DupStr(str);
    }
}

void GF_Load(const char *const path)
{
    GF_Shutdown();

    char *script_data = nullptr;
    if (!File_Load(path, &script_data, nullptr)) {
        Shell_ExitSystem("Failed to open script file");
    }

    JSON_PARSE_RESULT parse_result;
    JSON_VALUE *const root = JSON_ParseEx(
        script_data, strlen(script_data), JSON_PARSE_FLAGS_ALLOW_JSON5, nullptr,
        nullptr, &parse_result);
    if (root == nullptr) {
        Shell_ExitSystemFmt(
            "Failed to parse script file: %s in line %d, char %d",
            JSON_GetErrorDescription(parse_result.error),
            parse_result.error_line_no, parse_result.error_row_no, script_data);
    }
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);

    GAME_FLOW *const gf = &g_GameFlow;
    M_LoadRoot(root_obj, gf);
    M_LoadLevels(root_obj, gf);
    M_LoadCutscenes(root_obj, gf);
    M_LoadDemos(root_obj, gf);
    M_LoadFMVs(root_obj, gf);
    M_LoadTitleLevel(root_obj, gf);

    if (root != nullptr) {
        JSON_ValueFree(root);
    }
    Memory_FreePointer(&script_data);
}
