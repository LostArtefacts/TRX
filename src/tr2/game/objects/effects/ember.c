#include "game/objects/effects/ember.h"

#include "game/effects.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/math.h"
#include "game/room.h"
#include "global/vars.h"

void __cdecl Ember_Control(const int16_t fx_num)
{
    FX *const fx = &g_Effects[fx_num];
    fx->fall_speed += GRAVITY;
    fx->pos.z += (fx->speed * Math_Cos(fx->rot.y)) >> W2V_SHIFT;
    fx->pos.x += (fx->speed * Math_Sin(fx->rot.y)) >> W2V_SHIFT;
    fx->pos.y = fx->pos.y + fx->fall_speed;

    int16_t room_num = fx->room_num;
    const SECTOR *const sector =
        Room_GetSector(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);
    const int32_t ceiling =
        Room_GetCeiling(sector, fx->pos.x, fx->pos.y, fx->pos.z);
    const int32_t height =
        Room_GetHeight(sector, fx->pos.x, fx->pos.y, fx->pos.z);

    if (fx->pos.y >= height || fx->pos.y < ceiling) {
        Effect_Kill(fx_num);
    } else if (Lara_IsNearItem(&fx->pos, 200)) {
        Lara_TakeDamage(10, true);
        Effect_Kill(fx_num);
    } else if (room_num != fx->room_num) {
        Effect_NewRoom(fx_num, room_num);
    }
}
