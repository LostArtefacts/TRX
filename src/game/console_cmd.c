#include "game/console_cmd.h"

#include "config.h"
#include "game/console.h"
#include "game/effects/exploding_death.h"
#include "game/gameflow.h"
#include "game/inventory.h"
#include "game/inventory/inventory_vars.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/los.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/math.h"
#include "strings.h"
#include "util.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ENDS_WITH_ZERO(num) (fabsf((num)-roundf((num))) < 0.0001f)

typedef struct ITEM_NAME {
    const GAME_OBJECT_ID obj_id;
    const char *name;
    const INVENTORY_ITEM *inv_item;
} ITEM_NAME;

static const ITEM_NAME m_ItemNames[] = {
    { .obj_id = O_MEDI_ITEM, .name = "med" },
    { .obj_id = O_MEDI_ITEM, .name = "medi" },
    { .obj_id = O_MEDI_ITEM, .name = "small med" },
    { .obj_id = O_MEDI_ITEM, .name = "small medi" },
    { .obj_id = O_MEDI_ITEM, .name = "small medpack" },
    { .obj_id = O_MEDI_ITEM, .name = "small medipack" },
    { .obj_id = O_BIGMEDI_ITEM, .name = "big med" },
    { .obj_id = O_BIGMEDI_ITEM, .name = "big medi" },
    { .obj_id = O_BIGMEDI_ITEM, .name = "big medpack" },
    { .obj_id = O_BIGMEDI_ITEM, .name = "big medipack" },
    { .obj_id = O_BIGMEDI_ITEM, .name = "large med" },
    { .obj_id = O_BIGMEDI_ITEM, .name = "large medi" },
    { .obj_id = O_BIGMEDI_ITEM, .name = "large medpack" },
    { .obj_id = O_BIGMEDI_ITEM, .name = "large medipack" },
    { .obj_id = O_GUN_ITEM, .name = "pistols" },
    { .obj_id = O_SHOTGUN_ITEM, .name = "shotgun" },
    { .obj_id = O_MAGNUM_ITEM, .name = "magnum" },
    { .obj_id = O_MAGNUM_ITEM, .name = "magnums" },
    { .obj_id = O_UZI_ITEM, .name = "uzi" },
    { .obj_id = O_UZI_ITEM, .name = "uzis" },
    { .obj_id = O_GUN_AMMO_ITEM, .name = "pistol ammo" },
    { .obj_id = O_GUN_AMMO_ITEM, .name = "pistols ammo" },
    { .obj_id = O_SG_AMMO_ITEM, .name = "sg ammo" },
    { .obj_id = O_SG_AMMO_ITEM, .name = "shotgun ammo" },
    { .obj_id = O_MAG_AMMO_ITEM, .name = "magnum ammo" },
    { .obj_id = O_MAG_AMMO_ITEM, .name = "magnums ammo" },
    { .obj_id = O_MAG_AMMO_ITEM, .name = "magnum clip" },
    { .obj_id = O_MAG_AMMO_ITEM, .name = "magnums clip" },
    { .obj_id = O_MAG_AMMO_ITEM, .name = "magnum clips" },
    { .obj_id = O_MAG_AMMO_ITEM, .name = "magnums clips" },
    { .obj_id = O_UZI_AMMO_ITEM, .name = "uzi ammo" },
    { .obj_id = O_UZI_AMMO_ITEM, .name = "uzis ammo" },
    { .obj_id = O_UZI_AMMO_ITEM, .name = "uzi clip" },
    { .obj_id = O_UZI_AMMO_ITEM, .name = "uzis clip" },
    { .obj_id = O_UZI_AMMO_ITEM, .name = "uzi clips" },
    { .obj_id = O_UZI_AMMO_ITEM, .name = "uzis clips" },
    { .obj_id = O_KEY_ITEM1, .name = "key1" },
    { .obj_id = O_KEY_ITEM2, .name = "key2" },
    { .obj_id = O_KEY_ITEM3, .name = "key3" },
    { .obj_id = O_KEY_ITEM4, .name = "key4" },
    { .obj_id = O_PUZZLE_ITEM1, .name = "puzzle1" },
    { .obj_id = O_PUZZLE_ITEM2, .name = "puzzle2" },
    { .obj_id = O_PUZZLE_ITEM3, .name = "puzzle3" },
    { .obj_id = O_PUZZLE_ITEM4, .name = "puzzle4" },
    { .obj_id = O_PICKUP_ITEM1, .name = "pickup1" },
    { .obj_id = O_PICKUP_ITEM2, .name = "pickup2" },
    { .obj_id = O_LEADBAR_ITEM, .name = "leadbar" },
    { .obj_id = O_LEADBAR_ITEM, .name = "lead bar" },
    { .obj_id = O_SCION_ITEM, .name = "scion" },
    { .obj_id = O_MEDI_ITEM, .inv_item = &g_InvItemMedi },
    { .obj_id = O_BIGMEDI_ITEM, .inv_item = &g_InvItemBigMedi },
    { .obj_id = O_PUZZLE_ITEM1, .inv_item = &g_InvItemPuzzle1 },
    { .obj_id = O_PUZZLE_ITEM2, .inv_item = &g_InvItemPuzzle2 },
    { .obj_id = O_PUZZLE_ITEM3, .inv_item = &g_InvItemPuzzle3 },
    { .obj_id = O_PUZZLE_ITEM4, .inv_item = &g_InvItemPuzzle4 },
    { .obj_id = O_KEY_ITEM1, .inv_item = &g_InvItemKey1 },
    { .obj_id = O_KEY_ITEM2, .inv_item = &g_InvItemKey2 },
    { .obj_id = O_KEY_ITEM3, .inv_item = &g_InvItemKey3 },
    { .obj_id = O_KEY_ITEM4, .inv_item = &g_InvItemKey4 },
    { .obj_id = O_PICKUP_ITEM1, .inv_item = &g_InvItemPickup1 },
    { .obj_id = O_PICKUP_ITEM2, .inv_item = &g_InvItemPickup2 },
    { .obj_id = O_LEADBAR_ITEM, .inv_item = &g_InvItemLeadBar },
    { .obj_id = O_SCION_ITEM, .inv_item = &g_InvItemScion },
    { .obj_id = O_GUN_ITEM, .inv_item = &g_InvItemPistols },
    { .obj_id = O_SHOTGUN_ITEM, .inv_item = &g_InvItemShotgunAmmo },
    { .obj_id = O_MAGNUM_ITEM, .inv_item = &g_InvItemMagnum },
    { .obj_id = O_UZI_ITEM, .inv_item = &g_InvItemUzi },
    { .obj_id = O_GUN_AMMO_OPTION, .inv_item = &g_InvItemPistolAmmo },
    { .obj_id = O_SG_AMMO_ITEM, .inv_item = &g_InvItemShotgunAmmo },
    { .obj_id = O_MAG_AMMO_ITEM, .inv_item = &g_InvItemMagnumAmmo },
    { .obj_id = O_UZI_AMMO_ITEM, .inv_item = &g_InvItemUziAmmo },
    { .obj_id = NO_OBJECT },
};

static bool Console_Cmd_Pos(const char *const args)
{
    if (!g_Objects[O_LARA].loaded) {
        return false;
    }

    Console_Log(
        "Room: %d\nPosition: %.3f, %.3f, %.3f\nRotation: %.3f,%.3f,%.3f ",
        g_LaraItem->room_number, g_LaraItem->pos.x / (float)WALL_L,
        g_LaraItem->pos.y / (float)WALL_L, g_LaraItem->pos.z / (float)WALL_L,
        g_LaraItem->pos.x_rot * 360.0f / (float)PHD_ONE,
        g_LaraItem->pos.y_rot * 360.0f / (float)PHD_ONE,
        g_LaraItem->pos.z_rot * 360.0f / (float)PHD_ONE);
    return true;
}

static bool Console_Cmd_Teleport(const char *const args)
{
    if (!g_Objects[O_LARA].loaded || !g_LaraItem->hit_points) {
        return false;
    }

    {
        float x, y, z;
        if (sscanf(args, "%f %f %f", &x, &y, &z) == 3) {
            if (ENDS_WITH_ZERO(x)) {
                x += 0.5f;
            }
            if (ENDS_WITH_ZERO(z)) {
                z += 0.5f;
            }

            if (Item_Teleport(g_LaraItem, x * WALL_L, y * WALL_L, z * WALL_L)) {
                Console_Log("Teleported to position: %.3f %.3f %.3f", x, y, z);
                return true;
            }

            Console_Log(
                "Failed to teleport to position: %.3f %.3f %.3f", x, y, z);
            return true;
        }
    }

    {
        int16_t room_num = -1;
        if (sscanf(args, "%hd", &room_num) == 1) {
            if (room_num < 0 || room_num >= g_RoomCount) {
                Console_Log(
                    "Invalid room: %d. Valid rooms are 0-%d", room_num,
                    g_RoomCount - 1);
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
                    Console_Log("Teleported to room: %d", room_num);
                    return true;
                }
            }

            Console_Log("Failed to teleport to room: %d", room_num);
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
    Console_Log("Fly mode enabled");
    Lara_EnterFlyMode();
    return true;
}

static bool Console_Cmd_Braid(const char *const args)
{
    if (String_Equivalent(args, "off")) {
        g_Config.enable_braid = 0;
        Console_Log("Braid disabled");
        return true;
    }

    if (String_Equivalent(args, "on")) {
        g_Config.enable_braid = 1;
        Console_Log("Braid enabled");
        return true;
    }

    return false;
}

static bool Console_Cmd_Cheats(const char *const args)
{
    if (String_Equivalent(args, "off")) {
        g_Config.enable_cheats = 0;
        Console_Log("Cheats disabled");
        return true;
    }

    if (String_Equivalent(args, "on")) {
        g_Config.enable_cheats = 1;
        Console_Log("Cheats enabled");
        return true;
    }

    return false;
}

static bool Console_Cmd_GiveItem(const char *args)
{
    int32_t num = 1;
    if (sscanf(args, "%d ", &num) == 1) {
        args = strstr(args, " ");
        if (!args) {
            return false;
        }
        args++;
    }

    for (const ITEM_NAME *desc = m_ItemNames; desc->obj_id != NO_OBJECT;
         desc++) {
        const char *desc_name = NULL;
        if (desc->name) {
            desc_name = desc->name;
        } else if (desc->inv_item) {
            desc_name = desc->inv_item->string;
        } else {
            assert(false);
        }
        if (desc_name == NULL) {
            continue;
        }

        if (!String_Equivalent(args, desc_name)) {
            continue;
        }

        if (g_Objects[desc->obj_id].loaded) {
            Inv_AddItemNTimes(desc->obj_id, num);
            Console_Log("Added %s to Lara's inventory", desc_name);
        } else {
            Console_Log("This item is not currently available");
        }

        return true;
    }

    if (String_Equivalent(args, "keys")) {
        Inv_AddItem(O_PUZZLE_ITEM1);
        Inv_AddItem(O_PUZZLE_ITEM2);
        Inv_AddItem(O_PUZZLE_ITEM3);
        Inv_AddItem(O_PUZZLE_ITEM4);
        Inv_AddItem(O_KEY_ITEM1);
        Inv_AddItem(O_KEY_ITEM2);
        Inv_AddItem(O_KEY_ITEM3);
        Inv_AddItem(O_KEY_ITEM4);
        Inv_AddItem(O_PICKUP_ITEM1);
        Inv_AddItem(O_PICKUP_ITEM2);
        Console_Log("Added all keys to Lara's inventory");
        return true;
    }

    if (String_Equivalent(args, "guns")) {
        Inv_AddItem(O_GUN_ITEM);
        Inv_AddItem(O_MAGNUM_ITEM);
        Inv_AddItem(O_UZI_ITEM);
        Inv_AddItem(O_SHOTGUN_ITEM);
        g_Lara.shotgun.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 300;
        g_Lara.magnums.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 1000;
        g_Lara.uzis.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 2000;
        Console_Log("Added all guns to Lara's inventory");
        return true;
    }

    Console_Log("Unknown item: %s", args);
    return true;
}

static bool Console_Cmd_FlipMap(const char *args)
{
    bool flip = false;
    if (String_Equivalent(args, "on")) {
        if (g_FlipStatus) {
            Console_Log("Flipmap is already ON");
            return true;
        } else {
            flip = true;
        }
    }

    if (String_Equivalent(args, "off")) {
        if (!g_FlipStatus) {
            Console_Log("Flipmap is already OFF");
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
            Console_Log("Flipmap set to ON");
        } else {
            Console_Log("Flipmap set to OFF");
        }
        return true;
    }

    return false;
}

static bool Console_Cmd_Kill(const char *args)
{
    if (String_Equivalent(args, "all")) {
        int32_t num = 0;
        for (int16_t item_num = 0; item_num < g_LevelItemCount; item_num++) {
            struct ITEM_INFO *item = &g_Items[item_num];
            if (Object_IsObjectType(item->object_number, g_EnemyObjects)
                && item->hit_points > 0) {
                Effect_ExplodingDeath(item_num, -1, 0);
                Sound_Effect(SFX_EXPLOSION_CHEAT, &item->pos, SPM_NORMAL);
                Item_Kill(item_num);
                num++;
            }
        }
        if (num > 0) {
            Sound_Effect(SFX_EXPLOSION_CHEAT, &g_LaraItem->pos, SPM_NORMAL);
            Console_Log("Poof! %d enemies gone!", num);
        } else {
            Console_Log("Uh-oh, there are no enemies left to kill...", num);
        }
        return true;
    }

    if (strcmp(args, "") == 0) {
        int16_t best_item_num = NO_ITEM;
        int32_t best_dist = -1;
        for (int16_t item_num = 0; item_num < g_LevelItemCount; item_num++) {
            struct ITEM_INFO *item = &g_Items[item_num];
            if (Object_IsObjectType(item->object_number, g_EnemyObjects)
                && item->hit_points > 0) {
                GAME_VECTOR target;
                target.x = item->pos.x;
                target.y = item->pos.y - WALL_L;
                target.z = item->pos.z;
                target.room_number = item->room_number;

                GAME_VECTOR start;
                start.x = g_Camera.pos.x;
                start.y = g_Camera.pos.y;
                start.z = g_Camera.pos.z;
                start.room_number = g_Camera.pos.room_number;

                if (LOS_Check(&start, &target)) {
                    int32_t x = (item->pos.x - g_Camera.pos.x) >> WALL_SHIFT;
                    int32_t y = (item->pos.y - g_Camera.pos.y) >> WALL_SHIFT;
                    int32_t z = (item->pos.z - g_Camera.pos.z) >> WALL_SHIFT;
                    int32_t dist = Math_Sqrt(SQUARE(x) + SQUARE(y) + SQUARE(z));

                    if (best_dist == -1 || dist < best_dist) {
                        best_dist = dist;
                        best_item_num = item_num;
                    }
                }
            }
        }

        if (best_item_num != NO_ITEM) {
            struct ITEM_INFO *item = &g_Items[best_item_num];
            Effect_ExplodingDeath(best_item_num, -1, 0);
            Sound_Effect(SFX_EXPLOSION_CHEAT, &item->pos, SPM_NORMAL);
            Item_Kill(best_item_num);
            Console_Log("Bye-bye!");
        } else {
            Console_Log("No enemy in sight...");
        }
        return true;
    }

    return false;
}

static bool Console_Cmd_EndLevel(const char *args)
{
    if (strcmp(args, "") == 0) {
        g_LevelComplete = true;
        Console_Log("Level complete!");
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
        Console_Log("Invalid level");
        return true;
    }

    if (level_to_load != -1) {
        g_GameInfo.select_level_num = level_to_load;
        g_LevelComplete = true;
        Console_Log("Loading %s", g_GameFlow.levels[level_to_load].level_title);
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
    { .prefix = "pos", .proc = Console_Cmd_Pos },
    { .prefix = "tp", .proc = Console_Cmd_Teleport },
    { .prefix = "fly", .proc = Console_Cmd_Fly },
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
    { .prefix = "abortion", .proc = Console_Cmd_Abortion },
    { .prefix = NULL, .proc = NULL },
};
