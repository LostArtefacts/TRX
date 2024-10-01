#include "game/objects/general/scion1.h"

#include "game/input.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/overlay.h"
#include "global/vars.h"

#define EXTRA_ANIM_PEDESTAL_SCION 0
#define LF_PICKUPSCION 44

static XYZ_32 m_Scion1_Position = { 0, 640, -310 };

static const OBJECT_BOUNDS m_Scion1_Bounds = {
    .shift = {
        .min = { .x = -256, .y = +640 - 100, .z = -350, },
        .max = { .x = +256, .y = +640 + 100, .z = -200, },
    },
    .rot = {
        .min = { .x = -10 * PHD_DEGREE, .y = 0, .z = 0, },
        .max = { .x = +10 * PHD_DEGREE, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void);

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_Scion1_Bounds;
}

void Scion1_Setup(OBJECT *obj)
{
    obj->draw_routine = Object_DrawPickupItem;
    obj->collision = Scion1_Collision;
    obj->save_flags = 1;
    obj->bounds = M_Bounds;
}

void Scion1_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
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

    if (lara_item->current_anim_state == LS_PICKUP) {
        if (Item_TestFrameEqual(lara_item, LF_PICKUPSCION)) {
            Overlay_AddPickup(item->object_id);
            Inv_AddItem(item->object_id);
            item->status = IS_INVISIBLE;
            Item_RemoveDrawn(item_num);
            g_GameInfo.current[g_CurrentLevel].stats.pickup_count++;
        }
    } else if (
        g_Input.action && g_Lara.gun_status == LGS_ARMLESS
        && !lara_item->gravity && lara_item->current_anim_state == LS_STOP) {
        Lara_AlignPosition(item, &m_Scion1_Position);
        lara_item->current_anim_state = LS_PICKUP;
        lara_item->goal_anim_state = LS_PICKUP;
        Item_SwitchToObjAnim(
            lara_item, EXTRA_ANIM_PEDESTAL_SCION, 0, O_LARA_EXTRA);
        g_Lara.gun_status = LGS_HANDS_BUSY;
        g_Camera.type = CAM_CINEMATIC;
        g_CineFrame = 0;
        g_CinePosition.pos = lara_item->pos;
        g_CinePosition.rot.x = lara_item->rot.x;
        g_CinePosition.rot.y = lara_item->rot.y;
        g_CinePosition.rot.z = lara_item->rot.z;
    }
cleanup:
    item->rot.x = rotx;
    item->rot.y = roty;
    item->rot.z = rotz;
}
