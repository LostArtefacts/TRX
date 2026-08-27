#include <trx/game/const.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/general/door.h>
#include <trx/game/rooms.h>

#define M_LF_PULL 10

typedef enum {
    M_STATE_OFF = 0,
    M_STATE_ON = 1,
} M_STATE;

typedef struct {
    int16_t door_item_num;
} M_PRIV;

static const OBJECT_BOUNDS m_CogSwitchBounds = {
    .shift = {
        .min = { .x = -WALL_L / 2, .y = +0, .z = -WALL_L - WALL_L / 2, },
        .max = { .x = +WALL_L / 2, .y = +0, .z = -WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const XYZ_32 m_CogSwitchPosition = { .x = 0, .y = 0, .z = -856 };

static int16_t M_FindLinkedDoor(const ITEM *const item)
{
    // The cog switch operates the lift door targeted by the trigger laid on
    // the switch's own sector.
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    if (sector->trigger == nullptr) {
        return NO_ITEM;
    }

    for (const TRIGGER_CMD *cmd = sector->trigger->command; cmd != nullptr;
         cmd = cmd->next_cmd) {
        if (cmd->type != TO_ITEM) {
            continue;
        }

        const int16_t door_item_num = (int16_t)(intptr_t)cmd->parameter;
        const ITEM *const door_item = Item_Get(door_item_num);
        if (door_item->object_id >= O_DOOR_1
            && door_item->object_id <= O_DOOR_8) {
            return door_item_num;
        }
    }
    return NO_ITEM;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    Item_Animate(item);

    if (item->current_anim_state == M_STATE_ON) {
        if (item->goal_anim_state == M_STATE_ON && !g_Input.action) {
            lara_item->goal_anim_state = LS(LS_STOP);
            item->goal_anim_state = M_STATE_OFF;
        }

        if (Item_TestAnimEqual(lara_item, LA(LA_COGWHEEL_PULL))
            && Item_TestFrameEqual(lara_item, M_LF_PULL)) {
            const M_PRIV *const p = item->priv;
            if (p->door_item_num != NO_ITEM) {
                Door_LiftActivate(Item_Get(p->door_item_num), 40);
            }
        }
    } else if (item->frame_num == Item_GetAnim(item)->frame_end) {
        item->current_anim_state = M_STATE_OFF;
        Item_RemoveSimulated(item_num);
        Item_SwitchToAnim(lara_item, LA(LA_STAND_STILL), 0);
        lara_item->current_anim_state = LS(LS_STOP);
        lara_item->goal_anim_state = LS(LS_STOP);
        lara->gun_status = LGS_ARMLESS;
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (!Item_IsInactive(item)) {
        return;
    }

    if (!item->trigger.spent
        && Lara_Interact_CanControl(LARA_INTERACT_SWITCH, item_num)) {
        if (Lara_TestPosition(item, &m_CogSwitchBounds)) {
            if (Lara_MovePosition(item, &m_CogSwitchPosition)) {
                Item_SwitchToAnim(lara_item, LA(LA_COGWHEEL_GRAB), 0);
                lara_item->current_anim_state = LS(LS_COG_SWITCH);
                lara_item->goal_anim_state = LS(LS_COG_SWITCH);
                Lara_Interact_FinishControl(LARA_INTERACT_SWITCH);
                Item_AddSimulated(item_num);
                item->goal_anim_state = M_STATE_ON;

                M_PRIV *const p = item->priv;
                p->door_item_num = M_FindLinkedDoor(item);
                if (p->door_item_num != NO_ITEM) {
                    ITEM *const door_item = Item_Get(p->door_item_num);
                    if (!Item_IsInPlay(door_item)) {
                        Item_AddSimulated(p->door_item_num);
                        Item_SetVisible(door_item, true);
                        Item_SetFinished(door_item, false);
                    }
                }
            } else {
                lara->interact_target.item_num = item_num;
            }
            return;
        }

        if (Lara_Interact_HasActiveTarget(item_num)) {
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_ARMLESS;
        }
    }

    Object_Collision(item_num, lara_item, coll);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->priv_size = sizeof(M_PRIV);
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_COG_SWITCH, M_Setup)
