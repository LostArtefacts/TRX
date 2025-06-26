#include "game/effects.h"
#include "game/lara.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "utils.h"

#define M_DAMAGE_PROXIMITY 600
#if TR_VERSION == 1
    #define M_IGNITE_PROXIMITY 300
    #define M_TOO_NEAR_DAMAGE 3
    #define M_ON_FIRE_DAMAGE 5
#else
    #define M_IGNITE_PROXIMITY 450
    #define M_TOO_NEAR_DAMAGE 5
    #define M_ON_FIRE_DAMAGE 7
#endif

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->semi_transparent = true;
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
        Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &effect->pos, SPM_ALWAYS);
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
            Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &effect->pos, SPM_ALWAYS);
            Lara_TakeDamage(M_ON_FIRE_DAMAGE, false);
            lara_info->burn = true;
        }
    }
}

REGISTER_OBJECT(O_FLAME, M_Setup)
