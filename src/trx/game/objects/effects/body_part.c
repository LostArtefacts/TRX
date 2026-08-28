#include <trx/game/effects.h>
#include <trx/game/fx/debris.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks/spawners.h>
#include <trx/version.h>

static void M_SpawnSplash(const GAME_VECTOR pos)
{
    const int16_t effect_num = Effect_Create(pos.room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->pos = pos.pos;
        effect->rot.y = 0;
        effect->speed = 0;
        effect->frame_num = 0;
        effect->object_id = O_SPLASH_1;
    }
}

static void M_Control_TR12(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    effect->rot.x += 5 * DEG_1;
    effect->rot.z += 10 * DEG_1;
    effect->pos = XYZ_32_OffsetYaw(effect->pos, effect->rot.y, effect->speed);
    effect->pos.y += effect->fall_speed;
    effect->fall_speed += GRAVITY;

    int16_t room_num = effect->room_num;
    const SECTOR *const sector = Room_GetSector(effect->pos, &room_num);

    const ROOM *const current_room = Room_Get(effect->room_num);
    const ROOM *const next_room = Room_Get(room_num);
    if (!current_room->flags.underwater && next_room->flags.underwater) {
        M_SpawnSplash(
            (GAME_VECTOR) { .pos = effect->pos, .room_num = effect->room_num });
    }

    const int32_t ceiling = Room_GetCeiling(sector, effect->pos);
    if (effect->pos.y < ceiling) {
        effect->pos.y = ceiling;
        effect->fall_speed = MAX(1, -effect->fall_speed);
    }

    const int32_t height = Room_GetHeight(sector, effect->pos);
    if (effect->pos.y >= height) {
        if (effect->counter > 0) {
            effect->speed = 0;
            effect->frame_num = 0;
            effect->counter = 0;
            effect->object_id = O_EXPLOSION_1;
            effect->shade = SHADE_NEUTRAL;
            Sound_Effect(SFX_EXPLOSION_1, &effect->pos, SPM_NORMAL);
        } else {
            Effect_Destroy(effect_num);
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
            Effect_Destroy(effect_num);
        }
    }

    if (room_num != effect->room_num) {
        Effect_UpdateRoom(effect_num, room_num);
    }
}

static void M_Control_TR3(const int16_t effect_num)
{
    int32_t lp;

    EFFECT *const effect = Effect_Get(effect_num);
    effect->rot.x += 5 * DEG_1;
    effect->rot.z += 10 * DEG_1;
    effect->fall_speed += 3;
    const XYZ_32 step =
        XYZ_32_RotateYaw((XYZ_32) { .z = effect->speed }, effect->rot.y);
    effect->pos.x += step.x >> 2;
    effect->pos.y += effect->fall_speed;
    effect->pos.z += step.z >> 2;

    const int32_t time4 = (int32_t)Output_GetTimeInGame() * 4;
    if (!(time4 & 0xC)) {
        if (effect->counter & 1) {
            Sparks_TriggerFireFlame(effect->pos, effect_num, 0);
        }

        if (effect->counter & 2) {
            Sparks_TriggerFireSmoke(effect->pos, -1, 0);
        }
    }

    int16_t room_num = effect->room_num;
    SECTOR *const sector = Room_GetSector(effect->pos, &room_num);
    int32_t c = Room_GetCeiling(sector, effect->pos);

    if (effect->pos.y < c) {
        effect->pos.y = c;
        effect->fall_speed = -effect->fall_speed;
    }

    int32_t h = Room_GetHeight(sector, effect->pos);

    if (effect->pos.y >= h) {
        if (effect->counter & 3) {
            for (int32_t i = 0; i < 3; i++) {
                if (effect->counter & 1) {
                    Sparks_TriggerFireFlame(
                        (XYZ_32) { effect->pos.x, h, effect->pos.z }, -1, 0);
                }
                if (effect->counter & 2) {
                    Sparks_TriggerFireSmoke(
                        (XYZ_32) { effect->pos.x, h, effect->pos.z }, -1, 0);
                }
            }
            Sound_Effect(SFX_EXPLOSION_1, &effect->pos, SPM_NORMAL);
        }

        Effect_Destroy(effect_num);
        return;
    }

    if (Lara_IsNearItem(&effect->pos, effect->counter & ~3)) {
        Lara_TakeDamage(effect->counter >> 2, true);

        if (effect->counter & 3) {
            for (int32_t i = 0; i < 3; i++) {
                if (effect->counter & 1) {
                    Sparks_TriggerFireFlame(
                        (XYZ_32) { effect->pos.x, h, effect->pos.z }, -1, 0);
                }
                if (effect->counter & 2) {
                    Sparks_TriggerFireSmoke(
                        (XYZ_32) { effect->pos.x, h, effect->pos.z }, -1, 0);
                }
            }

            Sound_Effect(SFX_EXPLOSION_1, &effect->pos, SPM_NORMAL);
        }

        Effect_Destroy(effect_num);
    }

    if (effect->room_num != room_num) {
        Effect_UpdateRoom(effect_num, room_num);
    }
}

// TR4 has no explosion sprite object, so the burst is made of sparks the way
// grenades and mines do it (TriggerExplosionSparks in effect2.cpp).
static void M_SpawnTR4Explosion(const XYZ_32 pos, const int16_t room_num)
{
    Sparks_TriggerExplosionSparks(pos, 3, -2, 0, room_num);
    for (int32_t i = 0; i < 2; i++) {
        Sparks_TriggerExplosionSparks(pos, 3, -1, 0, room_num);
    }
    Sound_Effect(SFX_EXPLOSION_1, &pos, SPM_NORMAL);
}

static void M_SpawnTR4Shatter(
    const EFFECT *const effect, const int32_t xz_vel, const int32_t face_count)
{
    const SHATTER_ITEM shatter_item = {
        .mesh = Object_GetMesh(effect->frame_num),
        .pos = effect->pos,
        .yaw = effect->rot.y,
        .flags = effect->flag1 & 0x400,
    };
    FX_Debris_ShatterItem(&shatter_item, face_count, effect->room_num, xz_vel);
}

// Port of the TR4 ControlBodyPart (missile.cpp). OG only shatters the part
// into debris on landing; we trail fire sparks and burst instead, since the
// parts here come from an exploding death.
static void M_Control_TR4(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const XYZ_32 old_pos = effect->pos;

    if (effect->speed != 0) {
        effect->rot.x += effect->fall_speed * 4;
    }
    effect->fall_speed += GRAVITY;
    effect->pos = XYZ_32_OffsetYaw(effect->pos, effect->rot.y, effect->speed);
    effect->pos.y += effect->fall_speed;

    // Collapsible tiles don't burn/explode
    const bool do_burn_effects = (effect->flag1 & 0x800) == 0;

    const int32_t time4 = (int32_t)Output_GetTimeInGame() * 4;
    if ((time4 & 0xC) == 0 && (effect->counter & 3) != 0 && do_burn_effects) {
        Sparks_TriggerFireFlame(effect->pos, effect_num, 0);
    }

    int16_t room_num = effect->room_num;
    const SECTOR *const sector = Room_GetSector(effect->pos, &room_num);

    const int32_t ceiling = Room_GetCeiling(sector, effect->pos);
    if (effect->pos.y < ceiling) {
        effect->pos.y = ceiling;
        effect->fall_speed = -effect->fall_speed;
        effect->speed -= effect->speed >> 3;
    }

    const int32_t height = Room_GetHeight(sector, effect->pos);
    if (effect->pos.y >= height) {
        if ((effect->counter & 3) != 0 && do_burn_effects) {
            M_SpawnTR4Explosion(
                (XYZ_32) { effect->pos.x, height, effect->pos.z }, room_num);
            Effect_Destroy(effect_num);
            return;
        }

        if ((effect->counter & 1) != 0 && !do_burn_effects) {
            if ((effect->flag1 & 0x200) != 0) {
                M_SpawnTR4Shatter(effect, -2, 32);
            } else {
                M_SpawnTR4Shatter(effect, -1, 32);
            }
            Sound_Effect(SFX_ROCK_FALL_LAND, &effect->pos, SPM_NORMAL);
            Effect_Destroy(effect_num);
            return;
        }

        if (old_pos.y <= height) {
            if (effect->fall_speed <= 32) {
                effect->fall_speed = 0;
            } else {
                effect->fall_speed = -effect->fall_speed >> 2;
            }
        } else {
            effect->rot.y += DEG_180;
            effect->pos.x = old_pos.x;
            effect->pos.z = old_pos.z;
        }

        effect->speed -= effect->speed >> 2;
        if (ABS(effect->speed) < 4) {
            effect->speed = 0;
        }
        effect->pos.y = old_pos.y;
    }

    if (effect->speed == 0) {
        effect->flag1++;
        if (effect->flag1 > 32) {
            Effect_Destroy(effect_num);
            return;
        }
    }

    if (room_num != effect->room_num) {
        Effect_UpdateRoom(effect_num, room_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    switch (g_TRVersion) {
    case 4:
        obj->effect_control_func = M_Control_TR4;
        break;
    case 3:
        obj->effect_control_func = M_Control_TR3;
        break;
    default:
        obj->effect_control_func = M_Control_TR12;
        break;
    }
    obj->loaded = true;
    obj->mesh_count = 0;
}

REGISTER_OBJECT(O_BODY_PART, M_Setup)
