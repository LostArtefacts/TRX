#include "game/items/walkable.h"

#include "game/game_buf.h"
#include "game/items.h"
#include "game/objects/vars.h"
#include "game/rooms.h"
#include "memory.h"
#include "vector.h"

#include <stdlib.h>
#include <string.h>

static SECTOR *M_GetItemPitSector(const XYZ_32 pos, int16_t room_num)
{
    SECTOR *const sector = Room_GetSector(pos.x, pos.y, pos.z, &room_num);
    return Room_GetPitSector(sector, pos.x, pos.z);
}

static void M_InsertSorted(WALKABLE **walkables, WALKABLE *const node)
{
    while (*walkables && (*walkables)->pos.y >= node->pos.y) {
        walkables = &(*walkables)->next;
    }

    node->next = *walkables;
    *walkables = node;
}

void Walkable_Add(const int16_t item_num, const XYZ_32 pos)
{
    const ITEM *const item = Item_Get(item_num);
    SECTOR *const sector = M_GetItemPitSector(pos, item->room_num);

    // Check if the walkable is already in the sector.
    const WALKABLE *walkable = sector->walkable;
    while (walkable != nullptr) {
        if (walkable->item_num == item_num) {
            return;
        }
        walkable = walkable->next;
    }

    WALKABLE *const node = GameBuf_Alloc(sizeof(WALKABLE), GBUF_WALKABLES);
    node->item_num = item_num;
    node->pos = pos;
    node->next = nullptr;
    M_InsertSorted(&sector->walkable, node);
}

void Walkable_Remove(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    SECTOR *const sector = M_GetItemPitSector(item->pos, item->room_num);

    // Unlink the walkable.
    WALKABLE *walkable = sector->walkable;
    WALKABLE *prev = nullptr;
    while (walkable != nullptr) {
        if (walkable->item_num == item_num) {
            if (prev) {
                prev->next = walkable->next;
            } else {
                sector->walkable = walkable->next;
            }
            return;
        }
        prev = walkable;
        walkable = walkable->next;
    }
}

void Walkable_Reposition(
    const int16_t item_num, const GAME_VECTOR start, const GAME_VECTOR target)
{
    const ITEM *const item = Item_Get(item_num);
    SECTOR *old_sector = M_GetItemPitSector(start.pos, start.room_num);

    // Find and unlink the walkable from the old position.
    WALKABLE *prev = nullptr;
    WALKABLE *walkable = old_sector->walkable;
    while (walkable != nullptr) {
        if (walkable->item_num == item_num) {
            if (prev) {
                prev->next = walkable->next;
            } else {
                old_sector->walkable = walkable->next;
            }
            break;
        }
        prev = walkable;
        walkable = walkable->next;
    }
    // Did not find the walkable.
    if (walkable == nullptr) {
        return;
    }

    // Update position of walkable.
    walkable->pos = target.pos;
    walkable->next = nullptr;

    // Link walkable to sector of the new position.
    SECTOR *new_sector = M_GetItemPitSector(walkable->pos, item->room_num);
    M_InsertSorted(&new_sector->walkable, walkable);
}

void Walkable_Reset(void)
{
    const int32_t room_count = Room_GetCount();
    for (int32_t r = 0; r < room_count; r++) {
        const ROOM *const room = Room_Get(r);

        const int32_t x_max = room->size.x;
        const int32_t z_max = room->size.z;

        SECTOR *sector = room->sectors;
        for (int32_t z = 0; z < z_max; z++) {
            for (int32_t x = 0; x < x_max; x++) {
                sector->walkable = nullptr;
                sector++;
            }
        }
    }

    GameBuf_ResetSingle(GBUF_WALKABLES);
}
