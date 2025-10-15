// Tomb of Qualopec and Sanctuary Scion pickup.
// Triggers O_LARA_EXTRA pedestal pickup animation.

#include "game/camera.h"
#include "game/game.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/level.h"
#include "game/objects/common.h"
#include "game/overlay.h"
#include "game/savegame.h"
#include "game/stats.h"

#define EXTRA_ANIM_PEDESTAL_SCION 0
#define LF_PICKUPSCION 44

extern void Stats_AddPickup(void);

static XYZ_32 m_Scion1_Position = { 0, 640, -310 };

static const OBJECT_BOUNDS m_Scion1_Bounds = {
    .shift = {
        .min = { .x = -256, .y = +640 - 100, .z = -350, },
        .max = { .x = +256, .y = +640 + 100, .z = -200, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_Scion1_Bounds;
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->status == IS_DEACTIVATED) {
            const int16_t item_num = Item_GetIndex(item);
            Item_RemoveDrawn(item_num);
        }
    }
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

    if (lara_item->current_anim_state == LS(LS_PICKUP)) {
        if (Item_TestFrameEqual(lara_item, LF_PICKUPSCION)) {
            Overlay_AddDisplayPickup(item->object_id);
            Inv_AddItem(item->object_id);
            item->status = IS_INVISIBLE;
            Item_RemoveDrawn(item_num);
#if TR_VERSION == 1
            Stats_AddPickup();
#endif
        }
    } else if (
        g_Input.action && lara->gun_status == LGS_ARMLESS && !lara_item->gravity
        && lara_item->current_anim_state == LS(LS_STOP)) {
        Lara_AlignPosition(item, &m_Scion1_Position);
        lara_item->current_anim_state = LS(LS_PICKUP);
        lara_item->goal_anim_state = LS(LS_PICKUP);
        Item_SwitchToObjAnim(
            lara_item, EXTRA_ANIM_PEDESTAL_SCION, 0, O_LARA_EXTRA);
        lara->gun_status = LGS_HANDS_BUSY;
        lara->extra_anim = true;
        Camera_InvokeCinematic(lara_item, 0, 0);
    }
cleanup:
    item->rot = old_rot;
}

static void M_Setup(OBJECT *const obj)
{
    obj->handle_save_func = M_HandleSave;
    obj->draw_func = Object_DrawPickupItem;
    obj->collision_func = M_Collision;
    obj->save_flags = true;
    obj->bounds_func = M_Bounds;
}

REGISTER_OBJECT(O_SCION_ITEM_1, M_Setup)
