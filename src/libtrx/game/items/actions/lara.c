#include "game/camera.h"
#include "game/lara.h"
#include "game/viewport.h"

static void M_Normal(ITEM *const item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->extra_anim = false;
    item->current_anim_state = LS(LS_STOP);
    item->goal_anim_state = LS(LS_STOP);
    Item_SwitchToAnim(item, LA(LA_STAND_STILL), 0);
    g_Camera.type = CAM_CHASE;
    Viewport_AlterFOV(-1);
}

static void M_HandsFree(ITEM *const item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->gun_status = LGS_ARMLESS;
}

static void M_DrawRightGun(ITEM *const item)
{
    Object_SwapMesh(item->object_id, O_LARA_PISTOLS, LM_THIGH_R);
    Object_SwapMesh(item->object_id, O_LARA_PISTOLS, LM_HAND_R);
    Lara_Mesh_SwapSingle(LM_THIGH_R, item->object_id);
    Lara_Mesh_SwapSingle(LM_HAND_R, item->object_id);
}

REGISTER_ITEM_ACTION(ITEM_ACTION_LARA_NORMAL, M_Normal)
REGISTER_ITEM_ACTION(ITEM_ACTION_LARA_HANDS_FREE, M_HandsFree)
REGISTER_ITEM_ACTION(ITEM_ACTION_LARA_DRAW_RIGHT_GUN, M_DrawRightGun)
