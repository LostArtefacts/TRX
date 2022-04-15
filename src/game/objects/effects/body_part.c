#include "game/objects/effects/body_part.h"

#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

void BodyPart_Setup(OBJECT_INFO *obj)
{
    obj->control = BodyPart_Control;
    obj->nmeshes = 0;
    obj->loaded = 1;
}

void BodyPart_Control(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    fx->pos.x_rot += 5 * PHD_DEGREE;
    fx->pos.z_rot += 10 * PHD_DEGREE;
    fx->pos.z += (fx->speed * phd_cos(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->pos.x += (fx->speed * phd_sin(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->fall_speed += GRAVITY;
    fx->pos.y += fx->fall_speed;

    int16_t room_num = fx->room_number;
    FLOOR_INFO *floor = GetFloor(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);

    int32_t ceiling = Room_GetCeiling(floor, fx->pos.x, fx->pos.y, fx->pos.z);
    if (fx->pos.y < ceiling) {
        fx->fall_speed = -fx->fall_speed;
        fx->pos.y = ceiling;
    }

    int32_t height = Room_GetHeight(floor, fx->pos.x, fx->pos.y, fx->pos.z);
    if (fx->pos.y >= height) {
        if (fx->counter) {
            fx->speed = 0;
            fx->frame_number = 0;
            fx->counter = 0;
            fx->object_number = O_EXPLOSION1;
            Sound_Effect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);
        } else {
            KillEffect(fx_num);
        }
        return;
    }

    if (Lara_IsNearItem(&fx->pos, fx->counter * 2)) {
        g_LaraItem->hit_points -= fx->counter;
        g_LaraItem->hit_status = 1;

        if (fx->counter) {
            fx->speed = 0;
            fx->frame_number = 0;
            fx->counter = 0;
            fx->object_number = O_EXPLOSION1;
            Sound_Effect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);

            g_Lara.spaz_effect_count = 5;
            g_Lara.spaz_effect = fx;
        } else {
            KillEffect(fx_num);
        }
    }

    if (room_num != fx->room_number) {
        EffectNewRoom(fx_num, room_num);
    }
}
