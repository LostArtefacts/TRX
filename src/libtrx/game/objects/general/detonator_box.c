#include "game/camera.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/objects/general/pickup.h"
#include "game/output.h"
#include "game/sound.h"

#define M_EXPLOSION_START_FRAME 76
#define M_EXPLOSION_END_FRAME 99
#define M_EXPLOSION_ACTION_FRAME 80

static XYZ_32 m_Position = { .x = 0, .y = 0, .z = 0 };

static const OBJECT_BOUNDS m_Bounds = {
    .shift = {
        .min = { .x = -WALL_L / 2, .y = -100, .z = -WALL_L / 2 - 300, },
        .max = { .x = +WALL_L, .y = +100, .z = -WALL_L / 2 + 100, },
    },
    .rot = {
        .min = { .x = -30 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +30 * DEG_1, .y = 0, .z = 0, },
    },
    .ignore_rot = true,
};

static void M_ConsumeKeyItem(ITEM *const receptacle_item)
{
    const OBJECT_ID key_object_id =
        Object_FindReceptacleKey(receptacle_item->object_id);
    if (key_object_id != NO_OBJECT) {
        Inv_RemoveItem(key_object_id);
    }
}

static void M_Use(ITEM *const lara_item, ITEM *const receptacle_item)
{
    Lara_AlignPosition(receptacle_item, &m_Position);
    Item_SwitchToObjAnim(lara_item, LS_EXTRA_BREATH, 0, O_LARA_EXTRA);
    lara_item->current_anim_state = LS_EXTRA_BREATH;
    if (receptacle_item->object_id == O_DETONATOR_BOX) {
        lara_item->goal_anim_state = LS_EXTRA_PLUNGER;
    } else {
        lara_item->goal_anim_state = LS_EXTRA_GONG_BONG;
        lara_item->rot.y += DEG_180;
    }
    Item_Animate(lara_item);

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->extra_anim = true;
    lara->gun_status = LGS_HANDS_BUSY;
    lara->hit_direction = -1;

    if (Item_TestFrameEqual(lara_item, 0)) {
        M_ConsumeKeyItem(receptacle_item);
    }

    receptacle_item->status = IS_ACTIVE;
    Item_AddActive(Item_GetIndex(receptacle_item));

    lara->interact_target.is_moving = false;
    lara->interact_target.item_num = NO_ITEM;
}

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_Bounds;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_Animate(item);

    if (Item_TestFrameRange(
            item, M_EXPLOSION_START_FRAME, M_EXPLOSION_END_FRAME)) {
        Output_AddDynamicLight(item->pos, 13, 11);
    }

    if (Item_TestFrameEqual(item, M_EXPLOSION_ACTION_FRAME)) {
        g_Camera.bounce = -150;
        Sound_Effect(SFX_EXPLOSION_1, nullptr, SPM_ALWAYS);
    }

    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->extra_anim) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    if (lara->interact_target.is_moving
        && lara->interact_target.item_num == item_num) {
        M_Use(lara_item, item);
        return;
    }

    if (item->status != IS_INACTIVE || !g_Input.action
        || lara->gun_status != LGS_ARMLESS || lara_item->gravity
        || lara_item->current_anim_state != LS(LS_STOP)) {
        goto normal_collision;
    }

    const XYZ_16 old_rot = item->rot;
    item->rot.x = 0;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        item->rot = old_rot;
        goto normal_collision;
    }

    if (item->object_id == O_GONG) {
        item->rot = old_rot;
    }

    if (!GF_ShowInventoryKeys(item->object_id)) {
        Lara_RefuseInteraction();
    }

    return;

normal_collision:
    Object_Collision(item_num, lara_item, coll);
}

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;
    obj->bounds_func = Pickup_Bounds;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_DETONATOR_BOX, M_Setup)
