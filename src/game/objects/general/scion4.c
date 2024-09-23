#include "game/objects/general/scion4.h"

#include "game/input.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "global/vars.h"

#define EXTRA_ANIM_HOLDER_SCION 0

static XYZ_32 m_Scion4_Position = { 0, 280, -512 + 105 };

static const OBJECT_BOUNDS m_Scion4_Bounds = {
    .shift = {
        .min = { .x = -256, .y = +256 - 50, .z = -512 - 350, },
        .max = { .x = +256, .y = +256 + 50, .z = -200, },
    },
    .rot = {
        .min = { .x = -10 * PHD_DEGREE, .y = 0, .z = 0, },
        .max = { .x = +10 * PHD_DEGREE, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void);

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_Scion4_Bounds;
}

void Scion4_Setup(OBJECT *obj)
{
    obj->control = Scion4_Control;
    obj->collision = Scion4_Collision;
    obj->save_flags = 1;
    obj->bounds = M_Bounds;
}

void Scion4_Control(int16_t item_num)
{
    Item_Animate(&g_Items[item_num]);
}

void Scion4_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
{
    ITEM *item = &g_Items[item_num];
    const OBJECT *const obj = &g_Objects[item->object_id];
    int16_t rotx = item->rot.x;
    int16_t roty = item->rot.y;
    int16_t rotz = item->rot.z;
    item->rot.y = lara_item->rot.y;
    item->rot.x = 0;
    item->rot.z = 0;

    if (!Lara_TestPosition(item, obj->bounds())) {
        goto cleanup;
    }

    if (g_Input.action && g_Lara.gun_status == LGS_ARMLESS
        && !lara_item->gravity && lara_item->current_anim_state == LS_STOP) {
        Lara_AlignPosition(item, &m_Scion4_Position);
        lara_item->current_anim_state = LS_PICKUP;
        lara_item->goal_anim_state = LS_PICKUP;
        Item_SwitchToObjAnim(
            lara_item, EXTRA_ANIM_HOLDER_SCION, 0, O_LARA_EXTRA);
        g_Lara.gun_status = LGS_HANDS_BUSY;
        g_Camera.type = CAM_CINEMATIC;
        g_CineFrame = 0;
        g_CinePosition.pos = lara_item->pos;
        g_CinePosition.rot.x = lara_item->rot.x;
        g_CinePosition.rot.y = lara_item->rot.y;
        g_CinePosition.rot.z = lara_item->rot.z;
        g_CinePosition.rot.y -= PHD_90;
    }
cleanup:
    item->rot.x = rotx;
    item->rot.y = roty;
    item->rot.z = rotz;
}
