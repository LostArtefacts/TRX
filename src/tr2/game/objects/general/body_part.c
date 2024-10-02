#include "game/objects/general/body_part.h"

#include "game/effects.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/math.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/types.h"
#include "global/vars.h"

void __cdecl BodyPart_Control(const int16_t fx_num)
{
    FX *const fx = &g_Effects[fx_num];
    fx->rot.x += 5 * PHD_DEGREE;
    fx->rot.z += 10 * PHD_DEGREE;
    fx->pos.x += (fx->speed * Math_Sin(fx->rot.y)) >> W2V_SHIFT;
    fx->pos.z += (fx->speed * Math_Cos(fx->rot.y)) >> W2V_SHIFT;
    fx->pos.y += fx->fall_speed;
    fx->fall_speed += GRAVITY;

    int16_t room_num = fx->room_num;
    const SECTOR *const sector =
        Room_GetSector(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);

    if (!(g_Rooms[fx->room_num].flags & RF_UNDERWATER)
        && (g_Rooms[room_num].flags & RF_UNDERWATER)) {
        const int16_t fx_num = Effect_Create(fx->room_num);
        if (fx_num != NO_ITEM) {
            FX *const splash_fx = &g_Effects[fx_num];
            splash_fx->pos.x = fx->pos.x;
            splash_fx->pos.y = fx->pos.y;
            splash_fx->pos.z = fx->pos.z;
            splash_fx->rot.y = 0;
            splash_fx->speed = 0;
            splash_fx->frame_num = 0;
            splash_fx->object_id = O_SPLASH;
        }
    }

    const int32_t ceiling =
        Room_GetCeiling(sector, fx->pos.x, fx->pos.y, fx->pos.z);
    if (fx->pos.y < ceiling) {
        fx->pos.y = ceiling;
        fx->fall_speed = -fx->fall_speed;
    }

    const int32_t height =
        Room_GetHeight(sector, fx->pos.x, fx->pos.y, fx->pos.z);
    if (fx->pos.y >= height) {
        if (fx->counter) {
            fx->speed = 0;
            fx->frame_num = 0;
            fx->counter = 0;
            fx->object_id = O_EXPLOSION;
            fx->shade = HIGH_LIGHT;
            Sound_Effect(SFX_EXPLOSION1, &fx->pos, SPM_NORMAL);
        } else {
            Effect_Kill(fx_num);
        }
        return;
    }

    if (Lara_IsNearItem(&fx->pos, 2 * fx->counter)) {
        Lara_TakeDamage(fx->counter, true);

        if (fx->counter == 0) {
            fx->speed = 0;
            fx->frame_num = 0;
            fx->counter = 0;
            fx->object_id = O_EXPLOSION;
            fx->shade = HIGH_LIGHT;
            Sound_Effect(SFX_EXPLOSION1, &fx->pos, SPM_NORMAL);
            g_Lara.spaz_effect_count = 5;
            g_Lara.spaz_effect = fx;
        } else {
            Effect_Kill(fx_num);
        }
    }

    if (room_num != fx->room_num) {
        Effect_NewRoom(fx_num, room_num);
    }
}
