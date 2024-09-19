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

#include <libtrx/game/console/commands/config.h>
#include <libtrx/game/console/commands/pos.h>
#include <libtrx/game/console/commands/set_health.h>
#include <libtrx/game/console/common.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool Console_Cmd_IsFloatRound(const float num);
static COMMAND_RESULT Console_Cmd_Fps(const char *const args);
static COMMAND_RESULT Console_Cmd_VSync(const char *const args);
static COMMAND_RESULT Console_Cmd_Wireframe(const char *const args);
static COMMAND_RESULT Console_Cmd_Braid(const char *const args);
static COMMAND_RESULT Console_Cmd_Cheats(const char *const args);
static COMMAND_RESULT Console_Cmd_Teleport(const char *const args);
static COMMAND_RESULT Console_Cmd_Heal(const char *const args);
static COMMAND_RESULT Console_Cmd_Fly(const char *const args);
static COMMAND_RESULT Console_Cmd_Speed(const char *const args);
static COMMAND_RESULT Console_Cmd_GiveItem(const char *args);
static COMMAND_RESULT Console_Cmd_FlipMap(const char *args);
static COMMAND_RESULT Console_Cmd_Kill(const char *args);
static COMMAND_RESULT Console_Cmd_LoadGame(const char *args);
static COMMAND_RESULT Console_Cmd_SaveGame(const char *args);
static COMMAND_RESULT Console_Cmd_StartDemo(const char *args);
static COMMAND_RESULT Console_Cmd_ExitToTitle(const char *args);
static COMMAND_RESULT Console_Cmd_ExitGame(const char *args);
static COMMAND_RESULT Console_Cmd_EndLevel(const char *args);
static COMMAND_RESULT Console_Cmd_StartLevel(const char *args);
static COMMAND_RESULT Console_Cmd_Abortion(const char *args);

static inline bool Console_Cmd_IsFloatRound(const float num)
{
    return (fabsf(num) - roundf(num)) < 0.0001f;
}

static COMMAND_RESULT Console_Cmd_Fps(const char *const args)
{
    return Console_Cmd_Config_Helper(
        Console_Cmd_Config_GetOptionFromTarget(&g_Config.rendering.fps), args);
}

static COMMAND_RESULT Console_Cmd_VSync(const char *const args)
{
    return Console_Cmd_Config_Helper(
        Console_Cmd_Config_GetOptionFromTarget(
            &g_Config.rendering.enable_vsync),
        args);
}

static COMMAND_RESULT Console_Cmd_Wireframe(const char *const args)
{
    return Console_Cmd_Config_Helper(
        Console_Cmd_Config_GetOptionFromTarget(
            &g_Config.rendering.enable_wireframe),
        args);
}

static COMMAND_RESULT Console_Cmd_Braid(const char *const args)
{
    return Console_Cmd_Config_Helper(
        Console_Cmd_Config_GetOptionFromTarget(&g_Config.enable_braid), args);
}

static COMMAND_RESULT Console_Cmd_Cheats(const char *const args)
{
    return Console_Cmd_Config_Helper(
        Console_Cmd_Config_GetOptionFromTarget(&g_Config.enable_cheats), args);
}

static COMMAND_RESULT Console_Cmd_Teleport(const char *const args)
{
    if (g_GameInfo.current_level_type == GFL_TITLE
        || g_GameInfo.current_level_type == GFL_DEMO
        || g_GameInfo.current_level_type == GFL_CUTSCENE) {
        return CR_UNAVAILABLE;
    }

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

            if (Lara_Cheat_Teleport(x * WALL_L, y * WALL_L, z * WALL_L)) {
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
                if (Lara_Cheat_Teleport(x, y, z)) {
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
        int32_t best_distance = INT32_MAX;

        for (int16_t item_num = 0; item_num < Item_GetTotalCount();
             item_num++) {
            const ITEM_INFO *const item = &g_Items[item_num];
            const bool is_consumable =
                Object_IsObjectType(item->object_id, g_PickupObjects)
                || item->object_id == O_SAVEGAME_ITEM;
            if (is_consumable
                && (item->status == IS_INVISIBLE
                    || item->status == IS_DEACTIVATED)) {
                continue;
            }

            if (item->flags & IF_KILLED_ITEM) {
                continue;
            }

            bool is_matched = false;
            for (int32_t i = 0; i < match_count; i++) {
                if (matching_objs[i] == item->object_id) {
                    is_matched = true;
                    break;
                }
            }
            if (!is_matched) {
                continue;
            }

            const int32_t distance = Item_GetDistance(item, &g_LaraItem->pos);
            if (distance < best_distance) {
                best_distance = distance;
                best_item = item;
            }
        }

        if (best_item != NULL) {
            if (Lara_Cheat_Teleport(
                    best_item->pos.x, best_item->pos.y, best_item->pos.z)) {
                Console_Log(GS(OSD_POS_SET_ITEM), args);
            } else {
                Console_Log(GS(OSD_POS_SET_ITEM_FAIL), args);
            }
            return CR_SUCCESS;
        } else {
            Console_Log(GS(OSD_POS_SET_ITEM_FAIL), args);
            return CR_FAILURE;
        }
    }

    return CR_BAD_INVOCATION;
}

static COMMAND_RESULT Console_Cmd_Heal(const char *const args)
{
    if (g_GameInfo.current_level_type == GFL_TITLE
        || g_GameInfo.current_level_type == GFL_DEMO
        || g_GameInfo.current_level_type == GFL_CUTSCENE) {
        return CR_UNAVAILABLE;
    }

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
    if (g_GameInfo.current_level_type == GFL_TITLE
        || g_GameInfo.current_level_type == GFL_DEMO
        || g_GameInfo.current_level_type == GFL_CUTSCENE) {
        return CR_UNAVAILABLE;
    }

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

static COMMAND_RESULT Console_Cmd_GiveItem(const char *args)
{
    if (g_GameInfo.current_level_type == GFL_TITLE
        || g_GameInfo.current_level_type == GFL_DEMO
        || g_GameInfo.current_level_type == GFL_CUTSCENE) {
        return CR_UNAVAILABLE;
    }

    if (g_LaraItem == NULL) {
        return CR_UNAVAILABLE;
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
        const GAME_OBJECT_ID object_id = matching_objs[i];
        if (Object_IsObjectType(object_id, g_PickupObjects)) {
            if (g_Objects[object_id].loaded) {
                Inv_AddItemNTimes(object_id, num);
                Console_Log(
                    GS(OSD_GIVE_ITEM),
                    Object_GetCanonicalName(object_id, args));
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
    if (g_GameInfo.current_level_type == GFL_TITLE
        || g_GameInfo.current_level_type == GFL_DEMO
        || g_GameInfo.current_level_type == GFL_CUTSCENE) {
        return CR_UNAVAILABLE;
    }

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
    if (g_GameInfo.current_level_type == GFL_TITLE
        || g_GameInfo.current_level_type == GFL_DEMO
        || g_GameInfo.current_level_type == GFL_CUTSCENE) {
        return CR_UNAVAILABLE;
    }

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

            ITEM_INFO *const item = &g_Items[best_item_num];
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
            const GAME_OBJECT_ID object_id = matching_objs[i];
            if (Object_IsObjectType(object_id, g_EnemyObjects)) {
                matched = true;
                for (int16_t item_num = 0; item_num < Item_GetTotalCount();
                     item_num++) {
                    if (g_Items[item_num].object_id == object_id
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

static COMMAND_RESULT Console_Cmd_ExitGame(const char *args)
{
    g_GameInfo.override_gf_command =
        (GAMEFLOW_COMMAND) { .action = GF_EXIT_GAME };
    return CR_SUCCESS;
}

static COMMAND_RESULT Console_Cmd_LoadGame(const char *args)
{
    int32_t slot_num;
    if (!String_ParseInteger(args, &slot_num)) {
        return CR_BAD_INVOCATION;
    }

    const int32_t slot_idx = slot_num - 1; // convert 1-indexing to 0-indexing

    if (slot_idx < 0 || slot_idx >= g_Config.maximum_save_slots) {
        Console_Log(GS(OSD_LOAD_GAME_FAIL_INVALID_SLOT), slot_num);
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
    if (g_GameInfo.current_level_type == GFL_TITLE
        || g_GameInfo.current_level_type == GFL_DEMO
        || g_GameInfo.current_level_type == GFL_CUTSCENE) {
        return CR_UNAVAILABLE;
    }

    int32_t slot_num;
    if (!String_ParseInteger(args, &slot_num)) {
        return CR_BAD_INVOCATION;
    }
    const int32_t slot_idx = slot_num - 1; // convert 1-indexing to 0-indexing

    if (slot_idx < 0 || slot_idx >= g_Config.maximum_save_slots) {
        Console_Log(GS(OSD_SAVE_GAME_FAIL_INVALID_SLOT), slot_num);
        return CR_BAD_INVOCATION;
    }

    if (g_LaraItem == NULL) {
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

    Effect_ExplodingDeath(g_Lara.item_num, -1, 0);
    Sound_Effect(SFX_EXPLOSION_CHEAT, &g_LaraItem->pos, SPM_NORMAL);
    Sound_Effect(SFX_LARA_FALL, &g_LaraItem->pos, SPM_NORMAL);
    g_LaraItem->hit_points = 0;
    g_LaraItem->flags |= IS_INVISIBLE;
    return CR_SUCCESS;
}

CONSOLE_COMMAND *g_ConsoleCommands[] = {
    &(CONSOLE_COMMAND) { .prefix = "fps", .proc = Console_Cmd_Fps },
    &(CONSOLE_COMMAND) { .prefix = "vsync", .proc = Console_Cmd_VSync },
    &(CONSOLE_COMMAND) { .prefix = "wireframe", .proc = Console_Cmd_Wireframe },
    &(CONSOLE_COMMAND) { .prefix = "braid", .proc = Console_Cmd_Braid },
    &(CONSOLE_COMMAND) { .prefix = "cheats", .proc = Console_Cmd_Cheats },
    &(CONSOLE_COMMAND) { .prefix = "tp", .proc = Console_Cmd_Teleport },
    &(CONSOLE_COMMAND) { .prefix = "heal", .proc = Console_Cmd_Heal },
    &(CONSOLE_COMMAND) { .prefix = "fly", .proc = Console_Cmd_Fly },
    &(CONSOLE_COMMAND) { .prefix = "speed", .proc = Console_Cmd_Speed },
    &(CONSOLE_COMMAND) { .prefix = "give", .proc = Console_Cmd_GiveItem },
    &(CONSOLE_COMMAND) { .prefix = "gimme", .proc = Console_Cmd_GiveItem },
    &(CONSOLE_COMMAND) { .prefix = "flip", .proc = Console_Cmd_FlipMap },
    &(CONSOLE_COMMAND) { .prefix = "flipmap", .proc = Console_Cmd_FlipMap },
    &(CONSOLE_COMMAND) { .prefix = "kill", .proc = Console_Cmd_Kill },
    &(CONSOLE_COMMAND) { .prefix = "endlevel", .proc = Console_Cmd_EndLevel },
    &(CONSOLE_COMMAND) { .prefix = "play", .proc = Console_Cmd_StartLevel },
    &(CONSOLE_COMMAND) { .prefix = "level", .proc = Console_Cmd_StartLevel },
    &(CONSOLE_COMMAND) { .prefix = "load", .proc = Console_Cmd_LoadGame },
    &(CONSOLE_COMMAND) { .prefix = "save", .proc = Console_Cmd_SaveGame },
    &(CONSOLE_COMMAND) { .prefix = "demo", .proc = Console_Cmd_StartDemo },
    &(CONSOLE_COMMAND) { .prefix = "title", .proc = Console_Cmd_ExitToTitle },
    &(CONSOLE_COMMAND) { .prefix = "exit", .proc = Console_Cmd_ExitGame },
    &(CONSOLE_COMMAND) { .prefix = "abortion", .proc = Console_Cmd_Abortion },
    &(CONSOLE_COMMAND) { .prefix = "natlastinks",
                         .proc = Console_Cmd_Abortion },
    &g_Console_Cmd_Pos,
    &g_Console_Cmd_SetHealth,
    &g_Console_Cmd_Config,
    NULL,
};
