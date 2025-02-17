#include "game/objects/effects/body_part.h"

#include "game/effects.h"
#include "game/lara/common.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/game/math.h>

void BodyPart_Setup(OBJECT *obj)
{
    obj->control_func = BodyPart_Control;
    obj->mesh_count = 0;
    obj->loaded = 1;
}

void BodyPart_Control(int16_t effect_num)
{
    EFFECT *effect = Effect_Get(effect_num);
    effect->rot.x += 5 * DEG_1;
    effect->rot.z += 10 * DEG_1;
    effect->pos.z += (effect->speed * Math_Cos(effect->rot.y)) >> W2V_SHIFT;
    effect->pos.x += (effect->speed * Math_Sin(effect->rot.y)) >> W2V_SHIFT;
    effect->fall_speed += GRAVITY;
    effect->pos.y += effect->fall_speed;

    int16_t room_num = effect->room_num;
    const SECTOR *const sector =
        Room_GetSector(effect->pos.x, effect->pos.y, effect->pos.z, &room_num);

    const int32_t ceiling =
        Room_GetCeiling(sector, effect->pos.x, effect->pos.y, effect->pos.z);
    if (effect->pos.y < ceiling) {
        effect->fall_speed = -effect->fall_speed;
        effect->pos.y = ceiling;
    }

    const int32_t height =
        Room_GetHeight(sector, effect->pos.x, effect->pos.y, effect->pos.z);
    if (effect->pos.y >= height) {
        if (effect->counter > 0) {
            effect->speed = 0;
            effect->frame_num = 0;
            effect->counter = 0;
            effect->object_id = O_EXPLOSION_1;
            Sound_Effect(SFX_ATLANTEAN_EXPLODE, &effect->pos, SPM_NORMAL);
        } else {
            Effect_Kill(effect_num);
        }
        return;
    }

    if (Lara_IsNearItem(&effect->pos, ABS(effect->counter) * 2)) {
        Lara_TakeDamage(ABS(effect->counter), true);

        if (effect->counter > 0) {
            effect->speed = 0;
            effect->frame_num = 0;
            effect->counter = 0;
            effect->object_id = O_EXPLOSION_1;
            Sound_Effect(SFX_ATLANTEAN_EXPLODE, &effect->pos, SPM_NORMAL);

            g_Lara.hit_effect_count = 5;
            g_Lara.hit_effect = effect;
        } else {
            Effect_Kill(effect_num);
        }
    }

    if (room_num != effect->room_num) {
        Effect_NewRoom(effect_num, room_num);
    }
}
