#include "game/lara/flare.h"

#include "decomp/flares.h"
#include "game/gun/gun.h"

#include <libtrx/game/game.h>
#include <libtrx/game/inventory.h>
#include <libtrx/game/lara.h>

typedef enum {
    // clang-format off
    LA_FLARES_HOLD   = 0,
    LA_FLARES_THROW  = 1,
    LA_FLARES_DRAW   = 2,
    LA_FLARES_IGNITE = 3,
    LA_FLARES_IDLE   = 4,
    // clang-format on
} M_LARA_FLARE_ANIMATION;

static void M_InitialiseState(void);

static void M_InitialiseState(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->gun_status = LGS_ARMLESS;
    lara_info->left_arm.rot.x = 0;
    lara_info->left_arm.rot.y = 0;
    lara_info->left_arm.rot.z = 0;
    lara_info->right_arm.rot.x = 0;
    lara_info->right_arm.rot.y = 0;
    lara_info->right_arm.rot.z = 0;
    lara_info->left_arm.lock = 0;
    lara_info->right_arm.lock = 0;
    lara_info->target = nullptr;
}

void Lara_Flare_SetArm(const int32_t flare_frame)
{
    int16_t anim_idx;
    if (flare_frame < LF_FL_THROW) {
        anim_idx = LA_FLARES_HOLD;
    } else if (flare_frame < LF_FL_DRAW) {
        anim_idx = LA_FLARES_THROW;
    } else if (flare_frame < LF_FL_IGNITE) {
        anim_idx = LA_FLARES_DRAW;
    } else if (flare_frame < LF_FL_2_HOLD) {
        anim_idx = LA_FLARES_IGNITE;
    } else {
        anim_idx = LA_FLARES_IDLE;
    }

    const OBJECT *const obj = Object_Get(O_LARA_FLARE);
    const ANIM *const anim = Object_GetAnim(obj, anim_idx);
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->left_arm.anim_num = obj->anim_idx + anim_idx;
    lara_info->left_arm.frame_base = anim->frame_ptr;
}

void Lara_Flare_DrawMeshes(void)
{
    Lara_SwapSingleMesh(LM_HAND_L, O_LARA_FLARE);
}

void Lara_Flare_UndrawMeshes(void)
{
    Lara_SwapSingleMesh(LM_HAND_L, O_LARA);
}

void Lara_Flare_Draw(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    if (lara_item->current_anim_state == LS_FLARE_PICKUP
        || lara_item->current_anim_state == LS_PICKUP) {
        Flare_DoInHand(lara_info->flare.age);
        lara_info->flare.control = false;
        lara_info->left_arm.frame_num = LF_FL_2_HOLD - 2;
        Lara_Flare_SetArm(lara_info->left_arm.frame_num);
        return;
    }

    int32_t frame_num = lara_info->left_arm.frame_num + 1;
    lara_info->flare.control = true;

    if (frame_num < LF_FL_DRAW || frame_num > LF_FL_2_HOLD - 1) {
        frame_num = LF_FL_DRAW;
    } else if (frame_num == LF_FL_DRAW_GOT_IT) {
        Lara_Flare_DrawMeshes();
        if (!Game_IsBonusFlagSet(GBF_NGPLUS)) {
            Inv_RemoveItem(O_FLARES_ITEM);
        }
    } else if (frame_num >= LF_FL_IGNITE && frame_num <= LF_FL_2_HOLD - 2) {
        if (frame_num == LF_FL_IGNITE) {
            lara_info->flare.age = 0;
        }
        Flare_DoInHand(lara_info->flare.age);
    } else if (frame_num == LF_FL_2_HOLD - 1) {
        M_InitialiseState();
        Flare_DoInHand(lara_info->flare.age);
        frame_num = LF_FL_HOLD;
    }

    lara_info->left_arm.frame_num = frame_num;
    Lara_Flare_SetArm(frame_num);
}

void Lara_Flare_Undraw(void)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    int16_t frame_num_1 = lara_info->left_arm.frame_num;
    int16_t frame_num_2 = lara_info->flare.frame_num;

    lara_info->flare.control = true;

    if (lara_item->goal_anim_state == LS_STOP
        && lara_info->vehicle_item_num == NO_ITEM) {
        if (Item_TestAnimEqual(lara_item, LA_STAND_IDLE)) {
            Item_SwitchToAnim(lara_item, LA_FLARE_THROW, frame_num_1);
            lara_info->flare.frame_num = lara_item->frame_num;
            frame_num_2 = lara_item->frame_num;
        }

        if (Item_TestAnimEqual(lara_item, LA_FLARE_THROW)) {
            lara_info->flare.control = false;

            if (frame_num_2 >= Anim_GetAnim(LA_FLARE_THROW)->frame_base
                    + LF_FL_THROW_FT - 1) {
                lara_info->gun_type = lara_info->last_gun_type;
                lara_info->request_gun_type = lara_info->last_gun_type;
                lara_info->gun_status = LGS_ARMLESS;
                Gun_InitialiseNewWeapon();
                lara_info->target = nullptr;
                lara_info->right_arm.lock = 0;
                lara_info->left_arm.lock = 0;
                Item_SwitchToAnim(lara_item, LA_STAND_STILL, 0);
                lara_info->flare.frame_num = lara_item->frame_num;
                lara_item->current_anim_state = LS_STOP;
                lara_item->goal_anim_state = LS_STOP;
                return;
            }
            lara_info->flare.frame_num = frame_num_2 + 1;
        }
    } else if (
        lara_item->current_anim_state == LS_STOP
        && lara_info->vehicle_item_num == -1) {
        Item_SwitchToAnim(lara_item, LA_STAND_STILL, 0);
    }

    if (frame_num_1 == LF_FL_HOLD) {
        frame_num_1 = LF_FL_THROW;
    } else if (frame_num_1 >= LF_FL_IGNITE && frame_num_1 < LF_FL_2_HOLD) {
        frame_num_1++;
        if (frame_num_1 == LF_FL_2_HOLD - 1) {
            frame_num_1 = LF_FL_THROW;
        }
    } else if (frame_num_1 >= LF_FL_THROW && frame_num_1 < LF_FL_DRAW) {
        frame_num_1++;
        if (frame_num_1 == LF_FL_THROW_RELEASE) {
            Flare_Create(true);
            Lara_Flare_UndrawMeshes();
        } else if (frame_num_1 == LF_FL_DRAW) {
            frame_num_1 = 0;
            lara_info->gun_type = lara_info->last_gun_type;
            lara_info->request_gun_type = lara_info->last_gun_type;
            lara_info->gun_status = LGS_ARMLESS;
            Gun_InitialiseNewWeapon();
            lara_info->target = nullptr;
            lara_info->flare.control = false;
            lara_info->right_arm.lock = 0;
            lara_info->left_arm.lock = 0;
            lara_info->flare.frame_num = 0;
        }
    } else if (frame_num_1 >= LF_FL_2_HOLD && frame_num_1 < LF_FL_END) {
        frame_num_1++;
        if (frame_num_1 == LF_FL_END) {
            frame_num_1 = LF_FL_THROW;
        }
    }

    if (frame_num_1 >= LF_FL_THROW && frame_num_1 < LF_FL_THROW_RELEASE) {
        Flare_DoInHand(lara_info->flare.age);
    }

    lara_info->left_arm.frame_num = frame_num_1;
    Lara_Flare_SetArm(frame_num_1);
}
