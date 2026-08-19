#include <trx/core/math.h>
#include <trx/game/const.h>
#include <trx/game/effects.h>
#include <trx/game/fx/water.h>
#include <trx/game/objects.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/version.h>

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const int32_t old_x = effect->pos.x;
    const int32_t old_y = effect->pos.y;
    const int32_t old_z = effect->pos.z;

    effect->fall_speed += 6;
    effect->rot.x += 182 * ((effect->speed >> 1) + 7);
    effect->rot.y += 182 * effect->speed;
    effect->rot.z += 4186;

    const XYZ_32 step =
        XYZ_32_RotateYaw((XYZ_32) { .z = effect->speed }, effect->flag1);
    effect->pos.x += step.x >> 1;
    effect->pos.y += effect->fall_speed;
    effect->pos.z += step.z >> 1;

    int16_t room_num = effect->room_num;
    const SECTOR *const sector = Room_GetSector(effect->pos, &room_num);

    if (effect->room_num != room_num) {
        Effect_UpdateRoom(effect_num, room_num);
    }

    const ROOM *const room = Room_Get(room_num);
    if (room->flags.underwater) {
        Sparks_TriggerSmallSplash(
            (XYZ_32) { effect->pos.x, room->max_ceiling, effect->pos.z }, 8);
        const XYZ_32 pos = { effect->pos.x, room->max_ceiling, effect->pos.z };
        if (g_TRVersion == 4) {
            FX_Water_SetupRipple(
                pos, (Random_GetControl() & 3) + 8, FX_RIPPLE_SLOW);
        } else {
            FX_Water_SetupRipple(
                pos, 8 + (Random_GetControl() & 3),
                FX_RIPPLE_SLOW | FX_RIPPLE_DARK | FX_RIPPLE_JITTER);
        }
        Effect_Destroy(effect_num);
        return;
    }

    const int32_t ceiling = Room_GetCeiling(sector, effect->pos);
    if (effect->pos.y < ceiling) {
        Sound_Effect(SFX_LARA_SHOTGUN_SHELL, &effect->pos, SPM_NORMAL);
        effect->speed -= 4;
        effect->counter--;

        if (effect->counter < 0 || effect->speed < 8) {
            Effect_Destroy(effect_num);
            return;
        }

        effect->fall_speed = -effect->fall_speed;
        effect->pos.y = ceiling;
    }

    const int32_t height = Room_GetHeight(sector, effect->pos);
    if (effect->pos.y >= height) {
        Sound_Effect(SFX_LARA_SHOTGUN_SHELL, &effect->pos, SPM_NORMAL);
        effect->speed -= 8;
        effect->counter--;

        if (effect->counter < 0 || effect->speed < 8) {
            Effect_Destroy(effect_num);
            return;
        }

        if (old_y > height) {
            effect->flag1 += 0x8000;
            effect->pos.x = old_x;
            effect->pos.z = old_z;
        } else {
            effect->fall_speed = -effect->fall_speed >> 1;
        }

        effect->pos.y = old_y;
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->effect_control_func = M_Control;
    obj->mesh_count = 0;
}

REGISTER_OBJECT(O_GUN_SHELL, M_Setup)
REGISTER_OBJECT(O_SHOTGUN_SHELL, M_Setup)
