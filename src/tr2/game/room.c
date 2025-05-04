#include "game/room.h"

void Room_InitCinematic(void)
{
    const int32_t room_count = Room_GetCount();
    for (int32_t i = 0; i < room_count; i++) {
        ROOM *const room = Room_Get(i);
        if (room->flipped_room != NO_ROOM_NEG) {
            Room_Get(room->flipped_room)->bound_active = 1;
        }
        room->flags |= RF_OUTSIDE;
    }

    Room_DrawReset();
    for (int32_t i = 0; i < room_count; i++) {
        if (!Room_Get(i)->bound_active) {
            Room_MarkToBeDrawn(i);
        }
    }
}
