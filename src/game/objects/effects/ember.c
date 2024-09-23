#include "game/objects/effects/ember.h"

#include "game/effects.h"
#include "game/lara/common.h"
#include "game/room.h"
#include "global/vars.h"
#include "math/math.h"

#define EMBER_DAMAGE 10

void Ember_Setup(OBJECT *obj)
{
    obj->control = Ember_Control;
}

void Ember_Control(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];
    fx->pos.z += (fx->speed * Math_Cos(fx->rot.y)) >> W2V_SHIFT;
    fx->pos.x += (fx->speed * Math_Sin(fx->rot.y)) >> W2V_SHIFT;
    fx->fall_speed += GRAVITY;
    fx->pos.y += fx->fall_speed;

    int16_t room_num = fx->room_num;
    const SECTOR *const sector =
        Room_GetSector(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);
    if (fx->pos.y >= Room_GetHeight(sector, fx->pos.x, fx->pos.y, fx->pos.z)
        || fx->pos.y
            < Room_GetCeiling(sector, fx->pos.x, fx->pos.y, fx->pos.z)) {
        Effect_Kill(fx_num);
    } else if (Lara_IsNearItem(&fx->pos, 200)) {
        Lara_TakeDamage(EMBER_DAMAGE, true);
        Effect_Kill(fx_num);
    } else if (room_num != fx->room_num) {
        Effect_NewRoom(fx_num, room_num);
    }
}
