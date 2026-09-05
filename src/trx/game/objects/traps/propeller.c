#include <trx/game/objects/traps/propeller.h>

#include <trx/game/lara.h>
#include <trx/game/lara/common.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

typedef enum {
    // clang-format off
    M_STATE_ON  = 0,
    M_STATE_OFF = 1,
    // clang-format on
} M_STATE;

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsInPlay(item) && item->object_id == O_PROPELLER_2
        && item->current_anim_state == M_STATE_OFF) {
        Object_Collision(item_num, lara_item, coll);
    } else {
        Object_Collision_Trap(item_num, lara_item, coll);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->priv_size = sizeof(PROPELLER_PRIV);
    obj->control_func = Propeller_Control;
    obj->collision_func = M_Collision;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            PROPELLER_PRIV, damage, PROPELLER_DEFAULT_DAMAGE,
            "Damage dealt while Lara is touching the propeller."));
}

static void M_SpawnBlood(const ITEM *const item, const int32_t count)
{
    const ITEM *const lara_item = Lara_GetItem();
    const XYZ_32 pos = lara_item->pos;
    Spawn_BloodBath(
        pos.x, pos.y - WALL_L / 2, pos.z, Random_GetDraw() >> 10,
        item->rot.y + DEG_90, lara_item->room_num, count);
}

void Propeller_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const PROPELLER_PRIV *const p = item->priv;

    if (Item_IsTriggerActive(item) && !item->trigger.spent) {
        item->goal_anim_state = M_STATE_ON;
        item->is_collidable = true;

        if ((item->touch_bits & 6) != 0) {
            const ITEM *const lara_item = Lara_GetItem();
            Lara_TakeDamage(p->damage, true);
            if (lara_item->hit_points <= 0) {
                M_SpawnBlood(item, 5);
            }

            M_SpawnBlood(item, 3);

            if (item->object_id == O_POWER_SAW) {
                Sound_Effect(SFX_SAW_STOP, &item->pos, SPM_NORMAL);
            }
        } else if (item->object_id == O_POWER_SAW) {
            Sound_Effect(SFX_SAW_REVVING, &item->pos, SPM_NORMAL);
        } else if (item->object_id == O_PROPELLER_1) {
            Sound_Effect(SFX_AIRPLANE_IDLE, &item->pos, SPM_NORMAL);
        } else if (item->object_id == O_PROPELLER_2) {
            Sound_Effect(SFX_UNDERWATER_FAN_ON, &item->pos, SPM_UNDERWATER);
        } else {
            Sound_Effect(SFX_SMALL_FAN_ON, &item->pos, SPM_NORMAL);
        }
    } else if (item->goal_anim_state != M_STATE_OFF) {
        if (item->object_id == O_PROPELLER_1) {
            Sound_Effect(SFX_AIRPLANE_IDLE, &item->pos, SPM_NORMAL);
        } else if (item->object_id == O_PROPELLER_2) {
            Sound_Effect(SFX_UNDERWATER_FAN_OFF, &item->pos, SPM_UNDERWATER);
        }
        item->goal_anim_state = M_STATE_OFF;
    }

    Item_Animate(item);

    if (item->is_finished) {
        Item_RemoveSimulated(item_num);
        if (item->object_id != O_POWER_SAW) {
            item->is_collidable = false;
        }
    }
}

REGISTER_OBJECT(O_PROPELLER_1, M_Setup)
REGISTER_OBJECT(O_PROPELLER_2, M_Setup)
REGISTER_OBJECT(O_PROPELLER_3, M_Setup)
