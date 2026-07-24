#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_DEFAULT_DAMAGE 100
#define M_TOUCH_BITS 0b00000010 // = 2
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_CUT,
} M_STATE;

typedef enum {
    // clang-format off
    M_ANIM_RETURN   = 0,
    M_ANIM_FINISHED = 1,
    M_ANIM_SET      = 2,
    M_ANIM_CUT      = 3,
    // clang-format on
} M_ANIM;

static void M_Initialise(const int16_t item_num)
{
    const OBJECT *const obj = Object_Get(O_BLADE);
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, M_ANIM_SET, 0);
    item->current_anim_state = M_STATE_STOP;
}

static void M_Stop(ITEM *const item)
{
    const int16_t anim_idx = Item_GetRelativeAnim(item);
    if (anim_idx == M_ANIM_CUT) {
        const ANIM *const anim = Item_GetAnim(item);
        if (!Item_IsTriggerActive(item) && anim->jump_anim_num == item->anim_num
            && Item_TestFrameEqual(item, -1)) {
            Item_RemoveSimulated(Item_GetIndex(item));
            return;
        }
    }

    item->goal_anim_state = M_STATE_STOP;
}

static int32_t M_GetDamage(const ITEM *const item)
{
    TRX_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, "damage", &damage)) {
        return damage.as_int;
    }

    return M_DEFAULT_DAMAGE;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (Item_IsTriggerActive(item)
        && item->current_anim_state == M_STATE_STOP) {
        item->goal_anim_state = M_STATE_CUT;
    } else {
        M_Stop(item);
    }

    if ((item->touch_bits & M_TOUCH_BITS) != 0
        && item->current_anim_state == M_STATE_CUT) {
        Lara_TakeDamage(M_GetDamage(item), true);

        const ITEM *const lara_item = Lara_GetItem();
        Spawn_BloodBath(
            lara_item->pos.x, item->pos.y - STEP_L, lara_item->pos.z,
            lara_item->speed, lara_item->rot.y, lara_item->room_num, 2);
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision_Trap;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "damage", M_DEFAULT_DAMAGE,
            "Damage dealt while Lara is touching the blade trap."));
}

REGISTER_OBJECT(O_BLADE, M_Setup)
