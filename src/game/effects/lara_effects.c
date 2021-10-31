#include "game/effects/lara_effects.h"

#include "3dsystem/3d_gen.h"
#include "config.h"
#include "global/vars.h"

void LaraNormal(ITEM_INFO *item)
{
    item->current_anim_state = AS_STOP;
    item->goal_anim_state = AS_STOP;
    item->anim_number = AA_STOP;
    item->frame_number = AF_STOP;
    Camera.type = CAM_CHASE;
    AlterFOV(T1MConfig.fov_value * PHD_DEGREE);
}

void LaraHandsFree(ITEM_INFO *item)
{
    Lara.gun_status = LGS_ARMLESS;
}

void LaraDrawRightGun(ITEM_INFO *item)
{
    int16_t *tmp_mesh;
    OBJECT_INFO *obj = &Objects[item->object_number];
    OBJECT_INFO *obj2 = &Objects[O_PISTOLS];

    SWAP(
        Meshes[obj->mesh_index + LM_THIGH_R],
        Meshes[obj2->mesh_index + LM_THIGH_R], tmp_mesh);

    SWAP(
        Meshes[obj->mesh_index + LM_HAND_R],
        Meshes[obj2->mesh_index + LM_HAND_R], tmp_mesh);
}
