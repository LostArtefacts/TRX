#include "game/effects/blood.h"

#include "game/effects.h"
#include "global/const.h"
#include "global/types.h"

int16_t Effect_Blood(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t direction,
    int16_t room_num)
{
    int16_t fx_num = Effect_Create(room_num);
    if (fx_num != NO_ITEM) {
        FX *fx = &g_Effects[fx_num];
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->rot.y = direction;
        fx->object_id = O_BLOOD_1;
        fx->frame_num = 0;
        fx->counter = 0;
        fx->speed = speed;
    }
    return fx_num;
}
