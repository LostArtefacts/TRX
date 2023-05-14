#include "game/gameflow.h"

#include "config.h"
#include "filesystem.h"
#include "game/clock.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/inventory/inventory_vars.h"
#include "game/lara.h"
#include "game/music.h"
#include "game/objects/creatures/bacon_lara.h"
#include "game/output.h"
#include "game/room.h"
#include "game/shell.h"
#include "game/stats.h"
#include "global/const.h"
#include "global/vars.h"
#include "json/json_base.h"
#include "json/json_parse.h"
#include "log.h"
#include "memory.h"

#include <limits.h>
#include <string.h>

typedef struct ENUM_TO_STRING {
    const char *str;
    const int32_t val;
} ENUM_TO_STRING;

typedef struct GAMEFLOW_DISPLAY_PICTURE_DATA {
    char *path;
    int32_t display_time;
} GAMEFLOW_DISPLAY_PICTURE_DATA;

typedef struct GAMEFLOW_MESH_SWAP_DATA {
    GAME_OBJECT_ID object1_num;
    GAME_OBJECT_ID object2_num;
    int32_t mesh_num;
} GAMEFLOW_MESH_SWAP_DATA;

typedef struct GAMEFLOW_GIVE_ITEM_DATA {
    GAME_OBJECT_ID object_num;
    int quantity;
} GAMEFLOW_GIVE_ITEM_DATA;

static GAME_STRING_ID GameFlow_StringToGameStringID(const char *str)
{
    static const ENUM_TO_STRING map[] = {
        { "HEADING_INVENTORY", GS_HEADING_INVENTORY },
        { "HEADING_GAME_OVER", GS_HEADING_GAME_OVER },
        { "HEADING_OPTION", GS_HEADING_OPTION },
        { "HEADING_ITEMS", GS_HEADING_ITEMS },
        { "PASSPORT_SELECT_LEVEL", GS_PASSPORT_SELECT_LEVEL },
        { "PASSPORT_RESTART_LEVEL", GS_PASSPORT_RESTART_LEVEL },
        { "PASSPORT_STORY_SO_FAR", GS_PASSPORT_STORY_SO_FAR },
        { "PASSPORT_LEGACY_SELECT_LEVEL_1", GS_PASSPORT_LEGACY_SELECT_LEVEL_1 },
        { "PASSPORT_LEGACY_SELECT_LEVEL_2", GS_PASSPORT_LEGACY_SELECT_LEVEL_2 },
        { "PASSPORT_SELECT_MODE", GS_PASSPORT_SELECT_MODE },
        { "PASSPORT_MODE_NEW_GAME", GS_PASSPORT_MODE_NEW_GAME },
        { "PASSPORT_MODE_NEW_GAME_PLUS", GS_PASSPORT_MODE_NEW_GAME_PLUS },
        { "PASSPORT_MODE_NEW_GAME_JP", GS_PASSPORT_MODE_NEW_GAME_JP },
        { "PASSPORT_MODE_NEW_GAME_JP_PLUS", GS_PASSPORT_MODE_NEW_GAME_JP_PLUS },
        { "PASSPORT_NEW_GAME", GS_PASSPORT_NEW_GAME },
        { "PASSPORT_LOAD_GAME", GS_PASSPORT_LOAD_GAME },
        { "PASSPORT_SAVE_GAME", GS_PASSPORT_SAVE_GAME },
        { "PASSPORT_EXIT_GAME", GS_PASSPORT_EXIT_GAME },
        { "PASSPORT_EXIT_TO_TITLE", GS_PASSPORT_EXIT_TO_TITLE },
        { "DETAIL_SELECT_DETAIL", GS_DETAIL_SELECT_DETAIL },
        { "DETAIL_LEVEL_HIGH", GS_DETAIL_LEVEL_HIGH },
        { "DETAIL_LEVEL_MEDIUM", GS_DETAIL_LEVEL_MEDIUM },
        { "DETAIL_LEVEL_LOW", GS_DETAIL_LEVEL_LOW },
        { "DETAIL_PERSPECTIVE", GS_DETAIL_PERSPECTIVE },
        { "DETAIL_BILINEAR", GS_DETAIL_BILINEAR },
        { "DETAIL_VSYNC", GS_DETAIL_VSYNC },
        { "DETAIL_BRIGHTNESS", GS_DETAIL_BRIGHTNESS },
        { "DETAIL_UI_TEXT_SCALE", GS_DETAIL_UI_TEXT_SCALE },
        { "DETAIL_UI_BAR_SCALE", GS_DETAIL_UI_BAR_SCALE },
        { "DETAIL_RENDER_MODE", GS_DETAIL_RENDER_MODE },
        { "DETAIL_RENDER_MODE_LEGACY", GS_DETAIL_RENDER_MODE_LEGACY },
        { "DETAIL_RENDER_MODE_FBO", GS_DETAIL_RENDER_MODE_FBO },
        { "DETAIL_RESOLUTION", GS_DETAIL_RESOLUTION },
        { "DETAIL_STRING_FMT", GS_DETAIL_STRING_FMT },
        { "DETAIL_FLOAT_FMT", GS_DETAIL_FLOAT_FMT },
        { "DETAIL_RESOLUTION_FMT", GS_DETAIL_RESOLUTION_FMT },
        { "SOUND_SET_VOLUMES", GS_SOUND_SET_VOLUMES },
        { "CONTROL_CUSTOMIZE", GS_CONTROL_CUSTOMIZE },
        { "CONTROL_KEYBOARD", GS_CONTROL_KEYBOARD },
        { "CONTROL_CONTROLLER", GS_CONTROL_CONTROLLER },
        { "CONTROL_DEFAULT_KEYS", GS_CONTROL_DEFAULT_KEYS },
        { "CONTROL_CUSTOM_1", GS_CONTROL_CUSTOM_1 },
        { "CONTROL_CUSTOM_2", GS_CONTROL_CUSTOM_2 },
        { "CONTROL_CUSTOM_3", GS_CONTROL_CUSTOM_3 },
        { "CONTROL_RESET_DEFAULTS_KEY", GS_CONTROL_RESET_DEFAULTS_KEY },
        { "CONTROL_UNBIND_KEY", GS_CONTROL_UNBIND_KEY },
        { "CONTROL_RESET_DEFAULTS_BTN", GS_CONTROL_RESET_DEFAULTS_BTN },
        { "CONTROL_UNBIND_BTN", GS_CONTROL_UNBIND_BTN },
        { "KEYMAP_RUN", GS_KEYMAP_RUN },
        { "KEYMAP_BACK", GS_KEYMAP_BACK },
        { "KEYMAP_LEFT", GS_KEYMAP_LEFT },
        { "KEYMAP_RIGHT", GS_KEYMAP_RIGHT },
        { "KEYMAP_STEP_LEFT", GS_KEYMAP_STEP_LEFT },
        { "KEYMAP_STEP_RIGHT", GS_KEYMAP_STEP_RIGHT },
        { "KEYMAP_WALK", GS_KEYMAP_WALK },
        { "KEYMAP_JUMP", GS_KEYMAP_JUMP },
        { "KEYMAP_ACTION", GS_KEYMAP_ACTION },
        { "KEYMAP_DRAW_WEAPON", GS_KEYMAP_DRAW_WEAPON },
        { "KEYMAP_LOOK", GS_KEYMAP_LOOK },
        { "KEYMAP_ROLL", GS_KEYMAP_ROLL },
        { "KEYMAP_INVENTORY", GS_KEYMAP_INVENTORY },
        { "KEYMAP_FLY_CHEAT", GS_KEYMAP_FLY_CHEAT },
        { "KEYMAP_ITEM_CHEAT", GS_KEYMAP_ITEM_CHEAT },
        { "KEYMAP_LEVEL_SKIP_CHEAT", GS_KEYMAP_LEVEL_SKIP_CHEAT },
        { "KEYMAP_TURBO_CHEAT", GS_KEYMAP_TURBO_CHEAT },
        { "KEYMAP_PAUSE", GS_KEYMAP_PAUSE },
        { "KEYMAP_CAMERA_UP", GS_KEYMAP_CAMERA_UP },
        { "KEYMAP_CAMERA_DOWN", GS_KEYMAP_CAMERA_DOWN },
        { "KEYMAP_CAMERA_LEFT", GS_KEYMAP_CAMERA_LEFT },
        { "KEYMAP_CAMERA_RIGHT", GS_KEYMAP_CAMERA_RIGHT },
        { "KEYMAP_CAMERA_RESET", GS_KEYMAP_CAMERA_RESET },
        { "KEYMAP_EQUIP_PISTOLS", GS_KEYMAP_EQUIP_PISTOLS },
        { "KEYMAP_EQUIP_SHOTGUN", GS_KEYMAP_EQUIP_SHOTGUN },
        { "KEYMAP_EQUIP_MAGNUMS", GS_KEYMAP_EQUIP_MAGNUMS },
        { "KEYMAP_EQUIP_UZIS", GS_KEYMAP_EQUIP_UZIS },
        { "KEYMAP_USE_SMALL_MEDI", GS_KEYMAP_USE_SMALL_MEDI },
        { "KEYMAP_USE_BIG_MEDI", GS_KEYMAP_USE_BIG_MEDI },
        { "KEYMAP_SAVE", GS_KEYMAP_SAVE },
        { "KEYMAP_LOAD", GS_KEYMAP_LOAD },
        { "KEYMAP_FPS", GS_KEYMAP_FPS },
        { "KEYMAP_BILINEAR", GS_KEYMAP_BILINEAR },
        { "STATS_TIME_TAKEN_FMT", GS_STATS_TIME_TAKEN_FMT },
        { "STATS_SECRETS_FMT", GS_STATS_SECRETS_FMT },
        { "STATS_DEATHS_FMT", GS_STATS_DEATHS_FMT },
        { "STATS_PICKUPS_DETAIL_FMT", GS_STATS_PICKUPS_DETAIL_FMT },
        { "STATS_PICKUPS_BASIC_FMT", GS_STATS_PICKUPS_BASIC_FMT },
        { "STATS_KILLS_DETAIL_FMT", GS_STATS_KILLS_DETAIL_FMT },
        { "STATS_KILLS_BASIC_FMT", GS_STATS_KILLS_BASIC_FMT },
        { "STATS_FINAL_STATISTICS", GS_STATS_FINAL_STATISTICS },
        { "PAUSE_PAUSED", GS_PAUSE_PAUSED },
        { "PAUSE_EXIT_TO_TITLE", GS_PAUSE_EXIT_TO_TITLE },
        { "PAUSE_CONTINUE", GS_PAUSE_CONTINUE },
        { "PAUSE_QUIT", GS_PAUSE_QUIT },
        { "PAUSE_ARE_YOU_SURE", GS_PAUSE_ARE_YOU_SURE },
        { "PAUSE_YES", GS_PAUSE_YES },
        { "PAUSE_NO", GS_PAUSE_NO },
        { "MISC_ON", GS_MISC_ON },
        { "MISC_OFF", GS_MISC_OFF },
        { "MISC_EMPTY_SLOT_FMT", GS_MISC_EMPTY_SLOT_FMT },
        { "MISC_DEMO_MODE", GS_MISC_DEMO_MODE },
        { "INV_ITEM_MEDI", GS_INV_ITEM_MEDI },
        { "INV_ITEM_BIG_MEDI", GS_INV_ITEM_BIG_MEDI },
        { "INV_ITEM_PUZZLE1", GS_INV_ITEM_PUZZLE1 },
        { "INV_ITEM_PUZZLE2", GS_INV_ITEM_PUZZLE2 },
        { "INV_ITEM_PUZZLE3", GS_INV_ITEM_PUZZLE3 },
        { "INV_ITEM_PUZZLE4", GS_INV_ITEM_PUZZLE4 },
        { "INV_ITEM_KEY1", GS_INV_ITEM_KEY1 },
        { "INV_ITEM_KEY2", GS_INV_ITEM_KEY2 },
        { "INV_ITEM_KEY3", GS_INV_ITEM_KEY3 },
        { "INV_ITEM_KEY4", GS_INV_ITEM_KEY4 },
        { "INV_ITEM_PICKUP1", GS_INV_ITEM_PICKUP1 },
        { "INV_ITEM_PICKUP2", GS_INV_ITEM_PICKUP2 },
        { "INV_ITEM_LEADBAR", GS_INV_ITEM_LEADBAR },
        { "INV_ITEM_SCION", GS_INV_ITEM_SCION },
        { "INV_ITEM_PISTOLS", GS_INV_ITEM_PISTOLS },
        { "INV_ITEM_SHOTGUN", GS_INV_ITEM_SHOTGUN },
        { "INV_ITEM_MAGNUM", GS_INV_ITEM_MAGNUM },
        { "INV_ITEM_UZI", GS_INV_ITEM_UZI },
        { "INV_ITEM_GRENADE", GS_INV_ITEM_GRENADE },
        { "INV_ITEM_PISTOL_AMMO", GS_INV_ITEM_PISTOL_AMMO },
        { "INV_ITEM_SHOTGUN_AMMO", GS_INV_ITEM_SHOTGUN_AMMO },
        { "INV_ITEM_MAGNUM_AMMO", GS_INV_ITEM_MAGNUM_AMMO },
        { "INV_ITEM_UZI_AMMO", GS_INV_ITEM_UZI_AMMO },
        { "INV_ITEM_COMPASS", GS_INV_ITEM_COMPASS },
        { "INV_ITEM_GAME", GS_INV_ITEM_GAME },
        { "INV_ITEM_DETAILS", GS_INV_ITEM_DETAILS },
        { "INV_ITEM_SOUND", GS_INV_ITEM_SOUND },
        { "INV_ITEM_CONTROLS", GS_INV_ITEM_CONTROLS },
        { "INV_ITEM_GAMMA", GS_INV_ITEM_GAMMA },
        { "INV_ITEM_LARAS_HOME", GS_INV_ITEM_LARAS_HOME },
        { NULL, 0 },
    };

    const ENUM_TO_STRING *current = &map[0];
    while (current->str) {
        if (!strcmp(str, current->str)) {
            return current->val;
        }
        current++;
    }
    return -1;
}

GAMEFLOW g_GameFlow = { 0 };

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
    g_GameFlow.demo_delay = tmp_d * FRAMES_PER_SECOND;

    g_GameFlow.force_disable_game_modes =
        json_object_get_bool(obj, "force_disable_game_modes", false);

    g_GameFlow.force_enable_save_crystals =
        json_object_get_bool(obj, "force_enable_save_crystals", false);

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
        GAME_STRING_ID key =
            GameFlow_StringToGameStringID(strings_elem->name->string);
        struct json_string_s *value = json_value_as_string(strings_elem->value);
        if (!value || !value->string || key < 0 || key >= GS_NUMBER_OF) {
            LOG_ERROR("invalid string key %s", strings_elem->name->string);
        } else {
            g_GameFlow.strings[key] = Memory_DupStr(value->string);
        }
        strings_elem = strings_elem->next;
    }

    for (const GAMEFLOW_DEFAULT_STRING *def = g_GameFlowDefaultStrings;
         def->string; def++) {
        if (!g_GameFlow.strings[def->key]) {
            g_GameFlow.strings[def->key] = Memory_DupStr(def->string);
        }
    }

    return true;
}

static bool GameFlow_IsLegacySequence(const char *type_str)
{
    return !strcmp(type_str, "fix_pyramid_secret");
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

        if (!strcmp(type_str, "start_game")) {
            seq->type = GFS_START_GAME;
            seq->data = (void *)level_num;

        } else if (!strcmp(type_str, "stop_game")) {
            seq->type = GFS_STOP_GAME;
            seq->data = (void *)level_num;

        } else if (!strcmp(type_str, "loop_game")) {
            seq->type = GFS_LOOP_GAME;
            seq->data = (void *)level_num;

        } else if (!strcmp(type_str, "start_cine")) {
            seq->type = GFS_START_CINE;
            seq->data = (void *)level_num;

        } else if (!strcmp(type_str, "stop_cine")) {
            seq->type = GFS_STOP_CINE;
            seq->data = (void *)level_num;

        } else if (!strcmp(type_str, "loop_cine")) {
            seq->type = GFS_LOOP_CINE;
            seq->data = (void *)level_num;

        } else if (!strcmp(type_str, "play_fmv")) {
            seq->type = GFS_PLAY_FMV;
            const char *tmp_s = json_object_get_string(
                jseq_obj, "fmv_path", JSON_INVALID_STRING);
            if (tmp_s == JSON_INVALID_STRING) {
                LOG_ERROR(
                    "level %d, sequence %s: 'fmv_path' must be a string",
                    level_num, type_str);
                return false;
            }
            seq->data = Memory_DupStr(tmp_s);

        } else if (!strcmp(type_str, "display_picture")) {
            seq->type = GFS_DISPLAY_PICTURE;

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
            data->display_time = tmp_d * TICKS_PER_SECOND;
            if (!data->display_time) {
                data->display_time = INT_MAX;
            }

            seq->data = data;

        } else if (!strcmp(type_str, "level_stats")) {
            seq->type = GFS_LEVEL_STATS;
            int tmp =
                json_object_get_int(jseq_obj, "level_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'level_id' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "total_stats")) {
            seq->type = GFS_TOTAL_STATS;

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

        } else if (!strcmp(type_str, "exit_to_title")) {
            seq->type = GFS_EXIT_TO_TITLE;

        } else if (!strcmp(type_str, "exit_to_level")) {
            seq->type = GFS_EXIT_TO_LEVEL;
            int tmp =
                json_object_get_int(jseq_obj, "level_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'level_id' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "exit_to_cine")) {
            seq->type = GFS_EXIT_TO_CINE;
            int tmp =
                json_object_get_int(jseq_obj, "level_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'level_id' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "set_cam_x")) {
            seq->type = GFS_SET_CAM_X;
            int tmp =
                json_object_get_int(jseq_obj, "value", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "set_cam_y")) {
            seq->type = GFS_SET_CAM_Y;
            int tmp =
                json_object_get_int(jseq_obj, "value", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "set_cam_z")) {
            seq->type = GFS_SET_CAM_Z;
            int tmp =
                json_object_get_int(jseq_obj, "value", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "set_cam_angle")) {
            seq->type = GFS_SET_CAM_ANGLE;
            int tmp =
                json_object_get_int(jseq_obj, "value", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "flip_map")) {
            seq->type = GFS_FLIP_MAP;

        } else if (!strcmp(type_str, "remove_guns")) {
            seq->type = GFS_REMOVE_GUNS;

        } else if (!strcmp(type_str, "remove_scions")) {
            seq->type = GFS_REMOVE_SCIONS;

        } else if (!strcmp(type_str, "remove_ammo")) {
            seq->type = GFS_REMOVE_AMMO;

        } else if (!strcmp(type_str, "remove_medipacks")) {
            seq->type = GFS_REMOVE_MEDIPACKS;

        } else if (!strcmp(type_str, "give_item")) {
            seq->type = GFS_GIVE_ITEM;

            GAMEFLOW_GIVE_ITEM_DATA *give_item_data =
                Memory_Alloc(sizeof(GAMEFLOW_GIVE_ITEM_DATA));

            give_item_data->object_num =
                json_object_get_int(jseq_obj, "object_id", JSON_INVALID_NUMBER);
            if (give_item_data->object_num == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'object_id' must be a number",
                    level_num, type_str);
                return false;
            }

            give_item_data->quantity =
                json_object_get_int(jseq_obj, "quantity", 1);

            seq->data = give_item_data;

        } else if (!strcmp(type_str, "play_synced_audio")) {
            seq->type = GFS_PLAY_SYNCED_AUDIO;
            int tmp =
                json_object_get_int(jseq_obj, "audio_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'audio_id' must be a number",
                    level_num, type_str);
                return false;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "mesh_swap")) {
            seq->type = GFS_MESH_SWAP;

            GAMEFLOW_MESH_SWAP_DATA *swap_data =
                Memory_Alloc(sizeof(GAMEFLOW_MESH_SWAP_DATA));

            swap_data->object1_num = json_object_get_int(
                jseq_obj, "object1_id", JSON_INVALID_NUMBER);
            if (swap_data->object1_num == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'object1_id' must be a number",
                    level_num, type_str);
                return false;
            }

            swap_data->object2_num = json_object_get_int(
                jseq_obj, "object2_id", JSON_INVALID_NUMBER);
            if (swap_data->object2_num == JSON_INVALID_NUMBER) {
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

        } else if (!strcmp(type_str, "setup_bacon_lara")) {
            seq->type = GFS_SETUP_BACON_LARA;
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
            seq->data = (void *)tmp;

        } else if (GameFlow_IsLegacySequence(type_str)) {
            seq->type = GFS_LEGACY;
            LOG_WARNING(
                "level %d, sequence %s: legacy type ignored", level_num,
                type_str);

        } else {
            LOG_ERROR("unknown sequence type %s", type_str);
            return false;
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
        if (!strcmp(tmp_s, "title")) {
            cur->level_type = GFL_TITLE;
            if (g_GameFlow.title_level_num != -1) {
                LOG_ERROR(
                    "level %d: there can be only one title level", level_num);
                return false;
            }
            g_GameFlow.title_level_num = level_num;
        } else if (!strcmp(tmp_s, "gym")) {
            cur->level_type = GFL_GYM;
            if (g_GameFlow.gym_level_num != -1) {
                LOG_ERROR(
                    "level %d: there can be only one gym level", level_num);
                return false;
            }
            g_GameFlow.gym_level_num = level_num;
        } else if (!strcmp(tmp_s, "normal")) {
            cur->level_type = GFL_NORMAL;
            if (g_GameFlow.first_level_num == -1) {
                g_GameFlow.first_level_num = level_num;
            }
            g_GameFlow.last_level_num = level_num;
        } else if (!strcmp(tmp_s, "cutscene")) {
            cur->level_type = GFL_CUTSCENE;
        } else if (!strcmp(tmp_s, "current")) {
            cur->level_type = GFL_CURRENT;
        } else {
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

    for (int i = 0; i < GS_NUMBER_OF; i++) {
        Memory_FreePointer(&g_GameFlow.strings[i]);
    }

    for (int i = 0; i < g_GameFlow.injections.length; i++) {
        Memory_FreePointer(&g_GameFlow.injections.data_paths[i]);
    }

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

            GAMEFLOW_SEQUENCE *seq = g_GameFlow.levels[i].sequence;
            if (seq) {
                while (seq->type != GFS_END) {
                    switch (seq->type) {
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
                    case GFS_STOP_CINE:
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

    g_InvItemMedi.string = g_GameFlow.strings[GS_INV_ITEM_MEDI],
    g_InvItemBigMedi.string = g_GameFlow.strings[GS_INV_ITEM_BIG_MEDI],

    g_InvItemPuzzle1.string = g_GameFlow.strings[GS_INV_ITEM_PUZZLE1],
    g_InvItemPuzzle2.string = g_GameFlow.strings[GS_INV_ITEM_PUZZLE2],
    g_InvItemPuzzle3.string = g_GameFlow.strings[GS_INV_ITEM_PUZZLE3],
    g_InvItemPuzzle4.string = g_GameFlow.strings[GS_INV_ITEM_PUZZLE4],

    g_InvItemKey1.string = g_GameFlow.strings[GS_INV_ITEM_KEY1],
    g_InvItemKey2.string = g_GameFlow.strings[GS_INV_ITEM_KEY2],
    g_InvItemKey3.string = g_GameFlow.strings[GS_INV_ITEM_KEY3],
    g_InvItemKey4.string = g_GameFlow.strings[GS_INV_ITEM_KEY4],

    g_InvItemPickup1.string = g_GameFlow.strings[GS_INV_ITEM_PICKUP1],
    g_InvItemPickup2.string = g_GameFlow.strings[GS_INV_ITEM_PICKUP2],
    g_InvItemLeadBar.string = g_GameFlow.strings[GS_INV_ITEM_LEADBAR],
    g_InvItemScion.string = g_GameFlow.strings[GS_INV_ITEM_SCION],

    g_InvItemPistols.string = g_GameFlow.strings[GS_INV_ITEM_PISTOLS],
    g_InvItemShotgun.string = g_GameFlow.strings[GS_INV_ITEM_SHOTGUN],
    g_InvItemMagnum.string = g_GameFlow.strings[GS_INV_ITEM_MAGNUM],
    g_InvItemUzi.string = g_GameFlow.strings[GS_INV_ITEM_UZI],
    g_InvItemGrenade.string = g_GameFlow.strings[GS_INV_ITEM_GRENADE],

    g_InvItemPistolAmmo.string = g_GameFlow.strings[GS_INV_ITEM_PISTOL_AMMO],
    g_InvItemShotgunAmmo.string = g_GameFlow.strings[GS_INV_ITEM_SHOTGUN_AMMO],
    g_InvItemMagnumAmmo.string = g_GameFlow.strings[GS_INV_ITEM_MAGNUM_AMMO],
    g_InvItemUziAmmo.string = g_GameFlow.strings[GS_INV_ITEM_UZI_AMMO],

    g_InvItemCompass.string = g_GameFlow.strings[GS_INV_ITEM_COMPASS],
    g_InvItemGame.string = g_GameFlow.strings[GS_INV_ITEM_GAME];
    g_InvItemDetails.string = g_GameFlow.strings[GS_INV_ITEM_DETAILS];
    g_InvItemSound.string = g_GameFlow.strings[GS_INV_ITEM_SOUND];
    g_InvItemControls.string = g_GameFlow.strings[GS_INV_ITEM_CONTROLS];
    g_InvItemLarasHome.string = g_GameFlow.strings[GS_INV_ITEM_LARAS_HOME];

    if (g_GameFlow.force_enable_save_crystals) {
        g_Config.enable_save_crystals = true;
    }

    if (g_GameFlow.force_disable_game_modes) {
        g_Config.enable_game_modes = false;
    }

    return result;
}

GAMEFLOW_OPTION
GameFlow_InterpretSequence(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type)
{
    LOG_INFO("%d", level_num);

    g_GameInfo.remove_guns = false;
    g_GameInfo.remove_scions = false;
    g_GameInfo.remove_ammo = false;
    g_GameInfo.remove_medipacks = false;

    GAMEFLOW_SEQUENCE *seq = g_GameFlow.levels[level_num].sequence;
    GAMEFLOW_OPTION ret = GF_EXIT_TO_TITLE;
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
            if (!Game_Start((int32_t)seq->data, level_type)) {
                g_CurrentLevel = 0;
                return GF_EXIT_TO_TITLE;
            }
            break;

        case GFS_LOOP_GAME:
            ret = Game_Loop(level_type);
            LOG_DEBUG("Game_Loop() exited with %d", ret);
            if (ret != GF_NOP) {
                return ret;
            }
            break;

        case GFS_STOP_GAME:
            ret = Game_Stop();
            if (ret != GF_NOP
                && ((ret & ~((1 << 6) - 1)) != GF_LEVEL_COMPLETE)) {
                return ret;
            }
            if (level_type == GFL_SAVED) {
                level_type = GFL_NORMAL;
            }
            break;

        case GFS_START_CINE:
            if (level_type != GFL_SAVED) {
                ret = Game_Cutscene_Start((int32_t)seq->data);
            }
            break;

        case GFS_LOOP_CINE:
            if (level_type != GFL_SAVED) {
                ret = Game_Cutscene_Loop();
            }
            break;

        case GFS_STOP_CINE:
            if (level_type != GFL_SAVED) {
                ret = Game_Cutscene_Stop((int32_t)seq->data);
            }
            break;

        case GFS_PLAY_FMV:
            if (level_type != GFL_SAVED) {
                FMV_Play((char *)seq->data);
            }
            break;

        case GFS_LEVEL_STATS:
            Stats_Show((int32_t)seq->data);
            break;

        case GFS_TOTAL_STATS:
            if (g_Config.enable_total_stats && level_type != GFL_SAVED) {
                GAMEFLOW_DISPLAY_PICTURE_DATA *data = seq->data;
                Stats_ShowTotal(data->path);
            }
            break;

        case GFS_DISPLAY_PICTURE:
            if (level_type != GFL_SAVED) {
                GAMEFLOW_DISPLAY_PICTURE_DATA *data = seq->data;
                Output_LoadBackdropImage(data->path);
                Clock_SyncTicks(1);

                Output_FadeResetToBlack();
                Output_FadeToTransparent(true);
                while (Output_FadeIsAnimating()) {
                    Output_DrawBackdropImage();
                    Output_DumpScreen();
                    Input_Update();
                    if (g_InputDB.any) {
                        break;
                    }
                }

                if (!g_InputDB.any) {
                    Output_DrawBackdropImage();
                    Output_DumpScreen();
                    Shell_Wait(data->display_time);
                }

                // fade out
                Output_FadeToBlack(true);
                while (Output_FadeIsAnimating()) {
                    Output_DrawBackdropImage();
                    Output_DumpScreen();
                    Input_Update();
                    if (g_InputDB.any) {
                        break;
                    }
                }

                // draw black frame
                Output_DrawBlack();
                Output_DumpScreen();

                Output_FadeReset();
            }
            break;

        case GFS_EXIT_TO_TITLE:
            return GF_EXIT_TO_TITLE;

        case GFS_EXIT_TO_LEVEL:
            return GF_START_GAME | ((int32_t)seq->data & ((1 << 6) - 1));

        case GFS_EXIT_TO_CINE:
            return GF_START_CINE | ((int32_t)seq->data & ((1 << 6) - 1));

        case GFS_SET_CAM_X:
            g_Camera.pos.x = (int32_t)seq->data;
            break;
        case GFS_SET_CAM_Y:
            g_Camera.pos.y = (int32_t)seq->data;
            break;
        case GFS_SET_CAM_Z:
            g_Camera.pos.z = (int32_t)seq->data;
            break;
        case GFS_SET_CAM_ANGLE:
            g_Camera.target_angle = (int32_t)seq->data;
            break;
        case GFS_FLIP_MAP:
            Room_FlipMap();
            break;
        case GFS_PLAY_SYNCED_AUDIO:
            Music_Play((int32_t)seq->data);
            break;

        case GFS_GIVE_ITEM:
            if (level_type != GFL_SAVED) {
                const GAMEFLOW_GIVE_ITEM_DATA *give_item_data =
                    (const GAMEFLOW_GIVE_ITEM_DATA *)seq->data;
                Inv_AddItemNTimes(
                    give_item_data->object_num, give_item_data->quantity);
                if (g_Lara.gun_type == LGT_UNARMED) {
                    if (Inv_RequestItem(O_GUN_ITEM)) {
                        g_GameInfo.current[level_num].gun_type = LGT_PISTOLS;
                    } else if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
                        g_GameInfo.current[level_num].gun_type = LGT_SHOTGUN;
                    } else if (Inv_RequestItem(O_MAGNUM_ITEM)) {
                        g_GameInfo.current[level_num].gun_type = LGT_MAGNUMS;
                    } else if (Inv_RequestItem(O_UZI_ITEM)) {
                        g_GameInfo.current[level_num].gun_type = LGT_UZIS;
                    }
                    Lara_InitialiseMeshes(level_num);
                }
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
                [g_Objects[swap_data->object1_num].mesh_index
                 + swap_data->mesh_num];
            g_Meshes
                [g_Objects[swap_data->object1_num].mesh_index
                 + swap_data->mesh_num] = g_Meshes
                    [g_Objects[swap_data->object2_num].mesh_index
                     + swap_data->mesh_num];
            g_Meshes
                [g_Objects[swap_data->object2_num].mesh_index
                 + swap_data->mesh_num] = temp;
            break;
        }

        case GFS_SETUP_BACON_LARA: {
            int32_t anchor_room = (int32_t)seq->data;
            if (!BaconLara_InitialiseAnchor(anchor_room)) {
                LOG_ERROR(
                    "Could not anchor Bacon Lara to room %d", anchor_room);
                return GF_EXIT_TO_TITLE;
            }
            break;
        }

        case GFS_LEGACY:
            break;

        case GFS_END:
            return ret;
        }

        seq++;
    }

    return ret;
}

GAMEFLOW_OPTION
GameFlow_StorySoFar(int32_t level_num, int32_t savegame_level)
{
    LOG_INFO("%d", level_num);

    GAMEFLOW_SEQUENCE *seq = g_GameFlow.levels[level_num].sequence;
    GAMEFLOW_OPTION ret = GF_EXIT_TO_TITLE;
    while (seq->type != GFS_END) {
        LOG_INFO("seq %d %d", seq->type, seq->data);

        switch (seq->type) {
        case GFS_LOOP_GAME:
        case GFS_STOP_GAME:
        case GFS_LEVEL_STATS:
        case GFS_TOTAL_STATS:
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
                return GF_EXIT_TO_TITLE;
            }
            break;

        case GFS_START_CINE:
            ret = Game_Cutscene_Start((int32_t)seq->data);
            break;

        case GFS_LOOP_CINE:
            ret = Game_Cutscene_Loop();
            break;

        case GFS_STOP_CINE:
            ret = Game_Cutscene_Stop((int32_t)seq->data);
            break;

        case GFS_PLAY_FMV:
            FMV_Play((char *)seq->data);
            break;

        case GFS_EXIT_TO_TITLE:
            return GF_EXIT_TO_TITLE;

        case GFS_EXIT_TO_LEVEL:
            return GF_START_GAME | ((int32_t)seq->data & ((1 << 6) - 1));

        case GFS_EXIT_TO_CINE:
            return GF_START_CINE | ((int32_t)seq->data & ((1 << 6) - 1));

        case GFS_SET_CAM_X:
            g_Camera.pos.x = (int32_t)seq->data;
            break;
        case GFS_SET_CAM_Y:
            g_Camera.pos.y = (int32_t)seq->data;
            break;
        case GFS_SET_CAM_Z:
            g_Camera.pos.z = (int32_t)seq->data;
            break;
        case GFS_SET_CAM_ANGLE:
            g_Camera.target_angle = (int32_t)seq->data;
            break;
        case GFS_FLIP_MAP:
            Room_FlipMap();
            break;
        case GFS_PLAY_SYNCED_AUDIO:
            Music_Play((int32_t)seq->data);
            break;

        case GFS_MESH_SWAP: {
            GAMEFLOW_MESH_SWAP_DATA *swap_data = seq->data;
            int16_t *temp = g_Meshes
                [g_Objects[swap_data->object1_num].mesh_index
                 + swap_data->mesh_num];
            g_Meshes
                [g_Objects[swap_data->object1_num].mesh_index
                 + swap_data->mesh_num] = g_Meshes
                    [g_Objects[swap_data->object2_num].mesh_index
                     + swap_data->mesh_num];
            g_Meshes
                [g_Objects[swap_data->object2_num].mesh_index
                 + swap_data->mesh_num] = temp;
            break;
        }

        case GFS_END:
            return ret;
        }

        seq++;
    }

    return ret;
}
