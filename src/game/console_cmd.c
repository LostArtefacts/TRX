#include "game/console_cmd.h"

#include "config.h"
#include "game/clock.h"
#include "game/console.h"
#include "game/effects/exploding_death.h"
#include "game/game_string.h"
#include "game/gameflow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lara/lara_cheat.h"
#include "game/los.h"
#include "game/objects/common.h"
#include "game/objects/names.h"
#include "game/output.h"
#include "game/random.h"
#include "game/room.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/math.h"

#include <libtrx/memory.h>
#include <libtrx/strings.h>
#include <libtrx/utils.h>

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool Console_Cmd_IsFloatRound(const float num);
static bool Console_Cmd_Fps(const char *const args);
static bool Console_Cmd_Pos(const char *const args);
static bool Console_Cmd_Teleport(const char *const args);
static bool Console_Cmd_Fly(const char *const args);
static bool Console_Cmd_Speed(const char *const args);
static bool Console_Cmd_VSync(const char *const args);
static bool Console_Cmd_Braid(const char *const args);
static bool Console_Cmd_Cheats(const char *const args);
static bool Console_Cmd_GiveItem(const char *args);
static bool Console_Cmd_FlipMap(const char *args);
static bool Console_Cmd_Kill(const char *args);
static bool Console_Cmd_LoadGame(const char *args);
static bool Console_Cmd_SaveGame(const char *args);
static bool Console_Cmd_StartDemo(const char *args);
static bool Console_Cmd_ExitToTitle(const char *args);
static bool Console_Cmd_EndLevel(const char *args);
static bool Console_Cmd_Level(const char *args);
static bool Console_Cmd_Abortion(const char *args);

static inline bool Console_Cmd_IsFloatRound(const float num)
{
    return (fabsf(num) - roundf(num)) < 0.0001f;
}

static bool Console_Cmd_Fps(const char *const args)
{
    if (String_Equivalent(args, "60")) {
        g_Config.rendering.fps = 60;
        Config_Write();
        Console_Log(GS(OSD_FPS_SET), g_Config.rendering.fps);
        return true;
    }

    if (String_Equivalent(args, "30")) {
        g_Config.rendering.fps = 30;
        Config_Write();
        Console_Log(GS(OSD_FPS_SET), g_Config.rendering.fps);
        return true;
    }

    if (String_Equivalent(args, "")) {
        Console_Log(GS(OSD_FPS_GET), g_Config.rendering.fps);
        return true;
    }

    return false;
}

static bool Console_Cmd_Pos(const char *const args)
{
    if (!g_Objects[O_LARA].loaded) {
        return false;
    }

    Console_Log(
        GS(OSD_POS_GET), g_LaraItem->room_number,
        g_LaraItem->pos.x / (float)WALL_L, g_LaraItem->pos.y / (float)WALL_L,
        g_LaraItem->pos.z / (float)WALL_L,
        g_LaraItem->rot.x * 360.0f / (float)PHD_ONE,
        g_LaraItem->rot.y * 360.0f / (float)PHD_ONE,
        g_LaraItem->rot.z * 360.0f / (float)PHD_ONE);
    return true;
}

static bool Console_Cmd_Teleport(const char *const args)
{
    if (!g_Objects[O_LARA].loaded || !g_LaraItem->hit_points) {
        return false;
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
                return true;
            }

            Console_Log(GS(OSD_POS_SET_POS_FAIL), x, y, z);
            return true;
        }
    }

    // Room number
    {
        int16_t room_num = NO_ROOM;
        if (sscanf(args, "%hd", &room_num) == 1) {
            if (room_num < 0 || room_num >= g_RoomCount) {
                Console_Log(GS(OSD_INVALID_ROOM), room_num, g_RoomCount - 1);
                return true;
            }

            const ROOM_INFO *const room = &g_RoomInfo[room_num];

            const int32_t x1 = room->x + WALL_L;
            const int32_t x2 = (room->y_size << WALL_SHIFT) + room->x - WALL_L;
            const int32_t y1 = room->min_floor;
            const int32_t y2 = room->max_ceiling;
            const int32_t z1 = room->z + WALL_L;
            const int32_t z2 = (room->x_size << WALL_SHIFT) + room->z - WALL_L;

            for (int i = 0; i < 100; i++) {
                int32_t x = x1 + Random_GetControl() * (x2 - x1) / 0x7FFF;
                int32_t y = y1;
                int32_t z = z1 + Random_GetControl() * (z2 - z1) / 0x7FFF;
                if (Item_Teleport(g_LaraItem, x, y, z)) {
                    Console_Log(GS(OSD_POS_SET_ROOM), room_num);
                    return true;
                }
            }

            Console_Log(GS(OSD_POS_SET_ROOM_FAIL), room_num);
            return true;
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
            return true;
        } else {
            Console_Log(GS(OSD_POS_SET_ITEM_FAIL), args);
            return true;
        }
    }

    return false;
}

static bool Console_Cmd_Fly(const char *const args)
{
    if (!g_Objects[O_LARA].loaded) {
        return false;
    }
    Console_Log(GS(OSD_FLY_MODE_ON));
    Lara_Cheat_EnterFlyMode();
    return true;
}

static bool Console_Cmd_Speed(const char *const args)
{
    if (strcmp(args, "") == 0) {
        Console_Log(GS(OSD_SPEED_GET), Clock_GetTurboSpeed());
        return true;
    }

    int32_t num = -1;
    if (sscanf(args, "%d", &num) == 1) {
        Clock_SetTurboSpeed(num);
        return true;
    }

    return false;
}

static bool Console_Cmd_VSync(const char *const args)
{
    if (String_Equivalent(args, "off")) {
        g_Config.rendering.enable_vsync = false;
        Config_Write();
        Output_ApplyRenderSettings();
        Console_Log(GS(OSD_VSYNC_OFF));
        return true;
    }

    if (String_Equivalent(args, "on")) {
        g_Config.rendering.enable_vsync = true;
        Config_Write();
        Output_ApplyRenderSettings();
        Console_Log(GS(OSD_VSYNC_ON));
        return true;
    }

    return false;
}

static bool Console_Cmd_Braid(const char *const args)
{
    if (String_Equivalent(args, "off")) {
        g_Config.enable_braid = false;
        Config_Write();
        Console_Log(GS(OSD_BRAID_OFF));
        return true;
    }

    if (String_Equivalent(args, "on")) {
        g_Config.enable_braid = true;
        Config_Write();
        Console_Log(GS(OSD_BRAID_ON));
        return true;
    }

    return false;
}

static bool Console_Cmd_Cheats(const char *const args)
{
    if (String_Equivalent(args, "off")) {
        g_Config.enable_cheats = false;
        Config_Write();
        Console_Log(GS(OSD_CHEATS_OFF));
        return true;
    }

    if (String_Equivalent(args, "on")) {
        g_Config.enable_cheats = true;
        Config_Write();
        Console_Log(GS(OSD_CHEATS_ON));
        return true;
    }

    return false;
}

static bool Console_Cmd_GiveItem(const char *args)
{
    if (g_LaraItem == NULL) {
        Console_Log(GS(OSD_INVALID_LEVEL), args);
        return true;
    }

    if (String_Equivalent(args, "keys")) {
        return Lara_Cheat_GiveAllKeys();
    }

    if (String_Equivalent(args, "guns")) {
        return Lara_Cheat_GiveAllGuns();
    }

    if (String_Equivalent(args, "all")) {
        return Lara_Cheat_GiveAllItems();
    }

    int32_t num = 1;
    if (sscanf(args, "%d ", &num) == 1) {
        args = strstr(args, " ");
        if (!args) {
            return false;
        }
        args++;
    }

    if (strcmp(args, "") == 0) {
        return false;
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
            } else {
                Console_Log(
                    GS(OSD_UNAVAILABLE_ITEM),
                    Object_GetCanonicalName(obj_id, args));
            }
            found = true;
        }
    }
    Memory_FreePointer(&matching_objs);

    if (found) {
        return true;
    }

    Console_Log(GS(OSD_INVALID_ITEM), args);
    return true;
}

static bool Console_Cmd_FlipMap(const char *args)
{
    bool flip = false;
    if (String_Equivalent(args, "on")) {
        if (g_FlipStatus) {
            Console_Log(GS(OSD_FLIPMAP_FAIL_ALREADY_OFF));
            return true;
        } else {
            flip = true;
        }
    }

    if (String_Equivalent(args, "off")) {
        if (!g_FlipStatus) {
            Console_Log(GS(OSD_FLIPMAP_FAIL_ALREADY_OFF));
            return true;
        } else {
            flip = true;
        }
    }

    if (strcmp(args, "") == 0) {
        flip = true;
    }

    if (flip) {
        Room_FlipMap();
        if (g_FlipStatus) {
            Console_Log(GS(OSD_FLIPMAP_ON));
        } else {
            Console_Log(GS(OSD_FLIPMAP_OFF));
        }
        return true;
    }

    return false;
}

static bool Console_Cmd_Kill(const char *args)
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
        if (num > 0) {
            Sound_Effect(SFX_EXPLOSION_CHEAT, &g_LaraItem->pos, SPM_NORMAL);
            Console_Log(GS(OSD_KILL_ALL), num);
        } else {
            Console_Log(GS(OSD_KILL_ALL_FAIL), num);
        }
        return true;
    }

    // kill all the enemies around Lara within one tile, or a single nearest
    // enemy
    if (strcmp(args, "") == 0) {
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

        if (found) {
            Console_Log(GS(OSD_KILL));
        } else {
            Console_Log(GS(OSD_KILL_FAIL));
        }
        return true;
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
        if (matched) {
            if (num > 0) {
                Console_Log(GS(OSD_KILL_ALL), num);
            } else {
                Console_Log(GS(OSD_KILL_ALL_FAIL));
            }
            return true;
        }
    }

    return false;
}

static bool Console_Cmd_StartDemo(const char *args)
{
    g_GameInfo.override_option = GF_START_DEMO;
    return true;
}

static bool Console_Cmd_ExitToTitle(const char *args)
{
    g_GameInfo.override_option = GF_EXIT_TO_TITLE;
    return true;
}

static bool Console_Cmd_LoadGame(const char *args)
{
    int32_t slot_num = -1;
    if (sscanf(args, "%d", &slot_num) != 1) {
        return false;
    }
    const int32_t slot_idx = slot_num - 1; // convert 1-indexing to 0-indexing

    if (slot_idx < 0 || slot_idx >= g_Config.maximum_save_slots) {
        Console_Log(GS(OSD_LOAD_GAME_FAIL_INVALID_SLOT));
        return true;
    }

    if ((g_SavegameRequester.item_flags[slot_idx] & RIF_BLOCKED)) {
        Console_Log(GS(OSD_LOAD_GAME_FAIL_UNAVAILABLE_SLOT), slot_num);
        return true;
    }

    g_GameInfo.override_option = GF_START_SAVED_GAME | slot_idx;
    Console_Log(GS(OSD_LOAD_GAME), slot_num);
    return true;
}

static bool Console_Cmd_SaveGame(const char *args)
{
    int32_t slot_num = -1;
    if (sscanf(args, "%d", &slot_num) != 1) {
        return false;
    }
    const int32_t slot_idx = slot_num - 1; // convert 1-indexing to 0-indexing

    if (slot_idx < 0 || slot_idx >= g_Config.maximum_save_slots) {
        Console_Log(GS(OSD_SAVE_GAME_FAIL_INVALID_SLOT));
        return true;
    }

    if (g_LaraItem == NULL) {
        Console_Log(GS(OSD_SAVE_GAME_FAIL), slot_num);
        return true;
    }

    Savegame_Save(slot_idx, &g_GameInfo);
    Console_Log(GS(OSD_SAVE_GAME), slot_num);
    return true;
}

static bool Console_Cmd_EndLevel(const char *args)
{
    if (strcmp(args, "") == 0) {
        Lara_Cheat_EndLevel();
        return true;
    }
    return false;
}

static bool Console_Cmd_Level(const char *args)
{
    int32_t level_to_load = -1;

    if (level_to_load == -1) {
        int32_t num = 0;
        if (sscanf(args, "%d", &num) == 1) {
            level_to_load = num;
        }
    }

    if (level_to_load == -1 && strlen(args) >= 2) {
        for (int i = 0; i < g_GameFlow.level_count; i++) {
            if (String_CaseSubstring(g_GameFlow.levels[i].level_title, args)
                != NULL) {
                level_to_load = i;
                break;
            }
        }
    }

    if (level_to_load == -1 && String_Equivalent(args, "gym")) {
        level_to_load = g_GameFlow.gym_level_num;
    }

    if (level_to_load >= g_GameFlow.level_count) {
        Console_Log(GS(OSD_INVALID_LEVEL));
        return true;
    }

    if (level_to_load != -1) {
        g_GameInfo.override_option = GF_SELECT_GAME | level_to_load;
        Console_Log(
            GS(OSD_PLAY_LEVEL), g_GameFlow.levels[level_to_load].level_title);
        return true;
    }

    return false;
}

static bool Console_Cmd_Abortion(const char *args)
{
    if (!g_Objects[O_LARA].loaded) {
        return false;
    }

    if (g_LaraItem->hit_points <= 0) {
        return true;
    }

    Effect_ExplodingDeath(g_Lara.item_number, -1, 0);
    Sound_Effect(SFX_EXPLOSION_CHEAT, &g_LaraItem->pos, SPM_NORMAL);
    Sound_Effect(SFX_LARA_FALL, &g_LaraItem->pos, SPM_NORMAL);
    g_LaraItem->hit_points = 0;
    g_LaraItem->flags |= IS_INVISIBLE;
    return true;
}

CONSOLE_COMMAND g_ConsoleCommands[] = {
    { .prefix = "fps", .proc = Console_Cmd_Fps },
    { .prefix = "pos", .proc = Console_Cmd_Pos },
    { .prefix = "tp", .proc = Console_Cmd_Teleport },
    { .prefix = "fly", .proc = Console_Cmd_Fly },
    { .prefix = "speed", .proc = Console_Cmd_Speed },
    { .prefix = "vsync", .proc = Console_Cmd_VSync },
    { .prefix = "braid", .proc = Console_Cmd_Braid },
    { .prefix = "cheats", .proc = Console_Cmd_Cheats },
    { .prefix = "give", .proc = Console_Cmd_GiveItem },
    { .prefix = "gimme", .proc = Console_Cmd_GiveItem },
    { .prefix = "flip", .proc = Console_Cmd_FlipMap },
    { .prefix = "flipmap", .proc = Console_Cmd_FlipMap },
    { .prefix = "kill", .proc = Console_Cmd_Kill },
    { .prefix = "endlevel", .proc = Console_Cmd_EndLevel },
    { .prefix = "play", .proc = Console_Cmd_Level },
    { .prefix = "level", .proc = Console_Cmd_Level },
    { .prefix = "load", .proc = Console_Cmd_LoadGame },
    { .prefix = "save", .proc = Console_Cmd_SaveGame },
    { .prefix = "demo", .proc = Console_Cmd_StartDemo },
    { .prefix = "title", .proc = Console_Cmd_ExitToTitle },
    { .prefix = "abortion", .proc = Console_Cmd_Abortion },
    { .prefix = "natlastinks", .proc = Console_Cmd_Abortion },
    { .prefix = NULL, .proc = NULL },
};
