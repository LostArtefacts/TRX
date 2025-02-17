#include "game/objects/effects/ember.h"

#include "game/effects.h"
#include "game/lara/common.h"
#include "game/room.h"
#include "global/vars.h"

#include <libtrx/game/math.h>

#define EMBER_DAMAGE 10

void Ember_Setup(OBJECT *obj)
{
    obj->control_func = Ember_Control;
}

void Ember_Control(int16_t effect_num)
{
    EFFECT *effect = Effect_Get(effect_num);
    effect->pos.z += (effect->speed * Math_Cos(effect->rot.y)) >> W2V_SHIFT;
    effect->pos.x += (effect->speed * Math_Sin(effect->rot.y)) >> W2V_SHIFT;
    effect->fall_speed += GRAVITY;
    effect->pos.y += effect->fall_speed;

    int16_t room_num = effect->room_num;
    const SECTOR *const sector =
        Room_GetSector(effect->pos.x, effect->pos.y, effect->pos.z, &room_num);
    if (effect->pos.y >= Room_GetHeight(
            sector, effect->pos.x, effect->pos.y, effect->pos.z)
        || effect->pos.y < Room_GetCeiling(
               sector, effect->pos.x, effect->pos.y, effect->pos.z)) {
        Effect_Kill(effect_num);
    } else if (Lara_IsNearItem(&effect->pos, 200)) {
        Lara_TakeDamage(EMBER_DAMAGE, true);
        Effect_Kill(effect_num);
    } else if (room_num != effect->room_num) {
        Effect_NewRoom(effect_num, room_num);
    }
}
