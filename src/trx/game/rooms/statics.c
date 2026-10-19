#include <trx/game/rooms/statics.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
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

static void M_SetShattered(const int16_t draw_num)
{
    m_Shattered[draw_num >> 6] |= 1ULL << (draw_num & 63);
}

static void M_ClearStopper(const STATIC_MESH *const mesh)
{
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

    M_SetShattered(mesh->draw_num);
    M_ClearStopper(mesh);

    const ITEM trigger_item = {
        .object_id = NO_OBJECT,
        .pos = mesh->pos,
        .room_num = mesh->room_num,
    };
    Room_TestTriggersEx(&trigger_item, true);
}

void Room_SaveShatteredStatics(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    for (int16_t draw_num = 0; draw_num < MAX_ITEMS; draw_num++) {
        if (M_IsShattered(draw_num)) {
            JSONW_PUSH_VALUE(io, draw_num);
            JSONW_POP_AND_APPEND(io);
        }
    }
    JSONW_POP_AND_SET_NZ(io, "shattered_statics");
}

RESULT Room_LoadShatteredStatics(JSON_READ_IO *const io)
{
    // Introduced in TRX 1.12
    if (!JSON_ReadIO_HasKey(io, "shattered_statics")) {
        return OK;
    }

    MUST(JSON_PUSH(io, "shattered_statics"));
    const size_t count = JSON_ARRAY_LEN(io);
    for (size_t i = 0; i < count; i++) {
        int16_t draw_num = -1;
        MUST(JSON_READ_A(io, i, &draw_num));
        if (!M_IsValidDrawNum(draw_num)) {
            return JSON_ReadIO_Fail(io, "invalid static %d", draw_num);
        }
        M_SetShattered(draw_num);
    }
    MUST(JSON_POP(io));

    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        for (int32_t j = 0; j < room->num_static_meshes; j++) {
            const STATIC_MESH *const mesh = &room->static_meshes[j];
            if (Room_IsStaticShattered(mesh)) {
                M_ClearStopper(mesh);
            }
        }
    }
    return OK;
}
