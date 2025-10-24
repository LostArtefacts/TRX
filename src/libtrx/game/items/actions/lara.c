#include "game/camera.h"
#include "game/lara.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "game/viewport.h"

static void M_Normal(ITEM *const item)
{
    if (item == nullptr) {
        return;
    }

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

static void M_ToggleGun(
    ITEM *const item, const LARA_MESH thigh_mesh_idx,
    const LARA_MESH hand_mesh_idx)
{
    if (item == nullptr) {
        return;
    }

    const OBJECT *const obj = Object_Get(O_LARA_PISTOLS);
    const OBJECT_MESH *const mesh =
        Object_GetMesh(obj->mesh_idx + thigh_mesh_idx);
    const bool armed = Lara_Mesh_Get(thigh_mesh_idx) != mesh;

    if (armed) {
        Lara_Mesh_SwapSingle(thigh_mesh_idx, O_LARA_PISTOLS);
        Lara_Mesh_SwapSingle(hand_mesh_idx, O_LARA);
    } else {
        Lara_Mesh_SwapSingle(thigh_mesh_idx, O_LARA);
        Lara_Mesh_SwapSingle(hand_mesh_idx, O_LARA_PISTOLS);
    }
}

static void M_ToggleRightGun(ITEM *const item)
{
    M_ToggleGun(item, LM_THIGH_R, LM_HAND_R);
}

static void M_ToggleLeftGun(ITEM *const item)
{
    M_ToggleGun(item, LM_THIGH_L, LM_HAND_L);
}

static void M_ToggleLaraMeshSwap(ITEM *const item)
{
    if (item == nullptr) {
        return;
    }

    const OBJECT *const obj = Object_Get(O_LARA_SWAP);
    const OBJECT_MESH *const mesh = Object_GetMesh(obj->mesh_idx + LM_HIPS);
    const bool swapped = Lara_Mesh_Get(LM_HIPS) == mesh;

    if (swapped) {
        Lara_Mesh_SwapAll(O_LARA);
        Lara_Mesh_SwapSingle(LM_THIGH_L, O_LARA_PISTOLS);
        Lara_Mesh_SwapSingle(LM_THIGH_R, O_LARA_PISTOLS);
    } else {
        Lara_Mesh_SwapAll(O_LARA_SWAP);
    }
}

static void M_ResetHair(ITEM *const item)
{
    Lara_Hair_Initialise();
}

static void M_Bubbles(ITEM *const item)
{
    // XXX: until we get RoboLara, it makes sense for her to breathe underwater
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    if (lara->water_status == LWS_CHEAT
        && !Room_Get(lara_item->room_num)->flags.underwater) {
        return;
    }

    const int32_t count = (Random_GetDraw() * 3) / 0x8000;
    if (count == 0) {
        return;
    }

    Sound_Effect(SFX_LARA_BUBBLES, &item->pos, SPM_UNDERWATER);

    XYZ_32 offset = { .x = 0, .y = 0, .z = 50 };
    Collide_GetJointAbsPosition(item, &offset, LM_HEAD);
    for (int32_t i = 0; i < count; i++) {
        Spawn_Bubble(&offset, item->room_num);
    }
}

REGISTER_ITEM_ACTION(ITEM_ACTION_LARA_NORMAL, M_Normal)
REGISTER_ITEM_ACTION(ITEM_ACTION_LARA_HANDS_FREE, M_HandsFree)
REGISTER_ITEM_ACTION(ITEM_ACTION_LARA_DRAW_RIGHT_GUN, M_ToggleRightGun)
REGISTER_ITEM_ACTION(ITEM_ACTION_LARA_DRAW_LEFT_GUN, M_ToggleLeftGun)
REGISTER_ITEM_ACTION(
    ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_3, M_ToggleLaraMeshSwap)
REGISTER_ITEM_ACTION(ITEM_ACTION_RESET_HAIR, M_ResetHair)
REGISTER_ITEM_ACTION(ITEM_ACTION_BUBBLES, M_Bubbles)
