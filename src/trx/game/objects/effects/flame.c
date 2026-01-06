#include <trx/game/objects/effects/flame.h>

#include <trx/config.h>
#include <trx/game/effects.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/utils.h>
#include <trx/version.h>

#define M_LIGHT_INTENSITY 11
#define M_LIGHT_FALLOFF 10
#define M_DAMAGE_PROXIMITY 600
#define M_IGNITE_PROXIMITY (g_TRVersion == 1 ? 300 : 450)
#define M_TOO_NEAR_DAMAGE (g_TRVersion == 1 ? 3 : 5)
#define M_ON_FIRE_DAMAGE (g_TRVersion == 1 ? 5 : 7)

static void M_TR3_SideFlameDetection(
    const EFFECT *const effect, const int32_t length)
{
    ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - effect->pos.x;
    const int32_t dz = lara_item->pos.z - effect->pos.z;

    const int32_t max_dist = 20 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    int32_t x, z, xs, xe, zs, ze;
    switch (effect->rot.y) {
    case 0:
        x = effect->pos.x;
        z = effect->pos.z + 512;
        xs = -256;
        xe = 256;
        zs = -length;
        ze = 0;
        break;

    case DEG_90:
        x = effect->pos.x + 512;
        z = effect->pos.z;
        xs = -length;
        xe = 0;
        zs = -256;
        ze = 256;
        break;

    case -DEG_90:
        x = effect->pos.x - 512;
        z = effect->pos.z;
        xs = 0;
        xe = length;
        zs = -256;
        ze = 256;
        break;

    case -DEG_180:
        x = effect->pos.x;
        z = effect->pos.z - 512;
        xs = -256;
        xe = 256;
        zs = 0;
        ze = length;
        break;

    default:
        x = 0;
        z = 0;
        xs = 0;
        xe = 0;
        zs = 0;
        ze = 0;
        break;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(lara_item);
    if (lara_item->pos.x >= x + xs && lara_item->pos.x <= x + xe
        && lara_item->pos.z >= z + zs && lara_item->pos.z <= z + ze
        && lara_item->pos.y + bounds->min.y <= effect->pos.y + 128
        && lara_item->pos.y + bounds->max.y >= effect->pos.y - 384) {
        if (effect->flag1 >= 18) {
            Lara_CatchFire();
        } else {
            lara_item->hit_points -= 5;
            lara_item->hit_status = 1;
        }
    }
}

static void M_TR3_Control(const int16_t effect_num)
{
    XYZ_32 pos;
    int32_t rnd;
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t angle;
    int32_t rad;
    int32_t dist;
    int32_t f2;
    int32_t r;
    int32_t g;
    int32_t b;
    uint8_t xz_offsets[16][2] = {
        { 9, 9 },   { 24, 9 },  { 40, 9 },  { 55, 9 },  { 9, 24 },  { 24, 24 },
        { 40, 24 }, { 55, 24 }, { 9, 40 },  { 24, 40 }, { 40, 40 }, { 55, 40 },
        { 9, 55 },  { 24, 55 }, { 40, 55 }, { 55, 55 },
    };

    EFFECT *const effect = Effect_Get(effect_num);
    rnd = Random_GetControl();
    rad = 0;

    const int32_t time4 = Output_GetTimeInGame() * 4;

    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (effect->frame_num == FLAME_BIG) {
        if (!(time4 & 0xC)) {
            Sparks_TriggerFireFlame(effect->pos, -1, 0);
            Sparks_TriggerFireSmoke(effect->pos, -1, 0);
        }

        Sparks_TriggerStaticFlame(
            effect->pos, (Random_GetControl() & 0xF) + 96);
    } else if (effect->frame_num == FLAME_SMALL) {
        if (effect->counter >= 0) {
            angle = ((effect->rot.y >> 4) & 4095) << 1;
            const int32_t s = (288 * Math_Sin(angle << 3)) >> W2V_SHIFT;
            const int32_t c = (288 * Math_Cos(angle << 3)) >> W2V_SHIFT;

            Sparks_TriggerStaticFlame(
                (XYZ_32) {
                    .x = effect->pos.x + s,
                    .y = effect->pos.y - 192,
                    .z = effect->pos.z + c,
                },
                (Random_GetControl() & 15) + 32);

            if (!(time4 & 0x18)) {
                Sparks_TriggerFireFlame(
                    (XYZ_32) {
                        effect->pos.x + s,
                        effect->pos.y - 224,
                        effect->pos.z + c,
                    },
                    -1, 1);

                if (!(time4 & 0x18)) {
                    Sparks_TriggerFireSmoke(
                        (XYZ_32) {
                            effect->pos.x + s,
                            effect->pos.y,
                            effect->pos.z + c,
                        },
                        -1, 1);
                }
            }
        } else {
            if (lara->water_status == LWS_CHEAT) {
                effect->counter = 0;
                Effect_Kill(effect_num);
                lara->burn = 0;
                return;
            }

            for (int i = 0; i < LM_NUMBER_OF; i++) {
                if (!(time4 & 0xC)) {
                    effect->pos.x = 0;
                    effect->pos.y = 0;
                    effect->pos.z = 0;
                    Collide_GetJointAbsPosition(lara_item, &effect->pos, i);
#if 0
                        // TODO: implement this for Willard
                        Sparks_TriggerFireFlame(effect->pos, -1, 255 - lara->burn_green);
#else
                    Sparks_TriggerFireFlame(effect->pos, -1, 255);
#endif
                }
            }

#if 0
            // TODO: implement this for Willard
            if (lara->burn_green) {
                r = (rnd >> 2) & 0x3F;
                g = (rnd & 0x3F) + 192;
                b = ((rnd >> 4) & 0x1F) + 96;
            } else
#endif
            {
                r = (rnd & 0x3F) + 192;
                g = ((rnd >> 4) & 0x1F) + 96;
                b = 0;
            }

            Output_AddDynamicLightRGB(
                lara_item->pos, 13, (RGB_888) { r, g, b });

            if (lara_item->room_num != effect->room_num) {
                Effect_NewRoom(effect_num, lara_item->room_num);
            }

            const int32_t wh = Room_GetWaterHeight(
                effect->pos.x, effect->pos.y, effect->pos.z, effect->room_num);

            if (wh == NO_HEIGHT || effect->pos.y <= wh
                || (Room_Get(effect->room_num)->flags.swamp
                    && (GF_BadGetLevelNum() == 12 || GF_BadGetLevelNum() == 4
                        || GF_BadGetLevelNum() == 19))) {
                Sound_Effect(
                    SFX_LOOP_FOR_SMALL_FIRES, &effect->pos, SPM_NORMAL);
                Lara_TakeDamage(M_ON_FIRE_DAMAGE, true);
            } else {
                effect->counter = 0;
                Effect_Kill(effect_num);
                lara->burn = false;
            }

            return;
        }
    } else if (effect->frame_num == FLAME_JET) {
        if (effect->flag1) {
            effect->flag1--;
        } else {
            effect->flag1 = (Random_GetControl() & 3) + 8;
            f2 = Random_GetControl() & 0x3F;

            if (effect->flag2 == f2) {
                f2 = (f2 + 13) & 0x3F;
            }

            effect->flag2 = (int16_t)f2;
        }

        x = (xz_offsets[effect->flag2 & 7][0] << 4) - 512;
        z = (xz_offsets[effect->flag2 & 7][1] << 4) - 512;

        if (!(time4 & 4)) {
            Sparks_TriggerFireFlame(
                (XYZ_32) {
                    effect->pos.x + x,
                    effect->pos.y,
                    effect->pos.z + z,
                },
                -1, 2);
        }

        x = (xz_offsets[(effect->flag2 >> 3) + 8][0] << 4) - 512;
        z = (xz_offsets[(effect->flag2 >> 3) + 8][1] << 4) - 512;

        if (time4 & 4) {
            Sparks_TriggerFireFlame(
                (XYZ_32) {
                    effect->pos.x + x,
                    effect->pos.y,
                    effect->pos.z + z,
                },
                -1, 2);
        }
    } else {
        angle = (effect->rot.y >> 3) & 0x1FFE;
        dist = (rnd & 0xFF) + 512;
        const int32_t s = (dist * Math_Sin(angle << 3)) >> W2V_SHIFT;
        const int32_t c = (dist * Math_Cos(angle << 3)) >> W2V_SHIFT;

        if (effect->flag2) {
            if (time4 & 4) {
                Sparks_TriggerSideFlame(
                    (XYZ_32) {
                        effect->pos.x + s,
                        effect->pos.y,
                        effect->pos.z + c,
                    },
                    ((angle - 4096) & 0x1FFF) << 3,
                    (!(Random_GetControl() & 7)) ? 1 : 0, 1);
            }

            effect->flag2--;
        } else {
            if (effect->flag1) {
                if (time4 & 4) {
                    if (effect->flag1 > 112) {
                        Sparks_TriggerSideFlame(
                            (XYZ_32) {
                                effect->pos.x + s,
                                effect->pos.y,
                                effect->pos.z + c,
                            },
                            ((angle + 4096) & 0x1FFF) << 3,
                            (129 - effect->flag1) >> 1, 0);
                    } else if (effect->flag1 < 18) {
                        Sparks_TriggerSideFlame(
                            (XYZ_32) {
                                effect->pos.x + s,
                                effect->pos.y,
                                effect->pos.z + c,
                            },
                            ((angle + 4096) & 0x1FFF) << 3,
                            (effect->flag1 >> 1) + 1, 0);
                    } else {
                        Sparks_TriggerSideFlame(
                            (XYZ_32) {
                                effect->pos.x + s,
                                effect->pos.y,
                                effect->pos.z + c,
                            },
                            ((angle + 4096) & 0x1FFF) << 3, 9, 0);
                    }
                }

                effect->flag1 -= 2;
            } else {
                effect->flag1 = 128;

                // TODO: do not hardcode this
                if (GF_BadGetLevelNum() == 7) {
                    effect->flag2 = 120;
                } else {
                    effect->flag2 = 60;
                }
            }
        }
    }

    x = effect->pos.x + ((rnd & 0xF) << 5);
    y = effect->pos.y + ((rnd & 0xF0) << 1);
    z = effect->pos.z + ((rnd >> 3) & 0x1E0);
    r = (rnd & 0x3F) + 192;
    g = ((rnd >> 4) & 0x1F) + 96;
    b = 0;

    if (effect->frame_num == FLAME_SIDE) {
        if (effect->flag2) {
            dist = 0;
        } else if (effect->flag1 < 18) {
            dist = 2048;
        } else if (effect->flag1 < 64) {
            dist = 2048;
        } else {
            dist = (128 - effect->flag1) << 5;
        }

        angle = (((effect->rot.y >> 3) & 0xFFFE) - 4096) & 0x1FFE;
        const int32_t s = (dist * Math_Sin(angle << 3)) >> W2V_SHIFT;
        const int32_t c = (dist * Math_Cos(angle << 3)) >> W2V_SHIFT;
        Output_AddDynamicLightRGB(
            (XYZ_32) { x + s, y, z + c }, effect->flag2 ? 6 : 13,
            (RGB_888) { r, g, b });
    } else {
        Output_AddDynamicLightRGB(
            (XYZ_32) { x, y, z }, 16 - (effect->frame_num << 2),
            (RGB_888) { r, g, b });
    }

    Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &effect->pos, SPM_NORMAL);

    if (effect->counter) {
        effect->counter--;
    } else if (effect->frame_num == FLAME_SIDE) {
        if (!lara->burn && rad) {
            M_TR3_SideFlameDetection(effect, rad);
        }
    } else if (effect->frame_num != FLAME_SMALL) {
        pos.x = effect->pos.x;
        pos.y = effect->pos.y;
        pos.z = effect->pos.z;

        if (Lara_IsNearItem(&pos, M_DAMAGE_PROXIMITY)) {
            x = lara_item->pos.x - pos.x;
            z = lara_item->pos.z - pos.z;
            dist = SQUARE(x) + SQUARE(z);
            Lara_TakeDamage(M_TOO_NEAR_DAMAGE, true);

            if (dist < 202500) {
                effect->counter = 100;
                Lara_CatchFire();
            }
        }
    }
}

static void M_TR12_DoEffects(const EFFECT *const effect)
{
    if (!Object_Get(O_FLAME)->loaded) {
        return;
    }

    Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &effect->pos, SPM_NORMAL);
    if (!g_Config.visuals.enable_fire_lighting) {
        return;
    }

    const int32_t random = Random_GetControl();
    const XYZ_32 light_pos = {
        .x = effect->pos.x + (random & 0x140) - 0xA0,
        .y = effect->pos.y - STEP_L - (random & 0x50),
        .z = effect->pos.z + (random & 0x140) - 0xA0,
    };

    if (random > 0x4000) {
        Output_AddDynamicLight(light_pos, M_LIGHT_INTENSITY, M_LIGHT_FALLOFF);
    } else if (random > 0x2000) {
        Output_AddDynamicLight(
            light_pos, M_LIGHT_INTENSITY - (random & 2), M_LIGHT_FALLOFF);
    } else {
        Output_AddDynamicLight(
            light_pos, M_LIGHT_INTENSITY, M_LIGHT_FALLOFF / 2);
    }
}

static void M_TR12_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    effect->frame_num--;
    if (effect->frame_num <= Object_Get(O_FLAME)->mesh_count) {
        effect->frame_num = 0;
    }

    if (effect->counter >= 0) {
        M_TR12_DoEffects(effect);
        if (effect->counter != 0) {
            effect->counter--;
        } else if (Lara_IsNearItem(&effect->pos, M_DAMAGE_PROXIMITY)) {
            Lara_TakeDamage(M_TOO_NEAR_DAMAGE, true);
            const int32_t dx = lara_item->pos.x - effect->pos.x;
            const int32_t dz = lara_item->pos.z - effect->pos.z;
            const int32_t dist = SQUARE(dx) + SQUARE(dz);
            if (dist < SQUARE(M_IGNITE_PROXIMITY)) {
                effect->counter = 100;
                Lara_CatchFire();
            }
        }
    } else {
        effect->pos.x = 0;
        effect->pos.y = 0;
        if (effect->counter == -1) {
            effect->pos.z = -100;
        } else {
            effect->pos.z = 0;
        }

        Collide_GetJointAbsPosition(
            lara_item, &effect->pos, -1 - effect->counter);
        const int16_t room_num = lara_item->room_num;
        if (room_num != effect->room_num) {
            Effect_NewRoom(effect_num, room_num);
        }

        const int32_t water_height = Room_GetWaterHeight(
            effect->pos.x, effect->pos.y, effect->pos.z, effect->room_num);
        if ((water_height != NO_HEIGHT && effect->pos.y > water_height)
            || lara_info->water_status == LWS_CHEAT) {
            effect->counter = 0;
            Effect_Kill(effect_num);
            lara_info->burn = false;
        } else {
            M_TR12_DoEffects(effect);
            Lara_TakeDamage(M_ON_FIRE_DAMAGE, false);
            lara_info->burn = true;
        }
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = g_TRVersion == 3 ? M_TR3_Control : M_TR12_Control;
    obj->semi_transparent = true;
}

REGISTER_OBJECT(O_FLAME, M_Setup)
