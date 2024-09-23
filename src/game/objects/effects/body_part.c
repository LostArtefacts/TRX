#include "game/objects/effects/body_part.h"

#include "game/effects.h"
#include "game/lara/common.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

void BodyPart_Setup(OBJECT *obj)
{
    obj->control = BodyPart_Control;
    obj->nmeshes = 0;
    obj->loaded = 1;
}

void BodyPart_Control(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];
    fx->rot.x += 5 * PHD_DEGREE;
    fx->rot.z += 10 * PHD_DEGREE;
    fx->pos.z += (fx->speed * Math_Cos(fx->rot.y)) >> W2V_SHIFT;
    fx->pos.x += (fx->speed * Math_Sin(fx->rot.y)) >> W2V_SHIFT;
    fx->fall_speed += GRAVITY;
    fx->pos.y += fx->fall_speed;

    int16_t room_num = fx->room_num;
    const SECTOR *const sector =
        Room_GetSector(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);

    const int32_t ceiling =
        Room_GetCeiling(sector, fx->pos.x, fx->pos.y, fx->pos.z);
    if (fx->pos.y < ceiling) {
        fx->fall_speed = -fx->fall_speed;
        fx->pos.y = ceiling;
    }

    const int32_t height =
        Room_GetHeight(sector, fx->pos.x, fx->pos.y, fx->pos.z);
    if (fx->pos.y >= height) {
        if (fx->counter > 0) {
            fx->speed = 0;
            fx->frame_num = 0;
            fx->counter = 0;
            fx->object_id = O_EXPLOSION_1;
            Sound_Effect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);
        } else {
            Effect_Kill(fx_num);
        }
        return;
    }

    if (Lara_IsNearItem(&fx->pos, ABS(fx->counter) * 2)) {
        Lara_TakeDamage(ABS(fx->counter), true);

        if (fx->counter > 0) {
            fx->speed = 0;
            fx->frame_num = 0;
            fx->counter = 0;
            fx->object_id = O_EXPLOSION_1;
            Sound_Effect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);

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
