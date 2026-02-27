#include <trx/game/items/walkable.h>

#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/game/game_buf.h>
#include <trx/game/items.h>
#include <trx/game/objects/vars.h>
#include <trx/game/rooms.h>

#include <stdlib.h>
#include <string.h>

static SECTOR *M_GetItemPitSector(const XYZ_32 pos, int16_t room_num)
{
    SECTOR *const sector = Room_GetSector(pos, &room_num);
    return Room_GetPitSector(sector, pos.x, pos.z);
}

static VECTOR *M_GetCandidateSectors(const XYZ_32 base_pos, int16_t room_num)
{
    // Probe evenly around the centre position for cases where a walkable is
    // placed above a triangle portal, so detecting the correct sector at all
    // possible positions.
    const XYZ_32 mid_pos = {
        .x = ROUND_TO_SECTOR(base_pos.x) + STEP_L * 2,
        .y = base_pos.y,
        .z = ROUND_TO_SECTOR(base_pos.z) + STEP_L * 2,
    };

    const XZ_32 deltas[4] = {
        { -STEP_L, 0 },
        { STEP_L, 0 },
        { 0, -STEP_L },
        { 0, STEP_L },
    };

    VECTOR *const sectors = Vector_Create(sizeof(SECTOR *));
    for (int32_t i = 0; i < 4; i++) {
        const XZ_32 delta = deltas[i];
        const XYZ_32 pos = {
            .x = mid_pos.x + delta.x,
            .y = base_pos.y,
            .z = mid_pos.z + delta.z,
        };
        SECTOR *const sector = M_GetItemPitSector(pos, room_num);
        if (!Vector_Contains(sectors, &sector)) {
            Vector_Add(sectors, &sector);
        }
    }

    return sectors;
}

static bool M_SectorContainsWalkable(
    const SECTOR *const sector, const int16_t item_num)
{
    const WALKABLE *walkable = sector->walkable;
    while (walkable != nullptr) {
        if (walkable->item_num == item_num) {
            return true;
        }
        walkable = walkable->next;
    }
    return false;
}

static void M_InsertSorted(WALKABLE **walkables, WALKABLE *const node)
{
    while (*walkables != nullptr && (*walkables)->pos.y >= node->pos.y) {
        walkables = &(*walkables)->next;
    }

    node->next = *walkables;
    *walkables = node;
}

static void M_Remove(
    const int16_t item_num, const XYZ_32 pos, const int16_t room_num)
{
    VECTOR *const sectors = M_GetCandidateSectors(pos, room_num);

    // Unlink the walkable.
    for (int32_t i = 0; i < sectors->count; i++) {
        SECTOR *const sector = *(SECTOR **)Vector_Get(sectors, i);
        WALKABLE *walkable = sector->walkable;
        WALKABLE *prev = nullptr;
        while (walkable != nullptr) {
            if (walkable->item_num == item_num) {
                if (prev != nullptr) {
                    prev->next = walkable->next;
                } else {
                    sector->walkable = walkable->next;
                }
                break;
            }
            prev = walkable;
            walkable = walkable->next;
        }
    }

    Vector_Free(sectors);
}

void Walkable_Add(const int16_t item_num, const XYZ_32 pos)
{
    const ITEM *const item = Item_Get(item_num);
    VECTOR *const sectors = M_GetCandidateSectors(pos, item->room_num);

    for (int32_t i = 0; i < sectors->count; i++) {
        SECTOR *const sector = *(SECTOR **)Vector_Get(sectors, i);
        if (M_SectorContainsWalkable(sector, item_num)) {
            continue;
        }

        WALKABLE *const node = GameBuf_Alloc(sizeof(WALKABLE), GBUF_WALKABLES);
        node->item_num = item_num;
        node->pos = pos;
        node->next = nullptr;
        M_InsertSorted(&sector->walkable, node);
    }

    Vector_Free(sectors);
}

void Walkable_Remove(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    M_Remove(item_num, item->pos, item->room_num);
}

void Walkable_Reposition(
    const int16_t item_num, const GAME_VECTOR start, const GAME_VECTOR target)
{
    // Arena "leak"...
    M_Remove(item_num, start.pos, start.room_num);
    Walkable_Add(item_num, target.pos);
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
