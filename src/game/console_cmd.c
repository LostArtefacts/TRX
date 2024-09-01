#include "game/console_cmd.h"

#include "config.h"
#include "config_map.h"
#include "game/clock.h"
#include "game/console.h"
#include "game/effects/exploding_death.h"
#include "game/game_string.h"
#include "game/gameflow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lara/lara_cheat.h"
#include "game/objects/common.h"
#include "game/objects/names.h"
#include "game/output.h"
#include "game/random.h"
#include "game/room.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/enum_str.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/memory.h>
#include <libtrx/strings.h>

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *Console_Cmd_Set_Resolve(const char *option_name);
static bool Console_Cmd_Set_SameKey(const char *key1, const char *key2);
static bool Console_Cmd_IsFloatRound(const float num);
static COMMAND_RESULT Console_Cmd_Fps(const char *const args);
static COMMAND_RESULT Console_Cmd_Pos(const char *const args);
static COMMAND_RESULT Console_Cmd_Teleport(const char *const args);
static COMMAND_RESULT Console_Cmd_SetHealth(const char *const args);
static COMMAND_RESULT Console_Cmd_Heal(const char *const args);
static COMMAND_RESULT Console_Cmd_Fly(const char *const args);
static COMMAND_RESULT Console_Cmd_Speed(const char *const args);
static COMMAND_RESULT Console_Cmd_VSync(const char *const args);
static COMMAND_RESULT Console_Cmd_Braid(const char *const args);
static COMMAND_RESULT Console_Cmd_Wireframe(const char *const args);
static COMMAND_RESULT Console_Cmd_Cheats(const char *const args);
static COMMAND_RESULT Console_Cmd_GiveItem(const char *args);
static COMMAND_RESULT Console_Cmd_FlipMap(const char *args);
static COMMAND_RESULT Console_Cmd_Kill(const char *args);
static COMMAND_RESULT Console_Cmd_LoadGame(const char *args);
static COMMAND_RESULT Console_Cmd_SaveGame(const char *args);
static COMMAND_RESULT Console_Cmd_StartDemo(const char *args);
static COMMAND_RESULT Console_Cmd_ExitToTitle(const char *args);
static COMMAND_RESULT Console_Cmd_EndLevel(const char *args);
static COMMAND_RESULT Console_Cmd_StartLevel(const char *args);
static COMMAND_RESULT Console_Cmd_Abortion(const char *args);
static COMMAND_RESULT Console_Cmd_Set(const char *const args);

static inline bool Console_Cmd_IsFloatRound(const float num)
{
    return (fabsf(num) - roundf(num)) < 0.0001f;
}

static COMMAND_RESULT Console_Cmd_Fps(const char *const args)
{
    if (String_Equivalent(args, "60")) {
        g_Config.rendering.fps = 60;
        Config_Write();
        Console_Log(GS(OSD_FPS_SET), g_Config.rendering.fps);
        return CR_SUCCESS;
    }

    if (String_Equivalent(args, "30")) {
        g_Config.rendering.fps = 30;
        Config_Write();
        Console_Log(GS(OSD_FPS_SET), g_Config.rendering.fps);
        return CR_SUCCESS;
    }

    if (String_Equivalent(args, "")) {
        Console_Log(GS(OSD_FPS_GET), g_Config.rendering.fps);
        return CR_SUCCESS;
    }

    return CR_BAD_INVOCATION;
}

static COMMAND_RESULT Console_Cmd_Pos(const char *const args)
{
    if (!g_Objects[O_LARA].loaded) {
        return CR_UNAVAILABLE;
    }

    Console_Log(
        GS(OSD_POS_GET), g_LaraItem->room_number,
        g_LaraItem->pos.x / (float)WALL_L, g_LaraItem->pos.y / (float)WALL_L,
        g_LaraItem->pos.z / (float)WALL_L,
        g_LaraItem->rot.x * 360.0f / (float)PHD_ONE,
        g_LaraItem->rot.y * 360.0f / (float)PHD_ONE,
        g_LaraItem->rot.z * 360.0f / (float)PHD_ONE);
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_Teleport(const char *const args)
{
    if (!g_Objects[O_LARA].loaded || !g_LaraItem->hit_points) {
        return CR_UNAVAILABLE;
    }

    // X Y Z
    {
        float x, y, z;
        if (sscanf(args, "%f %f %f", &x, &y, &z) == 3) {
            if (Console_Cmd_IsFloatRound(x)) {
                x += 0.5f;
            }
            if (Console_Cmd_IsFloatRound(z)) {
                z += 0.5f;
            }

            if (Item_Teleport(g_LaraItem, x * WALL_L, y * WALL_L, z * WALL_L)) {
                Console_Log(GS(OSD_POS_SET_POS), x, y, z);
                return CR_SUCCESS;
            }

            Console_Log(GS(OSD_POS_SET_POS_FAIL), x, y, z);
            return CR_FAILURE;
        }
    }

    // Room number
    {
        int16_t room_num = NO_ROOM;
        if (sscanf(args, "%hd", &room_num) == 1) {
            if (room_num < 0 || room_num >= g_RoomCount) {
                Console_Log(GS(OSD_INVALID_ROOM), room_num, g_RoomCount - 1);
                return CR_SUCCESS;
            }

            const ROOM_INFO *const room = &g_RoomInfo[room_num];

            const int32_t x1 = room->x + WALL_L;
            const int32_t x2 = (room->x_size << WALL_SHIFT) + room->x - WALL_L;
            const int32_t y1 = room->min_floor;
            const int32_t y2 = room->max_ceiling;
            const int32_t z1 = room->z + WALL_L;
            const int32_t z2 = (room->z_size << WALL_SHIFT) + room->z - WALL_L;

            for (int i = 0; i < 100; i++) {
                int32_t x = x1 + Random_GetControl() * (x2 - x1) / 0x7FFF;
                int32_t y = y1;
                int32_t z = z1 + Random_GetControl() * (z2 - z1) / 0x7FFF;
                if (Item_Teleport(g_LaraItem, x, y, z)) {
                    Console_Log(GS(OSD_POS_SET_ROOM), room_num);
                    return CR_SUCCESS;
                }
            }

            Console_Log(GS(OSD_POS_SET_ROOM_FAIL), room_num);
            return CR_FAILURE;
        }
    }

    // Nearest item of this name
    if (!String_Equivalent(args, "")) {
        int32_t match_count = 0;
        GAME_OBJECT_ID *matching_objs = Object_IdsFromName(args, &match_count);

        const ITEM_INFO *best_item = NULL;
        for (int i = 0; i < match_count; i++) {
            const GAME_OBJECT_ID obj_id = matching_objs[i];
            const bool is_pickup = Object_IsObjectType(obj_id, g_PickupObjects);

            bool matched = false;
            int32_t best_distance = INT32_MAX;
            for (int16_t item_num = 0; item_num < Item_GetTotalCount();
                 item_num++) {
                const ITEM_INFO *const item = &g_Items[item_num];
                if (item->object_number != obj_id
                    || (item->flags & IF_KILLED_ITEM)
                    || (is_pickup
                        && (item->status == IS_INVISIBLE
                            || item->status == IS_DEACTIVATED))) {
                    continue;
                }

                const int32_t distance =
                    Item_GetDistance(item, &g_LaraItem->pos);
                if (distance >= best_distance) {
                    continue;
                }

                best_distance = distance;
                best_item = item;
                matched = true;
            }

            if (matched) {
                // abort for the first matching item
                break;
            }
        }

        if (best_item != NULL) {
            Item_Teleport(
                g_LaraItem, best_item->pos.x, best_item->pos.y,
                best_item->pos.z);
            Console_Log(GS(OSD_POS_SET_ITEM), args);
            return CR_SUCCESS;
        } else {
            Console_Log(GS(OSD_POS_SET_ITEM_FAIL), args);
            return CR_FAILURE;
        }
    }

    return CR_BAD_INVOCATION;
}

static COMMAND_RESULT Console_Cmd_SetHealth(const char *const args)
{
    if (!g_Objects[O_LARA].loaded) {
        return CR_UNAVAILABLE;
    }

    if (String_Equivalent(args, "")) {
        Console_Log(GS(OSD_CURRENT_HEALTH_GET), g_LaraItem->hit_points);
        return CR_SUCCESS;
    }

    int32_t hp;
    if (sscanf(args, "%d", &hp) != 1) {
        return CR_BAD_INVOCATION;
    }

    g_LaraItem->hit_points = hp;
    Console_Log(GS(OSD_CURRENT_HEALTH_SET), hp);
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_Heal(const char *const args)
{
    if (!g_Objects[O_LARA].loaded) {
        return CR_UNAVAILABLE;
    }

    if (g_LaraItem->hit_points == LARA_MAX_HITPOINTS) {
        Console_Log(GS(OSD_HEAL_ALREADY_FULL_HP));
        return CR_SUCCESS;
    }

    g_LaraItem->hit_points = LARA_MAX_HITPOINTS;
    Console_Log(GS(OSD_HEAL_SUCCESS));
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_Fly(const char *const args)
{
    if (!g_Objects[O_LARA].loaded) {
        return CR_UNAVAILABLE;
    }
    Console_Log(GS(OSD_FLY_MODE_ON));
    Lara_Cheat_EnterFlyMode();
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_Speed(const char *const args)
{
    if (String_Equivalent(args, "")) {
        Console_Log(GS(OSD_SPEED_GET), Clock_GetTurboSpeed());
        return CR_SUCCESS;
    }

    int32_t num = -1;
    if (sscanf(args, "%d", &num) == 1) {
        Clock_SetTurboSpeed(num);
        return CR_SUCCESS;
    }

    return CR_BAD_INVOCATION;
}

static COMMAND_RESULT Console_Cmd_VSync(const char *const args)
{
    bool enable;
    if (!String_ParseBool(args, &enable)) {
        return CR_BAD_INVOCATION;
    }

    g_Config.rendering.enable_vsync = enable;
    Config_Write();
    Output_ApplyRenderSettings();
    Console_Log(enable ? GS(OSD_VSYNC_ON) : GS(OSD_VSYNC_OFF));
    return CR_SUCCESS;
}

static const char *Console_Cmd_Set_Resolve(const char *const option_name)
{
    const char *dot = strrchr(option_name, '.');
    if (dot) {
        return dot + 1;
    }
    return option_name;
}

static bool Console_Cmd_Set_SameKey(const char *key1, const char *key2)
{
    key1 = Console_Cmd_Set_Resolve(key1);
    key2 = Console_Cmd_Set_Resolve(key2);
    const size_t len1 = strlen(key1);
    const size_t len2 = strlen(key2);
    if (len1 != len2) {
        return false;
    }
    for (uint32_t i = 0; i < len1; i++) {
        char c1 = key1[i];
        char c2 = key2[i];
        if (c1 == '_') {
            c1 = '-';
        }
        if (c2 == '_') {
            c2 = '-';
        }
        if (c1 != c2) {
            return false;
        }
    }
    return true;
}

static COMMAND_RESULT Console_Cmd_Set(const char *const args)
{
    COMMAND_RESULT result = CR_BAD_INVOCATION;

    char *key = Memory_DupStr(args);
    char *space = strchr(key, ' ');
    char *new_value = NULL;
    if (space != NULL) {
        new_value = space + 1;
        space[0] = '\0'; // NULL-terminate the key
    }

    if (new_value != NULL) {
        for (const CONFIG_OPTION *option = g_ConfigOptionMap;
             option->name != NULL; option++) {
            if (!Console_Cmd_Set_SameKey(option->name, key)) {
                continue;
            }

            switch (option->type) {
            case COT_BOOL:
                if (String_Match(new_value, "on|true|1")) {
                    *(bool *)option->target = true;
                    result = CR_SUCCESS;
                } else if (String_Match(new_value, "off|false|0")) {
                    *(bool *)option->target = false;
                    result = CR_SUCCESS;
                }
                break;

            case COT_INT32: {
                int32_t new_value_typed;
                if (sscanf(new_value, "%d", &new_value_typed) == 1) {
                    *(int32_t *)option->target = new_value_typed;
                    result = CR_SUCCESS;
                }
                break;
            }

            case COT_FLOAT: {
                float new_value_typed;
                if (sscanf(new_value, "%f", &new_value_typed) == 1) {
                    *(float *)option->target = new_value_typed;
                    result = CR_SUCCESS;
                }
                break;
            }

            case COT_DOUBLE: {
                double new_value_typed;
                if (sscanf(new_value, "%lf", &new_value_typed) == 1) {
                    *(double *)option->target = new_value_typed;
                    result = CR_SUCCESS;
                }
                break;
            }

            case COT_ENUM:
                for (const ENUM_STRING_MAP *enum_map = option->param;
                     enum_map->text != NULL; enum_map++) {
                    if (String_Equivalent(enum_map->text, new_value)) {
                        *(int32_t *)option->target = enum_map->value;
                        result = CR_SUCCESS;
                        break;
                    }
                }
                break;
            }

            if (result == CR_SUCCESS) {
                Console_Log(GS(OSD_CONFIG_OPTION_SET), key, new_value);
                Config_Write();
            }
            break;
        }
    } else {
        for (const CONFIG_OPTION *option = g_ConfigOptionMap;
             option->name != NULL; option++) {
            if (!Console_Cmd_Set_SameKey(option->name, key)) {
                continue;
            }

            char cur_value[128] = "";
            assert(option->target != NULL);
            switch (option->type) {
            case COT_BOOL:
                sprintf(
                    cur_value, "%s",
                    *(bool *)option->target ? GS(MISC_ON) : GS(MISC_OFF));
                break;
            case COT_INT32:
                sprintf(cur_value, "%d", *(int32_t *)option->target);
                break;
            case COT_FLOAT:
                sprintf(cur_value, "%.2f", *(float *)option->target);
                break;
            case COT_DOUBLE:
                sprintf(cur_value, "%.2f", *(double *)option->target);
                break;
            case COT_ENUM:
                for (const ENUM_STRING_MAP *enum_map = option->param;
                     enum_map->text != NULL; enum_map++) {
                    if (enum_map->value == *(int32_t *)option->target) {
                        strcpy(cur_value, enum_map->text);
                    }
                }
                break;
            }
            Console_Log(GS(OSD_CONFIG_OPTION_GET), key, cur_value);
            result = CR_SUCCESS;
            break;
        }
    }

cleanup:
    Memory_FreePointer(&key);
    return result;
}

static COMMAND_RESULT Console_Cmd_Braid(const char *const args)
{
    bool enable;
    if (!String_ParseBool(args, &enable)) {
        return CR_BAD_INVOCATION;
    }

    g_Config.enable_braid = enable;
    Config_Write();
    Console_Log(enable ? GS(OSD_BRAID_ON) : GS(OSD_BRAID_OFF));
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_Wireframe(const char *const args)
{
    bool enable;
    if (!String_ParseBool(args, &enable)) {
        return CR_BAD_INVOCATION;
    }

    g_Config.rendering.enable_wireframe = enable;
    Config_Write();
    Console_Log(enable ? GS(OSD_WIREFRAME_ON) : GS(OSD_WIREFRAME_OFF));
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_Cheats(const char *const args)
{
    bool enable;
    if (!String_ParseBool(args, &enable)) {
        return CR_BAD_INVOCATION;
    }

    g_Config.enable_cheats = enable;
    Config_Write();
    Console_Log(enable ? GS(OSD_CHEATS_ON) : GS(OSD_CHEATS_OFF));
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_GiveItem(const char *args)
{
    if (g_LaraItem == NULL) {
        Console_Log(GS(OSD_INVALID_LEVEL), args);
        return CR_SUCCESS;
    }

    if (String_Equivalent(args, "keys")) {
        return Lara_Cheat_GiveAllKeys() ? CR_SUCCESS : CR_FAILURE;
    }

    if (String_Equivalent(args, "guns")) {
        return Lara_Cheat_GiveAllGuns() ? CR_SUCCESS : CR_FAILURE;
    }

    if (String_Equivalent(args, "all")) {
        return Lara_Cheat_GiveAllItems() ? CR_SUCCESS : CR_FAILURE;
    }

    int32_t num = 1;
    if (sscanf(args, "%d ", &num) == 1) {
        args = strstr(args, " ");
        if (!args) {
            return CR_BAD_INVOCATION;
        }
        args++;
    }

    if (String_Equivalent(args, "")) {
        return CR_BAD_INVOCATION;
    }

    bool found = false;
    int32_t match_count = 0;
    GAME_OBJECT_ID *matching_objs = Object_IdsFromName(args, &match_count);
    for (int32_t i = 0; i < match_count; i++) {
        const GAME_OBJECT_ID obj_id = matching_objs[i];
        if (Object_IsObjectType(obj_id, g_PickupObjects)) {
            if (g_Objects[obj_id].loaded) {
                Inv_AddItemNTimes(obj_id, num);
                Console_Log(
                    GS(OSD_GIVE_ITEM), Object_GetCanonicalName(obj_id, args));
                found = true;
            }
        }
    }
    Memory_FreePointer(&matching_objs);

    if (!found) {
        Console_Log(GS(OSD_INVALID_ITEM), args);
        return CR_FAILURE;
    }

    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_FlipMap(const char *args)
{
    bool new_state;
    if (String_Equivalent(args, "")) {
        new_state = !g_FlipStatus;
    } else if (!String_ParseBool(args, &new_state)) {
        return CR_BAD_INVOCATION;
    }

    if (g_FlipStatus == new_state) {
        Console_Log(
            new_state ? GS(OSD_FLIPMAP_FAIL_ALREADY_ON)
                      : GS(OSD_FLIPMAP_FAIL_ALREADY_OFF));
        return CR_SUCCESS;
    }

    Room_FlipMap();
    Console_Log(new_state ? GS(OSD_FLIPMAP_ON) : GS(OSD_FLIPMAP_OFF));
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_Kill(const char *args)
{
    // kill all the enemies in the level
    if (String_Equivalent(args, "all")) {
        int32_t num = 0;
        for (int16_t item_num = 0; item_num < Item_GetTotalCount();
             item_num++) {
            if (Lara_Cheat_KillEnemy(item_num)) {
                num++;
            }
        }

        if (num == 0) {
            Console_Log(GS(OSD_KILL_ALL_FAIL), num);
            return CR_FAILURE;
        }

        Sound_Effect(SFX_EXPLOSION_CHEAT, &g_LaraItem->pos, SPM_NORMAL);
        Console_Log(GS(OSD_KILL_ALL), num);
        return CR_SUCCESS;
    }

    // kill all the enemies around Lara within one tile, or a single nearest
    // enemy
    if (String_Equivalent(args, "")) {
        bool found = false;
        while (true) {
            const int16_t best_item_num = Lara_GetNearestEnemy();
            if (best_item_num == NO_ITEM) {
                break;
            }

            struct ITEM_INFO *item = &g_Items[best_item_num];
            const int32_t distance = Item_GetDistance(item, &g_LaraItem->pos);
            found |= Lara_Cheat_KillEnemy(best_item_num);
            if (distance >= WALL_L) {
                break;
            }
        }

        if (!found) {
            Console_Log(GS(OSD_KILL_FAIL));
            return CR_FAILURE;
        }

        Console_Log(GS(OSD_KILL));
        return CR_SUCCESS;
    }

    // kill a single enemy type
    {
        int32_t match_count = 0;
        GAME_OBJECT_ID *matching_objs = Object_IdsFromName(args, &match_count);
        bool matched = false;
        int32_t num = 0;
        for (int32_t i = 0; i < match_count; i++) {
            const GAME_OBJECT_ID obj_id = matching_objs[i];
            if (Object_IsObjectType(obj_id, g_EnemyObjects)) {
                matched = true;
                for (int16_t item_num = 0; item_num < Item_GetTotalCount();
                     item_num++) {
                    if (g_Items[item_num].object_number == obj_id
                        && Lara_Cheat_KillEnemy(item_num)) {
                        num++;
                    }
                }
            }
        }
        Memory_FreePointer(&matching_objs);

        if (!matched) {
            return CR_BAD_INVOCATION;
        }
        if (num == 0) {
            Console_Log(GS(OSD_KILL_ALL_FAIL));
            return CR_FAILURE;
        }
        Console_Log(GS(OSD_KILL_ALL), num);
        return CR_SUCCESS;
    }
}

static COMMAND_RESULT Console_Cmd_StartDemo(const char *args)
{
    g_GameInfo.override_gf_command =
        (GAMEFLOW_COMMAND) { .action = GF_START_DEMO };
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_ExitToTitle(const char *args)
{
    g_GameInfo.override_gf_command =
        (GAMEFLOW_COMMAND) { .action = GF_EXIT_TO_TITLE };
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_LoadGame(const char *args)
{
    int32_t slot_num = -1;
    if (sscanf(args, "%d", &slot_num) != 1) {
        return CR_BAD_INVOCATION;
    }
    const int32_t slot_idx = slot_num - 1; // convert 1-indexing to 0-indexing

    if (slot_idx < 0 || slot_idx >= g_Config.maximum_save_slots) {
        Console_Log(GS(OSD_LOAD_GAME_FAIL_INVALID_SLOT));
        return CR_FAILURE;
    }

    if (g_SavegameRequester.items[slot_idx].is_blocked) {
        Console_Log(GS(OSD_LOAD_GAME_FAIL_UNAVAILABLE_SLOT), slot_num);
        return CR_FAILURE;
    }

    g_GameInfo.override_gf_command = (GAMEFLOW_COMMAND) {
        .action = GF_START_SAVED_GAME,
        .param = slot_idx,
    };
    Console_Log(GS(OSD_LOAD_GAME), slot_num);
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_SaveGame(const char *args)
{
    int32_t slot_num = -1;
    if (sscanf(args, "%d", &slot_num) != 1) {
        return CR_BAD_INVOCATION;
    }
    const int32_t slot_idx = slot_num - 1; // convert 1-indexing to 0-indexing

    if (slot_idx < 0 || slot_idx >= g_Config.maximum_save_slots) {
        Console_Log(GS(OSD_SAVE_GAME_FAIL_INVALID_SLOT));
        return CR_BAD_INVOCATION;
    }

    if (g_LaraItem == NULL) {
        Console_Log(GS(OSD_SAVE_GAME_FAIL), slot_num);
        return CR_UNAVAILABLE;
    }

    Savegame_Save(slot_idx, &g_GameInfo);
    Console_Log(GS(OSD_SAVE_GAME), slot_num);
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_EndLevel(const char *const args)
{
    if (!String_Equivalent(args, "")) {
        return CR_BAD_INVOCATION;
    }

    Lara_Cheat_EndLevel();
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_StartLevel(const char *const args)
{
    if (String_Equivalent(args, "")) {
        return CR_BAD_INVOCATION;
    }

    int32_t level_to_load = -1;
    if (!String_ParseInteger(args, &level_to_load)) {
        for (int i = 0; i < g_GameFlow.level_count; i++) {
            if (String_CaseSubstring(g_GameFlow.levels[i].level_title, args)
                != NULL) {
                level_to_load = i;
                break;
            }
        }
        if (level_to_load == -1 && String_Equivalent(args, "gym")) {
            level_to_load = g_GameFlow.gym_level_num;
        }
        if (level_to_load == -1) {
            Console_Log(GS(OSD_INVALID_LEVEL));
            return CR_FAILURE;
        }
    }

    if (level_to_load == -1) {
        return CR_BAD_INVOCATION;
    }

    if (level_to_load >= g_GameFlow.level_count) {
        Console_Log(GS(OSD_INVALID_LEVEL));
        return CR_FAILURE;
    }

    g_GameInfo.override_gf_command = (GAMEFLOW_COMMAND) {
        .action = GF_SELECT_GAME,
        .param = level_to_load,
    };
    Console_Log(
        GS(OSD_PLAY_LEVEL), g_GameFlow.levels[level_to_load].level_title);
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_Abortion(const char *args)
{
    if (!g_Objects[O_LARA].loaded) {
        return CR_UNAVAILABLE;
    }

    if (g_LaraItem->hit_points <= 0) {
        return CR_UNAVAILABLE;
    }

    Effect_ExplodingDeath(g_Lara.item_number, -1, 0);
    Sound_Effect(SFX_EXPLOSION_CHEAT, &g_LaraItem->pos, SPM_NORMAL);
    Sound_Effect(SFX_LARA_FALL, &g_LaraItem->pos, SPM_NORMAL);
    g_LaraItem->hit_points = 0;
    g_LaraItem->flags |= IS_INVISIBLE;
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_ConsoleCommands[] = {
    { .prefix = "fps", .proc = Console_Cmd_Fps },
    { .prefix = "pos", .proc = Console_Cmd_Pos },
    { .prefix = "tp", .proc = Console_Cmd_Teleport },
    { .prefix = "hp", .proc = Console_Cmd_SetHealth },
    { .prefix = "heal", .proc = Console_Cmd_Heal },
    { .prefix = "fly", .proc = Console_Cmd_Fly },
    { .prefix = "speed", .proc = Console_Cmd_Speed },
    { .prefix = "vsync", .proc = Console_Cmd_VSync },
    { .prefix = "braid", .proc = Console_Cmd_Braid },
    { .prefix = "wireframe", .proc = Console_Cmd_Wireframe },
    { .prefix = "cheats", .proc = Console_Cmd_Cheats },
    { .prefix = "give", .proc = Console_Cmd_GiveItem },
    { .prefix = "set", .proc = Console_Cmd_Set },
    { .prefix = "gimme", .proc = Console_Cmd_GiveItem },
    { .prefix = "flip", .proc = Console_Cmd_FlipMap },
    { .prefix = "flipmap", .proc = Console_Cmd_FlipMap },
    { .prefix = "kill", .proc = Console_Cmd_Kill },
    { .prefix = "endlevel", .proc = Console_Cmd_EndLevel },
    { .prefix = "play", .proc = Console_Cmd_StartLevel },
    { .prefix = "level", .proc = Console_Cmd_StartLevel },
    { .prefix = "load", .proc = Console_Cmd_LoadGame },
    { .prefix = "save", .proc = Console_Cmd_SaveGame },
    { .prefix = "demo", .proc = Console_Cmd_StartDemo },
    { .prefix = "title", .proc = Console_Cmd_ExitToTitle },
    { .prefix = "abortion", .proc = Console_Cmd_Abortion },
    { .prefix = "natlastinks", .proc = Console_Cmd_Abortion },
    { .prefix = NULL, .proc = NULL },
};
