#include "game/objects/general/detonator.h"

#include "game/camera.h"
#include "game/game_flow.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/inventory_ring.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/objects/general/pickup.h"
#include "game/output.h"
#include "game/sound.h"
#include "global/vars.h"

#define EXPLOSION_START_FRAME 76
#define EXPLOSION_END_FRAME 99
#define EXPLOSION_ACTION_FRAME 80

static XYZ_32 m_DetonatorPosition = { .x = 0, .y = 0, .z = 0 };

static int16_t m_GongBounds[12] = {
    -WALL_L / 2,
    +WALL_L,
    -100,
    +100,
    -WALL_L / 2 - 300,
    -WALL_L / 2 + 100,
    -30 * DEG_1,
    +30 * DEG_1,
    +0,
    +0,
    +0,
    +0,
};

static void M_CreateGongBonger(ITEM *lara_item);

static void M_CreateGongBonger(ITEM *const lara_item)
{
    const int16_t item_gong_bonger_num = Item_Create();
    if (item_gong_bonger_num == NO_ITEM) {
        return;
    }

    ITEM *const item_gong_bonger = Item_Get(item_gong_bonger_num);
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

void Detonator1_Setup(void)
{
    OBJECT *const obj = Object_Get(O_DETONATOR_1);
    obj->collision_func = Detonator_Collision;
}

void Detonator2_Setup(void)
{
    OBJECT *const obj = Object_Get(O_DETONATOR_2);
    obj->collision_func = Detonator_Collision;
    obj->control_func = Detonator_Control;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void Detonator_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_Animate(item);

    if (Item_TestFrameRange(item, EXPLOSION_START_FRAME, EXPLOSION_END_FRAME)) {
        Output_AddDynamicLight(item->pos, 13, 11);
    }

    if (Item_TestFrameEqual(item, EXPLOSION_ACTION_FRAME)) {
        g_Camera.bounce = -150;
        Sound_Effect(SFX_EXPLOSION_1, nullptr, SPM_ALWAYS);
    }

    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
    }
}

// TODO: split gong shenanigans into a separate routine
void Detonator_Collision(
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

    if (item->status == IS_DEACTIVATED
        || (g_Inv_Chosen == NO_OBJECT && !g_Input.action)
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->gravity
        || lara_item->current_anim_state != LS_STOP) {
        goto normal_collision;
    }

    if (item->object_id == O_DETONATOR_2) {
        if (!Item_TestPosition(g_PickupBounds, item, lara_item)) {
            goto normal_collision;
        }
    } else {
        if (!Item_TestPosition(m_GongBounds, item, lara_item)) {
            goto normal_collision;
        } else {
            item->rot = old_rot;
        }
    }

    if (g_Inv_Chosen == NO_OBJECT) {
        GF_ShowInventoryKeys(item->object_id);
    }

    if (g_Inv_Chosen != O_KEY_OPTION_2) {
        goto normal_collision;
    }

    Inv_RemoveItem(O_KEY_OPTION_2);
    Item_AlignPosition(&m_DetonatorPosition, item, lara_item);
    Item_SwitchToObjAnim(lara_item, LA_EXTRA_BREATH, 0, O_LARA_EXTRA);
    lara_item->current_anim_state = LA_EXTRA_BREATH;
    if (item->object_id == O_DETONATOR_2) {
        lara_item->goal_anim_state = LA_EXTRA_PLUNGER;
    } else {
        lara_item->goal_anim_state = LA_EXTRA_GONG_BONG;
        lara_item->rot.y += DEG_180;
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
