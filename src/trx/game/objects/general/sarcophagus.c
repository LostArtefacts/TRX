#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/general/pickup.h>

#define M_PICKUP_FRAME 113

static const OBJECT_BOUNDS m_Bounds = {
    .shift = {
        .min = { .x = -STEP_L * 2, .y = -100, .z = -STEP_L * 2, },
        .max = { .x = +STEP_L * 2, .y = +100, .z = +0, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = 0, },
    },
};

static const XYZ_32 m_Position = { .x = 0, .y = 0, .z = -300 };

static bool M_CanInteract(const ITEM *const item)
{
    if (!Item_IsInactive(item)) {
        return false;
    }

    const int16_t item_num = Item_GetIndex(item);
    return Lara_Interact_CanControl(LARA_INTERACT_RECEPTACLE, item_num)
        || Lara_Interact_HasActiveTarget(item_num);
}

static bool M_Interact(const ITEM *const item, ITEM *const lara_item)
{
    const int16_t item_num = Item_GetIndex(item);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    bool result = false;
    if (Lara_TestPosition(item, &m_Bounds)) {
        if (Lara_MovePosition(item, &m_Position)) {
            Item_SwitchToAnim(lara_item, LA(LA_PICKUP_SARCOPHAGUS), 0);
            lara_item->current_anim_state = LS(LS_CONTROLLED);
            result = true;
        }
        lara->interact_target.item_num = item_num;
    } else if (Lara_Interact_HasActiveTarget(item_num)) {
        lara->interact_target.is_moving = false;
        lara->gun_status = LGS_ARMLESS;
    }

    return result;
}

static bool M_CanCollectPickups(
    const int16_t item_num, const ITEM *const lara_item)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    return lara->interact_target.item_num == item_num
        && Item_TestAnimEqual(lara_item, LA(LA_PICKUP_SARCOPHAGUS))
        && Item_GetRelativeFrame(lara_item) == M_PICKUP_FRAME;
}

static void M_CollectPickups(ITEM *const item)
{
    const GAME_VECTOR pos = { .pos = item->pos, .room_num = item->room_num };
    Pickup_Collect(pos, PICKUP_MODE_SARCOPHAGUS);
    Item_SetFinished(item, true);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->interact_target.item_num = NO_ITEM;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (M_CanInteract(item)) {
        if (M_Interact(item, lara_item)) {
            Lara_Interact_FinishControl(LARA_INTERACT_RECEPTACLE);
            Item_AddSimulated(item_num);
            item->trigger.mask = TRIGGER_MASK_ALL;
        }
    } else if (M_CanCollectPickups(item_num, lara_item)) {
        M_CollectPickups(item);
    } else {
        Object_Collision(item_num, lara_item, coll);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    if (item->is_finished) {
        Item_RemoveSimulated(item_num);
    } else if (Lara_GetLaraInfo()->interact_target.item_num == item_num) {
        Item_Animate(item);
    } else {
        Item_SwitchToAnim(item, 0, 0);
        Item_RemoveSimulated(item_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;
    obj->save_anim = true;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_SARCOPHAGUS, M_Setup)
