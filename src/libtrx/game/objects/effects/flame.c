#include "config.h"
#include "game/effects.h"
#include "game/lara.h"
#include "game/output.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "utils.h"
#include "version.h"

#define M_LIGHT_INTENSITY 11
#define M_LIGHT_FALLOFF 10
#define M_DAMAGE_PROXIMITY 600
#define M_IGNITE_PROXIMITY (g_TRVersion == 1 ? 300 : 450)
#define M_TOO_NEAR_DAMAGE (g_TRVersion == 1 ? 3 : 5)
#define M_ON_FIRE_DAMAGE (g_TRVersion == 1 ? 5 : 7)

static void M_DoEffects(const EFFECT *const effect)
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

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    effect->frame_num--;
    if (effect->frame_num <= Object_Get(O_FLAME)->mesh_count) {
        effect->frame_num = 0;
    }

    if (effect->counter >= 0) {
        M_DoEffects(effect);
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
            M_DoEffects(effect);
            Lara_TakeDamage(M_ON_FIRE_DAMAGE, false);
            lara_info->burn = true;
        }
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->semi_transparent = true;
}

REGISTER_OBJECT(O_FLAME, M_Setup)
