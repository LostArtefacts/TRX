#include "game/objects/general/pickup.h"

#include "game/gameflow.h"
#include "game/gun/gun.h"
#include "game/input.h"
#include "game/inventory/backpack.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/overlay.h"
#include "global/funcs.h"
#include "global/vars.h"

#define LF_PICKUP_ERASE 42
#define LF_PICKUP_FLARE 58
#define LF_PICKUP_FLARE_UW 20
#define LF_PICKUP_UW 18

static void M_DoPickup(int16_t item_num);
static void M_DoFlarePickup(int16_t item_num);

static void M_DoAboveWater(int16_t item, ITEM *lara_item);
static void M_DoUnderwater(int16_t item, ITEM *lara_item);

static void M_DoPickup(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->object_id == O_FLARE_ITEM) {
        return;
    }

    Overlay_AddDisplayPickup(item->object_id);
    Inv_AddItem(item->object_id);

    if ((item->object_id == O_SECRET_1 || item->object_id == O_SECRET_2
         || item->object_id == O_SECRET_3)
        && (g_SaveGame.statistics.secrets & 1)
                + ((g_SaveGame.statistics.secrets >> 1) & 1)
                + ((g_SaveGame.statistics.secrets >> 2) & 1)
            >= 3) {
        GF_ModifyInventory(g_CurrentLevel, 1);
    }

    item->status = IS_INVISIBLE;
    Item_RemoveDrawn(item_num);
}

static void M_DoFlarePickup(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    g_Lara.request_gun_type = LGT_FLARE;
    g_Lara.gun_type = LGT_FLARE;
    Gun_InitialiseNewWeapon();
    g_Lara.gun_status = LGS_SPECIAL;
    g_Lara.flare_age = (int32_t)item->data & 0x7FFF;
    Item_Kill(item_num);
}

static void M_DoAboveWater(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    const XYZ_16 old_rot = item->rot;

    item->rot.x = 0;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (!Item_TestPosition(g_PickupBounds, item, lara_item)) {
        item->rot = old_rot;
        return;
    }

    if (lara_item->current_anim_state == LS_PICKUP) {
        if (lara_item->frame_num
            == g_Anims[LA_PICKUP].frame_base + LF_PICKUP_ERASE) {
            M_DoPickup(item_num);
        }
        return;
    }

    if (lara_item->current_anim_state == LS_FLARE_PICKUP) {
        if (lara_item->frame_num
                == g_Anims[LA_FLARE_PICKUP].frame_base + LF_PICKUP_FLARE
            && item->object_id == O_FLARE_ITEM
            && g_Lara.gun_type != LGT_FLARE) {
            M_DoFlarePickup(item_num);
        }
        return;
    }

    if ((g_Input & IN_ACTION) && !lara_item->gravity
        && lara_item->current_anim_state == LS_STOP
        && g_Lara.gun_status == LGS_ARMLESS
        && (g_Lara.gun_type != LGT_FLARE || item->object_id != O_FLARE_ITEM)) {
        if (item->object_id == O_FLARE_ITEM) {
            lara_item->goal_anim_state = LS_FLARE_PICKUP;
            do {
                Lara_Animate(lara_item);
            } while (lara_item->current_anim_state != LS_FLARE_PICKUP);
            lara_item->goal_anim_state = LS_STOP;
            g_Lara.gun_status = LGS_HANDS_BUSY;
        } else {
            Item_AlignPosition(&g_PickupPosition, item, lara_item);
            lara_item->goal_anim_state = LS_PICKUP;
            do {
                Lara_Animate(lara_item);
            } while (lara_item->current_anim_state != LS_PICKUP);
            lara_item->goal_anim_state = LS_STOP;
            g_Lara.gun_status = LGS_HANDS_BUSY;
        }
        return;
    }

    item->rot = old_rot;
}

static void M_DoUnderwater(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    const XYZ_16 old_rot = item->rot;

    item->rot.x = -25 * PHD_DEGREE;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (!Item_TestPosition(g_PickupBoundsUW, item, lara_item)) {
        item->rot = old_rot;
        return;
    }

    if (lara_item->current_anim_state == LS_PICKUP) {
        if (lara_item->frame_num
            == g_Anims[LA_UNDERWATER_PICKUP].frame_base + LF_PICKUP_UW) {
            M_DoPickup(item_num);
        }
        return;
    }

    if (lara_item->current_anim_state == LS_FLARE_PICKUP) {
        if (lara_item->frame_num
                == g_Anims[LA_UNDERWATER_FLARE_PICKUP].frame_base
                    + LF_PICKUP_FLARE_UW
            && item->object_id == O_FLARE_ITEM
            && g_Lara.gun_type != LGT_FLARE) {
            M_DoFlarePickup(item_num);
            Flare_DrawMeshes();
        }
        return;
    }

    if ((g_Input & IN_ACTION) && lara_item->current_anim_state == LS_TREAD
        && g_Lara.gun_status == LGS_ARMLESS
        && (g_Lara.gun_type != LGT_FLARE || item->object_id != O_FLARE_ITEM)) {
        if (!Lara_MovePosition(&g_PickupPositionUW, item, lara_item)) {
            return;
        }

        if (item->object_id == O_FLARE_ITEM) {
            lara_item->fall_speed = 0;
            lara_item->anim_num = LA_UNDERWATER_FLARE_PICKUP;
            lara_item->frame_num = g_Anims[lara_item->anim_num].frame_base;
            lara_item->goal_anim_state = LS_TREAD;
            lara_item->current_anim_state = LS_FLARE_PICKUP;
        } else {
            lara_item->goal_anim_state = LS_PICKUP;
            do {
                Lara_Animate(lara_item);
            } while (lara_item->current_anim_state != LS_PICKUP);
            lara_item->goal_anim_state = LS_TREAD;
        }
        return;
    }

    item->rot = old_rot;
}

void __cdecl Pickup_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (g_Lara.water_status == LWS_ABOVE_WATER
        || g_Lara.water_status == LWS_WADE) {
        M_DoAboveWater(item_num, lara_item);
    } else if (g_Lara.water_status == LWS_UNDERWATER) {
        M_DoUnderwater(item_num, lara_item);
    }
}
