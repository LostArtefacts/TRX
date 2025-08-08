#include "game/items/walkable.h"

#include "game/game_buf.h"
#include "game/items.h"
#include "game/objects/vars.h"
#include "game/rooms.h"
#include "memory.h"
#include "vector.h"

#include <stdlib.h>
#include <string.h>

static SECTOR *M_GetItemPitSector(const ITEM *item, XYZ_32 pos);
static void M_InsertSorted(WALKABLE **walkables, WALKABLE *node);
void M_Add(int16_t item_num, const ITEM *item, XYZ_32 pos);

static SECTOR *M_GetItemPitSector(const ITEM *const item, const XYZ_32 pos)
{
    int16_t room_num = item->room_num;
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

void Walkable_Add(
    const int16_t item_num, const ITEM *const item, const XYZ_32 pos)
{
    SECTOR *const sector = M_GetItemPitSector(item, pos);

    // Check if the walkable is already in the sector.
    WALKABLE **walkables = &sector->walkable;
    while (*walkables) {
        if ((*walkables)->item_num == item_num) {
            return;
        }
        walkables = &(*walkables)->next;
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
    SECTOR *const sector = M_GetItemPitSector(item, item->pos);

    WALKABLE **walkables = &sector->walkable;
    while (*walkables) {
        WALKABLE *const node = *walkables;
        if (node->item_num == item_num) {
            *walkables = node->next;
            return;
        }
        walkables = &node->next;
    }
}

void Walkable_Reposition(const int16_t item_num, const XYZ_32 old_pos)
{
    const ITEM *const item = Item_Get(item_num);
    SECTOR *old_sector = M_GetItemPitSector(item, old_pos);

    // Unlink the walkable from the old position.
    WALKABLE **walkables = &old_sector->walkable;
    WALKABLE *node = nullptr;
    while (*walkables) {
        if ((*walkables)->item_num == item_num) {
            node = *walkables;
            *walkables = node->next;
            break;
        }
        walkables = &(*walkables)->next;
    }
    if (node == nullptr) {
        return;
    }

    // Update position of walkable.
    node->pos = item->pos;
    node->next = nullptr;

    // Link walkable to sector of the new position.
    SECTOR *new_sector = M_GetItemPitSector(item, item->pos);
    M_InsertSorted(&new_sector->walkable, node);
}

void Walkable_Shutdown(void)
{
    const int32_t room_count = Room_GetCount();
    for (int32_t r = 0; r < room_count; r++) {
        const ROOM *const room = Room_Get(r);

        const int32_t x_max = room->size.x;
        const int32_t z_max = room->size.z;

        SECTOR *sec = room->sectors;
        for (int32_t z = 0; z < z_max; z++)
            for (int32_t x = 0; x < x_max; x++, sec++) {
                sec->walkable = nullptr;
            }
    }

    GameBuf_ResetSingle(GBUF_WALKABLES);
}
