#include "game/effect_routines/lara_effects.h"

#include "config.h"
#include "game/items.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"
#include "util.h"

#include <stdint.h>

void FX_LaraNormal(ITEM_INFO *item)
{
    item->current_anim_state = LS_STOP;
    item->goal_anim_state = LS_STOP;
    Item_SwitchToAnim(item, LA_STOP, AF_STOP);
    g_Camera.type = CAM_CHASE;
    Viewport_AlterFOV(g_Config.fov_value * PHD_DEGREE);
}

void FX_LaraHandsFree(ITEM_INFO *item)
{
    g_Lara.gun_status = LGS_ARMLESS;
}

void FX_LaraDrawRightGun(ITEM_INFO *item)
{
    int16_t *tmp_mesh;
    OBJECT_INFO *obj = &g_Objects[item->object_number];
    OBJECT_INFO *obj2 = &g_Objects[O_PISTOLS];

    SWAP(
        g_Meshes[obj->mesh_index + LM_THIGH_R],
        g_Meshes[obj2->mesh_index + LM_THIGH_R], tmp_mesh);

    SWAP(
        g_Meshes[obj->mesh_index + LM_HAND_R],
        g_Meshes[obj2->mesh_index + LM_HAND_R], tmp_mesh);
}
