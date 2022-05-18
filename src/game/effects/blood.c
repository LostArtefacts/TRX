#include "game/effects/blood.h"

#include "game/effects.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

int16_t Effect_Blood(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t direction,
    int16_t room_num)
{
    int16_t fx_num = Effect_Create(room_num);
    if (fx_num != NO_ITEM) {
        FX_INFO *fx = &g_Effects[fx_num];
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->pos.y_rot = direction;
        fx->object_number = O_BLOOD1;
        fx->frame_number = 0;
        fx->counter = 0;
        fx->speed = speed;
    }
    return fx_num;
}
