#include "game/console_cmd.h"

#include "config.h"
#include "game/console.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/random.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ENDS_WITH_ZERO(num) (fabsf((num)-roundf((num))) < 0.0001f)

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
    if (strcmp(args, "off") == 0) {
        g_Config.enable_braid = 0;
        Console_Log("Braid disabled");
        return true;
    }

    if (strcmp(args, "on") == 0) {
        g_Config.enable_braid = 1;
        Console_Log("Braid enabled");
        return true;
    }

    return false;
}

static bool Console_Cmd_Cheats(const char *const args)
{
    if (strcmp(args, "off") == 0) {
        g_Config.enable_cheats = 0;
        Console_Log("Cheats disabled");
        return true;
    }

    if (strcmp(args, "on") == 0) {
        g_Config.enable_cheats = 1;
        Console_Log("Cheats enabled");
        return true;
    }

    return false;
}

CONSOLE_COMMAND g_ConsoleCommands[] = {
    {
        .prefix = "pos",
        .proc = Console_Cmd_Pos,
    },

    {
        .prefix = "tp",
        .proc = Console_Cmd_Teleport,
    },

    {
        .prefix = "fly",
        .proc = Console_Cmd_Fly,
    },

    {
        .prefix = "braid",
        .proc = Console_Cmd_Braid,
    },

    {
        .prefix = "cheats",
        .proc = Console_Cmd_Cheats,
    },

    {
        .prefix = NULL,
        .proc = NULL,
    },
};
