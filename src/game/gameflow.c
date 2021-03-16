#include "game/gameflow.h"

#include "game/cinema.h"
#include "game/const.h"
#include "game/control.h"
#include "game/game.h"
#include "game/lara.h"
#include "game/vars.h"
#include "specific/display.h"
#include "specific/file.h"
#include "specific/frontend.h"
#include "specific/output.h"
#include "specific/shed.h"
#include "specific/sndpc.h"

#include "config.h"
#include "json_parser/json.h"
#include "json_utils.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *str;
    const int32_t val;
} EnumToString;

typedef struct {
    int32_t object1_num;
    int32_t object2_num;
    int32_t mesh_num;
} GameFlowMeshSwapData;

static void SetRequesterHeading(REQUEST_INFO *req, char *text)
{
    req->heading_text = text;
}

static void
SetRequesterItemText(REQUEST_INFO *req, int8_t index, const char *text)
{
    if (!text) {
        return;
    }
    strncpy(
        &req->item_texts[index * req->item_text_len], text, req->item_text_len);
}

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
        { "STATS_TIME_TAKEN_FMT", GS_STATS_TIME_TAKEN_FMT },
        { "STATS_SECRETS_FMT", GS_STATS_SECRETS_FMT },
        { "STATS_PICKUPS_FMT", GS_STATS_PICKUPS_FMT },
        { "STATS_KILLS_FMT", GS_STATS_KILLS_FMT },
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

static int8_t S_LoadScriptMeta(struct json_value_s *json)
{
    const char *tmp;
    if (!JSONGetStringValue(json, "savegame_fmt", &tmp)) {
        TRACE("'savegame_fmt' must be a string");
        return 0;
    } else {
        GF.save_game_fmt = strdup(tmp);
    }

    return 1;
}

static int8_t S_LoadScriptGameStrings(struct json_value_s *json)
{
    struct json_value_s *strings = JSONGetField(json, "strings");
    if (!strings) {
        TRACE("missing 'strings' entry");
        return 0;
    }
    if (strings->type != json_type_object) {
        TRACE("'strings' must be a dictionary");
        return 0;
    }

    struct json_object_s *object = json_value_as_object(strings);
    struct json_object_element_s *item = object->start;
    while (item) {
        GAME_STRING_ID key = StringToGameStringID(item->name->string);
        struct json_string_s *value = json_value_as_string(item->value);
        if (!value) {
            TRACE("invalid string key %s", item->name->string);
            item = item->next;
            continue;
        }
        if (key < 0 || key >= GS_NUMBER_OF) {
            TRACE("invalid string key %s", item->name->string);
            item = item->next;
            continue;
        }

        GF.strings[key] = strdup(value->string);
        item = item->next;
    }

    return 1;
}

static int8_t GF_LoadLevelSequence(struct json_value_s *json, int32_t level_num)
{
    struct json_value_s *level_sequence = JSONGetField(json, "sequence");
    if (!level_sequence || level_sequence->type != json_type_array) {
        TRACE("level %d: 'sequence' must be a list", level_num);
        return 0;
    }

    struct json_array_s *arr = json_value_as_array(level_sequence);
    struct json_array_element_s *item = arr->start;

    GF.levels[level_num].sequence =
        malloc(sizeof(GameFlowSequence) * (arr->length + 1));

    GameFlowSequence *seq = GF.levels[level_num].sequence;
    int32_t i = 0;
    while (item) {
        const char *type_str;

        if (!JSONGetStringValue(item->value, "type", &type_str)) {
            TRACE("level %d: sequence 'type' must be a string", level_num);
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
            if (!JSONGetIntegerValue(
                    item->value, "fmv_id", (int32_t *)&seq->data)) {
                TRACE(
                    "level %d, sequence %s: 'fmv_id' must be a number",
                    level_num, type_str);
                return 0;
            }

        } else if (!strcmp(type_str, "display_picture")) {
            seq->type = GFS_DISPLAY_PICTURE;
            const char *tmp;
            if (!JSONGetStringValue(item->value, "picture_path", &tmp)) {
                TRACE(
                    "level %d, sequence %s: 'picture_path' must be a string",
                    level_num, type_str);
                return 0;
            }
            seq->data = strdup(tmp);
            if (!seq->data) {
                TRACE("failed to allocate memory");
                return 0;
            }

        } else if (!strcmp(type_str, "level_stats")) {
            seq->type = GFS_LEVEL_STATS;
            if (!JSONGetIntegerValue(
                    item->value, "level_id", (int32_t *)&seq->data)) {
                TRACE(
                    "level %d, sequence %s: 'level_id' must be a number",
                    level_num, type_str);
                return 0;
            }

        } else if (!strcmp(type_str, "exit_to_title")) {
            seq->type = GFS_EXIT_TO_TITLE;

        } else if (!strcmp(type_str, "exit_to_level")) {
            seq->type = GFS_EXIT_TO_LEVEL;
            if (!JSONGetIntegerValue(
                    item->value, "level_id", (int32_t *)&seq->data)) {
                TRACE(
                    "level %d, sequence %s: 'level_id' must be a number",
                    level_num, type_str);
                return 0;
            }

        } else if (!strcmp(type_str, "exit_to_cine")) {
            seq->type = GFS_EXIT_TO_CINE;
            if (!JSONGetIntegerValue(
                    item->value, "level_id", (int32_t *)&seq->data)) {
                TRACE(
                    "level %d, sequence %s: 'level_id' must be a number",
                    level_num, type_str);
                return 0;
            }

        } else if (!strcmp(type_str, "set_cam_x")) {
            seq->type = GFS_SET_CAM_X;
            if (!JSONGetIntegerValue(
                    item->value, "value", (int32_t *)&seq->data)) {
                TRACE(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return 0;
            }

        } else if (!strcmp(type_str, "set_cam_y")) {
            seq->type = GFS_SET_CAM_Y;
            if (!JSONGetIntegerValue(
                    item->value, "value", (int32_t *)&seq->data)) {
                TRACE(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return 0;
            }

        } else if (!strcmp(type_str, "set_cam_z")) {
            seq->type = GFS_SET_CAM_Z;
            if (!JSONGetIntegerValue(
                    item->value, "value", (int32_t *)&seq->data)) {
                TRACE(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return 0;
            }

        } else if (!strcmp(type_str, "set_cam_angle")) {
            seq->type = GFS_SET_CAM_ANGLE;
            if (!JSONGetIntegerValue(
                    item->value, "value", (int32_t *)&seq->data)) {
                TRACE(
                    "level %d, sequence %s: 'value' must be a number",
                    level_num, type_str);
                return 0;
            }

        } else if (!strcmp(type_str, "flip_map")) {
            seq->type = GFS_FLIP_MAP;

        } else if (!strcmp(type_str, "remove_guns")) {
            seq->type = GFS_REMOVE_GUNS;

        } else if (!strcmp(type_str, "remove_scions")) {
            seq->type = GFS_REMOVE_SCIONS;

        } else if (!strcmp(type_str, "play_synced_audio")) {
            seq->type = GFS_PLAY_SYNCED_AUDIO;
            if (!JSONGetIntegerValue(
                    item->value, "audio_id", (int32_t *)&seq->data)) {
                TRACE(
                    "level %d, sequence %s: 'audio_id' must be a number",
                    level_num, type_str);
                return 0;
            }

        } else if (!strcmp(type_str, "mesh_swap")) {
            seq->type = GFS_MESH_SWAP;
            GameFlowMeshSwapData *swap_data =
                malloc(sizeof(GameFlowMeshSwapData));
            if (!swap_data) {
                TRACE("failed to allocate memory");
                return 0;
            }
            if (!JSONGetIntegerValue(
                    item->value, "object1_id",
                    (int32_t *)&swap_data->object1_num)) {
                TRACE(
                    "level %d, sequence %s: 'object1_id' must be a number",
                    level_num, type_str);
                return 0;
            }
            if (!JSONGetIntegerValue(
                    item->value, "object2_id",
                    (int32_t *)&swap_data->object2_num)) {
                TRACE(
                    "level %d, sequence %s: 'object2_id' must be a number",
                    level_num, type_str);
                return 0;
            }
            if (!JSONGetIntegerValue(
                    item->value, "mesh_id", (int32_t *)&swap_data->mesh_num)) {
                TRACE(
                    "level %d, sequence %s: 'mesh_id' must be a number",
                    level_num, type_str);
                return 0;
            }
            seq->data = swap_data;

        } else if (!strcmp(type_str, "fix_pyramid_secret")) {
            seq->type = GFS_FIX_PYRAMID_SECRET_TRIGGER;

        } else {
            TRACE("unknown sequence type %s", type_str);
            return 0;
        }

        item = item->next;
        i++;
        seq++;
    }

    seq->type = GFS_END;
    seq->data = NULL;

    return 1;
}

static int8_t S_LoadScriptLevels(struct json_value_s *json)
{
    struct json_value_s *levels = JSONGetField(json, "levels");
    if (!levels || levels->type != json_type_array) {
        TRACE("'levels' must be a list");
        return 0;
    }

    struct json_array_s *arr = json_value_as_array(levels);
    int32_t level_count = arr->length;

    GF.levels = malloc(sizeof(GameFlowLevel) * level_count);
    if (!GF.levels) {
        TRACE("failed to allocate memory");
        return 0;
    }

    SaveGame.start = malloc(sizeof(START_INFO) * level_count);
    if (!SaveGame.start) {
        TRACE("failed to allocate memory");
        return 0;
    }

    struct json_array_element_s *item = arr->start;
    int level_num = 0;

    GF.has_demo = 0;
    GF.gym_level_num = -1;
    GF.first_level_num = -1;
    GF.last_level_num = -1;
    GF.title_level_num = -1;
    GF.level_count = arr->length;

    GameFlowLevel *cur = &GF.levels[0];
    while (item) {
        const char *str;
        int32_t num;
        int8_t enabled;

        if (JSONGetIntegerValue(item->value, "music", &num)) {
            cur->music = num;
        } else {
            TRACE("level %d: 'music' must be a number", level_num);
            return 0;
        }

        if (JSONGetStringValue(item->value, "file", &str)) {
            cur->level_file = strdup(str);
            if (!cur->level_file) {
                TRACE("failed to allocate memory");
                return 0;
            }
        } else {
            TRACE("level %d: 'file' must be a string", level_num);
            return 0;
        }

        if (JSONGetStringValue(item->value, "title", &str)) {
            cur->level_title = strdup(str);
            if (!cur->level_title) {
                TRACE("failed to allocate memory");
                return 0;
            }
        } else {
            TRACE("level %d: 'title' must be a string", level_num);
            return 0;
        }

        if (!JSONGetStringValue(item->value, "type", &str)) {
            TRACE("level %d: 'type' must be a string", level_num);
            return 0;
        }
        if (!strcmp(str, "title")) {
            cur->level_type = GFL_TITLE;
            if (GF.title_level_num != -1) {
                TRACE("level %d: there can be only one title level", level_num);
                return 0;
            }
            GF.title_level_num = level_num;
        } else if (!strcmp(str, "gym")) {
            cur->level_type = GFL_GYM;
            if (GF.gym_level_num != -1) {
                TRACE("level %d: there can be only one gym level", level_num);
                return 0;
            }
            GF.gym_level_num = level_num;
        } else if (!strcmp(str, "normal")) {
            cur->level_type = GFL_NORMAL;
            if (GF.first_level_num == -1) {
                GF.first_level_num = level_num;
            }
            GF.last_level_num = level_num;
        } else if (!strcmp(str, "cutscene")) {
            cur->level_type = GFL_CUTSCENE;
        } else if (!strcmp(str, "current")) {
            cur->level_type = GFL_CURRENT;
        } else {
            TRACE("level %d: unknown level type %s", level_num, str);
            return 0;
        }

        if (JSONGetBooleanValue(item->value, "demo", &enabled)) {
            cur->demo = enabled;
            GF.has_demo |= enabled;
        }

        struct json_value_s *level_strings =
            JSONGetField(item->value, "strings");
        if (!level_strings || level_strings->type != json_type_object) {
            TRACE("level %d: 'strings' must be a dictionary", level_num);
            return 0;
        } else {
            if (JSONGetStringValue(level_strings, "pickup1", &str)) {
                cur->pickup1 = strdup(str);
            } else {
                cur->pickup1 = NULL;
            }

            if (JSONGetStringValue(level_strings, "pickup2", &str)) {
                cur->pickup2 = strdup(str);
            } else {
                cur->pickup2 = NULL;
            }

            if (JSONGetStringValue(level_strings, "key1", &str)) {
                cur->key1 = strdup(str);
            } else {
                cur->key1 = NULL;
            }

            if (JSONGetStringValue(level_strings, "key2", &str)) {
                cur->key2 = strdup(str);
            } else {
                cur->key2 = NULL;
            }

            if (JSONGetStringValue(level_strings, "key3", &str)) {
                cur->key3 = strdup(str);
            } else {
                cur->key3 = NULL;
            }

            if (JSONGetStringValue(level_strings, "key4", &str)) {
                cur->key4 = strdup(str);
            } else {
                cur->key4 = NULL;
            }

            if (JSONGetStringValue(level_strings, "puzzle1", &str)) {
                cur->puzzle1 = strdup(str);
            } else {
                cur->puzzle1 = NULL;
            }

            if (JSONGetStringValue(level_strings, "puzzle2", &str)) {
                cur->puzzle2 = strdup(str);
            } else {
                cur->puzzle2 = NULL;
            }

            if (JSONGetStringValue(level_strings, "puzzle3", &str)) {
                cur->puzzle3 = strdup(str);
            } else {
                cur->puzzle3 = NULL;
            }

            if (JSONGetStringValue(level_strings, "puzzle4", &str)) {
                cur->puzzle4 = strdup(str);
            } else {
                cur->puzzle4 = NULL;
            }
        }

        if (!GF_LoadLevelSequence(item->value, level_num)) {
            return 0;
        }

        item = item->next;
        level_num++;
        cur++;
    }

    if (GF.title_level_num == -1) {
        TRACE("at least one level must be of title type");
        return 0;
    }
    if (GF.first_level_num == -1 || GF.last_level_num == -1) {
        TRACE("at least one level must be of normal type");
        return 0;
    }

    TRACE("gym: %d", GF.gym_level_num);
    TRACE("first: %d", GF.first_level_num);
    TRACE("last: %d", GF.last_level_num);
    TRACE("title: %d", GF.title_level_num);
    return 1;
}

static int8_t S_LoadGameFlow(const char *file_name)
{
    int8_t result = 0;
    struct json_value_s *json = NULL;
    FILE *fp = NULL;
    char *script_data = NULL;

    const char *file_path = GetFullPath(file_name);
    fp = fopen(file_path, "rb");
    if (!fp) {
        TRACE("failed to open script file");
        goto cleanup;
    }

    fseek(fp, 0, SEEK_END);
    size_t script_data_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    script_data = malloc(script_data_size + 1);
    if (!script_data) {
        TRACE("failed to allocate memory");
        goto cleanup;
    }
    fread(script_data, 1, script_data_size, fp);
    script_data[script_data_size] = '\0';
    fclose(fp);
    fp = NULL;

    struct json_parse_result_s parse_result;
    json = json_parse_ex(
        script_data, strlen(script_data), json_parse_flags_allow_json5, NULL,
        NULL, &parse_result);
    if (!json) {
        TRACE(
            "failed to parse script file: %s in line %d, char %d",
            JSONGetErrorDescription(parse_result.error),
            parse_result.error_line_no, parse_result.error_row_no, script_data);
        goto cleanup;
    }

    result = 1;
    result &= S_LoadScriptMeta(json);
    result &= S_LoadScriptGameStrings(json);
    result &= S_LoadScriptLevels(json);

cleanup:
    if (fp) {
        fclose(fp);
    }
    if (json) {
        free(json);
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
    InvItemGamma.string = GF.strings[GS_INV_ITEM_GAMMA];
    InvItemLarasHome.string = GF.strings[GS_INV_ITEM_LARAS_HOME];

    SetRequesterHeading(
        &LoadSaveGameRequester, GF.strings[GS_PASSPORT_SELECT_LEVEL]);

    SetRequesterHeading(&NewGameRequester, GF.strings[GS_PASSPORT_SELECT_MODE]);
    SetRequesterItemText(
        &NewGameRequester, 0, GF.strings[GS_PASSPORT_MODE_NEW_GAME]);
    SetRequesterItemText(
        &NewGameRequester, 1, GF.strings[GS_PASSPORT_MODE_NEW_GAME_PLUS]);
    SetRequesterItemText(
        &NewGameRequester, 2, GF.strings[GS_PASSPORT_MODE_JAPANESE_NEW_GAME]);
    SetRequesterItemText(
        &NewGameRequester, 3,
        GF.strings[GS_PASSPORT_MODE_JAPANESE_NEW_GAME_PLUS]);

    return result;
}

static void FixPyramidSecretTrigger()
{
    TRACE("");
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
    TRACE("%d", level_num);

    GameFlowSequence *seq = GF.levels[level_num].sequence;
    GAMEFLOW_OPTION ret = GF_EXIT_TO_TITLE;
    while (seq->type != GFS_END) {
        TRACE("seq %d %d", seq->type, seq->data);

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
            break;

        case GFS_LOOP_GAME:
            ret = GameLoop(0);
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
                TempVideoAdjust(2, 1.0);
                S_DisplayPicture((const char *)seq->data);
                sub_408E41();
                S_Wait(450);
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
            S_StartSyncedAudio((int32_t)seq->data);
            break;

        case GFS_REMOVE_GUNS:
            if (!(SaveGame.bonus_flag & GBF_NGPLUS)) {
                SaveGame.start[level_num].got_pistols = 0;
                SaveGame.start[level_num].got_shotgun = 0;
                SaveGame.start[level_num].got_magnums = 0;
                SaveGame.start[level_num].got_uzis = 0;
                SaveGame.start[level_num].gun_type = LGT_UNARMED;
                SaveGame.start[level_num].gun_type = LGS_ARMLESS;
                InitialiseLaraInventory(level_num);
            }
            break;

        case GFS_REMOVE_SCIONS:
            SaveGame.start[level_num].num_scions = 0;
            InitialiseLaraInventory(level_num);
            break;

        case GFS_MESH_SWAP: {
            GameFlowMeshSwapData *swap_data = seq->data;
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
