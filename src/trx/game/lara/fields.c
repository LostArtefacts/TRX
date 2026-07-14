// Lara's position, room and hit points are not here. She is an item like any
// other, and those live on it - see trx.lara.item.

#include <trx/core/field.h>
#include <trx/game/lara/types.h>

// clang-format off
static const FIELD_DESC M_LARA_FIELDS[] = {
    // condition
    FIELD(LARA_INFO, air),
    FIELD(LARA_INFO, exposure_timer),
    FIELD(LARA_INFO, poison.value),
    FIELD(LARA_INFO, poison.target),
    FIELD(LARA_INFO, electric),
    FIELD_RO(LARA_INFO, burn),

    // what she is doing
    FIELD_RO(LARA_INFO, water_status),
    FIELD_RO(LARA_INFO, gun_status),
    FIELD_RO(LARA_INFO, gun_type),
    FIELD_RO(LARA_INFO, request_gun_type),
    FIELD_RO(LARA_INFO, is_crouched),
    FIELD_RO(LARA_INFO, climb_status),
    FIELD_RO(LARA_INFO, extra_anim),
    FIELD_RO(LARA_INFO, killed_loyal_item),

    // timers
    FIELD_RO(LARA_INFO, dive_timer),
    FIELD_RO(LARA_INFO, death_timer),
    FIELD(LARA_INFO, sprint_timer),
    FIELD_RO(LARA_INFO, hit_direction),
    FIELD_RO(LARA_INFO, hit_frame),
    FIELD_RO(LARA_INFO, pose_count),

    // Deliberately absent: the arms, the LOT, the mesh pointers, the rope and
    // interaction state, and the interpolation snapshots. They are how the
    // engine drives Lara, not a contract, and a script writing them mid-frame
    // would wedge her.
};
// clang-format on

TYPE_DEFINE(LARA_INFO, M_LARA_FIELDS)
