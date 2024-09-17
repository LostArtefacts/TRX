#include "game/gameflow.h"

#include "config.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/inventory.h"
#include "game/inventory/inventory_vars.h"
#include "game/lara.h"
#include "game/music.h"
#include "game/objects/creatures/bacon_lara.h"
#include "game/phase/phase.h"
#include "game/phase/phase_cutscene.h"
#include "game/phase/phase_picture.h"
#include "game/phase/phase_stats.h"
#include "game/room.h"
#include "global/vars.h"

#include <libtrx/filesystem.h>
#include <libtrx/json.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <string.h>

typedef struct STRING_TO_ENUM_TYPE {
    const char *str;
    const int32_t val;
} STRING_TO_ENUM_TYPE;

typedef struct GAMEFLOW_DISPLAY_PICTURE_DATA {
    char *path;
    double display_time;
} GAMEFLOW_DISPLAY_PICTURE_DATA;

typedef struct GAMEFLOW_MESH_SWAP_DATA {
    GAME_OBJECT_ID object1_id;
    GAME_OBJECT_ID object2_id;
    int32_t mesh_num;
} GAMEFLOW_MESH_SWAP_DATA;

typedef struct GAMEFLOW_GIVE_ITEM_DATA {
    GAME_OBJECT_ID object_id;
    int quantity;
} GAMEFLOW_GIVE_ITEM_DATA;

GAMEFLOW g_GameFlow = { 0 };

static int32_t GameFlow_StringToEnumType(
    const char *const str, const STRING_TO_ENUM_TYPE *map);
static TRISTATE_BOOL GameFlow_ReadTristateBool(
    struct json_object_s *obj, const char *key);
static bool GameFlow_LoadScriptMeta(struct json_object_s *obj);
static bool GameFlow_LoadScriptGameStrings(struct json_object_s *obj);
static bool GameFlow_IsLegacySequence(const char *type_str);
static bool GameFlow_LoadLevelSequence(
    struct json_object_s *obj, int32_t level_num);
static bool GameFlow_LoadScriptLevels(struct json_object_s *obj);
static bool GameFlow_LoadFromFileImpl(const char *file_name);

static const STRING_TO_ENUM_TYPE m_GameflowLevelTypeEnumMap[] = {
    { "title", GFL_TITLE },
    { "normal", GFL_NORMAL },
    { "cutscene", GFL_CUTSCENE },
    { "gym", GFL_GYM },
    { "current", GFL_CURRENT },
    { "bonus", GFL_BONUS },
    { "title_demo_pc", GFL_TITLE_DEMO_PC },
    { "level_demo_pc", GFL_LEVEL_DEMO_PC },
    { NULL, -1 },
};

static const STRING_TO_ENUM_TYPE m_GameflowSeqTypeEnumMap[] = {
    { "start_game", GFS_START_GAME },
    { "stop_game", GFS_STOP_GAME },
    { "loop_game", GFS_LOOP_GAME },
    { "start_cine", GFS_START_CINE },
    { "loop_cine", GFS_LOOP_CINE },
    { "play_fmv", GFS_PLAY_FMV },
    { "loading_screen", GFS_LOADING_SCREEN },
    { "display_picture", GFS_DISPLAY_PICTURE },
    { "level_stats", GFS_LEVEL_STATS },
    { "total_stats", GFS_TOTAL_STATS },
    { "exit_to_title", GFS_EXIT_TO_TITLE },
    { "exit_to_level", GFS_EXIT_TO_LEVEL },
    { "exit_to_cine", GFS_EXIT_TO_CINE },
    { "set_cam_x", GFS_SET_CAM_X },
    { "set_cam_y", GFS_SET_CAM_Y },
    { "set_cam_z", GFS_SET_CAM_Z },
    { "set_cam_angle", GFS_SET_CAM_ANGLE },
    { "flip_map", GFS_FLIP_MAP },
    { "remove_guns", GFS_REMOVE_GUNS },
    { "remove_scions", GFS_REMOVE_SCIONS },
    { "remove_ammo", GFS_REMOVE_AMMO },
    { "remove_medipacks", GFS_REMOVE_MEDIPACKS },
    { "give_item", GFS_GIVE_ITEM },
    { "play_synced_audio", GFS_PLAY_SYNCED_AUDIO },
    { "mesh_swap", GFS_MESH_SWAP },
    { "setup_bacon_lara", GFS_SETUP_BACON_LARA },
    { NULL, -1 },
};

static int32_t GameFlow_StringToEnumType(
    const char *const str, const STRING_TO_ENUM_TYPE *map)
{
    while (map->str) {
        if (!strcmp(str, map->str)) {
            break;
        }
        map++;
    }
    return map->val;
}

static TRISTATE_BOOL GameFlow_ReadTristateBool(
    struct json_object_s *obj, const char *key)
{
    struct json_value_s *value = json_object_get_value(obj, key);
    if (json_value_is_true(value)) {
        return TB_ON;
    } else if (json_value_is_false(value)) {
        return TB_OFF;
    }
    return TB_UNSPECIFIED;
}

static bool GameFlow_LoadScriptMeta(struct json_object_s *obj)
{
    const char *tmp_s;
    int tmp_i;
    double tmp_d;
    struct json_array_s *tmp_arr;

    tmp_s =
        json_object_get_string(obj, "main_menu_picture", JSON_INVALID_STRING);
    if (tmp_s == JSON_INVALID_STRING) {
        LOG_ERROR("'main_menu_picture' must be a string");
        return false;
    }
    g_GameFlow.main_menu_background_path = Memory_DupStr(tmp_s);

    tmp_s =
        json_object_get_string(obj, "savegame_fmt_legacy", JSON_INVALID_STRING);
    if (tmp_s == JSON_INVALID_STRING) {
        LOG_ERROR("'savegame_fmt_legacy' must be a string");
        return false;
    }
    g_GameFlow.savegame_fmt_legacy = Memory_DupStr(tmp_s);

    tmp_s =
        json_object_get_string(obj, "savegame_fmt_bson", JSON_INVALID_STRING);
    if (tmp_s == JSON_INVALID_STRING) {
        LOG_ERROR("'savegame_fmt_bson' must be a string");
        return false;
    }
    g_GameFlow.savegame_fmt_bson = Memory_DupStr(tmp_s);

    tmp_d = json_object_get_double(obj, "demo_delay", -1.0);
    if (tmp_d < 0.0) {
        LOG_ERROR("'demo_delay' must be a positive number");
        return false;
    }
    g_GameFlow.demo_delay = tmp_d;

    g_GameFlow.force_game_modes =
        GameFlow_ReadTristateBool(obj, "force_game_modes");
    if (json_object_get_bool(obj, "force_disable_game_modes", false)) {
        // backwards compatibility
        g_GameFlow.force_game_modes = TB_OFF;
    }

    g_GameFlow.force_save_crystals =
        GameFlow_ReadTristateBool(obj, "force_save_crystals");
    if (json_object_get_bool(obj, "force_enable_save_crystals", false)) {
        // backwards compatibility
        g_GameFlow.force_save_crystals = TB_ON;
    }

    tmp_arr = json_object_get_array(obj, "water_color");
    g_GameFlow.water_color.r = 0.6;
    g_GameFlow.water_color.g = 0.7;
    g_GameFlow.water_color.b = 1.0;
    if (tmp_arr) {
        g_GameFlow.water_color.r =
            json_array_get_double(tmp_arr, 0, g_GameFlow.water_color.r);
        g_GameFlow.water_color.g =
            json_array_get_double(tmp_arr, 1, g_GameFlow.water_color.g);
        g_GameFlow.water_color.b =
            json_array_get_double(tmp_arr, 2, g_GameFlow.water_color.b);
    }

    if (json_object_get_value(obj, "draw_distance_fade")) {
        double value = json_object_get_double(
            obj, "draw_distance_fade", JSON_INVALID_NUMBER);
        if (value == JSON_INVALID_NUMBER) {
            LOG_ERROR("'draw_distance_fade' must be a number");
            return false;
        }
        g_GameFlow.draw_distance_fade = value;
    } else {
        g_GameFlow.draw_distance_fade = 12.0f;
    }

    if (json_object_get_value(obj, "draw_distance_max")) {
        double value = json_object_get_double(
            obj, "draw_distance_max", JSON_INVALID_NUMBER);
        if (value == JSON_INVALID_NUMBER) {
            LOG_ERROR("'draw_distance_max' must be a number");
            return false;
        }
        g_GameFlow.draw_distance_max = value;
    } else {
        g_GameFlow.draw_distance_max = 20.0f;
    }

    tmp_arr = json_object_get_array(obj, "injections");
    if (tmp_arr) {
        g_GameFlow.injections.length = tmp_arr->length;
        g_GameFlow.injections.data_paths =
            Memory_Alloc(sizeof(char *) * tmp_arr->length);
        for (size_t i = 0; i < tmp_arr->length; i++) {
            struct json_value_s *value = json_array_get_value(tmp_arr, i);
            struct json_string_s *str = json_value_as_string(value);
            g_GameFlow.injections.data_paths[i] = Memory_DupStr(str->string);
        }
    } else {
        g_GameFlow.injections.length = 0;
    }

    g_GameFlow.convert_dropped_guns =
        json_object_get_bool(obj, "convert_dropped_guns", false);

    return true;
}

static bool GameFlow_LoadScriptGameStrings(struct json_object_s *obj)
{
    struct json_object_s *strings_obj = json_object_get_object(obj, "strings");
    if (!strings_obj) {
        LOG_ERROR("'strings' must be a dictionary");
        return false;
    }

    struct json_object_element_s *strings_elem = strings_obj->start;
    while (strings_elem) {
        const char *const key = strings_elem->name->string;
        struct json_string_s *value = json_value_as_string(strings_elem->value);
        if (!GameString_IsKnown(key)) {
            LOG_ERROR("invalid game string key: %s", key);
        } else if (!value || value->string == NULL) {
            LOG_ERROR("invalid game string value: %s", key);
        } else {
            GameString_Define(key, value->string);
        }
        strings_elem = strings_elem->next;
    }

    return true;
}

static bool GameFlow_IsLegacySequence(const char *type_str)
{
    return !strcmp(type_str, "fix_pyramid_secret")
        || !strcmp(type_str, "stop_cine");
}

static bool GameFlow_LoadLevelSequence(
    struct json_object_s *obj, int32_t level_num)
{
    struct json_array_s *jseq_arr = json_object_get_array(obj, "sequence");
    if (!jseq_arr) {
        LOG_ERROR("level %d: 'sequence' must be a list", level_num);
        return false;
    }

    struct json_array_element_s *jseq_elem = jseq_arr->start;

    g_GameFlow.levels[level_num].sequence =
        Memory_Alloc(sizeof(GAMEFLOW_SEQUENCE) * (jseq_arr->length + 1));

    GAMEFLOW_SEQUENCE *seq = g_GameFlow.levels[level_num].sequence;
    int32_t i = 0;
    while (jseq_elem) {
        struct json_object_s *jseq_obj = json_value_as_object(jseq_elem->value);
        if (!jseq_obj) {
            LOG_ERROR("level %d: 'sequence' elements must be dictionaries");
            return false;
        }

        const char *type_str =
            json_object_get_string(jseq_obj, "type", JSON_INVALID_STRING);
        if (type_str == JSON_INVALID_STRING) {
            LOG_ERROR("level %d: sequence 'type' must be a string", level_num);
            return false;
        }

        seq->type =
            GameFlow_StringToEnumType(type_str, m_GameflowSeqTypeEnumMap);

        switch (seq->type) {
        case GFS_START_GAME:
        case GFS_STOP_GAME:
        case GFS_LOOP_GAME:
        case GFS_START_CINE:
        case GFS_LOOP_CINE:
            seq->data = (void *)(intptr_t)level_num;
            break;

        case GFS_PLAY_FMV: {
            const char *tmp_s = json_object_get_string(
                jseq_obj, "fmv_path", JSON_INVALID_STRING);
            if (tmp_s == JSON_INVALID_STRING) {
                LOG_ERROR(
                    "level %d, sequence %s: 'fmv_path' must be a string",
                    level_num, type_str);
                return false;
            }
            seq->data = Memory_DupStr(tmp_s);
            break;
        }

        case GFS_LOADING_SCREEN:
        case GFS_DISPLAY_PICTURE: {
            GAMEFLOW_DISPLAY_PICTURE_DATA *data =
                Memory_Alloc(sizeof(GAMEFLOW_DISPLAY_PICTURE_DATA));

            const char *tmp_s = json_object_get_string(
                jseq_obj, "picture_path", JSON_INVALID_STRING);
            if (tmp_s == JSON_INVALID_STRING) {
                LOG_ERROR(
                    "level %d, sequence %s: 'picture_path' must be a string",
                    level_num, type_str);
                return false;
            }
            data->path = Memory_DupStr(tmp_s);

            double tmp_d =
                json_object_get_double(jseq_obj, "display_time", -1.0);
            if (tmp_d < 0.0) {
                LOG_ERROR(
                    "level %d, sequence %s: 'display_time' must be a positive "
                    "number",
                    level_num, type_str);
                return false;
            }
            data->display_time = tmp_d;
            seq->data = data;
            break;
        }

        case GFS_LEVEL_STATS: {
            int tmp =
                json_object_get_int(jseq_obj, "level_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'level_id' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)(intptr_t)tmp;
            break;
        }

        case GFS_TOTAL_STATS: {
            GAMEFLOW_DISPLAY_PICTURE_DATA *data =
                Memory_Alloc(sizeof(GAMEFLOW_DISPLAY_PICTURE_DATA));

            const char *tmp_s = json_object_get_string(
                jseq_obj, "picture_path", JSON_INVALID_STRING);
            if (tmp_s == JSON_INVALID_STRING) {
                LOG_ERROR(
                    "level %d, sequence %s: 'picture_path' must be a string",
                    level_num, type_str);
                return false;
            }
            data->path = Memory_DupStr(tmp_s);
            data->display_time = 0;
            seq->data = data;
            break;
        }

        case GFS_EXIT_TO_TITLE:
            break;

        case GFS_EXIT_TO_LEVEL:
        case GFS_EXIT_TO_CINE: {
            int tmp =
                json_object_get_int(jseq_obj, "level_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'level_id' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)(intptr_t)tmp;
            break;
        }

        case GFS_SET_CAM_X:
        case GFS_SET_CAM_Y:
        case GFS_SET_CAM_Z:
        case GFS_SET_CAM_ANGLE: {
            int tmp =
                json_object_get_int(jseq_obj, "value", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)(intptr_t)tmp;
            break;
        }

        case GFS_FLIP_MAP:
        case GFS_REMOVE_GUNS:
        case GFS_REMOVE_SCIONS:
        case GFS_REMOVE_AMMO:
        case GFS_REMOVE_MEDIPACKS:
            break;

        case GFS_GIVE_ITEM: {
            GAMEFLOW_GIVE_ITEM_DATA *give_item_data =
                Memory_Alloc(sizeof(GAMEFLOW_GIVE_ITEM_DATA));

            give_item_data->object_id =
                json_object_get_int(jseq_obj, "object_id", JSON_INVALID_NUMBER);
            if (give_item_data->object_id == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'object_id' must be a number",
                    level_num, type_str);
                return false;
            }

            give_item_data->quantity =
                json_object_get_int(jseq_obj, "quantity", 1);

            seq->data = give_item_data;
            break;
        }

        case GFS_PLAY_SYNCED_AUDIO: {
            int tmp =
                json_object_get_int(jseq_obj, "audio_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'audio_id' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)(intptr_t)tmp;
            break;
        }

        case GFS_MESH_SWAP: {
            GAMEFLOW_MESH_SWAP_DATA *swap_data =
                Memory_Alloc(sizeof(GAMEFLOW_MESH_SWAP_DATA));

            swap_data->object1_id = json_object_get_int(
                jseq_obj, "object1_id", JSON_INVALID_NUMBER);
            if (swap_data->object1_id == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'object1_id' must be a number",
                    level_num, type_str);
                return false;
            }

            swap_data->object2_id = json_object_get_int(
                jseq_obj, "object2_id", JSON_INVALID_NUMBER);
            if (swap_data->object2_id == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'object2_id' must be a number",
                    level_num, type_str);
                return false;
            }

            swap_data->mesh_num =
                json_object_get_int(jseq_obj, "mesh_id", JSON_INVALID_NUMBER);
            if (swap_data->mesh_num == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'mesh_id' must be a number",
                    level_num, type_str);
                return false;
            }

            seq->data = swap_data;
            break;
        }

        case GFS_SETUP_BACON_LARA: {
            int tmp = json_object_get_int(
                jseq_obj, "anchor_room", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'anchor_room' must be a number",
                    level_num, type_str);
                return false;
            }
            if (tmp < 0) {
                LOG_ERROR(
                    "level %d, sequence %s: 'anchor_room' must be >= 0",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)(intptr_t)tmp;
            break;
        }

        default:
            if (GameFlow_IsLegacySequence(type_str)) {
                seq->type = GFS_LEGACY;
                LOG_WARNING(
                    "level %d, sequence %s: legacy type ignored", level_num,
                    type_str);

            } else {
                LOG_ERROR("unknown sequence type %s", type_str);
                return false;
            }
            break;
        }

        jseq_elem = jseq_elem->next;
        i++;
        seq++;
    }

    seq->type = GFS_END;
    seq->data = NULL;

    return true;
}

static bool GameFlow_LoadScriptLevels(struct json_object_s *obj)
{
    struct json_array_s *jlvl_arr = json_object_get_array(obj, "levels");
    if (!jlvl_arr) {
        LOG_ERROR("'levels' must be a list");
        return false;
    }

    int32_t level_count = jlvl_arr->length;

    g_GameFlow.levels = Memory_Alloc(sizeof(GAMEFLOW_LEVEL) * level_count);
    g_GameInfo.current = Memory_Alloc(sizeof(RESUME_INFO) * level_count);

    struct json_array_element_s *jlvl_elem = jlvl_arr->start;
    int level_num = 0;

    g_GameFlow.has_demo = 0;
    g_GameFlow.gym_level_num = -1;
    g_GameFlow.first_level_num = -1;
    g_GameFlow.last_level_num = -1;
    g_GameFlow.title_level_num = -1;
    g_GameFlow.level_count = jlvl_arr->length;

    GAMEFLOW_LEVEL *cur = &g_GameFlow.levels[0];
    while (jlvl_elem) {
        struct json_object_s *jlvl_obj = json_value_as_object(jlvl_elem->value);
        if (!jlvl_obj) {
            LOG_ERROR("'levels' elements must be dictionaries");
            return false;
        }

        const char *tmp_s;
        int32_t tmp_i;
        struct json_array_s *tmp_arr;

        tmp_i = json_object_get_int(jlvl_obj, "music", JSON_INVALID_NUMBER);
        if (tmp_i == JSON_INVALID_NUMBER) {
            LOG_ERROR("level %d: 'music' must be a number", level_num);
            return false;
        }
        cur->music = tmp_i;

        tmp_s = json_object_get_string(jlvl_obj, "file", JSON_INVALID_STRING);
        if (tmp_s == JSON_INVALID_STRING) {
            LOG_ERROR("level %d: 'file' must be a string", level_num);
            return false;
        }
        cur->level_file = Memory_DupStr(tmp_s);

        tmp_s = json_object_get_string(jlvl_obj, "title", JSON_INVALID_STRING);
        if (tmp_s == JSON_INVALID_STRING) {
            LOG_ERROR("level %d: 'title' must be a string", level_num);
            return false;
        }
        cur->level_title = Memory_DupStr(tmp_s);

        tmp_s = json_object_get_string(jlvl_obj, "type", JSON_INVALID_STRING);
        if (tmp_s == JSON_INVALID_STRING) {
            LOG_ERROR("level %d: 'type' must be a string", level_num);
            return false;
        }

        cur->level_type =
            GameFlow_StringToEnumType(tmp_s, m_GameflowLevelTypeEnumMap);

        switch (cur->level_type) {
        case GFL_TITLE:
        case GFL_TITLE_DEMO_PC:
            if (g_GameFlow.title_level_num != -1) {
                LOG_ERROR(
                    "level %d: there can be only one title level", level_num);
                return false;
            }
            g_GameFlow.title_level_num = level_num;
            break;

        case GFL_GYM:
            if (g_GameFlow.gym_level_num != -1) {
                LOG_ERROR(
                    "level %d: there can be only one gym level", level_num);
                return false;
            }
            g_GameFlow.gym_level_num = level_num;
            break;

        case GFL_LEVEL_DEMO_PC:
        case GFL_NORMAL:
            if (g_GameFlow.first_level_num == -1) {
                g_GameFlow.first_level_num = level_num;
            }
            g_GameFlow.last_level_num = level_num;
            break;

        case GFL_BONUS:
        case GFL_CUTSCENE:
        case GFL_CURRENT:
            break;

        default:
            LOG_ERROR("level %d: unknown level type %s", level_num, tmp_s);
            return false;
        }

        tmp_i = json_object_get_bool(jlvl_obj, "demo", JSON_INVALID_BOOL);
        if (tmp_i != JSON_INVALID_BOOL) {
            cur->demo = tmp_i;
            g_GameFlow.has_demo |= tmp_i;
        } else {
            cur->demo = 0;
        }

        {
            double value = json_object_get_double(
                jlvl_obj, "draw_distance_fade", JSON_INVALID_NUMBER);
            if (value != JSON_INVALID_NUMBER) {
                cur->draw_distance_fade.override = true;
                cur->draw_distance_fade.value = value;
            } else {
                cur->draw_distance_fade.override = false;
            }
        }

        {
            double value = json_object_get_double(
                jlvl_obj, "draw_distance_max", JSON_INVALID_NUMBER);
            if (value != JSON_INVALID_NUMBER) {
                cur->draw_distance_max.override = true;
                cur->draw_distance_max.value = value;
            } else {
                cur->draw_distance_max.override = false;
            }
        }

        tmp_arr = json_object_get_array(jlvl_obj, "water_color");
        if (tmp_arr) {
            cur->water_color.override = true;
            cur->water_color.value.r =
                json_array_get_double(tmp_arr, 0, g_GameFlow.water_color.r);
            cur->water_color.value.g =
                json_array_get_double(tmp_arr, 1, g_GameFlow.water_color.g);
            cur->water_color.value.b =
                json_array_get_double(tmp_arr, 2, g_GameFlow.water_color.b);
        } else {
            cur->water_color.override = false;
        }

        cur->unobtainable.pickups =
            json_object_get_int(jlvl_obj, "unobtainable_pickups", 0);

        cur->unobtainable.kills =
            json_object_get_int(jlvl_obj, "unobtainable_kills", 0);

        cur->unobtainable.secrets =
            json_object_get_int(jlvl_obj, "unobtainable_secrets", 0);

        struct json_object_s *jlbl_strings_obj =
            json_object_get_object(jlvl_obj, "strings");
        if (!jlbl_strings_obj) {
            LOG_ERROR("level %d: 'strings' must be a dictionary", level_num);
            return false;
        } else {
            tmp_s = json_object_get_string(
                jlbl_strings_obj, "pickup1", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->pickup1 = Memory_DupStr(tmp_s);
            } else {
                cur->pickup1 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "pickup2", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->pickup2 = Memory_DupStr(tmp_s);
            } else {
                cur->pickup2 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "key1", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->key1 = Memory_DupStr(tmp_s);
            } else {
                cur->key1 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "key2", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->key2 = Memory_DupStr(tmp_s);
            } else {
                cur->key2 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "key3", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->key3 = Memory_DupStr(tmp_s);
            } else {
                cur->key3 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "key4", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->key4 = Memory_DupStr(tmp_s);
            } else {
                cur->key4 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "puzzle1", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->puzzle1 = Memory_DupStr(tmp_s);
            } else {
                cur->puzzle1 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "puzzle2", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->puzzle2 = Memory_DupStr(tmp_s);
            } else {
                cur->puzzle2 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "puzzle3", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->puzzle3 = Memory_DupStr(tmp_s);
            } else {
                cur->puzzle3 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "puzzle4", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->puzzle4 = Memory_DupStr(tmp_s);
            } else {
                cur->puzzle4 = NULL;
            }
        }

        tmp_i = json_object_get_bool(jlvl_obj, "inherit_injections", 1);
        tmp_arr = json_object_get_array(jlvl_obj, "injections");
        if (tmp_arr) {
            cur->injections.length = tmp_arr->length;
            if (tmp_i) {
                cur->injections.length += g_GameFlow.injections.length;
            }
            cur->injections.data_paths =
                Memory_Alloc(sizeof(char *) * cur->injections.length);

            int inj_base_index = 0;
            if (tmp_i) {
                for (int i = 0; i < g_GameFlow.injections.length; i++) {
                    cur->injections.data_paths[i] =
                        Memory_DupStr(g_GameFlow.injections.data_paths[i]);
                }
                inj_base_index = g_GameFlow.injections.length;
            }

            for (size_t i = 0; i < tmp_arr->length; i++) {
                struct json_value_s *value = json_array_get_value(tmp_arr, i);
                struct json_string_s *str = json_value_as_string(value);
                cur->injections.data_paths[inj_base_index + i] =
                    Memory_DupStr(str->string);
            }
        } else if (tmp_i) {
            cur->injections.length = g_GameFlow.injections.length;
            cur->injections.data_paths =
                Memory_Alloc(sizeof(char *) * cur->injections.length);
            for (int i = 0; i < g_GameFlow.injections.length; i++) {
                cur->injections.data_paths[i] =
                    Memory_DupStr(g_GameFlow.injections.data_paths[i]);
            }
        } else {
            cur->injections.length = 0;
        }

        tmp_i = json_object_get_int(jlvl_obj, "lara_type", (int32_t)O_LARA);
        if (tmp_i < 0 || tmp_i >= O_NUMBER_OF) {
            LOG_ERROR(
                "level %d: 'lara_type' must be a valid game object id",
                level_num);
            return false;
        }
        cur->lara_type = (GAME_OBJECT_ID)tmp_i;

        tmp_arr = json_object_get_array(jlvl_obj, "item_drops");
        if (tmp_arr) {
            cur->item_drops.count = (signed)tmp_arr->length;
            cur->item_drops.data = Memory_Alloc(
                sizeof(GAMEFLOW_DROP_ITEM_DATA) * (signed)tmp_arr->length);

            for (int i = 0; i < cur->item_drops.count; i++) {
                GAMEFLOW_DROP_ITEM_DATA *data = &cur->item_drops.data[i];
                struct json_object_s *jlvl_data =
                    json_array_get_object(tmp_arr, i);

                data->enemy_num = json_object_get_int(
                    jlvl_data, "enemy_num", JSON_INVALID_NUMBER);
                if (data->enemy_num == JSON_INVALID_NUMBER) {
                    LOG_ERROR(
                        "level %d, item drop %d: 'enemy_num' must be a number",
                        level_num, i);
                    return false;
                }

                struct json_array_s *object_arr =
                    json_object_get_array(jlvl_data, "object_ids");
                if (!object_arr) {
                    LOG_ERROR(
                        "level %d, item drop %d: 'object_ids' must be an array",
                        level_num, i);
                    return false;
                }

                data->count = (signed)object_arr->length;
                data->object_ids = Memory_Alloc(sizeof(int16_t) * data->count);
                for (int j = 0; j < data->count; j++) {
                    int id = json_array_get_int(object_arr, j, -1);
                    if (id < 0 || id >= O_NUMBER_OF) {
                        LOG_ERROR(
                            "level %d, item drop %d, index %d: 'object_id' "
                            "must be a valid object id",
                            level_num, i, j);
                        return false;
                    }
                    data->object_ids[j] = (int16_t)id;
                }
            }
        } else {
            cur->item_drops.count = 0;
        }

        if (!GameFlow_LoadLevelSequence(jlvl_obj, level_num)) {
            return false;
        }

        jlvl_elem = jlvl_elem->next;
        level_num++;
        cur++;
    }

    if (g_GameFlow.title_level_num == -1) {
        LOG_ERROR("at least one level must be of title type");
        return false;
    }
    if (g_GameFlow.first_level_num == -1 || g_GameFlow.last_level_num == -1) {
        LOG_ERROR("at least one level must be of normal type");
        return false;
    }
    return true;
}

static bool GameFlow_LoadFromFileImpl(const char *file_name)
{
    GameFlow_Shutdown();
    bool result = false;
    struct json_value_s *root = NULL;
    char *script_data = NULL;

    if (!File_Load(file_name, &script_data, NULL)) {
        LOG_ERROR("failed to open script file");
        goto cleanup;
    }

    struct json_parse_result_s parse_result;
    root = json_parse_ex(
        script_data, strlen(script_data), json_parse_flags_allow_json5, NULL,
        NULL, &parse_result);
    if (!root) {
        LOG_ERROR(
            "failed to parse script file: %s in line %d, char %d",
            json_get_error_description(parse_result.error),
            parse_result.error_line_no, parse_result.error_row_no, script_data);
        goto cleanup;
    }

    struct json_object_s *root_obj = json_value_as_object(root);

    result = true;
    result &= GameFlow_LoadScriptMeta(root_obj);
    result &= GameFlow_LoadScriptGameStrings(root_obj);
    result &= GameFlow_LoadScriptLevels(root_obj);

cleanup:
    if (root) {
        json_value_free(root);
        root = NULL;
    }

    Memory_FreePointer(&script_data);
    return result;
}

void GameFlow_Shutdown(void)
{
    Memory_FreePointer(&g_GameFlow.main_menu_background_path);
    Memory_FreePointer(&g_GameFlow.savegame_fmt_legacy);
    Memory_FreePointer(&g_GameFlow.savegame_fmt_bson);
    Memory_FreePointer(&g_GameInfo.current);

    for (int i = 0; i < g_GameFlow.injections.length; i++) {
        Memory_FreePointer(&g_GameFlow.injections.data_paths[i]);
    }
    Memory_FreePointer(&g_GameFlow.injections.data_paths);

    if (g_GameFlow.levels) {
        for (int i = 0; i < g_GameFlow.level_count; i++) {
            Memory_FreePointer(&g_GameFlow.levels[i].level_title);
            Memory_FreePointer(&g_GameFlow.levels[i].level_file);
            Memory_FreePointer(&g_GameFlow.levels[i].key1);
            Memory_FreePointer(&g_GameFlow.levels[i].key2);
            Memory_FreePointer(&g_GameFlow.levels[i].key3);
            Memory_FreePointer(&g_GameFlow.levels[i].key4);
            Memory_FreePointer(&g_GameFlow.levels[i].pickup1);
            Memory_FreePointer(&g_GameFlow.levels[i].pickup2);
            Memory_FreePointer(&g_GameFlow.levels[i].puzzle1);
            Memory_FreePointer(&g_GameFlow.levels[i].puzzle2);
            Memory_FreePointer(&g_GameFlow.levels[i].puzzle3);
            Memory_FreePointer(&g_GameFlow.levels[i].puzzle4);

            for (int j = 0; j < g_GameFlow.levels[i].injections.length; j++) {
                Memory_FreePointer(
                    &g_GameFlow.levels[i].injections.data_paths[j]);
            }
            Memory_FreePointer(&g_GameFlow.levels[i].injections.data_paths);

            if (g_GameFlow.levels[i].item_drops.count) {
                for (int j = 0; j < g_GameFlow.levels[i].item_drops.count;
                     j++) {
                    Memory_FreePointer(
                        &g_GameFlow.levels[i].item_drops.data[j].object_ids);
                }
                Memory_FreePointer(&g_GameFlow.levels[i].item_drops.data);
            }

            GAMEFLOW_SEQUENCE *seq = g_GameFlow.levels[i].sequence;
            if (seq) {
                while (seq->type != GFS_END) {
                    switch (seq->type) {
                    case GFS_LOADING_SCREEN:
                    case GFS_DISPLAY_PICTURE:
                    case GFS_TOTAL_STATS: {
                        GAMEFLOW_DISPLAY_PICTURE_DATA *data = seq->data;
                        Memory_FreePointer(&data->path);
                        Memory_FreePointer(&data);
                        break;
                    }
                    case GFS_PLAY_FMV:
                    case GFS_MESH_SWAP:
                    case GFS_GIVE_ITEM:
                        Memory_FreePointer(&seq->data);
                        break;
                    case GFS_END:
                    case GFS_START_GAME:
                    case GFS_LOOP_GAME:
                    case GFS_STOP_GAME:
                    case GFS_START_CINE:
                    case GFS_LOOP_CINE:
                    case GFS_LEVEL_STATS:
                    case GFS_EXIT_TO_TITLE:
                    case GFS_EXIT_TO_LEVEL:
                    case GFS_EXIT_TO_CINE:
                    case GFS_SET_CAM_X:
                    case GFS_SET_CAM_Y:
                    case GFS_SET_CAM_Z:
                    case GFS_SET_CAM_ANGLE:
                    case GFS_FLIP_MAP:
                    case GFS_REMOVE_GUNS:
                    case GFS_REMOVE_SCIONS:
                    case GFS_PLAY_SYNCED_AUDIO:
                    case GFS_REMOVE_AMMO:
                    case GFS_REMOVE_MEDIPACKS:
                    case GFS_SETUP_BACON_LARA:
                    case GFS_LEGACY:
                        break;
                    }
                    seq++;
                }
            }
            Memory_FreePointer(&g_GameFlow.levels[i].sequence);
        }
        Memory_FreePointer(&g_GameFlow.levels);
    }
}

bool GameFlow_LoadFromFile(const char *file_name)
{
    bool result = GameFlow_LoadFromFileImpl(file_name);

    g_InvItemMedi.string = GS(INV_ITEM_MEDI),
    g_InvItemBigMedi.string = GS(INV_ITEM_BIG_MEDI),

    g_InvItemPuzzle1.string = GS(INV_ITEM_PUZZLE1),
    g_InvItemPuzzle2.string = GS(INV_ITEM_PUZZLE2),
    g_InvItemPuzzle3.string = GS(INV_ITEM_PUZZLE3),
    g_InvItemPuzzle4.string = GS(INV_ITEM_PUZZLE4),

    g_InvItemKey1.string = GS(INV_ITEM_KEY1),
    g_InvItemKey2.string = GS(INV_ITEM_KEY2),
    g_InvItemKey3.string = GS(INV_ITEM_KEY3),
    g_InvItemKey4.string = GS(INV_ITEM_KEY4),

    g_InvItemPickup1.string = GS(INV_ITEM_PICKUP1),
    g_InvItemPickup2.string = GS(INV_ITEM_PICKUP2),
    g_InvItemLeadBar.string = GS(INV_ITEM_LEADBAR),
    g_InvItemScion.string = GS(INV_ITEM_SCION),

    g_InvItemPistols.string = GS(INV_ITEM_PISTOLS),
    g_InvItemShotgun.string = GS(INV_ITEM_SHOTGUN),
    g_InvItemMagnum.string = GS(INV_ITEM_MAGNUM),
    g_InvItemUzi.string = GS(INV_ITEM_UZI),
    g_InvItemGrenade.string = GS(INV_ITEM_GRENADE),

    g_InvItemPistolAmmo.string = GS(INV_ITEM_PISTOL_AMMO),
    g_InvItemShotgunAmmo.string = GS(INV_ITEM_SHOTGUN_AMMO),
    g_InvItemMagnumAmmo.string = GS(INV_ITEM_MAGNUM_AMMO),
    g_InvItemUziAmmo.string = GS(INV_ITEM_UZI_AMMO),

    g_InvItemCompass.string = GS(INV_ITEM_COMPASS),
    g_InvItemGame.string = GS(INV_ITEM_GAME);
    g_InvItemDetails.string = GS(INV_ITEM_DETAILS);
    g_InvItemSound.string = GS(INV_ITEM_SOUND);
    g_InvItemControls.string = GS(INV_ITEM_CONTROLS);
    g_InvItemLarasHome.string = GS(INV_ITEM_LARAS_HOME);

    if (g_GameFlow.force_save_crystals == TB_ON) {
        g_Config.enable_save_crystals = true;
    } else if (g_GameFlow.force_save_crystals == TB_OFF) {
        g_Config.enable_save_crystals = false;
    }

    if (g_GameFlow.force_game_modes == TB_ON) {
        g_Config.enable_game_modes = true;
    } else if (g_GameFlow.force_game_modes == TB_OFF) {
        g_Config.enable_game_modes = false;
    }

    return result;
}

GAMEFLOW_COMMAND
GameFlow_InterpretSequence(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type)
{
    LOG_INFO("level_num=%d level_type=%d", level_num, level_type);

    g_GameInfo.remove_guns = false;
    g_GameInfo.remove_scions = false;
    g_GameInfo.remove_ammo = false;
    g_GameInfo.remove_medipacks = false;

    GAMEFLOW_SEQUENCE *seq = g_GameFlow.levels[level_num].sequence;
    GAMEFLOW_COMMAND command = { .action = GF_EXIT_TO_TITLE };

    while (seq->type != GFS_END) {
        LOG_INFO("seq %d %d", seq->type, seq->data);

        if (!g_Config.enable_cine
            && g_GameFlow.levels[level_num].level_type == GFL_CUTSCENE) {
            bool skip;
            switch (seq->type) {
            case GFS_EXIT_TO_TITLE:
            case GFS_EXIT_TO_LEVEL:
            case GFS_EXIT_TO_CINE:
            case GFS_PLAY_FMV:
            case GFS_LEVEL_STATS:
            case GFS_TOTAL_STATS:
                skip = false;
                break;
            default:
                skip = true;
                break;
            }
            if (skip) {
                seq++;
                continue;
            }
        }

        switch (seq->type) {
        case GFS_START_GAME:
            if (!Game_Start((int32_t)(intptr_t)seq->data, level_type)) {
                g_CurrentLevel = -1;
                return (GAMEFLOW_COMMAND) { .action = GF_EXIT_TO_TITLE };
            }
            break;

        case GFS_LOOP_GAME:
            if (level_type != GFL_SAVED
                && level_num != g_GameFlow.first_level_num) {
                Lara_RevertToPistolsIfNeeded();
            }
            Phase_Set(PHASE_GAME, NULL);
            command = Phase_Run();
            if (command.action != GF_CONTINUE_SEQUENCE) {
                return command;
            }
            break;

        case GFS_STOP_GAME:
            command = Game_Stop();
            if (command.action != GF_CONTINUE_SEQUENCE
                && command.action != GF_LEVEL_COMPLETE) {
                return command;
            }
            if (level_type == GFL_SAVED) {
                if (g_GameFlow.levels[level_num].level_type == GFL_BONUS) {
                    level_type = GFL_BONUS;
                } else {
                    level_type = GFL_NORMAL;
                }
            }
            break;

        case GFS_START_CINE:
            if (level_type != GFL_SAVED) {
                PHASE_CUTSCENE_DATA phase_args = {
                    .level_num = (int32_t)(intptr_t)seq->data,
                };
                Phase_Set(PHASE_CUTSCENE, &phase_args);
            }
            break;

        case GFS_LOOP_CINE:
            if (level_type != GFL_SAVED) {
                command = Phase_Run();
                if (command.action != GF_CONTINUE_SEQUENCE
                    && command.action != GF_LEVEL_COMPLETE) {
                    return command;
                }
            }
            break;

        case GFS_PLAY_FMV:
            if (level_type != GFL_SAVED) {
                FMV_Play((char *)seq->data);
            }
            break;

        case GFS_LEVEL_STATS: {
            PHASE_STATS_DATA phase_args = {
                .level_num = (int32_t)(intptr_t)seq->data,
            };
            Phase_Set(PHASE_STATS, &phase_args);
            command = Phase_Run();
            if (command.action != GF_CONTINUE_SEQUENCE) {
                return command;
            }
            break;
        }

        case GFS_TOTAL_STATS:
            if (g_Config.enable_total_stats && level_type != GFL_SAVED) {
                const GAMEFLOW_DISPLAY_PICTURE_DATA *data = seq->data;
                PHASE_STATS_DATA phase_args = {
                    .level_num = level_num,
                    .background_path = data->path,
                    .total = true,
                    .level_type =
                        level_type == GFL_BONUS ? GFL_BONUS : GFL_NORMAL,
                };
                Phase_Set(PHASE_STATS, &phase_args);
                command = Phase_Run();
                if (command.action != GF_CONTINUE_SEQUENCE) {
                    return command;
                }
            }
            break;

        case GFS_LOADING_SCREEN:
        case GFS_DISPLAY_PICTURE:
            if (seq->type == GFS_LOADING_SCREEN
                && !g_Config.enable_loading_screens) {
                break;
            }

            if (level_type == GFL_SAVED) {
                break;
            }

            if (g_CurrentLevel == -1 && !g_Config.enable_eidos_logo) {
                break;
            }

            GAMEFLOW_DISPLAY_PICTURE_DATA *data = seq->data;
            PHASE_PICTURE_DATA phase_arg = {
                .path = data->path,
                .display_time = data->display_time,
            };
            Phase_Set(PHASE_PICTURE, &phase_arg);
            command = Phase_Run();
            if (command.action != GF_CONTINUE_SEQUENCE) {
                return command;
            }
            break;

        case GFS_EXIT_TO_TITLE:
            return (GAMEFLOW_COMMAND) { .action = GF_EXIT_TO_TITLE };

        case GFS_EXIT_TO_LEVEL: {
            int32_t next_level = (int32_t)(intptr_t)seq->data & ((1 << 6) - 1);
            if (g_GameFlow.levels[next_level].level_type == GFL_BONUS
                && !g_GameInfo.bonus_level_unlock) {
                return (GAMEFLOW_COMMAND) { .action = GF_EXIT_TO_TITLE };
            }
            return (GAMEFLOW_COMMAND) {
                .action = GF_START_GAME,
                .param = next_level,
            };
        }

        case GFS_EXIT_TO_CINE:
            return (GAMEFLOW_COMMAND) {
                .action = GF_START_CINE,
                .param = (int32_t)(intptr_t)seq->data & ((1 << 6) - 1),
            };

        case GFS_SET_CAM_X:
            g_CinePosition.pos.x = (int32_t)(intptr_t)seq->data;
            break;
        case GFS_SET_CAM_Y:
            g_CinePosition.pos.y = (int32_t)(intptr_t)seq->data;
            break;
        case GFS_SET_CAM_Z:
            g_CinePosition.pos.z = (int32_t)(intptr_t)seq->data;
            break;
        case GFS_SET_CAM_ANGLE:
            g_Camera.target_angle = (int32_t)(intptr_t)seq->data;
            break;
        case GFS_FLIP_MAP:
            Room_FlipMap();
            break;
        case GFS_PLAY_SYNCED_AUDIO:
            Music_Play((int32_t)(intptr_t)seq->data);
            break;

        case GFS_GIVE_ITEM:
            if (level_type != GFL_SAVED) {
                const GAMEFLOW_GIVE_ITEM_DATA *give_item_data =
                    (const GAMEFLOW_GIVE_ITEM_DATA *)seq->data;
                Inv_AddItemNTimes(
                    give_item_data->object_id, give_item_data->quantity);
            }
            break;

        case GFS_REMOVE_GUNS:
            if (level_type != GFL_SAVED
                && !(g_GameInfo.bonus_flag & GBF_NGPLUS)) {
                g_GameInfo.remove_guns = true;
            }
            break;

        case GFS_REMOVE_SCIONS:
            if (level_type != GFL_SAVED) {
                g_GameInfo.remove_scions = true;
            }
            break;

        case GFS_REMOVE_AMMO:
            if (level_type != GFL_SAVED
                && !(g_GameInfo.bonus_flag & GBF_NGPLUS)) {
                g_GameInfo.remove_ammo = true;
            }
            break;

        case GFS_REMOVE_MEDIPACKS:
            if (level_type != GFL_SAVED) {
                g_GameInfo.remove_medipacks = true;
            }
            break;

        case GFS_MESH_SWAP: {
            GAMEFLOW_MESH_SWAP_DATA *swap_data = seq->data;
            int16_t *temp;

            temp = g_Meshes
                [g_Objects[swap_data->object1_id].mesh_index
                 + swap_data->mesh_num];
            g_Meshes
                [g_Objects[swap_data->object1_id].mesh_index
                 + swap_data->mesh_num] = g_Meshes
                    [g_Objects[swap_data->object2_id].mesh_index
                     + swap_data->mesh_num];
            g_Meshes
                [g_Objects[swap_data->object2_id].mesh_index
                 + swap_data->mesh_num] = temp;
            break;
        }

        case GFS_SETUP_BACON_LARA: {
            int32_t anchor_room = (int32_t)(intptr_t)seq->data;
            if (!BaconLara_InitialiseAnchor(anchor_room)) {
                LOG_ERROR(
                    "Could not anchor Bacon Lara to room %d", anchor_room);
                return (GAMEFLOW_COMMAND) { .action = GF_EXIT_TO_TITLE };
            }
            break;
        }

        case GFS_LEGACY:
            break;

        case GFS_END:
            return command;
        }

        seq++;
    }

    return command;
}

GAMEFLOW_COMMAND
GameFlow_StorySoFar(int32_t level_num, int32_t savegame_level)
{
    LOG_INFO("%d", level_num);

    GAMEFLOW_SEQUENCE *seq = g_GameFlow.levels[level_num].sequence;
    GAMEFLOW_COMMAND command = { .action = GF_EXIT_TO_TITLE };

    while (seq->type != GFS_END) {
        LOG_INFO("seq %d %d", seq->type, seq->data);

        switch (seq->type) {
        case GFS_LOOP_GAME:
        case GFS_STOP_GAME:
        case GFS_LEVEL_STATS:
        case GFS_TOTAL_STATS:
        case GFS_LOADING_SCREEN:
        case GFS_DISPLAY_PICTURE:
        case GFS_GIVE_ITEM:
        case GFS_REMOVE_GUNS:
        case GFS_REMOVE_SCIONS:
        case GFS_REMOVE_AMMO:
        case GFS_REMOVE_MEDIPACKS:
        case GFS_SETUP_BACON_LARA:
        case GFS_LEGACY:
            break;

        case GFS_START_GAME:
            if (level_num == savegame_level) {
                return (GAMEFLOW_COMMAND) { .action = GF_EXIT_TO_TITLE };
            }
            break;

        case GFS_START_CINE: {
            PHASE_CUTSCENE_DATA phase_args = {
                .level_num = (int32_t)(intptr_t)seq->data,
            };
            Phase_Set(PHASE_CUTSCENE, &phase_args);
            break;
        }

        case GFS_LOOP_CINE:
            command = Phase_Run();
            if (command.action != GF_CONTINUE_SEQUENCE) {
                return command;
            }
            break;

        case GFS_PLAY_FMV:
            FMV_Play((char *)seq->data);
            break;

        case GFS_EXIT_TO_TITLE:
            Music_Stop();
            return (GAMEFLOW_COMMAND) { .action = GF_EXIT_TO_TITLE };

        case GFS_EXIT_TO_LEVEL:
            Music_Stop();
            return (GAMEFLOW_COMMAND) {
                .action = GF_START_GAME,
                .param = (int32_t)(intptr_t)seq->data & ((1 << 6) - 1),
            };

        case GFS_EXIT_TO_CINE:
            Music_Stop();
            return (GAMEFLOW_COMMAND) {
                .action = GF_START_CINE,
                .param = (int32_t)(intptr_t)seq->data & ((1 << 6) - 1),
            };

        case GFS_SET_CAM_X:
            g_CinePosition.pos.x = (int32_t)(intptr_t)seq->data;
            break;
        case GFS_SET_CAM_Y:
            g_CinePosition.pos.y = (int32_t)(intptr_t)seq->data;
            break;
        case GFS_SET_CAM_Z:
            g_CinePosition.pos.z = (int32_t)(intptr_t)seq->data;
            break;
        case GFS_SET_CAM_ANGLE:
            g_Camera.target_angle = (int32_t)(intptr_t)seq->data;
            break;
        case GFS_FLIP_MAP:
            Room_FlipMap();
            break;
        case GFS_PLAY_SYNCED_AUDIO:
            Music_Play((int32_t)(intptr_t)seq->data);
            break;

        case GFS_MESH_SWAP: {
            GAMEFLOW_MESH_SWAP_DATA *swap_data = seq->data;
            int16_t *temp = g_Meshes
                [g_Objects[swap_data->object1_id].mesh_index
                 + swap_data->mesh_num];
            g_Meshes
                [g_Objects[swap_data->object1_id].mesh_index
                 + swap_data->mesh_num] = g_Meshes
                    [g_Objects[swap_data->object2_id].mesh_index
                     + swap_data->mesh_num];
            g_Meshes
                [g_Objects[swap_data->object2_id].mesh_index
                 + swap_data->mesh_num] = temp;
            break;
        }

        case GFS_END:
            return command;
        }

        seq++;
    }

    return command;
}
