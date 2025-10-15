#include "game/effects.h"
#include "game/lara.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "version.h"

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    effect->rot.x += 5 * DEG_1;
    effect->rot.z += 10 * DEG_1;
    effect->pos.x += (effect->speed * Math_Sin(effect->rot.y)) >> W2V_SHIFT;
    effect->pos.z += (effect->speed * Math_Cos(effect->rot.y)) >> W2V_SHIFT;
    effect->pos.y += effect->fall_speed;
    effect->fall_speed += GRAVITY;

    int16_t room_num = effect->room_num;
    const SECTOR *const sector =
        Room_GetSector(effect->pos.x, effect->pos.y, effect->pos.z, &room_num);

    const ROOM *const current_room = Room_Get(effect->room_num);
    const ROOM *const next_room = Room_Get(room_num);
    if ((current_room->flags & RF_UNDERWATER) == 0
        && (next_room->flags & RF_UNDERWATER) != 0) {
        const int16_t effect_num = Effect_Create(effect->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const splash_fx = Effect_Get(effect_num);
            splash_fx->pos.x = effect->pos.x;
            splash_fx->pos.y = effect->pos.y;
            splash_fx->pos.z = effect->pos.z;
            splash_fx->rot.y = 0;
            splash_fx->speed = 0;
            splash_fx->frame_num = 0;
            splash_fx->object_id = O_SPLASH_1;
        }
    }

    const int32_t ceiling =
        Room_GetCeiling(sector, effect->pos.x, effect->pos.y, effect->pos.z);
    if (effect->pos.y < ceiling) {
        effect->pos.y = ceiling;
        effect->fall_speed = -effect->fall_speed;
    }

    const int32_t height =
        Room_GetHeight(sector, effect->pos.x, effect->pos.y, effect->pos.z);
    if (effect->pos.y >= height) {
        if (effect->counter > 0) {
            effect->speed = 0;
            effect->frame_num = 0;
            effect->counter = 0;
            effect->object_id = O_EXPLOSION_1;
            effect->shade = SHADE_NEUTRAL;
            Sound_Effect(SFX_EXPLOSION_1, &effect->pos, SPM_NORMAL);
        } else {
            Effect_Kill(effect_num);
        }
        return;
    }

    const int16_t counter_value =
        (g_TRVersion == 1) ? ABS(effect->counter) : effect->counter;
    const bool trigger_explosion =
        (g_TRVersion == 1) ? (effect->counter > 0) : (effect->counter == 0);

    if (Lara_IsNearItem(&effect->pos, counter_value * 2)) {
        Lara_TakeDamage(counter_value, true);

        if (trigger_explosion) {
            effect->speed = 0;
            effect->frame_num = 0;
            effect->counter = 0;
            effect->object_id = O_EXPLOSION_1;
            effect->shade = SHADE_NEUTRAL;
            Sound_Effect(SFX_EXPLOSION_1, &effect->pos, SPM_NORMAL);

            LARA_INFO *const lara = Lara_GetLaraInfo();
            lara->hit_effect_count = 5;
            lara->hit_effect = effect;
        } else {
            Effect_Kill(effect_num);
        }
    }

    if (room_num != effect->room_num) {
        Effect_NewRoom(effect_num, room_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->loaded = true;
    obj->mesh_count = 0;
}

REGISTER_OBJECT(O_BODY_PART, M_Setup)
