#include <trx/game/rooms/statics.h>

#include <trx/game/items/const.h>
#include <trx/game/objects/ids.h>
#include <trx/game/rooms/common.h>
#include <trx/game/rooms/floor_data.h>
#include <trx/game/rooms/geometry.h>

#include <string.h>

static uint64_t m_Shattered[MAX_ITEMS / 64] = {};

static bool M_IsValidDrawNum(const int16_t draw_num)
{
    return draw_num >= 0 && draw_num < MAX_ITEMS;
}

static bool M_IsShattered(const int16_t draw_num)
{
    if (!M_IsValidDrawNum(draw_num)) {
        return false;
    }
    return ((m_Shattered[draw_num >> 6] >> (draw_num & 63)) & 1ULL) != 0;
}

static void M_MarkShattered(const STATIC_MESH *const mesh)
{
    m_Shattered[mesh->draw_num >> 6] |= 1ULL << (mesh->draw_num & 63);

    int16_t room_num = mesh->room_num;
    SECTOR *const sector = Room_GetSector(mesh->pos, &room_num);
    sector->stopper = false;
}

void Room_ResetShatteredStatics(void)
{
    memset(m_Shattered, 0, sizeof(m_Shattered));
}

bool Room_IsStaticShattered(const STATIC_MESH *const mesh)
{
    return M_IsShattered(mesh->draw_num);
}

void Room_ShatterStatic(const STATIC_MESH *const mesh)
{
    if (!M_IsValidDrawNum(mesh->draw_num) || Room_IsStaticShattered(mesh)) {
        return;
    }

    M_MarkShattered(mesh);

    const ITEM trigger_item = {
        .object_id = NO_OBJECT,
        .pos = mesh->pos,
        .room_num = mesh->room_num,
    };
    Room_TestTriggersEx(&trigger_item, true);
}
