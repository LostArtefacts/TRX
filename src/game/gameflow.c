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
        { "HEADING_INVENTORY", GSI_HEADING_INVENTORY },
        { "HEADING_GAME_OVER", GSI_HEADING_GAME_OVER },
        { "HEADING_OPTION", GSI_HEADING_OPTION },
        { "HEADING_ITEMS", GSI_HEADING_ITEMS },
        { "PASSPORT_SELECT_LEVEL", GSI_PASSPORT_SELECT_LEVEL },
        { "PASSPORT_SELECT_MODE", GSI_PASSPORT_SELECT_MODE },
        { "PASSPORT_MODE_NEW_GAME", GSI_PASSPORT_MODE_NEW_GAME },
        { "PASSPORT_MODE_NEW_GAME_PLUS", GSI_PASSPORT_MODE_NEW_GAME_PLUS },
        { "PASSPORT_NEW_GAME", GSI_PASSPORT_NEW_GAME },
        { "PASSPORT_LOAD_GAME", GSI_PASSPORT_LOAD_GAME },
        { "PASSPORT_SAVE_GAME", GSI_PASSPORT_SAVE_GAME },
        { "PASSPORT_EXIT_GAME", GSI_PASSPORT_EXIT_GAME },
        { "PASSPORT_EXIT_TO_TITLE", GSI_PASSPORT_EXIT_TO_TITLE },
        { "DETAIL_SELECT_DETAIL", GSI_DETAIL_SELECT_DETAIL },
        { "DETAIL_LEVEL_HIGH", GSI_DETAIL_LEVEL_HIGH },
        { "DETAIL_LEVEL_MEDIUM", GSI_DETAIL_LEVEL_MEDIUM },
        { "DETAIL_LEVEL_LOW", GSI_DETAIL_LEVEL_LOW },
        { "DETAIL_PERSPECTIVE_FMT", GSI_DETAIL_PERSPECTIVE_FMT },
        { "DETAIL_BILINEAR_FMT", GSI_DETAIL_BILINEAR_FMT },
        { "DETAIL_VIDEO_MODE_FMT", GSI_DETAIL_VIDEO_MODE_FMT },
        { "SOUND_SET_VOLUMES", GSI_SOUND_SET_VOLUMES },
        { "CONTROL_DEFAULT_KEYS", GSI_CONTROL_DEFAULT_KEYS },
        { "CONTROL_USER_KEYS", GSI_CONTROL_USER_KEYS },
        { "KEYMAP_RUN", GSI_KEYMAP_RUN },
        { "KEYMAP_BACK", GSI_KEYMAP_BACK },
        { "KEYMAP_LEFT", GSI_KEYMAP_LEFT },
        { "KEYMAP_RIGHT", GSI_KEYMAP_RIGHT },
        { "KEYMAP_STEP_LEFT", GSI_KEYMAP_STEP_LEFT },
        { "KEYMAP_STEP_RIGHT", GSI_KEYMAP_STEP_RIGHT },
        { "KEYMAP_WALK", GSI_KEYMAP_WALK },
        { "KEYMAP_JUMP", GSI_KEYMAP_JUMP },
        { "KEYMAP_ACTION", GSI_KEYMAP_ACTION },
        { "KEYMAP_DRAW_WEAPON", GSI_KEYMAP_DRAW_WEAPON },
        { "KEYMAP_LOOK", GSI_KEYMAP_LOOK },
        { "KEYMAP_ROLL", GSI_KEYMAP_ROLL },
        { "KEYMAP_INVENTORY", GSI_KEYMAP_INVENTORY },
        { "STATS_TIME_TAKEN_FMT", GSI_STATS_TIME_TAKEN_FMT },
        { "STATS_SECRETS_FMT", GSI_STATS_SECRETS_FMT },
        { "STATS_PICKUPS_FMT", GSI_STATS_PICKUPS_FMT },
        { "STATS_KILLS_FMT", GSI_STATS_KILLS_FMT },
        { "MISC_ON", GSI_MISC_ON },
        { "MISC_OFF", GSI_MISC_OFF },
        { "MISC_EMPTY_SLOT_FMT", GSI_MISC_EMPTY_SLOT_FMT },
        { "MISC_DEMO_MODE", GSI_MISC_DEMO_MODE },
        { "INV_ITEM_MEDI", GSI_INV_ITEM_MEDI },
        { "INV_ITEM_BIG_MEDI", GSI_INV_ITEM_BIG_MEDI },
        { "INV_ITEM_PUZZLE1", GSI_INV_ITEM_PUZZLE1 },
        { "INV_ITEM_PUZZLE2", GSI_INV_ITEM_PUZZLE2 },
        { "INV_ITEM_PUZZLE3", GSI_INV_ITEM_PUZZLE3 },
        { "INV_ITEM_PUZZLE4", GSI_INV_ITEM_PUZZLE4 },
        { "INV_ITEM_KEY1", GSI_INV_ITEM_KEY1 },
        { "INV_ITEM_KEY2", GSI_INV_ITEM_KEY2 },
        { "INV_ITEM_KEY3", GSI_INV_ITEM_KEY3 },
        { "INV_ITEM_KEY4", GSI_INV_ITEM_KEY4 },
        { "INV_ITEM_PICKUP1", GSI_INV_ITEM_PICKUP1 },
        { "INV_ITEM_PICKUP2", GSI_INV_ITEM_PICKUP2 },
        { "INV_ITEM_LEADBAR", GSI_INV_ITEM_LEADBAR },
        { "INV_ITEM_SCION", GSI_INV_ITEM_SCION },
        { "INV_ITEM_PISTOLS", GSI_INV_ITEM_PISTOLS },
        { "INV_ITEM_SHOTGUN", GSI_INV_ITEM_SHOTGUN },
        { "INV_ITEM_MAGNUM", GSI_INV_ITEM_MAGNUM },
        { "INV_ITEM_UZI", GSI_INV_ITEM_UZI },
        { "INV_ITEM_GRENADE", GSI_INV_ITEM_GRENADE },
        { "INV_ITEM_PISTOL_AMMO", GSI_INV_ITEM_PISTOL_AMMO },
        { "INV_ITEM_SHOTGUN_AMMO", GSI_INV_ITEM_SHOTGUN_AMMO },
        { "INV_ITEM_MAGNUM_AMMO", GSI_INV_ITEM_MAGNUM_AMMO },
        { "INV_ITEM_UZI_AMMO", GSI_INV_ITEM_UZI_AMMO },
        { "INV_ITEM_COMPASS", GSI_INV_ITEM_COMPASS },
        { "INV_ITEM_GAME", GSI_INV_ITEM_GAME },
        { "INV_ITEM_DETAILS", GSI_INV_ITEM_DETAILS },
        { "INV_ITEM_SOUND", GSI_INV_ITEM_SOUND },
        { "INV_ITEM_CONTROLS", GSI_INV_ITEM_CONTROLS },
        { "INV_ITEM_GAMMA", GSI_INV_ITEM_GAMMA },
        { "INV_ITEM_LARAS_HOME", GSI_INV_ITEM_LARAS_HOME },
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
        if (key < 0 || key >= GSI_NUMBER_OF) {
            TRACE("invalid string key %s", item->name->string);
            item = item->next;
            continue;
        }

        GF_GameStringTable[key] = malloc(strlen(value->string) + 1);
        strcpy(GF_GameStringTable[key], value->string);
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

    InvItemMedi.string = GF_GameStringTable[GSI_INV_ITEM_MEDI],
    InvItemBigMedi.string = GF_GameStringTable[GSI_INV_ITEM_BIG_MEDI],

    InvItemPuzzle1.string = GF_GameStringTable[GSI_INV_ITEM_PUZZLE1],
    InvItemPuzzle2.string = GF_GameStringTable[GSI_INV_ITEM_PUZZLE2],
    InvItemPuzzle3.string = GF_GameStringTable[GSI_INV_ITEM_PUZZLE3],
    InvItemPuzzle4.string = GF_GameStringTable[GSI_INV_ITEM_PUZZLE4],

    InvItemKey1.string = GF_GameStringTable[GSI_INV_ITEM_KEY1],
    InvItemKey2.string = GF_GameStringTable[GSI_INV_ITEM_KEY2],
    InvItemKey3.string = GF_GameStringTable[GSI_INV_ITEM_KEY3],
    InvItemKey4.string = GF_GameStringTable[GSI_INV_ITEM_KEY4],

    InvItemPickup1.string = GF_GameStringTable[GSI_INV_ITEM_PICKUP1],
    InvItemPickup2.string = GF_GameStringTable[GSI_INV_ITEM_PICKUP2],
    InvItemLeadBar.string = GF_GameStringTable[GSI_INV_ITEM_LEADBAR],
    InvItemScion.string = GF_GameStringTable[GSI_INV_ITEM_SCION],

    InvItemPistols.string = GF_GameStringTable[GSI_INV_ITEM_PISTOLS],
    InvItemShotgun.string = GF_GameStringTable[GSI_INV_ITEM_SHOTGUN],
    InvItemMagnum.string = GF_GameStringTable[GSI_INV_ITEM_MAGNUM],
    InvItemUzi.string = GF_GameStringTable[GSI_INV_ITEM_UZI],
    InvItemGrenade.string = GF_GameStringTable[GSI_INV_ITEM_GRENADE],

    InvItemPistolAmmo.string = GF_GameStringTable[GSI_INV_ITEM_PISTOL_AMMO],
    InvItemShotgunAmmo.string = GF_GameStringTable[GSI_INV_ITEM_SHOTGUN_AMMO],
    InvItemMagnumAmmo.string = GF_GameStringTable[GSI_INV_ITEM_MAGNUM_AMMO],
    InvItemUziAmmo.string = GF_GameStringTable[GSI_INV_ITEM_UZI_AMMO],

    InvItemCompass.string = GF_GameStringTable[GSI_INV_ITEM_COMPASS],
    InvItemGame.string = GF_GameStringTable[GSI_INV_ITEM_GAME];
    InvItemDetails.string = GF_GameStringTable[GSI_INV_ITEM_DETAILS];
    InvItemSound.string = GF_GameStringTable[GSI_INV_ITEM_SOUND];
    InvItemControls.string = GF_GameStringTable[GSI_INV_ITEM_CONTROLS];
    InvItemGamma.string = GF_GameStringTable[GSI_INV_ITEM_GAMMA];
    InvItemLarasHome.string = GF_GameStringTable[GSI_INV_ITEM_LARAS_HOME];

    SetRequesterHeading(
        &LoadSaveGameRequester, GF_GameStringTable[GSI_PASSPORT_SELECT_LEVEL]);

#ifdef T1M_FEAT_GAMEPLAY
    SetRequesterHeading(
        &NewGameRequester, GF_GameStringTable[GSI_PASSPORT_SELECT_MODE]);
    SetRequesterItemText(
        &NewGameRequester, 0, GF_GameStringTable[GSI_PASSPORT_MODE_NEW_GAME]);
    SetRequesterItemText(
        &NewGameRequester, 1,
        GF_GameStringTable[GSI_PASSPORT_MODE_NEW_GAME_PLUS]);
#endif

    return result;
}
