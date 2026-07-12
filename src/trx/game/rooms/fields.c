#include <trx/core/field.h>
#include <trx/game/rooms.h>

// Scripts count rooms from 1; the engine counts from 0. There is no
// Room_GetIndex, so derive it the way Item_GetIndex does.
static bool M_GetIndex(const void *const self, FIELD_VALUE *const out)
{
    const ROOM *const room = self;
    *out = (FIELD_VALUE) {
        .type = FT_INT16,
        .as_int = (int32_t)(room - Room_Get(0)) + 1,
    };
    return true;
}

// clang-format off
static const FIELD_DESC M_ROOM_FIELDS[] = {
    FIELD_FN("room_index", FT_INT16, M_GetIndex, nullptr),

    FIELD(ROOM, flags.underwater,    FT_BOOL),
    FIELD(ROOM, flags.wind,          FT_BOOL),
    FIELD(ROOM, flags.damaging,      FT_BOOL),
    FIELD(ROOM, flags.cold,          FT_BOOL),
    FIELD(ROOM, flags.outside,       FT_BOOL),
    FIELD(ROOM, flags.inside,        FT_BOOL),
    FIELD(ROOM, flags.swamp,         FT_BOOL),
    FIELD(ROOM, flags.dynamic_lit,   FT_BOOL),
    FIELD(ROOM, flags.no_lens_flare, FT_BOOL),

    FIELD_RO(ROOM, flip_status,       FT_INT32),
    FIELD_RO(ROOM, flipped_room,      FT_INT16),
    FIELD_RO(ROOM, pos,               FT_XYZ_32),
    FIELD_RO(ROOM, min_floor,         FT_INT32),
    FIELD_RO(ROOM, max_ceiling,       FT_INT32),
    FIELD_RO(ROOM, size.x,            FT_INT16),
    FIELD_RO(ROOM, size.z,            FT_INT16),
    FIELD_RO(ROOM, ambient,           FT_INT16),
    FIELD_RO(ROOM, num_lights,        FT_INT16),
    FIELD_RO(ROOM, num_static_meshes, FT_INT16),
    FIELD_RO(ROOM, item_num,          FT_INT16),
    FIELD_RO(ROOM, effect_num,        FT_INT16),
    FIELD_RO(ROOM, water_scheme,      FT_UINT8),
    FIELD_RO(ROOM, reverb_info,       FT_UINT8),
    FIELD_RO(ROOM, alternate_group,   FT_UINT8),
};
// clang-format on

TYPE_DEFINE(ROOM, M_ROOM_FIELDS)
