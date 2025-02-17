#include "game/objects/effects/natla_gun.h"

#include "game/effects.h"
#include "game/room.h"
#include "global/vars.h"

#include <libtrx/game/math.h>

void NatlaGun_Setup(OBJECT *obj)
{
    obj->control_func = NatlaGun_Control;
}

void NatlaGun_Control(int16_t effect_num)
{
    EFFECT *effect = Effect_Get(effect_num);
    const OBJECT *const obj = Object_Get(effect->object_id);

    effect->frame_num--;
    if (effect->frame_num <= obj->mesh_count) {
        Effect_Kill(effect_num);
    }

    if (effect->frame_num == -1) {
        return;
    }

    int32_t z = effect->pos.z
        + ((effect->speed * Math_Cos(effect->rot.y)) >> W2V_SHIFT);
    int32_t x = effect->pos.x
        + ((effect->speed * Math_Sin(effect->rot.y)) >> W2V_SHIFT);
    int32_t y = effect->pos.y;
    int16_t room_num = effect->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);

    if (y >= Room_GetHeight(sector, x, y, z)
        || y <= Room_GetCeiling(sector, x, y, z)) {
        return;
    }

    effect_num = Effect_Create(room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *new_effect = Effect_Get(effect_num);
        new_effect->pos.x = x;
        new_effect->pos.y = y;
        new_effect->pos.z = z;
        new_effect->rot.y = effect->rot.y;
        new_effect->room_num = room_num;
        new_effect->speed = effect->speed;
        new_effect->frame_num = 0;
        new_effect->object_id = O_MISSILE_1;
    }
}
