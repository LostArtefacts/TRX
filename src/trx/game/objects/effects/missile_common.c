#include <trx/game/objects/effects/missile_common.h>

#include <trx/game/effects.h>
#include <trx/game/lara.h>
#include <trx/game/math.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>
#include <trx/utils.h>
#include <trx/version.h>

void Missile_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);

    if (effect->object_id == O_MISSILE_HARPOON
        && !Room_Get(effect->room_num)->flags.underwater) {
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
    const SECTOR *const sector = Room_GetSector(effect->pos, &room_num);

    if (g_TRVersion == 3 && effect->object_id == O_MISSILE_FLAME
        && Room_Get(room_num)->flags.underwater && !effect->flag1) {
        if (Random_GetControl() & 1) {
            Sparks_TriggerFlamethrowerSmoke(effect->pos, true);
        }
        Effect_Kill(effect_num);
        return;
    }

    const int32_t height = Room_GetHeight(sector, effect->pos);
    const int32_t ceiling = Room_GetCeiling(sector, effect->pos);
    if (effect->pos.y >= height || effect->pos.y <= ceiling) {
        if (effect->object_id == O_MISSILE_KNIFE
            || effect->object_id == O_MISSILE_HARPOON) {
            effect->object_id = O_RICOCHET;
            effect->frame_num = Random_GetControl() / -11000;
            effect->speed = 0;
            effect->counter = 6;
            Sound_Effect(SFX_PROJECTILE_HIT, &effect->pos, SPM_NORMAL);
        } else if (effect->object_id == O_MISSILE_FLAME) {
            if (!effect->flag1) {
                Sparks_TriggerFlamethrowerHitFlame(effect->pos);
                if (g_TRVersion == 3) {
                    Output_AddDynamicLightRGB(
                        effect->pos, 24,
                        (RGB_888) { 255, 192, Random_GetControl() & 0x3F });
                } else {
                    Output_AddDynamicLight(effect->pos, 14, 11);
                }
            }

            Effect_Kill(effect_num);
        }
        return;
    }

    if (room_num != effect->room_num) {
        Effect_NewRoom(effect_num, room_num);
    }

    if (effect->object_id == O_MISSILE_FLAME) {
        if (Lara_IsNearItem(&effect->pos, 350)) {
            if (effect->flag1) {
                LARA_INFO *const lara_info = Lara_GetLaraInfo();
                lara_info->poison_timer += 4;
            } else {
                Lara_TakeDamage(3, true);
                Lara_CatchFire();
                return;
            }
        }
    } else if (Lara_IsNearItem(&effect->pos, 200)) {
        ITEM *const lara_item = Lara_GetItem();
        if (effect->object_id == O_MISSILE_KNIFE
            || effect->object_id == O_MISSILE_HARPOON) {
            lara_item->hit_points -= 50;
            effect->object_id = O_BLOOD_1;
            Sound_Effect(SFX_CRUNCH_1, &effect->pos, SPM_NORMAL);
        }
        lara_item->hit_status = true;

        effect->rot.y = lara_item->rot.y;
        effect->frame_num = 0;
        effect->speed = lara_item->speed;
        effect->counter = 0;
    }

    if (effect->object_id == O_MISSILE_HARPOON
        && Room_Get(effect->room_num)->flags.underwater) {
        if (g_TRVersion == 3) {
            const int32_t time4 = Output_GetTimeInGame() * 4;
            if ((time4 & 0xF) == 0) {
                Spawn_BubbleEx(&effect->pos, effect->room_num, 8, 8);
            }
            Sparks_TriggerRocketSmoke(effect->pos, 64, effect->room_num);
        } else {
            Spawn_Bubble(&effect->pos, effect->room_num);
        }
    } else if (effect->object_id == O_MISSILE_FLAME) {
        if (!effect->counter--) {
            if (g_TRVersion < 3) {
                Output_AddDynamicLight(effect->pos, 14, 11);
                Sound_Effect(SFX_DRAGON_FIRE, &effect->pos, SPM_NORMAL);
            }
            Effect_Kill(effect_num);
        }
    } else if (effect->object_id == O_MISSILE_KNIFE) {
        effect->rot.z += 30 * DEG_1;
    }
}
