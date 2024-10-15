#include "game/objects/general/detonator.h"

#include "game/input.h"
#include "game/inventory/backpack.h"
#include "game/inventory/common.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/sound.h"
#include "global/funcs.h"
#include "global/vars.h"

static void M_CreateGongBonger(ITEM *lara_item);

static void M_CreateGongBonger(ITEM *const lara_item)
{
    const int16_t item_gong_bonger_num = Item_Create();
    if (item_gong_bonger_num == NO_ITEM) {
        return;
    }

    ITEM *const item_gong_bonger = &g_Items[item_gong_bonger_num];
    item_gong_bonger->object_id = O_GONG_BONGER;
    item_gong_bonger->pos.x = lara_item->pos.x;
    item_gong_bonger->pos.y = lara_item->pos.y;
    item_gong_bonger->pos.z = lara_item->pos.z;
    item_gong_bonger->rot.x = 0;
    item_gong_bonger->rot.y = lara_item->rot.y;
    lara_item->rot.z = 0;

    item_gong_bonger->room_num = lara_item->room_num;

    Item_Initialise(item_gong_bonger_num);
    Item_AddActive(item_gong_bonger_num);
    item_gong_bonger->status = IS_ACTIVE;
}

void __cdecl Detonator_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_Animate(item);

    const int32_t frame_num =
        item->frame_num - g_Anims[item->anim_num].frame_base;
    if (frame_num > 75 && frame_num < 100) {
        AddDynamicLight(item->pos.x, item->pos.y, item->pos.z, 13, 11);
    }

    if (frame_num == 80) {
        g_Camera.bounce = -150;
        Sound_Effect(SFX_EXPLOSION1, NULL, SPM_ALWAYS);
    }

    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
    }
}

// TODO: split gong shenanigans into a separate routine
void __cdecl Detonator_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (g_Lara.extra_anim) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const XYZ_16 old_rot = item->rot;
    const int16_t x = item->rot.x;
    const int16_t y = item->rot.y;
    const int16_t z = item->rot.z;
    item->rot.x = 0;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (item->status == IS_DEACTIVATED || !(g_Input & IN_ACTION)
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->gravity
        || lara_item->current_anim_state != LS_STOP) {
        goto normal_collision;
    }

    if (item->object_id == O_DETONATOR_2) {
        if (!Item_TestPosition(g_PickupBounds, item, lara_item)) {
            goto normal_collision;
        }
    } else {
        if (!Item_TestPosition(g_GongBounds, item, lara_item)) {
            goto normal_collision;
        } else {
            item->rot = old_rot;
        }
    }

    if (g_Inv_Chosen == NO_OBJECT) {
        Inv_Display(INV_KEYS_MODE);
    }

    if (g_Inv_Chosen != O_KEY_OPTION_2) {
        goto normal_collision;
    }

    Inv_RemoveItem(O_KEY_OPTION_2);
    Item_AlignPosition(&g_DetonatorPosition, item, lara_item);
    lara_item->anim_num = g_Objects[O_LARA_EXTRA].anim_idx;
    lara_item->frame_num = g_Anims[lara_item->anim_num].frame_base;
    lara_item->current_anim_state = LA_EXTRA_BREATH;
    if (item->object_id == O_DETONATOR_2) {
        lara_item->goal_anim_state = LA_EXTRA_PLUNGER;
    } else {
        lara_item->goal_anim_state = LA_EXTRA_GONG_BONG;
        lara_item->rot.y += PHD_180;
    }

    Item_Animate(lara_item);

    g_Lara.extra_anim = 1;
    g_Lara.gun_status = LGS_HANDS_BUSY;

    if (item->object_id == O_DETONATOR_2) {
        item->status = IS_ACTIVE;
        Item_AddActive(item_num);
    } else {
        M_CreateGongBonger(lara_item);
    }
    return;

normal_collision:
    item->rot = old_rot;
    Object_Collision(item_num, lara_item, coll);
}
