#include "game/item_actions/lara_effects.h"

#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/game/camera.h>
#include <libtrx/utils.h>

#include <stdint.h>

void ItemAction_LaraNormal(ITEM *item)
{
    g_Lara.extra_anim = false;
    item->current_anim_state = LS_STOP;
    item->goal_anim_state = LS_STOP;
    Item_SwitchToAnim(item, LA_STAND_STILL, 0);
    g_Camera.type = CAM_CHASE;
    Viewport_AlterFOV(-1);
}

void ItemAction_LaraHandsFree(ITEM *item)
{
    g_Lara.gun_status = LGS_ARMLESS;
}

void ItemAction_LaraDrawRightGun(ITEM *item)
{
    Object_SwapMesh(item->object_id, O_LARA_PISTOLS, LM_THIGH_R);
    Object_SwapMesh(item->object_id, O_LARA_PISTOLS, LM_HAND_R);
}
