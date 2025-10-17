#include "game/stats.h"

#include <libtrx/config.h>
#include <libtrx/game/input.h>
#include <libtrx/game/inventory.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/lua/common.h>
#include <libtrx/game/lua/events.h>
#include <libtrx/game/objects.h>
#include <libtrx/game/objects/general/pickup.h>
#include <libtrx/game/overlay.h>

#define LF_PICKUP_ERASE 42
#define LF_PICKUP_UW 18

static XYZ_32 m_PickUpPosition = { 0, 0, -100 };
static XYZ_32 m_PickUpPositionUW = { 0, -200, -350 };

static void M_GetItem(int16_t item_num, ITEM *item, ITEM *lara_item)
{
    Overlay_AddDisplayPickup(item->object_id);
    Inv_AddPickup(item);

    item->status = IS_INVISIBLE;
    item->flags |= IF_KILLED;
    Item_RemoveDrawn(item_num);
    Item_RemoveActive(item_num);

    Stats_AddPickup();
    // Notify Lua pickup listeners
    Lua_FireEvent(LUA_EVENT_PICKUP, item_num + 1); // LUA uses 1-indexing

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->interact_target.is_moving = false;
}

static void M_GetAllAtLaraPos(ITEM *item, ITEM *lara_item)
{
    int16_t pickup_num = Room_Get(item->room_num)->item_num;
    while (pickup_num != NO_ITEM) {
        ITEM *const check_item = Item_Get(pickup_num);
        if (check_item->pos.x == item->pos.x && check_item->pos.z == item->pos.z
            && Object_Get(check_item->object_id)->collision_func
                == Pickup_Collision) {
            M_GetItem(pickup_num, check_item, lara_item);
        }
        pickup_num = check_item->next_item;
    }
}

static void M_CollisionControlled(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    if (item->status == IS_INVISIBLE) {
        return;
    }

    bool have_item = false;
    int16_t rotx = item->rot.x;
    int16_t roty = item->rot.y;
    int16_t rotz = item->rot.z;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status == LWS_ABOVE_WATER
        || lara->water_status == LWS_WADE) {
        if ((g_Input.action && lara->gun_status == LGS_ARMLESS
             && !lara_item->gravity
             && lara_item->current_anim_state == LS(LS_STOP)
             && !lara->interact_target.is_moving)
            || (lara->interact_target.is_moving
                && lara->interact_target.item_num == item_num)) {

            have_item = false;
            item->rot.x = 0;

            if (Lara_TestPosition(item, obj->bounds_func())) {
                m_PickUpPosition.y = lara_item->pos.y - item->pos.y;
                if (Lara_MovePosition(item, &m_PickUpPosition)) {
                    Item_SwitchToAnim(lara_item, LA(LA_PICKUP), 0);
                    lara_item->current_anim_state = LS(LS_PICKUP);
                    have_item = true;
                }
                lara->interact_target.item_num = item_num;
            } else if (
                lara->interact_target.is_moving
                && lara->interact_target.item_num == item_num) {
                lara->interact_target.is_moving = false;
                lara->interact_target.item_num = NO_ITEM;
                lara->gun_status = LGS_ARMLESS;
            }
            if (have_item) {
                lara->head_rot.y = 0;
                lara->head_rot.x = 0;
                lara->torso_rot.y = 0;
                lara->torso_rot.x = 0;
                lara->interact_target.is_moving = false;
                lara->gun_status = LGS_HANDS_BUSY;
            }
        } else if (
            lara->interact_target.item_num == item_num
            && lara_item->current_anim_state == LS(LS_PICKUP)) {
            if (Item_TestFrameEqual(lara_item, LF_PICKUP_ERASE)) {
                M_GetAllAtLaraPos(item, lara_item);
                lara->interact_target.item_num = NO_ITEM;
            }
        }
    } else if (
        lara->water_status == LWS_UNDERWATER
        || lara->water_status == LWS_CHEAT) {
        item->rot.x = -25 * DEG_1;

        if ((g_Input.action && lara_item->current_anim_state == LS(LS_TREAD)
             && lara->gun_status == LGS_ARMLESS
             && !lara->interact_target.is_moving)
            || (lara->interact_target.is_moving
                && lara->interact_target.item_num == item_num)) {

            if (Lara_TestPosition(item, obj->bounds_func())) {
                if (Lara_MovePosition(item, &m_PickUpPositionUW)) {
                    Item_SwitchToAnim(lara_item, LA(LA_UNDERWATER_PICKUP), 0);
                    lara_item->current_anim_state = LS(LS_PICKUP);

                    lara_item->goal_anim_state = LS(LS_TREAD);
                    lara->interact_target.is_moving = false;
                    lara->gun_status = LGS_HANDS_BUSY;
                }
                lara->interact_target.item_num = item_num;
            } else if (
                lara->interact_target.is_moving
                && lara->interact_target.item_num == item_num) {
                lara->interact_target.is_moving = false;
                lara->interact_target.item_num = NO_ITEM;
                lara->gun_status = LGS_ARMLESS;
            }
        } else if (
            lara->interact_target.item_num == item_num
            && lara_item->current_anim_state == LS(LS_PICKUP)
            && Item_TestFrameEqual(lara_item, LF_PICKUP_UW)) {
            M_GetAllAtLaraPos(item, lara_item);
            lara->gun_status = LGS_ARMLESS;
            lara->interact_target.item_num = NO_ITEM;
        }
    }

    item->rot.x = rotx;
    item->rot.y = roty;
    item->rot.z = rotz;
}

void Pickup_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (g_Config.gameplay.enable_walk_to_items) {
        M_CollisionControlled(item_num, lara_item, coll);
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);
    int16_t rotx = item->rot.x;
    int16_t roty = item->rot.y;
    int16_t rotz = item->rot.z;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status == LWS_ABOVE_WATER
        || lara->water_status == LWS_WADE) {
        item->rot.x = 0;
        if (!Lara_TestPosition(item, obj->bounds_func())) {
            goto cleanup;
        }

        if (lara_item->current_anim_state == LS(LS_PICKUP)) {
            if (!Item_TestFrameEqual(lara_item, LF_PICKUP_ERASE)) {
                goto cleanup;
            }
            M_GetAllAtLaraPos(item, lara_item);
            goto cleanup;
        }

        if (g_Input.action && lara->gun_status == LGS_ARMLESS
            && !lara_item->gravity
            && lara_item->current_anim_state == LS(LS_STOP)) {
            Lara_AlignPosition(item, &m_PickUpPosition);
            Lara_AnimateUntil(lara_item, LS(LS_PICKUP));
            lara_item->goal_anim_state = LS(LS_STOP);
            lara->gun_status = LGS_HANDS_BUSY;
            goto cleanup;
        }
    } else if (
        lara->water_status == LWS_UNDERWATER
        || lara->water_status == LWS_CHEAT) {
        item->rot.x = -25 * DEG_1;
        if (!Lara_TestPosition(item, obj->bounds_func())) {
            goto cleanup;
        }

        if (lara_item->current_anim_state == LS(LS_PICKUP)) {
            if (!Item_TestFrameEqual(lara_item, LF_PICKUP_UW)) {
                goto cleanup;
            }
            M_GetAllAtLaraPos(item, lara_item);
            goto cleanup;
        }

        if (g_Input.action && lara_item->current_anim_state == LS(LS_TREAD)) {
            if (!Lara_MovePosition(item, &m_PickUpPositionUW)) {
                goto cleanup;
            }
            Lara_AnimateUntil(lara_item, LS(LS_PICKUP));
            lara_item->goal_anim_state = LS(LS_TREAD);
        }
    }

cleanup:
    item->rot.x = rotx;
    item->rot.y = roty;
    item->rot.z = rotz;
}
