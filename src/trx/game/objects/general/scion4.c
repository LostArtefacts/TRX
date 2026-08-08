// Atlantis Scion - triggers O_LARA_EXTRA reach anim.

#include <trx/game/camera.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>

static XYZ_32 m_Position = { 0, 280, -512 + 105 };

static const OBJECT_BOUNDS m_Bounds = {
    .shift = {
        .min = { .x = -256, .y = +256 - 50, .z = -512 - 350, },
        .max = { .x = +256, .y = +256 + 50, .z = -200, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_Bounds;
}

static void M_Control(const int16_t item_num)
{
    Item_Animate(Item_Get(item_num));
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(item->object_id);

    const XYZ_16 old_rot = item->rot;
    item->rot.y = lara_item->rot.y;
    item->rot.x = 0;
    item->rot.z = 0;

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        goto cleanup;
    }

    if (g_Input.action && lara->gun_status == LGS_ARMLESS && !lara_item->gravity
        && Lara_Interact_CanBegin(LARA_INTERACT_RECEPTACLE)) {
        Lara_AlignPosition(item, &m_Position);
        Lara_SwitchToExtraState(LS_EXTRA_SCION_PICKUP_2);
        Camera_InvokeCinematic(lara_item, 0, -DEG_90);
    }

cleanup:
    item->rot = old_rot;
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->save_flags = true;
    obj->bounds_func = M_Bounds;
}

REGISTER_OBJECT(O_SCION_ITEM_4, M_Setup)
