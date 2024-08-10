#include "game/objects/general/pickup.h"

#include "config.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/overlay.h"
#include "global/const.h"
#include "global/vars.h"

#define LF_PICKUP_ERASE 42
#define LF_PICKUP_UW 18

static XYZ_32 m_PickUpPosition = { 0, 0, -100 };
static XYZ_32 m_PickUpPositionUW = { 0, -200, -350 };

static const OBJECT_BOUNDS m_PickUpBounds = {
    .shift = {
        .min = { .x = -256, .y = -100, .z = -256, },
        .max = { .x = +256, .y = +100, .z = +256, },
    },
    .rot = {
        .min = { .x = -10 * PHD_DEGREE, .y = 0, .z = 0, },
        .max = { .x = +10 * PHD_DEGREE, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_PickUpBoundsControlled = {
    .shift = {
        .min = { .x = -256, .y = -200, .z = -256, },
        .max = { .x = +256, .y = +200, .z = +256, },
    },
    .rot = {
        .min = { .x = -10 * PHD_DEGREE, .y = 0, .z = 0, },
        .max = { .x = +10 * PHD_DEGREE, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_PickUpBoundsUW = {
    .shift = {
        .min = { .x = -512, .y = -512, .z = -512, },
        .max = { .x = +512, .y = +512, .z = +512, },
    },
    .rot = {
        .min = { .x = -45 * PHD_DEGREE, .y = -45 * PHD_DEGREE, .z = -45 * PHD_DEGREE, },
        .max = { .x = +45 * PHD_DEGREE, .y = +45 * PHD_DEGREE, .z = +45 * PHD_DEGREE, },
    },
};

static void PickUp_AddGunToMesh(
    const GAME_OBJECT_ID obj_num, const ITEM_INFO *lara_item);
static void PickUp_GetItem(
    int16_t item_num, ITEM_INFO *item, ITEM_INFO *lara_item);
static void PickUp_GetAllAtLaraPos(ITEM_INFO *item, ITEM_INFO *lara_item);

static void PickUp_AddGunToMesh(
    const GAME_OBJECT_ID obj_num, const ITEM_INFO *const lara_item)
{
    const bool lara_has_pistols = Inv_RequestItem(O_PISTOL_ITEM)
        || Inv_RequestItem(O_MAGNUM_ITEM) || Inv_RequestItem(O_UZI_ITEM);

    if (!Inv_RequestItem(O_SHOTGUN_ITEM) && obj_num == O_SHOTGUN_ITEM) {
        g_Lara.mesh_ptrs[LM_TORSO] =
            g_Meshes[g_Objects[O_SHOTGUN_ANIM].mesh_index + LM_TORSO];
    } else if (!lara_has_pistols && obj_num == O_PISTOL_ITEM) {
        g_Lara.mesh_ptrs[LM_THIGH_L] =
            g_Meshes[g_Objects[O_PISTOL_ANIM].mesh_index + LM_THIGH_L];
        g_Lara.mesh_ptrs[LM_THIGH_R] =
            g_Meshes[g_Objects[O_PISTOL_ANIM].mesh_index + LM_THIGH_R];
    } else if (!lara_has_pistols && obj_num == O_MAGNUM_ITEM) {
        g_Lara.mesh_ptrs[LM_THIGH_L] =
            g_Meshes[g_Objects[O_MAGNUM_ANIM].mesh_index + LM_THIGH_L];
        g_Lara.mesh_ptrs[LM_THIGH_R] =
            g_Meshes[g_Objects[O_MAGNUM_ANIM].mesh_index + LM_THIGH_R];
    } else if (!lara_has_pistols && obj_num == O_UZI_ITEM) {
        g_Lara.mesh_ptrs[LM_THIGH_L] =
            g_Meshes[g_Objects[O_UZI_ANIM].mesh_index + LM_THIGH_L];
        g_Lara.mesh_ptrs[LM_THIGH_R] =
            g_Meshes[g_Objects[O_UZI_ANIM].mesh_index + LM_THIGH_R];
    }
}

static void PickUp_GetItem(
    int16_t item_num, ITEM_INFO *item, ITEM_INFO *lara_item)
{
    if (Object_IsObjectType(item->object_number, g_GunObjects)) {
        PickUp_AddGunToMesh(item->object_number, lara_item);
    }
    Overlay_AddPickup(item->object_number);
    Inv_AddItem(item->object_number);
    item->status = IS_INVISIBLE;
    Item_RemoveDrawn(item_num);
    g_GameInfo.current[g_CurrentLevel].stats.pickup_count++;
    g_Lara.interact_target.is_moving = false;
}

static void PickUp_GetAllAtLaraPos(ITEM_INFO *item, ITEM_INFO *lara_item)
{
    int16_t pickup_num = g_RoomInfo[item->room_number].item_number;
    while (pickup_num != NO_ITEM) {
        ITEM_INFO *check_item = &g_Items[pickup_num];
        if (check_item->pos.x == item->pos.x && check_item->pos.z == item->pos.z
            && g_Objects[check_item->object_number].collision
                == Pickup_Collision) {
            PickUp_GetItem(pickup_num, check_item, lara_item);
        }
        pickup_num = check_item->next_item;
    }
}

void Pickup_Setup(OBJECT_INFO *obj)
{
    obj->draw_routine = Object_DrawPickupItem;
    obj->collision = Pickup_Collision;
    obj->save_flags = 1;
    obj->bounds = Pickup_Bounds;
}

const OBJECT_BOUNDS *Pickup_Bounds(void)
{
    if (g_Lara.water_status == LWS_UNDERWATER) {
        return &m_PickUpBoundsUW;
    } else if (g_Config.walk_to_items) {
        return &m_PickUpBoundsControlled;
    } else {
        return &m_PickUpBounds;
    }
}

void Pickup_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    if (g_Config.walk_to_items) {
        Pickup_CollisionControlled(item_num, lara_item, coll);
        return;
    }

    ITEM_INFO *item = &g_Items[item_num];
    const OBJECT_INFO *const obj = &g_Objects[item->object_number];
    int16_t rotx = item->rot.x;
    int16_t roty = item->rot.y;
    int16_t rotz = item->rot.z;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (g_Lara.water_status == LWS_ABOVE_WATER) {
        item->rot.x = 0;
        if (!Lara_TestPosition(item, obj->bounds())) {
            goto cleanup;
        }

        if (lara_item->current_anim_state == LS_PICKUP) {
            if (!Item_TestFrameEqual(lara_item, LF_PICKUP_ERASE)) {
                goto cleanup;
            }
            PickUp_GetAllAtLaraPos(item, lara_item);
            goto cleanup;
        }

        if (g_Input.action && g_Lara.gun_status == LGS_ARMLESS
            && !lara_item->gravity_status
            && lara_item->current_anim_state == LS_STOP) {
            Lara_AlignPosition(item, &m_PickUpPosition);
            Lara_AnimateUntil(lara_item, LS_PICKUP);
            lara_item->goal_anim_state = LS_STOP;
            g_Lara.gun_status = LGS_HANDS_BUSY;
            goto cleanup;
        }
    } else if (g_Lara.water_status == LWS_UNDERWATER) {
        item->rot.x = -25 * PHD_DEGREE;
        if (!Lara_TestPosition(item, obj->bounds())) {
            goto cleanup;
        }

        if (lara_item->current_anim_state == LS_PICKUP) {
            if (!Item_TestFrameEqual(lara_item, LF_PICKUP_UW)) {
                goto cleanup;
            }
            PickUp_GetAllAtLaraPos(item, lara_item);
            goto cleanup;
        }

        if (g_Input.action && lara_item->current_anim_state == LS_TREAD) {
            if (!Lara_MovePosition(item, &m_PickUpPositionUW)) {
                goto cleanup;
            }
            Lara_AnimateUntil(lara_item, LS_PICKUP);
            lara_item->goal_anim_state = LS_TREAD;
        }
    }

cleanup:
    item->rot.x = rotx;
    item->rot.y = roty;
    item->rot.z = rotz;
}

void Pickup_CollisionControlled(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    const OBJECT_INFO *const obj = &g_Objects[item->object_number];

    if (item->status == IS_INVISIBLE) {
        return;
    }

    bool have_item = false;
    int16_t rotx = item->rot.x;
    int16_t roty = item->rot.y;
    int16_t rotz = item->rot.z;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (g_Lara.water_status == LWS_ABOVE_WATER) {
        if ((g_Input.action && g_Lara.gun_status == LGS_ARMLESS
             && !lara_item->gravity_status
             && lara_item->current_anim_state == LS_STOP
             && !g_Lara.interact_target.is_moving)
            || (g_Lara.interact_target.is_moving
                && g_Lara.interact_target.item_num == item_num)) {

            have_item = false;
            item->rot.x = 0;

            if (Lara_TestPosition(item, obj->bounds())) {
                m_PickUpPosition.y = lara_item->pos.y - item->pos.y;
                if (Lara_MovePosition(item, &m_PickUpPosition)) {
                    Item_SwitchToAnim(lara_item, LA_PICKUP, 0);
                    lara_item->current_anim_state = LS_PICKUP;
                    have_item = true;
                }
                g_Lara.interact_target.item_num = item_num;
            } else if (
                g_Lara.interact_target.is_moving
                && g_Lara.interact_target.item_num == item_num) {
                g_Lara.interact_target.is_moving = false;
                g_Lara.interact_target.item_num = NO_OBJECT;
                g_Lara.gun_status = LGS_ARMLESS;
            }
            if (have_item) {
                g_Lara.head_rot.y = 0;
                g_Lara.head_rot.x = 0;
                g_Lara.torso_rot.y = 0;
                g_Lara.torso_rot.x = 0;
                g_Lara.interact_target.is_moving = false;
                g_Lara.gun_status = LGS_HANDS_BUSY;
            }
        } else if (
            g_Lara.interact_target.item_num == item_num
            && lara_item->current_anim_state == LS_PICKUP) {
            if (Item_TestFrameEqual(lara_item, LF_PICKUP_ERASE)) {
                PickUp_GetAllAtLaraPos(item, lara_item);
            }
        }
    } else if (g_Lara.water_status == LWS_UNDERWATER) {
        item->rot.x = -25 * PHD_DEGREE;

        if ((g_Input.action && lara_item->current_anim_state == LS_TREAD
             && g_Lara.gun_status == LGS_ARMLESS
             && !g_Lara.interact_target.is_moving)
            || (g_Lara.interact_target.is_moving
                && g_Lara.interact_target.item_num == item_num)) {

            if (Lara_TestPosition(item, obj->bounds())) {
                if (Lara_MovePosition(item, &m_PickUpPositionUW)) {
                    Item_SwitchToAnim(lara_item, LA_PICKUP_UW, 0);
                    lara_item->current_anim_state = LS_PICKUP;

                    lara_item->goal_anim_state = LS_TREAD;
                    g_Lara.interact_target.is_moving = false;
                    g_Lara.gun_status = LGS_HANDS_BUSY;
                }
                g_Lara.interact_target.item_num = item_num;
            } else if (
                g_Lara.interact_target.is_moving
                && g_Lara.interact_target.item_num == item_num) {
                g_Lara.interact_target.is_moving = false;
                g_Lara.interact_target.item_num = NO_OBJECT;
                g_Lara.gun_status = LGS_ARMLESS;
            }
        } else if (
            g_Lara.interact_target.item_num == item_num
            && lara_item->current_anim_state == LS_PICKUP
            && Item_TestFrameEqual(lara_item, LF_PICKUP_UW)) {
            PickUp_GetAllAtLaraPos(item, lara_item);
            g_Lara.gun_status = LGS_ARMLESS;
        }
    }
    item->rot.x = rotx;
    item->rot.y = roty;
    item->rot.z = rotz;
}

bool Pickup_Trigger(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (item->status != IS_INVISIBLE) {
        return false;
    }
    item->status = IS_DEACTIVATED;
    return true;
}
