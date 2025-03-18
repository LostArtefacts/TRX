#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/spawn.h"
#include "global/vars.h"

// clang-format off
#define BLADE_CUT_DAMAGE 100
#define BLADE_TOUCH_BITS 0b00000010 // = 2
// clang-format on

typedef enum {
    // clang-format off
    BLADE_STATE_EMPTY = 0,
    BLADE_STATE_STOP  = 1,
    BLADE_STATE_CUT   = 2,
    // clang-format on
} BLADE_STATE;

typedef enum {
    // clang-format off
    BLADE_ANIM_RETURN   = 0,
    BLADE_ANIM_FINISHED = 1,
    BLADE_ANIM_SET      = 2,
    BLADE_ANIM_CUT      = 3,
    // clang-format on
} BLADE_ANIM;

static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision_Trap;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Initialise(const int16_t item_num)
{
    const OBJECT *const obj = Object_Get(O_BLADE);
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, BLADE_ANIM_SET, 0);
    item->current_anim_state = BLADE_STATE_STOP;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (Item_IsTriggerActive(item)
        && item->current_anim_state == BLADE_STATE_STOP) {
        item->goal_anim_state = BLADE_STATE_CUT;
    } else {
        item->goal_anim_state = BLADE_STATE_STOP;
    }

    if ((item->touch_bits & BLADE_TOUCH_BITS) != 0
        && item->current_anim_state == BLADE_STATE_CUT) {
        Lara_TakeDamage(BLADE_CUT_DAMAGE, true);
        Spawn_BloodBath(
            g_LaraItem->pos.x, item->pos.y - STEP_L, g_LaraItem->pos.z,
            g_LaraItem->speed, g_LaraItem->rot.y, g_LaraItem->room_num, 2);
    }

    Item_Animate(item);
}

REGISTER_OBJECT(O_BLADE, M_Setup)
