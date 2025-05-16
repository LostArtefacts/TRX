#include "game/item_actions/lara_effects.h"

#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/game/camera.h>
#include <libtrx/utils.h>

#include <stdint.h>

void ItemAction_LaraNormal(ITEM *item)
{
    item->current_anim_state = LS_STOP;
    item->goal_anim_state = LS_STOP;
    Item_SwitchToAnim(item, LA_STOP, 0);
    g_Camera.type = CAM_CHASE;
    Viewport_SetFOV(-1);
}

void ItemAction_LaraHandsFree(ITEM *item)
{
    g_Lara.gun_status = LGS_ARMLESS;
}

void ItemAction_LaraDrawRightGun(ITEM *item)
{
    Object_SwapMesh(item->object_id, O_PISTOL_ANIM, LM_THIGH_R);
    Object_SwapMesh(item->object_id, O_PISTOL_ANIM, LM_HAND_R);
}
