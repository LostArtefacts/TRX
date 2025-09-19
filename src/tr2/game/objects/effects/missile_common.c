#include "game/objects/effects/missile_common.h"

#include "game/effects.h"
#include "game/spawn.h"

#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>
#include <libtrx/game/output.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>
#include <libtrx/utils.h>

void Missile_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);

    if (effect->object_id == O_MISSILE_HARPOON
        && !(Room_Get(effect->room_num)->flags & RF_UNDERWATER)) {
        if (effect->rot.x > -0x3000) {
            effect->rot.x -= DEG_1;
        }
    }

    effect->pos.y += (effect->speed * Math_Sin(-effect->rot.x)) >> W2V_SHIFT;
    const int32_t speed =
        (effect->speed * Math_Cos(effect->rot.x)) >> W2V_SHIFT;
    effect->pos.z += (speed * Math_Cos(effect->rot.y)) >> W2V_SHIFT;
    effect->pos.x += (speed * Math_Sin(effect->rot.y)) >> W2V_SHIFT;

    int16_t room_num = effect->room_num;
    const SECTOR *const sector =
        Room_GetSector(effect->pos.x, effect->pos.y, effect->pos.z, &room_num);

    const int32_t height =
        Room_GetHeight(sector, effect->pos.x, effect->pos.y, effect->pos.z);
    const int32_t ceiling =
        Room_GetCeiling(sector, effect->pos.x, effect->pos.y, effect->pos.z);
    if (effect->pos.y >= height || effect->pos.y <= ceiling) {
        if (effect->object_id == O_MISSILE_KNIFE
            || effect->object_id == O_MISSILE_HARPOON) {
            effect->object_id = O_RICOCHET;
            effect->frame_num = Random_GetControl() / -11000;
            effect->speed = 0;
            effect->counter = 6;
            Sound_Effect(SFX_CIRCLE_BLADE_HIT, &effect->pos, SPM_NORMAL);
        } else if (effect->object_id == O_MISSILE_FLAME) {
            Output_AddDynamicLight(effect->pos, 14, 11);
            Effect_Kill(effect_num);
        }
        return;
    }

    if (room_num != effect->room_num) {
        Effect_NewRoom(effect_num, room_num);
    }

    if (effect->object_id == O_MISSILE_FLAME) {
        if (Lara_IsNearItem(&effect->pos, 350)) {
            Lara_TakeDamage(3, true);
            Lara_CatchFire();
            return;
        }
    } else if (Lara_IsNearItem(&effect->pos, 200)) {
        ITEM *const lara_item = Lara_GetItem();
        if (effect->object_id == O_MISSILE_KNIFE
            || effect->object_id == O_MISSILE_HARPOON) {
            lara_item->hit_points -= 50;
            effect->object_id = O_BLOOD_1;
            Sound_Effect(SFX_CRUNCH_1, &effect->pos, SPM_NORMAL);
        }
        lara_item->hit_status = 1;

        effect->rot.y = lara_item->rot.y;
        effect->frame_num = 0;
        effect->speed = lara_item->speed;
        effect->counter = 0;
    }

    if (effect->object_id == O_MISSILE_HARPOON
        && (Room_Get(effect->room_num)->flags & RF_UNDERWATER)) {
        Spawn_Bubble(&effect->pos, effect->room_num);
    } else if (effect->object_id == O_MISSILE_FLAME) {
        if (!effect->counter--) {
            Output_AddDynamicLight(effect->pos, 14, 11);
            Sound_Effect(SFX_DRAGON_FIRE, &effect->pos, SPM_NORMAL);
            Effect_Kill(effect_num);
        }
    } else if (effect->object_id == O_MISSILE_KNIFE) {
        effect->rot.z += 30 * DEG_1;
    }
}

void Missile_ShootAtLara(EFFECT *const effect)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - effect->pos.x;
    const int32_t dy = lara_item->pos.y - effect->pos.y;
    const int32_t dz = lara_item->pos.z - effect->pos.z;

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(lara_item);
    const int32_t dist_vert =
        dy + bounds->max.y + 3 * (bounds->min.y - bounds->max.y) / 4;
    const int32_t dist_horz = Math_Sqrt(SQUARE(dz) + SQUARE(dx));
    effect->rot.x = -Math_Atan(dist_horz, dist_vert);
    effect->rot.y = Math_Atan(dz, dx);
    effect->rot.x += (Random_GetControl() - 0x4000) / 64;
    effect->rot.y += (Random_GetControl() - 0x4000) / 64;
}
