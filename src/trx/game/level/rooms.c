#include <trx/game/game_buf.h>
#include <trx/game/rooms.h>
#include <trx/memory.h>

#include <string.h>

static void M_CollectOverlaps(void)
{
    int32_t total_rooms = Room_GetCount();
    for (int32_t i = 0; i < total_rooms; i++) {
        ROOM *const room = Room_Get(i);
        room->flags.overlapping = false;
    }
    for (int32_t i = 0; i < total_rooms; i++) {
        ROOM *const room1 = Room_Get(i);
        for (int32_t j = i + 1; j < total_rooms; j++) {
            if (Room_CheckOverlap(i, j)) {
                ROOM *const room2 = Room_Get(j);
                room1->flags.overlapping = true;
                room2->flags.overlapping = true;
            }
        }
    }
}

void Level_LoadRooms(void)
{
    M_CollectOverlaps();
}
