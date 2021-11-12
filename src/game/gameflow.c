#include "game/gameflow.h"

#include "config.h"
#include "filesystem.h"
#include "game/cinema.h"
#include "game/control.h"
#include "game/game.h"
#include "game/inv.h"
#include "game/lara.h"
#include "game/music.h"
#include "game/savegame.h"
#include "game/settings.h"
#include "global/const.h"
#include "global/vars.h"
#include "json.h"
#include "log.h"
#include "specific/display.h"
#include "specific/file.h"
#include "specific/frontend.h"
#include "specific/output.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *str;
    const int32_t val;
} EnumToString;

typedef struct GAME_FLOW_DISPLAY_PICTURE_DATA {
    char *path;
    int32_t display_time;
} GAME_FLOW_DISPLAY_PICTURE_DATA;

typedef struct GAME_FLOW_MESH_SWAP_DATA {
    int32_t object1_num;
    int32_t object2_num;
    int32_t mesh_num;
} GAME_FLOW_MESH_SWAP_DATA;

static GAME_STRING_ID StringToGameStringID(const char *str)
{
    static const EnumToString map[] = {
        { "HEADING_INVENTORY", GS_HEADING_INVENTORY },
        { "HEADING_GAME_OVER", GS_HEADING_GAME_OVER },
        { "HEADING_OPTION", GS_HEADING_OPTION },
        { "HEADING_ITEMS", GS_HEADING_ITEMS },
        { "PASSPORT_SELECT_LEVEL", GS_PASSPORT_SELECT_LEVEL },
        { "PASSPORT_SELECT_MODE", GS_PASSPORT_SELECT_MODE },
        { "PASSPORT_MODE_NEW_GAME", GS_PASSPORT_MODE_NEW_GAME },
        { "PASSPORT_MODE_NEW_GAME_PLUS", GS_PASSPORT_MODE_NEW_GAME_PLUS },
        { "PASSPORT_MODE_JAPANESE_NEW_GAME",
          GS_PASSPORT_MODE_JAPANESE_NEW_GAME },
        { "PASSPORT_MODE_JAPANESE_NEW_GAME_PLUS",
          GS_PASSPORT_MODE_JAPANESE_NEW_GAME_PLUS },
        { "PASSPORT_NEW_GAME", GS_PASSPORT_NEW_GAME },
        { "PASSPORT_LOAD_GAME", GS_PASSPORT_LOAD_GAME },
        { "PASSPORT_SAVE_GAME", GS_PASSPORT_SAVE_GAME },
        { "PASSPORT_EXIT_GAME", GS_PASSPORT_EXIT_GAME },
        { "PASSPORT_EXIT_TO_TITLE", GS_PASSPORT_EXIT_TO_TITLE },
        { "DETAIL_SELECT_DETAIL", GS_DETAIL_SELECT_DETAIL },
        { "DETAIL_LEVEL_HIGH", GS_DETAIL_LEVEL_HIGH },
        { "DETAIL_LEVEL_MEDIUM", GS_DETAIL_LEVEL_MEDIUM },
        { "DETAIL_LEVEL_LOW", GS_DETAIL_LEVEL_LOW },
        { "DETAIL_PERSPECTIVE_FMT", GS_DETAIL_PERSPECTIVE_FMT },
        { "DETAIL_BILINEAR_FMT", GS_DETAIL_BILINEAR_FMT },
        { "DETAIL_BRIGHTNESS_FMT", GS_DETAIL_BRIGHTNESS_FMT },
        { "DETAIL_UI_TEXT_SCALE_FMT", GS_DETAIL_UI_TEXT_SCALE_FMT },
        { "DETAIL_UI_BAR_SCALE_FMT", GS_DETAIL_UI_BAR_SCALE_FMT },
        { "DETAIL_VIDEO_MODE_FMT", GS_DETAIL_VIDEO_MODE_FMT },
        { "SOUND_SET_VOLUMES", GS_SOUND_SET_VOLUMES },
        { "CONTROL_DEFAULT_KEYS", GS_CONTROL_DEFAULT_KEYS },
        { "CONTROL_USER_KEYS", GS_CONTROL_USER_KEYS },
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
        { "KEYMAP_PAUSE", GS_KEYMAP_PAUSE },
        { "KEYMAP_CAMERA_UP", GS_KEYMAP_CAMERA_UP },
        { "KEYMAP_CAMERA_DOWN", GS_KEYMAP_CAMERA_DOWN },
        { "KEYMAP_CAMERA_LEFT", GS_KEYMAP_CAMERA_LEFT },
        { "KEYMAP_CAMERA_RIGHT", GS_KEYMAP_CAMERA_RIGHT },
        { "KEYMAP_CAMERA_RESET", GS_KEYMAP_CAMERA_RESET },
        { "STATS_TIME_TAKEN_FMT", GS_STATS_TIME_TAKEN_FMT },
        { "STATS_SECRETS_FMT", GS_STATS_SECRETS_FMT },
        { "STATS_PICKUPS_FMT", GS_STATS_PICKUPS_FMT },
        { "STATS_KILLS_FMT", GS_STATS_KILLS_FMT },
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

    const EnumToString *current = &map[0];
    while (current->str) {
        if (!strcmp(str, current->str)) {
            return current->val;
        }
        current++;
    }
    return -1;
}

static int8_t S_LoadScriptMeta(struct json_object_s *obj)
{
    const char *tmp_s;
    int tmp_i;
    double tmp_d;
    struct json_array_s *tmp_arr;

    tmp_s = json_object_get_string(obj, "savegame_fmt", JSON_INVALID_STRING);
    if (tmp_s == JSON_INVALID_STRING) {
        LOG_ERROR("'savegame_fmt' must be a string");
        return 0;
    }
    GF.save_game_fmt = strdup(tmp_s);

    tmp_d = json_object_get_number_double(obj, "demo_delay", -1.0);
    if (tmp_d < 0.0) {
        LOG_ERROR("'demo_delay' must be a positive number");
        return 0;
    }
    GF.demo_delay = tmp_d * FRAMES_PER_SECOND;

    tmp_i = json_object_get_bool(obj, "enable_game_modes", JSON_INVALID_BOOL);
    if (tmp_i == JSON_INVALID_BOOL) {
        LOG_ERROR("'enable_game_modes' must be a boolean");
        return 0;
    }
    GF.enable_game_modes = tmp_i;

    tmp_i =
        json_object_get_bool(obj, "enable_save_crystals", JSON_INVALID_BOOL);
    if (tmp_i == JSON_INVALID_BOOL) {
        LOG_ERROR("'enable_save_crystals' must be a boolean");
        return 0;
    }
    GF.enable_save_crystals = tmp_i;

    tmp_arr = json_object_get_array(obj, "water_color");
    GF.water_color.r = 0.6;
    GF.water_color.g = 0.7;
    GF.water_color.b = 1.0;
    if (tmp_arr) {
        GF.water_color.r =
            json_array_get_number_double(tmp_arr, 0, GF.water_color.r);
        GF.water_color.g =
            json_array_get_number_double(tmp_arr, 1, GF.water_color.g);
        GF.water_color.b =
            json_array_get_number_double(tmp_arr, 2, GF.water_color.b);
    }

    if (json_object_get_value(obj, "draw_distance_fade")) {
        double value = json_object_get_number_double(
            obj, "draw_distance_fade", JSON_INVALID_NUMBER);
        if (value == JSON_INVALID_NUMBER) {
            LOG_ERROR("'draw_distance_fade' must be a number");
            return 0;
        }
        GF.draw_distance_fade = value;
    } else {
        GF.draw_distance_fade = 12.0f;
    }

    if (json_object_get_value(obj, "draw_distance_max")) {
        double value = json_object_get_number_double(
            obj, "draw_distance_max", JSON_INVALID_NUMBER);
        if (value == JSON_INVALID_NUMBER) {
            LOG_ERROR("'draw_distance_max' must be a number");
            return 0;
        }
        GF.draw_distance_max = value;
    } else {
        GF.draw_distance_max = 20.0f;
    }

    return 1;
}

static int8_t S_LoadScriptGameStrings(struct json_object_s *obj)
{
    struct json_object_s *strings_obj = json_object_get_object(obj, "strings");
    if (!strings_obj) {
        LOG_ERROR("'strings' must be a dictionary");
        return 0;
    }

    struct json_object_element_s *strings_elem = strings_obj->start;
    while (strings_elem) {
        GAME_STRING_ID key = StringToGameStringID(strings_elem->name->string);
        struct json_string_s *value = json_value_as_string(strings_elem->value);
        if (!value || key < 0 || key >= GS_NUMBER_OF) {
            LOG_ERROR("invalid string key %s", strings_elem->name->string);
        } else {
            GF.strings[key] = strdup(value->string);
        }
        strings_elem = strings_elem->next;
    }

    return 1;
}

static int8_t GF_LoadLevelSequence(struct json_object_s *obj, int32_t level_num)
{
    struct json_array_s *jseq_arr = json_object_get_array(obj, "sequence");
    if (!jseq_arr) {
        LOG_ERROR("level %d: 'sequence' must be a list", level_num);
        return 0;
    }

    struct json_array_element_s *jseq_elem = jseq_arr->start;

    GF.levels[level_num].sequence =
        malloc(sizeof(GAMEFLOW_SEQUENCE) * (jseq_arr->length + 1));

    GAMEFLOW_SEQUENCE *seq = GF.levels[level_num].sequence;
    int32_t i = 0;
    while (jseq_elem) {
        struct json_object_s *jseq_obj = json_value_as_object(jseq_elem->value);
        if (!jseq_obj) {
            LOG_ERROR("level %d: 'sequence' elements must be dictionaries");
            return 0;
        }

        const char *type_str =
            json_object_get_string(jseq_obj, "type", JSON_INVALID_STRING);
        if (type_str == JSON_INVALID_STRING) {
            LOG_ERROR("level %d: sequence 'type' must be a string", level_num);
            return 0;
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
            int tmp = json_object_get_number_int(
                jseq_obj, "fmv_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'fmv_id' must be a number",
                    level_num, type_str);
                return 0;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "display_picture")) {
            seq->type = GFS_DISPLAY_PICTURE;

            GAME_FLOW_DISPLAY_PICTURE_DATA *data =
                malloc(sizeof(GAME_FLOW_DISPLAY_PICTURE_DATA));
            if (!data) {
                LOG_ERROR("failed to allocate memory");
                return 0;
            }

            const char *tmp_s = json_object_get_string(
                jseq_obj, "picture_path", JSON_INVALID_STRING);
            if (tmp_s == JSON_INVALID_STRING) {
                LOG_ERROR(
                    "level %d, sequence %s: 'picture_path' must be a string",
                    level_num, type_str);
                return 0;
            }
            data->path = strdup(tmp_s);
            if (!data->path) {
                LOG_ERROR("failed to allocate memory");
                return 0;
            }

            double tmp_d =
                json_object_get_number_double(jseq_obj, "display_time", -1.0);
            if (tmp_d < 0.0) {
                LOG_ERROR(
                    "level %d, sequence %s: 'display_time' must be a positive "
                    "number",
                    level_num, type_str);
                return 0;
            }
            data->display_time = tmp_d * TICKS_PER_SECOND;
            if (!data->display_time) {
                data->display_time = INT_MAX;
            }

            seq->data = data;

        } else if (!strcmp(type_str, "level_stats")) {
            seq->type = GFS_LEVEL_STATS;
            int tmp = json_object_get_number_int(
                jseq_obj, "level_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'level_id' must be a number",
                    level_num, type_str);
                return 0;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "exit_to_title")) {
            seq->type = GFS_EXIT_TO_TITLE;

        } else if (!strcmp(type_str, "exit_to_level")) {
            seq->type = GFS_EXIT_TO_LEVEL;
            int tmp = json_object_get_number_int(
                jseq_obj, "level_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'level_id' must be a number",
                    level_num, type_str);
                return 0;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "exit_to_cine")) {
            seq->type = GFS_EXIT_TO_CINE;
            int tmp = json_object_get_number_int(
                jseq_obj, "level_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'level_id' must be a number",
                    level_num, type_str);
                return 0;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "set_cam_x")) {
            seq->type = GFS_SET_CAM_X;
            int tmp = json_object_get_number_int(
                jseq_obj, "value", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return 0;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "set_cam_y")) {
            seq->type = GFS_SET_CAM_Y;
            int tmp = json_object_get_number_int(
                jseq_obj, "value", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return 0;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "set_cam_z")) {
            seq->type = GFS_SET_CAM_Z;
            int tmp = json_object_get_number_int(
                jseq_obj, "value", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return 0;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "set_cam_angle")) {
            seq->type = GFS_SET_CAM_ANGLE;
            int tmp = json_object_get_number_int(
                jseq_obj, "value", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return 0;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "flip_map")) {
            seq->type = GFS_FLIP_MAP;

        } else if (!strcmp(type_str, "remove_guns")) {
            seq->type = GFS_REMOVE_GUNS;

        } else if (!strcmp(type_str, "remove_scions")) {
            seq->type = GFS_REMOVE_SCIONS;

        } else if (!strcmp(type_str, "play_synced_audio")) {
            seq->type = GFS_PLAY_SYNCED_AUDIO;
            int tmp = json_object_get_number_int(
                jseq_obj, "audio_id", JSON_INVALID_NUMBER);
            if (tmp == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'audio_id' must be a number",
                    level_num, type_str);
                return 0;
            }
            seq->data = (void *)tmp;

        } else if (!strcmp(type_str, "mesh_swap")) {
            seq->type = GFS_MESH_SWAP;

            GAME_FLOW_MESH_SWAP_DATA *swap_data =
                malloc(sizeof(GAME_FLOW_MESH_SWAP_DATA));
            if (!swap_data) {
                LOG_ERROR("failed to allocate memory");
                return 0;
            }

            swap_data->object1_num = json_object_get_number_int(
                jseq_obj, "object1_id", JSON_INVALID_NUMBER);
            if (swap_data->object1_num == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'object1_id' must be a number",
                    level_num, type_str);
                return 0;
            }

            swap_data->object2_num = json_object_get_number_int(
                jseq_obj, "object2_id", JSON_INVALID_NUMBER);
            if (swap_data->object2_num == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'object2_id' must be a number",
                    level_num, type_str);
                return 0;
            }

            swap_data->mesh_num = json_object_get_number_int(
                jseq_obj, "mesh_id", JSON_INVALID_NUMBER);
            if (swap_data->mesh_num == JSON_INVALID_NUMBER) {
                LOG_ERROR(
                    "level %d, sequence %s: 'mesh_id' must be a number",
                    level_num, type_str);
                return 0;
            }

            seq->data = swap_data;

        } else if (!strcmp(type_str, "fix_pyramid_secret")) {
            seq->type = GFS_FIX_PYRAMID_SECRET_TRIGGER;

        } else {
            LOG_ERROR("unknown sequence type %s", type_str);
            return 0;
        }

        jseq_elem = jseq_elem->next;
        i++;
        seq++;
    }

    seq->type = GFS_END;
    seq->data = NULL;

    return 1;
}

static int8_t S_LoadScriptLevels(struct json_object_s *obj)
{
    struct json_array_s *jlvl_arr = json_object_get_array(obj, "levels");
    if (!jlvl_arr) {
        LOG_ERROR("'levels' must be a list");
        return 0;
    }

    int32_t level_count = jlvl_arr->length;

    GF.levels = malloc(sizeof(GAMEFLOW_LEVEL) * level_count);
    if (!GF.levels) {
        LOG_ERROR("failed to allocate memory");
        return 0;
    }

    SaveGame.start = malloc(sizeof(START_INFO) * level_count);
    if (!SaveGame.start) {
        LOG_ERROR("failed to allocate memory");
        return 0;
    }

    struct json_array_element_s *jlvl_elem = jlvl_arr->start;
    int level_num = 0;

    GF.has_demo = 0;
    GF.gym_level_num = -1;
    GF.first_level_num = -1;
    GF.last_level_num = -1;
    GF.title_level_num = -1;
    GF.level_count = jlvl_arr->length;

    GAMEFLOW_LEVEL *cur = &GF.levels[0];
    while (jlvl_elem) {
        struct json_object_s *jlvl_obj = json_value_as_object(jlvl_elem->value);
        if (!jlvl_obj) {
            LOG_ERROR("'levels' elements must be dictionaries");
            return 0;
        }

        const char *tmp_s;
        int32_t tmp_i;
        struct json_array_s *tmp_arr;

        tmp_i =
            json_object_get_number_int(jlvl_obj, "music", JSON_INVALID_NUMBER);
        if (tmp_i == JSON_INVALID_NUMBER) {
            LOG_ERROR("level %d: 'music' must be a number", level_num);
            return 0;
        }
        cur->music = tmp_i;

        tmp_s = json_object_get_string(jlvl_obj, "file", JSON_INVALID_STRING);
        if (tmp_s == JSON_INVALID_STRING) {
            LOG_ERROR("level %d: 'file' must be a string", level_num);
            return 0;
        }
        cur->level_file = strdup(tmp_s);
        if (!cur->level_file) {
            LOG_ERROR("failed to allocate memory");
            return 0;
        }

        tmp_s = json_object_get_string(jlvl_obj, "title", JSON_INVALID_STRING);
        if (tmp_s == JSON_INVALID_STRING) {
            LOG_ERROR("level %d: 'title' must be a string", level_num);
            return 0;
        }
        cur->level_title = strdup(tmp_s);
        if (!cur->level_title) {
            LOG_ERROR("failed to allocate memory");
            return 0;
        }

        tmp_s = json_object_get_string(jlvl_obj, "type", JSON_INVALID_STRING);
        if (tmp_s == JSON_INVALID_STRING) {
            LOG_ERROR("level %d: 'type' must be a string", level_num);
            return 0;
        }
        if (!strcmp(tmp_s, "title")) {
            cur->level_type = GFL_TITLE;
            if (GF.title_level_num != -1) {
                LOG_ERROR(
                    "level %d: there can be only one title level", level_num);
                return 0;
            }
            GF.title_level_num = level_num;
        } else if (!strcmp(tmp_s, "gym")) {
            cur->level_type = GFL_GYM;
            if (GF.gym_level_num != -1) {
                LOG_ERROR(
                    "level %d: there can be only one gym level", level_num);
                return 0;
            }
            GF.gym_level_num = level_num;
        } else if (!strcmp(tmp_s, "normal")) {
            cur->level_type = GFL_NORMAL;
            if (GF.first_level_num == -1) {
                GF.first_level_num = level_num;
            }
            GF.last_level_num = level_num;
        } else if (!strcmp(tmp_s, "cutscene")) {
            cur->level_type = GFL_CUTSCENE;
        } else if (!strcmp(tmp_s, "current")) {
            cur->level_type = GFL_CURRENT;
        } else {
            LOG_ERROR("level %d: unknown level type %s", level_num, tmp_s);
            return 0;
        }

        tmp_i = json_object_get_bool(jlvl_obj, "demo", JSON_INVALID_BOOL);
        if (tmp_i != JSON_INVALID_BOOL) {
            cur->demo = tmp_i;
            GF.has_demo |= tmp_i;
        } else {
            cur->demo = 0;
        }

        {
            double value = json_object_get_number_double(
                jlvl_obj, "draw_distance_fade", JSON_INVALID_NUMBER);
            if (value != JSON_INVALID_NUMBER) {
                cur->draw_distance_fade.override = true;
                cur->draw_distance_fade.value = value;
            } else {
                cur->draw_distance_fade.override = false;
            }
        }

        {
            double value = json_object_get_number_double(
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
                json_array_get_number_double(tmp_arr, 0, GF.water_color.r);
            cur->water_color.value.g =
                json_array_get_number_double(tmp_arr, 1, GF.water_color.g);
            cur->water_color.value.b =
                json_array_get_number_double(tmp_arr, 2, GF.water_color.b);
        } else {
            cur->water_color.override = false;
        }

        struct json_object_s *jlbl_strings_obj =
            json_object_get_object(jlvl_obj, "strings");
        if (!jlbl_strings_obj) {
            LOG_ERROR("level %d: 'strings' must be a dictionary", level_num);
            return 0;
        } else {
            tmp_s = json_object_get_string(
                jlbl_strings_obj, "pickup1", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->pickup1 = strdup(tmp_s);
            } else {
                cur->pickup1 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "pickup2", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->pickup2 = strdup(tmp_s);
            } else {
                cur->pickup2 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "key1", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->key1 = strdup(tmp_s);
            } else {
                cur->key1 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "key2", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->key2 = strdup(tmp_s);
            } else {
                cur->key2 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "key3", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->key3 = strdup(tmp_s);
            } else {
                cur->key3 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "key4", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->key4 = strdup(tmp_s);
            } else {
                cur->key4 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "puzzle1", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->puzzle1 = strdup(tmp_s);
            } else {
                cur->puzzle1 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "puzzle2", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->puzzle2 = strdup(tmp_s);
            } else {
                cur->puzzle2 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "puzzle3", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->puzzle3 = strdup(tmp_s);
            } else {
                cur->puzzle3 = NULL;
            }

            tmp_s = json_object_get_string(
                jlbl_strings_obj, "puzzle4", JSON_INVALID_STRING);
            if (tmp_s != JSON_INVALID_STRING) {
                cur->puzzle4 = strdup(tmp_s);
            } else {
                cur->puzzle4 = NULL;
            }
        }

        if (!GF_LoadLevelSequence(jlvl_obj, level_num)) {
            return 0;
        }

        jlvl_elem = jlvl_elem->next;
        level_num++;
        cur++;
    }

    if (GF.title_level_num == -1) {
        LOG_ERROR("at least one level must be of title type");
        return 0;
    }
    if (GF.first_level_num == -1 || GF.last_level_num == -1) {
        LOG_ERROR("at least one level must be of normal type");
        return 0;
    }
    return 1;
}

static int8_t S_LoadGameFlow(const char *file_name)
{
    int8_t result = 0;
    struct json_value_s *root = NULL;
    MYFILE *fp = NULL;
    char *script_data = NULL;

    const char *file_path = GetFullPath(file_name);
    fp = FileOpen(file_path, FILE_OPEN_READ);
    if (!fp) {
        LOG_ERROR("failed to open script file");
        goto cleanup;
    }

    size_t script_data_size = FileSize(fp);

    script_data = malloc(script_data_size + 1);
    if (!script_data) {
        LOG_ERROR("failed to allocate memory");
        goto cleanup;
    }
    FileRead(script_data, 1, script_data_size, fp);
    script_data[script_data_size] = '\0';
    FileClose(fp);
    fp = NULL;

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

    result = 1;
    result &= S_LoadScriptMeta(root_obj);
    result &= S_LoadScriptGameStrings(root_obj);
    result &= S_LoadScriptLevels(root_obj);

cleanup:
    if (fp) {
        FileClose(fp);
    }
    if (root) {
        json_value_free(root);
    }
    if (script_data) {
        free(script_data);
    }
    return result;
}

int8_t GF_LoadScriptFile(const char *file_name)
{
    int8_t result = S_LoadGameFlow(file_name);

    InvItemMedi.string = GF.strings[GS_INV_ITEM_MEDI],
    InvItemBigMedi.string = GF.strings[GS_INV_ITEM_BIG_MEDI],

    InvItemPuzzle1.string = GF.strings[GS_INV_ITEM_PUZZLE1],
    InvItemPuzzle2.string = GF.strings[GS_INV_ITEM_PUZZLE2],
    InvItemPuzzle3.string = GF.strings[GS_INV_ITEM_PUZZLE3],
    InvItemPuzzle4.string = GF.strings[GS_INV_ITEM_PUZZLE4],

    InvItemKey1.string = GF.strings[GS_INV_ITEM_KEY1],
    InvItemKey2.string = GF.strings[GS_INV_ITEM_KEY2],
    InvItemKey3.string = GF.strings[GS_INV_ITEM_KEY3],
    InvItemKey4.string = GF.strings[GS_INV_ITEM_KEY4],

    InvItemPickup1.string = GF.strings[GS_INV_ITEM_PICKUP1],
    InvItemPickup2.string = GF.strings[GS_INV_ITEM_PICKUP2],
    InvItemLeadBar.string = GF.strings[GS_INV_ITEM_LEADBAR],
    InvItemScion.string = GF.strings[GS_INV_ITEM_SCION],

    InvItemPistols.string = GF.strings[GS_INV_ITEM_PISTOLS],
    InvItemShotgun.string = GF.strings[GS_INV_ITEM_SHOTGUN],
    InvItemMagnum.string = GF.strings[GS_INV_ITEM_MAGNUM],
    InvItemUzi.string = GF.strings[GS_INV_ITEM_UZI],
    InvItemGrenade.string = GF.strings[GS_INV_ITEM_GRENADE],

    InvItemPistolAmmo.string = GF.strings[GS_INV_ITEM_PISTOL_AMMO],
    InvItemShotgunAmmo.string = GF.strings[GS_INV_ITEM_SHOTGUN_AMMO],
    InvItemMagnumAmmo.string = GF.strings[GS_INV_ITEM_MAGNUM_AMMO],
    InvItemUziAmmo.string = GF.strings[GS_INV_ITEM_UZI_AMMO],

    InvItemCompass.string = GF.strings[GS_INV_ITEM_COMPASS],
    InvItemGame.string = GF.strings[GS_INV_ITEM_GAME];
    InvItemDetails.string = GF.strings[GS_INV_ITEM_DETAILS];
    InvItemSound.string = GF.strings[GS_INV_ITEM_SOUND];
    InvItemControls.string = GF.strings[GS_INV_ITEM_CONTROLS];
    InvItemLarasHome.string = GF.strings[GS_INV_ITEM_LARAS_HOME];

    return result;
}

static void FixPyramidSecretTrigger()
{
    uint32_t global_secrets = 0;

    for (int i = 0; i < RoomCount; i++) {
        uint32_t room_secrets = 0;
        ROOM_INFO *r = &RoomInfo[i];
        FLOOR_INFO *floor = &r->floor[0];
        for (int j = 0; j < r->y_size * r->x_size; j++, floor++) {
            int k = floor->index;
            if (!k) {
                continue;
            }

            while (1) {
                uint16_t floor = FloorData[k++];

                switch (floor & DATA_TYPE) {
                case FT_DOOR:
                case FT_ROOF:
                case FT_TILT:
                    k++;
                    break;

                case FT_LAVA:
                    break;

                case FT_TRIGGER: {
                    uint16_t trig_type = (floor & 0x3F00) >> 8;
                    k++; // skip basic trigger stuff

                    if (trig_type == TT_SWITCH || trig_type == TT_KEY
                        || trig_type == TT_PICKUP) {
                        k++;
                    }

                    while (1) {
                        int16_t *command = &FloorData[k++];
                        if (TRIG_BITS(*command) == TO_CAMERA) {
                            k++;
                        } else if (TRIG_BITS(*command) == TO_SECRET) {
                            int16_t number = *command & VALUE_BITS;
                            if (global_secrets & (1 << number) && number == 0) {
                                // the secret number was already used.
                                // update the number to 2.
                                *command |= 2;
                            } else {
                                room_secrets |= (1 << number);
                            }
                        }

                        if (*command & END_BIT) {
                            break;
                        }
                    }
                    break;
                }
                }

                if (floor & END_BIT) {
                    break;
                }
            }
        }
        global_secrets |= room_secrets;
    }

    GF.levels[CurrentLevel].secrets = GetSecretCount();
}

GAMEFLOW_OPTION
GF_InterpretSequence(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type)
{
    LOG_INFO("%d", level_num);

    GAMEFLOW_SEQUENCE *seq = GF.levels[level_num].sequence;
    GAMEFLOW_OPTION ret = GF_EXIT_TO_TITLE;
    while (seq->type != GFS_END) {
        LOG_INFO("seq %d %d", seq->type, seq->data);

        if (T1MConfig.disable_cine
            && GF.levels[level_num].level_type == GFL_CUTSCENE) {
            int8_t skip;
            switch (seq->type) {
            case GFS_EXIT_TO_TITLE:
            case GFS_EXIT_TO_LEVEL:
            case GFS_EXIT_TO_CINE:
            case GFS_PLAY_FMV:
            case GFS_LEVEL_STATS:
                skip = 0;
                break;
            default:
                skip = 1;
                break;
            }
            if (skip) {
                seq++;
                continue;
            }
        }

        switch (seq->type) {
        case GFS_START_GAME:
            ret = StartGame((int32_t)seq->data, level_type);
            if (GF.enable_save_crystals && level_type != GFL_SAVED
                && (int32_t)seq->data != GF.first_level_num) {
                int32_t return_val = Display_Inventory(INV_SAVE_CRYSTAL_MODE);
                if (return_val != GF_NOP) {
                    CreateSaveGameInfo();
                    S_SaveGame(&SaveGame, InvExtraData[1]);
                    S_WriteUserSettings();
                }
            }

            break;

        case GFS_LOOP_GAME:
            ret = GameLoop(0);
            LOG_DEBUG("GameLoop() exited with %d", ret);
            if (ret != GF_NOP) {
                return ret;
            }
            break;

        case GFS_STOP_GAME:
            ret = StopGame();
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
                ret = StartCinematic((int32_t)seq->data);
            }
            break;

        case GFS_LOOP_CINE:
            if (level_type != GFL_SAVED) {
                ret = CinematicLoop();
            }
            break;

        case GFS_STOP_CINE:
            if (level_type != GFL_SAVED) {
                ret = StopCinematic((int32_t)seq->data);
            }
            break;

        case GFS_PLAY_FMV:
            if (level_type != GFL_SAVED) {
                S_PlayFMV((int32_t)seq->data, 1);
            }
            break;

        case GFS_LEVEL_STATS:
            LevelStats((int32_t)seq->data);
            break;

        case GFS_DISPLAY_PICTURE:
            if (level_type != GFL_SAVED) {
                GAME_FLOW_DISPLAY_PICTURE_DATA *data = seq->data;
                TempVideoAdjust(2);
                S_DisplayPicture(data->path);
                S_InitialisePolyList();
                S_CopyBufferToScreen();
                S_OutputPolyList();
                S_DumpScreen();
                S_Wait(data->display_time);
                S_FadeToBlack();
                S_NoFade();
            }
            break;

        case GFS_EXIT_TO_TITLE:
            return GF_EXIT_TO_TITLE;

        case GFS_EXIT_TO_LEVEL:
            return GF_START_GAME | ((int32_t)seq->data & ((1 << 6) - 1));

        case GFS_EXIT_TO_CINE:
            return GF_START_CINE | ((int32_t)seq->data & ((1 << 6) - 1));

        case GFS_SET_CAM_X:
            Camera.pos.x = (int32_t)seq->data;
            break;
        case GFS_SET_CAM_Y:
            Camera.pos.y = (int32_t)seq->data;
            break;
        case GFS_SET_CAM_Z:
            Camera.pos.z = (int32_t)seq->data;
            break;
        case GFS_SET_CAM_ANGLE:
            Camera.target_angle = (int32_t)seq->data;
            break;
        case GFS_FLIP_MAP:
            FlipMap();
            break;
        case GFS_PLAY_SYNCED_AUDIO:
            Music_Play((int32_t)seq->data);
            break;

        case GFS_REMOVE_GUNS:
            if (level_type != GFL_SAVED
                && !(SaveGame.bonus_flag & GBF_NGPLUS)) {
                SaveGame.start[level_num].got_pistols = 0;
                SaveGame.start[level_num].got_shotgun = 0;
                SaveGame.start[level_num].got_magnums = 0;
                SaveGame.start[level_num].got_uzis = 0;
                SaveGame.start[level_num].gun_type = LGT_UNARMED;
                SaveGame.start[level_num].gun_status = LGS_ARMLESS;
                InitialiseLaraInventory(level_num);
            }
            break;

        case GFS_REMOVE_SCIONS:
            if (level_type != GFL_SAVED) {
                SaveGame.start[level_num].num_scions = 0;
                InitialiseLaraInventory(level_num);
            }
            break;

        case GFS_MESH_SWAP: {
            GAME_FLOW_MESH_SWAP_DATA *swap_data = seq->data;
            int16_t *temp;

            temp = Meshes
                [Objects[swap_data->object1_num].mesh_index
                 + swap_data->mesh_num];
            Meshes
                [Objects[swap_data->object1_num].mesh_index
                 + swap_data->mesh_num] = Meshes
                    [Objects[swap_data->object2_num].mesh_index
                     + swap_data->mesh_num];
            Meshes
                [Objects[swap_data->object2_num].mesh_index
                 + swap_data->mesh_num] = temp;
            break;
        }

        case GFS_FIX_PYRAMID_SECRET_TRIGGER:
            if (T1MConfig.fix_pyramid_secret_trigger) {
                FixPyramidSecretTrigger();
            }
            break;

        case GFS_END:
            return ret;
        }

        seq++;
    }

    return ret;
}
