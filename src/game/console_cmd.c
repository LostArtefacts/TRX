#include "game/console_cmd.h"

#include "config.h"
#include "game/console.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/random.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool Console_Cmd_Pos(const char *const input)
{
    if (strcmp(input, "pos") == 0) {
        if (!g_Objects[O_LARA].loaded) {
            return true;
        }
        Console_Log(
            "Room: %d  XYZ: %.3f, %.3f, %.3f  Rotation: %.3f,%.3f,%.3f ",
            g_LaraItem->room_number + 1, g_LaraItem->pos.x / (float)WALL_L,
            g_LaraItem->pos.y / (float)WALL_L,
            g_LaraItem->pos.z / (float)WALL_L,
            g_LaraItem->pos.x_rot * 360.0f / (float)PHD_ONE,
            g_LaraItem->pos.y_rot * 360.0f / (float)PHD_ONE,
            g_LaraItem->pos.z_rot * 360.0f / (float)PHD_ONE);
        return true;
    }

    return false;
}

static bool Console_Cmd_Teleport(const char *const input)
{
    {
        float x, y, z;
        if (sscanf(input, "tp %f %f %f", &x, &y, &z) == 3) {
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
        if (sscanf(input, "tp %hd", &room_num) == 1) {
            if (!g_Objects[O_LARA].loaded || !g_LaraItem->hit_points) {
                return true;
            }
            if (room_num < 1 || room_num > g_RoomCount) {
                Console_Log(
                    "Invalid room: %d. Valid rooms are 1-%d", room_num,
                    g_RoomCount);
                return true;
            }
            int16_t room_idx = room_num - 1;

            const ROOM_INFO *const room = &g_RoomInfo[room_idx];

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

static bool Console_Cmd_Fly(const char *const input)
{
    if (strcmp(input, "fly") == 0) {
        if (!g_Objects[O_LARA].loaded) {
            return true;
        }
        Console_Log("Fly mode enabled");
        Lara_EnterFlyMode();
        return true;
    }

    return false;
}

static bool Console_Cmd_Braid(const char *const input)
{
    if (strcmp(input, "braid off") == 0) {
        g_Config.enable_braid = 0;
        Console_Log("Braid disabled");
        return true;
    }

    if (strcmp(input, "braid on") == 0) {
        g_Config.enable_braid = 1;
        Console_Log("Braid enabled");
        return true;
    }

    return false;
}

static bool Console_Cmd_Cheats(const char *const input)
{
    if (strcmp(input, "cheats off") == 0) {
        g_Config.enable_cheats = 0;
        Console_Log("Cheats disabled");
        return true;
    }

    if (strcmp(input, "cheats on") == 0) {
        g_Config.enable_cheats = 1;
        Console_Log("Cheats enabled");
        return true;
    }

    return false;
}

ConsoleCmd g_ConsoleCommands[] = {
    Console_Cmd_Pos,   Console_Cmd_Teleport, Console_Cmd_Fly,
    Console_Cmd_Braid, Console_Cmd_Cheats,   NULL,
};
