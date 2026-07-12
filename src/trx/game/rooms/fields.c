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

    FIELD(ROOM, flags.underwater),
    FIELD(ROOM, flags.wind),
    FIELD(ROOM, flags.damaging),
    FIELD(ROOM, flags.cold),
    FIELD(ROOM, flags.outside),
    FIELD(ROOM, flags.inside),
    FIELD(ROOM, flags.swamp),
    FIELD(ROOM, flags.dynamic_lit),
    FIELD(ROOM, flags.no_lens_flare),

    FIELD_RO(ROOM, flip_status),
    FIELD_RO(ROOM, flipped_room),
    FIELD_RO(ROOM, pos),
    FIELD_RO(ROOM, min_floor),
    FIELD_RO(ROOM, max_ceiling),
    FIELD_RO(ROOM, size.x),
    FIELD_RO(ROOM, size.z),
    FIELD_RO(ROOM, ambient),
    FIELD_RO(ROOM, num_lights),
    FIELD_RO(ROOM, num_static_meshes),
    FIELD_RO(ROOM, item_num),
    FIELD_RO(ROOM, effect_num),
    FIELD_RO(ROOM, water_scheme),
    FIELD_RO(ROOM, reverb_info),
    FIELD_RO(ROOM, alternate_group),
};
// clang-format on

TYPE_DEFINE(ROOM, M_ROOM_FIELDS)
