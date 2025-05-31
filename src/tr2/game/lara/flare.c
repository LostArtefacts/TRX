#include "game/lara/flare.h"

#include "decomp/flares.h"

#include <libtrx/game/lara.h>

typedef enum {
    // clang-format off
    LA_FLARES_HOLD   = 0,
    LA_FLARES_THROW  = 1,
    LA_FLARES_DRAW   = 2,
    LA_FLARES_IGNITE = 3,
    LA_FLARES_IDLE   = 4,
    // clang-format on
} M_LARA_FLARE_ANIMATION;

void Lara_Flare_SetArm(const int32_t flare_frame)
{
    int16_t anim_idx;
    if (flare_frame < LF_FL_THROW) {
        anim_idx = LA_FLARES_HOLD;
    } else if (flare_frame < LF_FL_DRAW) {
        anim_idx = LA_FLARES_THROW;
    } else if (flare_frame < LF_FL_IGNITE) {
        anim_idx = LA_FLARES_DRAW;
    } else if (flare_frame < LF_FL_2_HOLD) {
        anim_idx = LA_FLARES_IGNITE;
    } else {
        anim_idx = LA_FLARES_IDLE;
    }

    const OBJECT *const obj = Object_Get(O_LARA_FLARE);
    const ANIM *const anim = Object_GetAnim(obj, anim_idx);
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->left_arm.anim_num = obj->anim_idx + anim_idx;
    lara_info->left_arm.frame_base = anim->frame_ptr;
}
