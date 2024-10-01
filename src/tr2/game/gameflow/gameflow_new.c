#include "game/gameflow/gameflow_new.h"

#include "game/game_string.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/enum_map.h>
#include <libtrx/game/gameflow/types.h>
#include <libtrx/game/objects/names.h>
#include <libtrx/log.h>

#include <assert.h>

GAMEFLOW_NEW g_GameflowNew;
GAME_INFO g_GameInfo;

static void M_LoadObjectString(const char *key, const char *value);
static void M_LoadGameString(const char *key, const char *value);
static void M_LoadObjectStrings(const int32_t level_num);
static void M_LoadGameStrings(const int32_t level_num);

static void M_LoadObjectString(const char *const key, const char *const value)
{
    const GAME_OBJECT_ID object_id =
        ENUM_MAP_GET(GAME_OBJECT_ID, key, NO_OBJECT);
    if (object_id != NO_OBJECT) {
        Object_SetName(object_id, value);
    }
}

static void M_LoadGameString(const char *const key, const char *const value)
{
    if (!GameString_IsKnown(key)) {
        LOG_ERROR("Invalid game string key: %s", key);
    } else if (value == NULL) {
        LOG_ERROR("Invalid game string value: %s", key);
    } else {
        GameString_Define(key, value);
    }
}

static void M_LoadObjectStrings(const int32_t level_num)
{
    const GAMEFLOW_NEW *const gf = &g_GameflowNew;

    const GAMEFLOW_NEW_STRING_ENTRY *entry = gf->object_strings;
    while (entry != NULL && entry->key != NULL) {
        M_LoadObjectString(entry->key, entry->value);
        entry++;
    }

    if (level_num >= 0) {
        assert(level_num < gf->level_count);
        const GAMEFLOW_NEW_LEVEL *const level = &gf->levels[level_num];
        entry = level->object_strings;
        while (entry != NULL && entry->key != NULL) {
            M_LoadObjectString(entry->key, entry->value);
            entry++;
        }
    }
}

static void M_LoadGameStrings(const int32_t level_num)
{
    const GAMEFLOW_NEW *const gf = &g_GameflowNew;

    const GAMEFLOW_NEW_STRING_ENTRY *entry = gf->game_strings;
    while (entry != NULL && entry->key != NULL) {
        M_LoadGameString(entry->key, entry->value);
        entry++;
    }

    if (level_num >= 0) {
        assert(level_num < gf->level_count);
        const GAMEFLOW_NEW_LEVEL *const level = &gf->levels[level_num];
        entry = level->game_strings;
        while (entry != NULL && entry->key != NULL) {
            M_LoadGameString(entry->key, entry->value);
            entry++;
        }
    }
}

void GF_N_LoadStrings(const int32_t level_num)
{
    Object_ResetNames();

    M_LoadObjectStrings(level_num);
    M_LoadGameStrings(level_num);

    struct {
        GAME_OBJECT_ID object_id;
        GF_GAME_STRING game_string;
    } game_string_defs[] = {
        { O_COMPASS_OPTION, GF_S_GAME_INV_ITEM_STATISTICS },
        { O_COMPASS_ITEM, GF_S_GAME_INV_ITEM_STATISTICS },
        { O_PISTOL_ITEM, GF_S_GAME_INV_ITEM_PISTOLS },
        { O_PISTOL_OPTION, GF_S_GAME_INV_ITEM_PISTOLS },
        { O_FLARE_ITEM, GF_S_GAME_INV_ITEM_FLARE },
        { O_FLARES_OPTION, GF_S_GAME_INV_ITEM_FLARE },
        { O_SHOTGUN_ITEM, GF_S_GAME_INV_ITEM_SHOTGUN },
        { O_SHOTGUN_OPTION, GF_S_GAME_INV_ITEM_SHOTGUN },
        { O_MAGNUM_ITEM, GF_S_GAME_INV_ITEM_MAGNUMS },
        { O_MAGNUM_OPTION, GF_S_GAME_INV_ITEM_MAGNUMS },
        { O_UZI_ITEM, GF_S_GAME_INV_ITEM_UZIS },
        { O_UZI_OPTION, GF_S_GAME_INV_ITEM_UZIS },
        { O_HARPOON_ITEM, GF_S_GAME_INV_ITEM_HARPOON },
        { O_HARPOON_OPTION, GF_S_GAME_INV_ITEM_HARPOON },
        { O_M16_ITEM, GF_S_GAME_INV_ITEM_M16 },
        { O_M16_OPTION, GF_S_GAME_INV_ITEM_M16 },
        { O_GRENADE_ITEM, GF_S_GAME_INV_ITEM_GRENADE },
        { O_GRENADE_OPTION, GF_S_GAME_INV_ITEM_GRENADE },
        { O_PISTOL_AMMO_ITEM, GF_S_GAME_INV_ITEM_PISTOL_AMMO },
        { O_PISTOL_AMMO_OPTION, GF_S_GAME_INV_ITEM_PISTOL_AMMO },
        { O_SHOTGUN_AMMO_ITEM, GF_S_GAME_INV_ITEM_SHOTGUN_AMMO },
        { O_SHOTGUN_AMMO_OPTION, GF_S_GAME_INV_ITEM_SHOTGUN_AMMO },
        { O_MAGNUM_AMMO_ITEM, GF_S_GAME_INV_ITEM_MAGNUM_AMMO },
        { O_MAGNUM_AMMO_OPTION, GF_S_GAME_INV_ITEM_MAGNUM_AMMO },
        { O_UZI_AMMO_ITEM, GF_S_GAME_INV_ITEM_UZI_AMMO },
        { O_UZI_AMMO_OPTION, GF_S_GAME_INV_ITEM_UZI_AMMO },
        { O_HARPOON_AMMO_ITEM, GF_S_GAME_INV_ITEM_HARPOON_AMMO },
        { O_HARPOON_AMMO_OPTION, GF_S_GAME_INV_ITEM_HARPOON_AMMO },
        { O_M16_AMMO_ITEM, GF_S_GAME_INV_ITEM_M16_AMMO },
        { O_M16_AMMO_OPTION, GF_S_GAME_INV_ITEM_M16_AMMO },
        { O_GRENADE_AMMO_ITEM, GF_S_GAME_INV_ITEM_GRENADE_AMMO },
        { O_GRENADE_AMMO_OPTION, GF_S_GAME_INV_ITEM_GRENADE_AMMO },
        { O_SMALL_MEDIPACK_ITEM, GF_S_GAME_INV_ITEM_SMALL_MEDIPACK },
        { O_SMALL_MEDIPACK_OPTION, GF_S_GAME_INV_ITEM_SMALL_MEDIPACK },
        { O_LARGE_MEDIPACK_ITEM, GF_S_GAME_INV_ITEM_LARGE_MEDIPACK },
        { O_LARGE_MEDIPACK_OPTION, GF_S_GAME_INV_ITEM_LARGE_MEDIPACK },
        { O_PICKUP_ITEM_1, GF_S_GAME_INV_ITEM_PICKUP },
        { O_PICKUP_OPTION_1, GF_S_GAME_INV_ITEM_PICKUP },
        { O_PICKUP_ITEM_2, GF_S_GAME_INV_ITEM_PICKUP },
        { O_PICKUP_OPTION_2, GF_S_GAME_INV_ITEM_PICKUP },
        { O_PUZZLE_ITEM_1, GF_S_GAME_INV_ITEM_PUZZLE },
        { O_PUZZLE_OPTION_1, GF_S_GAME_INV_ITEM_PUZZLE },
        { O_PUZZLE_ITEM_2, GF_S_GAME_INV_ITEM_PUZZLE },
        { O_PUZZLE_OPTION_2, GF_S_GAME_INV_ITEM_PUZZLE },
        { O_PUZZLE_ITEM_3, GF_S_GAME_INV_ITEM_PUZZLE },
        { O_PUZZLE_OPTION_3, GF_S_GAME_INV_ITEM_PUZZLE },
        { O_PUZZLE_ITEM_4, GF_S_GAME_INV_ITEM_PUZZLE },
        { O_PUZZLE_OPTION_4, GF_S_GAME_INV_ITEM_PUZZLE },
        { O_KEY_ITEM_1, GF_S_GAME_INV_ITEM_KEY },
        { O_KEY_OPTION_1, GF_S_GAME_INV_ITEM_KEY },
        { O_KEY_ITEM_2, GF_S_GAME_INV_ITEM_KEY },
        { O_KEY_OPTION_2, GF_S_GAME_INV_ITEM_KEY },
        { O_KEY_ITEM_3, GF_S_GAME_INV_ITEM_KEY },
        { O_KEY_OPTION_3, GF_S_GAME_INV_ITEM_KEY },
        { O_KEY_ITEM_4, GF_S_GAME_INV_ITEM_KEY },
        { O_KEY_OPTION_4, GF_S_GAME_INV_ITEM_KEY },
        { O_PASSPORT_OPTION, GF_S_GAME_INV_ITEM_GAME },
        { O_PASSPORT_CLOSED, GF_S_GAME_INV_ITEM_GAME },
        { O_PHOTO_OPTION, GF_S_GAME_INV_ITEM_LARA_HOME },
        { NO_OBJECT, -1 },
    };

    for (int32_t i = 0; game_string_defs[i].object_id != NO_OBJECT; i++) {
        const char *const new_name =
            g_GF_GameStrings[game_string_defs[i].game_string];
        if (new_name != NULL) {
            Object_SetName(game_string_defs[i].object_id, new_name);
        }
    }

    struct {
        GAME_OBJECT_ID object_id;
        GF_PC_STRING pc_string;
    } pc_string_defs[] = {
        { O_DETAIL_OPTION, GF_S_PC_DETAIL_LEVELS },
        { O_SOUND_OPTION, GF_S_PC_SOUND },
        { O_CONTROL_OPTION, GF_S_PC_CONTROLS },
        { NO_OBJECT, -1 },
    };

    for (int32_t i = 0; pc_string_defs[i].object_id != NO_OBJECT; i++) {
        const char *const new_name =
            g_GF_PCStrings[pc_string_defs[i].pc_string];
        if (new_name != NULL) {
            Object_SetName(pc_string_defs[i].object_id, new_name);
        }
    }

    struct {
        GAME_OBJECT_ID object_id;
        char **strings;
    } level_item_defs[] = {
        { O_PUZZLE_ITEM_1, g_GF_Puzzle1Strings },
        { O_PUZZLE_ITEM_2, g_GF_Puzzle2Strings },
        { O_PUZZLE_ITEM_3, g_GF_Puzzle3Strings },
        { O_PUZZLE_ITEM_4, g_GF_Puzzle4Strings },
        { O_KEY_ITEM_1, g_GF_Key1Strings },
        { O_KEY_ITEM_2, g_GF_Key2Strings },
        { O_KEY_ITEM_3, g_GF_Key3Strings },
        { O_KEY_ITEM_4, g_GF_Key4Strings },
        { O_PICKUP_ITEM_1, g_GF_Pickup1Strings },
        { O_PICKUP_ITEM_2, g_GF_Pickup2Strings },
        { O_PUZZLE_OPTION_1, g_GF_Puzzle1Strings },
        { O_PUZZLE_OPTION_2, g_GF_Puzzle2Strings },
        { O_PUZZLE_OPTION_3, g_GF_Puzzle3Strings },
        { O_PUZZLE_OPTION_4, g_GF_Puzzle4Strings },
        { O_KEY_OPTION_1, g_GF_Key1Strings },
        { O_KEY_OPTION_2, g_GF_Key2Strings },
        { O_KEY_OPTION_3, g_GF_Key3Strings },
        { O_KEY_OPTION_4, g_GF_Key4Strings },
        { O_PICKUP_OPTION_1, g_GF_Pickup1Strings },
        { O_PICKUP_OPTION_2, g_GF_Pickup2Strings },
        { NO_OBJECT, NULL },
    };

    if (level_num >= 0 && level_num < g_GameFlow.num_levels) {
        for (int32_t i = 0; level_item_defs[i].object_id != NO_OBJECT; i++) {
            const char *const new_name = level_item_defs[i].strings[level_num];
            if (new_name != NULL) {
                Object_SetName(level_item_defs[i].object_id, new_name);
            }
        }
    }
}

int32_t Gameflow_GetLevelCount(void)
{
    return g_GameflowNew.level_count;
}

const char *Gameflow_GetLevelFileName(int32_t level_num)
{
    return g_GF_LevelFileNames[level_num];
}

const char *Gameflow_GetLevelTitle(int32_t level_num)
{
    return g_GF_LevelNames[level_num];
}

int32_t Gameflow_GetGymLevelNumber(void)
{
    return g_GameFlow.gym_enabled ? LV_GYM : -1;
}

void Gameflow_OverrideCommand(const GAMEFLOW_COMMAND command)
{
    switch (command.action) {
    case GF_START_GAME:
        g_GF_OverrideDir = GFD_START_GAME | command.param;
        break;
    case GF_START_SAVED_GAME:
        g_GF_OverrideDir = GFD_START_SAVED_GAME | command.param;
        break;
    case GF_START_CINE:
        g_GF_OverrideDir = GFD_START_CINE;
        break;
    case GF_START_FMV:
        g_GF_OverrideDir = GFD_START_FMV;
        break;
    case GF_START_DEMO:
        g_GF_OverrideDir = GFD_START_DEMO;
        break;
    case GF_EXIT_TO_TITLE:
        g_GF_OverrideDir = GFD_EXIT_TO_TITLE;
        break;
    case GF_LEVEL_COMPLETE:
        g_GF_OverrideDir = GFD_LEVEL_COMPLETE;
        break;
    case GF_EXIT_GAME:
        g_GF_OverrideDir = GFD_EXIT_GAME;
        break;
    default:
        LOG_ERROR("Not implemented");
        assert(false);
        break;
    }
}
