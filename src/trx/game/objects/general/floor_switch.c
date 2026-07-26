#include <trx/game/inventory.h>
#include <trx/game/inventory_ring/control.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/version.h>

typedef enum {
    M_STATE_OFF = 0,
    M_STATE_ON = 1,
} M_STATE;

typedef struct {
    OBJECT_BOUNDS bounds;
    XYZ_32 position;
    M_STATE item_goal_state;
    LARA_TRX_ANIMATION lara_animation;
    bool flip_lara;
    bool requires_crowbar;
} M_INTERACTION;

static const M_INTERACTION m_InteractFloorOff = {
    .bounds = {
        .shift = {
            .min = { .x = -STEP_L, .y = 0, .z = -STEP_L * 3, },
            .max = { .x = +STEP_L, .y = 0, .z = -224, },
        },
        .rot = {
            .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
            .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
        },
    },
    .position = { .x = 0, .y = 0, .z = -550 },
    .item_goal_state = M_STATE_ON,
    .lara_animation = LA_LEVERSWITCH_PUSH,
    .flip_lara = false,
    .requires_crowbar = false,
};

static const M_INTERACTION m_InteractFloorOn = {
    .bounds = {
        .shift = {
            .min = { .x = -STEP_L, .y = 0, .z = 224, },
            .max = { .x = +STEP_L, .y = 0, .z = STEP_L * 3, },
        },
        .rot = {
            .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
            .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
        },
    },
    .position = { .x = 0, .y = 0, .z = 550 },
    .item_goal_state = M_STATE_OFF,
    .lara_animation = LA_LEVERSWITCH_PUSH,
    .flip_lara = true,
    .requires_crowbar = false,
};

static const M_INTERACTION m_InteractCrowbarOff = {
    .bounds = {
        .shift = {
            .min = { .x = -STEP_L, .y = 0, .z = -STEP_L * 2, },
            .max = { .x = +STEP_L, .y = 0, .z = -STEP_L, },
        },
        .rot = {
            .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
            .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
        },
    },
    .position = { .x = -89, .y = 0, .z = -328 },
    .item_goal_state = M_STATE_ON,
    .lara_animation = LA_CROWBAR_USE_ON_FLOOR,
    .flip_lara = false,
    .requires_crowbar = true,
};

static const M_INTERACTION m_InteractCrowbarOn = {
    .bounds = {
        .shift = {
            .min = { .x = -STEP_L, .y = 0, .z = STEP_L, },
            .max = { .x = +STEP_L, .y = 0, .z = STEP_L * 2, },
        },
        .rot = {
            .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
            .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
        },
    },
    .position = { .x = 89, .y = 0, .z = 328 },
    .item_goal_state = M_STATE_OFF,
    .lara_animation = LA_CROWBAR_USE_ON_FLOOR,
    .flip_lara = true,
    .requires_crowbar = true,
};

static const M_INTERACTION *M_GetInteraction(const ITEM *const item)
{
    const bool is_crowbar_type = item->object_id == O_SWITCH_TYPE_CROWBAR;
    switch (item->current_anim_state) {
    case M_STATE_OFF:
        return is_crowbar_type ? &m_InteractCrowbarOff : &m_InteractFloorOff;
    case M_STATE_ON:
        return is_crowbar_type ? &m_InteractCrowbarOn : &m_InteractFloorOn;
    default:
        return nullptr;
    }
}

static bool M_ShowCrowbarInventory(void)
{
    if (Inv_RequestItem(O_CROWBAR_ITEM) == 0) {
        return false;
    }

    InvRing_SetRequestedObjectID(O_CROWBAR_OPTION);
    const GF_COMMAND gf_cmd = GF_ShowInventory(INV_KEYS_MODE);
    if (gf_cmd.action != GF_NOOP) {
        GF_OverrideCommand(gf_cmd);
    }
    return true;
}

static bool M_Interact(ITEM *const item, ITEM *const lara_item)
{
    const M_INTERACTION *const interaction = M_GetInteraction(item);
    if (interaction == nullptr) {
        return false;
    }

    const int16_t item_num = Item_GetIndex(item);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (interaction->flip_lara) {
        lara_item->rot.y += DEG_180;
    }

    bool result = false;
    if (Lara_TestPosition(item, &interaction->bounds)) {
        if (!lara->interact_target.is_moving && interaction->requires_crowbar) {
            if (interaction->flip_lara) {
                lara_item->rot.y += DEG_180;
            }
            const bool inv_result = M_ShowCrowbarInventory();
            if (interaction->flip_lara) {
                lara_item->rot.y += DEG_180;
            }

            if (!inv_result) {
                Lara_RefuseInteraction();
            }

            if (!inv_result || lara->interact_target.item_num != item_num) {
                goto finish;
            }
        }

        if (lara_item->current_anim_state == LS(LS_STOP)) {
            lara->interact_target.is_moving = false;
        }

        if (Lara_MovePosition(item, &interaction->position)) {
            Item_SwitchToAnim(lara_item, LA(interaction->lara_animation), 0);
            const ANIM *const anim = Item_GetAnim(lara_item);
            lara_item->current_anim_state = anim->current_anim_state;
            lara_item->goal_anim_state = anim->current_anim_state;
            item->goal_anim_state = interaction->item_goal_state;
            result = true;
        } else {
            lara->interact_target.item_num = item_num;
        }
    } else if (Lara_Interact_HasActiveTarget(item_num)) {
        lara->interact_target.is_moving = false;
        lara->gun_status = LGS_ARMLESS;
    }

finish:
    if (interaction->flip_lara) {
        lara_item->rot.y += DEG_180;
    }

    return result;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (Lara_Interact_CanControl(LARA_INTERACT_FLOOR_SWITCH, item_num)
        && M_Interact(item, lara_item)) {
        Lara_Interact_FinishControl(LARA_INTERACT_FLOOR_SWITCH);
        Item_AddSimulated(item_num);
        Item_Animate(item);
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!lara->interact_target.is_moving) {
        Object_Collision(item_num, lara_item, coll);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->trigger.mask = TRIGGER_MASK_ALL;
    if (!Item_IsTriggerActive(item)) {
        item->goal_anim_state = M_STATE_ON;
        item->timer = 0;
    }
    Item_Animate(item);

    if (g_TRVersion >= 3 && item->trigger.switch_spent) {
        item->trigger.switch_spent = false;
        item->trigger.spent = true;
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_SWITCH_TYPE_FLOOR, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_CROWBAR, M_Setup)
