#include "game/gameflow.h"
#include "game/vars.h"
#include "specific/file.h"
#include "json_utils.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *str;
    const int32_t val;
} EnumToString;

static void SetRequesterHeading(REQUEST_INFO *req, char *text)
{
    req->heading_text = text;
}

static void
SetRequesterItemText(REQUEST_INFO *req, int8_t index, const char *text)
{
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

static void GF_LoadGameStrings(struct json_value_s *json)
{
    struct json_value_s *strings = JSONGetField(json, "strings");
    if (!strings) {
        TRACE("missing 'strings' entry");
        return;
    }
    if (strings->type != json_type_object) {
        TRACE("'strings' must be a dictionary");
        return;
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

        GF_GameStrings[key] = malloc(strlen(value->string) + 1);
        strcpy(GF_GameStrings[key], value->string);
        item = item->next;
    }
}

static void GF_LoadLevels(struct json_value_s *json)
{
    struct json_value_s *levels = JSONGetField(json, "levels");
    if (!levels) {
        TRACE("missing 'levels' entry");
        return;
    }
    if (levels->type != json_type_array) {
        TRACE("'levels' must be a list");
    }

    struct json_array_s *arr = json_value_as_array(levels);
    int32_t level_count = arr->length;
    if (level_count != LV_NUMBER_OF) {
        TRACE(
            "currently only fixed number of levels is supported! got "
            "%d, expected %d.",
            level_count, LV_NUMBER_OF);
        return;
    }

    // GF_LevelTitles = malloc(sizeof(char *) * level_count);
    // GV_LevelNames = malloc(sizeof(char *) * level_count);

    struct json_array_element_s *item = arr->start;
    int level_num = 0;

    while (item) {
        const char *str;

        if (JSONGetStringValue(item->value, "file", &str)) {
            GF_LevelNames[level_num] = malloc(strlen(str) + 1);
            strcpy(GF_LevelNames[level_num], str);
        } else {
            TRACE("level %d: 'file' must be a string", level_num);
        }

        if (JSONGetStringValue(item->value, "title", &str)) {
            GF_LevelTitles[level_num] = malloc(strlen(str) + 1);
            strcpy(GF_LevelTitles[level_num], str);
        } else {
            TRACE("level %d: 'title' must be a string", level_num);
        }

        item = item->next;
        level_num++;
    }
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
        TRACE("failed to allocate script data");
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
            "failed to parse script file: %s in line %d, char %d (script: %s)",
            JSONGetErrorDescription(parse_result.error),
            parse_result.error_line_no, parse_result.error_row_no, script_data);
        goto cleanup;
    }

    result = 1;

    GF_LoadGameStrings(json);
    GF_LoadLevels(json);

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

    InvItemMedi.string = GF_GameStrings[GS_INV_ITEM_MEDI],
    InvItemBigMedi.string = GF_GameStrings[GS_INV_ITEM_BIG_MEDI],

    InvItemPuzzle1.string = GF_GameStrings[GS_INV_ITEM_PUZZLE1],
    InvItemPuzzle2.string = GF_GameStrings[GS_INV_ITEM_PUZZLE2],
    InvItemPuzzle3.string = GF_GameStrings[GS_INV_ITEM_PUZZLE3],
    InvItemPuzzle4.string = GF_GameStrings[GS_INV_ITEM_PUZZLE4],

    InvItemKey1.string = GF_GameStrings[GS_INV_ITEM_KEY1],
    InvItemKey2.string = GF_GameStrings[GS_INV_ITEM_KEY2],
    InvItemKey3.string = GF_GameStrings[GS_INV_ITEM_KEY3],
    InvItemKey4.string = GF_GameStrings[GS_INV_ITEM_KEY4],

    InvItemPickup1.string = GF_GameStrings[GS_INV_ITEM_PICKUP1],
    InvItemPickup2.string = GF_GameStrings[GS_INV_ITEM_PICKUP2],
    InvItemLeadBar.string = GF_GameStrings[GS_INV_ITEM_LEADBAR],
    InvItemScion.string = GF_GameStrings[GS_INV_ITEM_SCION],

    InvItemPistols.string = GF_GameStrings[GS_INV_ITEM_PISTOLS],
    InvItemShotgun.string = GF_GameStrings[GS_INV_ITEM_SHOTGUN],
    InvItemMagnum.string = GF_GameStrings[GS_INV_ITEM_MAGNUM],
    InvItemUzi.string = GF_GameStrings[GS_INV_ITEM_UZI],
    InvItemGrenade.string = GF_GameStrings[GS_INV_ITEM_GRENADE],

    InvItemPistolAmmo.string = GF_GameStrings[GS_INV_ITEM_PISTOL_AMMO],
    InvItemShotgunAmmo.string = GF_GameStrings[GS_INV_ITEM_SHOTGUN_AMMO],
    InvItemMagnumAmmo.string = GF_GameStrings[GS_INV_ITEM_MAGNUM_AMMO],
    InvItemUziAmmo.string = GF_GameStrings[GS_INV_ITEM_UZI_AMMO],

    InvItemCompass.string = GF_GameStrings[GS_INV_ITEM_COMPASS],
    InvItemGame.string = GF_GameStrings[GS_INV_ITEM_GAME];
    InvItemDetails.string = GF_GameStrings[GS_INV_ITEM_DETAILS];
    InvItemSound.string = GF_GameStrings[GS_INV_ITEM_SOUND];
    InvItemControls.string = GF_GameStrings[GS_INV_ITEM_CONTROLS];
    InvItemGamma.string = GF_GameStrings[GS_INV_ITEM_GAMMA];
    InvItemLarasHome.string = GF_GameStrings[GS_INV_ITEM_LARAS_HOME];

    SetRequesterHeading(
        &LoadSaveGameRequester, GF_GameStrings[GS_PASSPORT_SELECT_LEVEL]);

#ifdef T1M_FEAT_GAMEPLAY
    SetRequesterHeading(
        &NewGameRequester, GF_GameStrings[GS_PASSPORT_SELECT_MODE]);
    SetRequesterItemText(
        &NewGameRequester, 0, GF_GameStrings[GS_PASSPORT_MODE_NEW_GAME]);
    SetRequesterItemText(
        &NewGameRequester, 1, GF_GameStrings[GS_PASSPORT_MODE_NEW_GAME_PLUS]);
#endif

    return result;
}
