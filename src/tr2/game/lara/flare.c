#include "game/lara/flare.h"

#include "game/gun/gun_misc.h"
#include "game/lara/misc.h"
#include "game/objects/general/flare_item.h"

#include <libtrx/config.h>
#include <libtrx/game/game.h>
#include <libtrx/game/gun.h>
#include <libtrx/game/inventory.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/rooms.h>
#include <libtrx/game/sound.h>

typedef enum {
    // clang-format off
    LA_FLARES_HOLD   = 0,
    LA_FLARES_THROW  = 1,
    LA_FLARES_DRAW   = 2,
    LA_FLARES_IGNITE = 3,
    LA_FLARES_IDLE   = 4,
    // clang-format on
} M_LARA_FLARE_ANIMATION;

typedef enum {
    LF_FL_HOLD_FT = 1,
    LF_FL_THROW_FT = 32,
    LF_FL_DRAW_FT = 39,
    LF_FL_IGNITE_FT = 23,
    LF_FL_2_HOLD_FT = 15,

    LF_FL_HOLD = 0,
    LF_FL_THROW = (LF_FL_HOLD + LF_FL_HOLD_FT), // = 1
    LF_FL_THROW_RELEASE = (LF_FL_THROW + 20), // = 21
    LF_FL_DRAW = (LF_FL_THROW + LF_FL_THROW_FT), // = 33
    LF_FL_IGNITE = (LF_FL_DRAW + LF_FL_DRAW_FT), // = 72
    LF_FL_2_HOLD = (LF_FL_IGNITE + LF_FL_IGNITE_FT), // = 95
    LF_FL_END = (LF_FL_2_HOLD + LF_FL_2_HOLD_FT), // = 110
    LF_FL_DRAW_GOT_IT = (LF_FL_DRAW + 13), // = 46
} M_LARA_FLARE_FRAME;

static const LARA_TRX_STATE m_HoldStates[] = {
    LS_WALK,      LS_STOP,      LS_POSE,       LS_TURN_RIGHT,  LS_TURN_LEFT,
    LS_WALK_BACK, LS_FAST_TURN, LS_STEP_LEFT,  LS_STEP_RIGHT,  LS_WADE,
    LS_PICKUP,    LS_SWITCH_ON, LS_SWITCH_OFF, LS_TRX_INVALID, // sentinel
};

static const LARA_TRX_STATE m_ThrowStates[] = {
    LS_FAST_FALL, LS_SWAN_DIVE, LS_FAST_DIVE,
    LS_TRX_INVALID, // sentinel
};

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

static void M_DoIgniteEffects(const XYZ_32 flare_pos, int16_t room_num)
{
    Room_GetSector(flare_pos.x, flare_pos.y, flare_pos.z, &room_num);
    const ROOM *const room = Room_Get(room_num);
    const SOUND_PLAY_MODE mode =
        (room->flags & RF_UNDERWATER) != 0 ? SPM_UNDERWATER : SPM_NORMAL;
    Sound_Effect(SFX_LARA_FLARE_IGNITE, &flare_pos, mode);
}

static bool M_CanThrowFlare(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->gun_status != LGS_ARMLESS) {
        return false;
    }

    if (!g_Config.gameplay.fix_flare_throw_priority) {
        return true;
    }

    if (lara_info->water_status != LWS_ABOVE_WATER
        && lara_info->water_status != LWS_WADE) {
        return true;
    }

    // Airborne states that would not allow ledge grabbing anyway.
    if (Lara_HasState(m_ThrowStates)) {
        return true;
    }

    // Neither airborne nor about to be.
    return !lara_item->gravity && !g_Input.jump;
}

static void M_ControlInHand(const int32_t flare_age)
{
    XYZ_32 vec = {
        .x = 11,
        .y = 32,
        .z = 41,
    };
    Lara_GetJointAbsPosition(&vec, LM_HAND_L);

    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (flare_age == 0) {
        M_DoIgniteEffects(vec, lara_item->room_num);
    }

    lara_info->left_arm.flash_gun = Flare_GenerateLight(vec, flare_age);

    if (lara_info->flare.age < Flare_GetMaxAge()) {
        lara_info->flare.age++;
        Flare_GenerateEffects(lara_item->pos, vec, lara_item->room_num);
    } else if (M_CanThrowFlare()) {
        lara_info->gun_status = LGS_UNDRAW;
    }
}

static void M_SetArm(const int32_t flare_frame)
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

static void M_ControlArmless(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    const bool is_mounted_or_armed =
        Lara_Vehicle_IsMounted() || Lara_HasState(m_HoldStates);

    if (is_mounted_or_armed) {
        if (!lara_info->flare.control) {
            lara_info->left_arm.frame_num = LF_FL_2_HOLD;
            lara_info->flare.control = true;
        } else if (lara_info->left_arm.frame_num != LF_FL_HOLD) {
            lara_info->left_arm.frame_num++;
            if (lara_info->left_arm.frame_num == LF_FL_END) {
                lara_info->left_arm.frame_num = LF_FL_HOLD;
            }
        }
    } else {
        lara_info->flare.control = false;
    }

    M_ControlInHand(lara_info->flare.age);
    M_SetArm(lara_info->left_arm.frame_num);
}

static void M_ControlBusyHands(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->flare.control =
        Lara_Vehicle_IsMounted() || Lara_HasState(m_HoldStates);
    M_ControlInHand(lara_info->flare.age);
    M_SetArm(lara_info->left_arm.frame_num);
}

static void M_UndrawMeshes(void)
{
    Lara_Mesh_SwapSingle(LM_HAND_L, O_LARA);
}

void Lara_Flare_Control(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->gun_status == LGS_ARMLESS) {
        M_ControlArmless();
    } else if (lara_info->gun_status == LGS_HANDS_BUSY) {
        M_ControlBusyHands();
    }
}

void Lara_Flare_Draw(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    if (lara_item->current_anim_state == LS(LS_FLARE_PICKUP)
        || lara_item->current_anim_state == LS(LS_PICKUP)) {
        M_ControlInHand(lara_info->flare.age);
        lara_info->flare.control = false;
        lara_info->left_arm.frame_num = LF_FL_2_HOLD - 2;
        M_SetArm(lara_info->left_arm.frame_num);
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
        M_ControlInHand(lara_info->flare.age);
    } else if (frame_num == LF_FL_2_HOLD - 1) {
        M_InitialiseState();
        M_ControlInHand(lara_info->flare.age);
        frame_num = LF_FL_HOLD;
    }

    lara_info->left_arm.frame_num = frame_num;
    M_SetArm(frame_num);
}

void Lara_Flare_Undraw(void)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    int16_t frame_num_1 = lara_info->left_arm.frame_num;
    int16_t frame_num_2 = lara_info->flare.frame_num;

    lara_info->flare.control = true;

    if (lara_item->goal_anim_state == LS(LS_STOP)
        && !Lara_Vehicle_IsMounted()) {
        if (Item_TestAnimEqual(lara_item, LA(LA_STAND_IDLE))) {
            Item_SwitchToAnim(lara_item, LA(LA_FLARE_THROW), frame_num_1);
            lara_info->flare.frame_num = lara_item->frame_num;
            frame_num_2 = lara_item->frame_num;
        }

        if (Item_TestAnimEqual(lara_item, LA(LA_FLARE_THROW))) {
            lara_info->flare.control = false;

            if (frame_num_2 >= Anim_GetAnim(LA(LA_FLARE_THROW))->frame_base
                    + LF_FL_THROW_FT - 1) {
                lara_info->gun_type = lara_info->last_gun_type;
                lara_info->request_gun_type = lara_info->last_gun_type;
                lara_info->gun_status = LGS_ARMLESS;
                Gun_InitialiseNewWeapon();
                lara_info->target = nullptr;
                lara_info->right_arm.lock = 0;
                lara_info->left_arm.lock = 0;
                Item_SwitchToAnim(lara_item, LA(LA_STAND_STILL), 0);
                lara_info->flare.frame_num = lara_item->frame_num;
                lara_item->current_anim_state = LS(LS_STOP);
                lara_item->goal_anim_state = LS(LS_STOP);
                return;
            }
            lara_info->flare.frame_num = frame_num_2 + 1;
        }
    } else if (
        lara_item->current_anim_state == LS(LS_STOP)
        && !Lara_Vehicle_IsMounted()) {
        Item_SwitchToAnim(lara_item, LA(LA_STAND_STILL), 0);
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
            Lara_Flare_Dispose(true);
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
        M_ControlInHand(lara_info->flare.age);
    }

    lara_info->left_arm.frame_num = frame_num_1;
    M_SetArm(frame_num_1);
}

void Lara_Flare_Dispose(const bool thrown)
{
    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        goto finish;
    }

    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    ITEM *const item = Item_Get(item_num);
    item->object_id = O_FLARE_ITEM;
    item->room_num = lara_item->room_num;

    XYZ_32 vec = {
        .x = -16,
        .y = 32,
        .z = 42,
    };
    Lara_GetJointAbsPosition(&vec, LM_HAND_L);

    const SECTOR *const sector =
        Room_GetSector(vec.x, vec.y, vec.z, &item->room_num);
    const int32_t height = Room_GetHeight(sector, vec.x, vec.y, vec.z);
    if (height < vec.y) {
        item->pos.x = lara_item->pos.x;
        item->pos.y = vec.y;
        item->pos.z = lara_item->pos.z;
        item->rot.y = -lara_item->rot.y;
        item->room_num = lara_item->room_num;
    } else {
        item->pos.x = vec.x;
        item->pos.y = vec.y;
        item->pos.z = vec.z;
        if (thrown) {
            item->rot.y = lara_item->rot.y;
        } else {
            item->rot.y = lara_item->rot.y - DEG_45;
        }
    }

    Item_Initialise(item_num);

    item->rot.z = 0;
    item->rot.x = 0;
    item->shade.value_1 = -1;

    if (thrown) {
        item->speed = lara_item->speed + 50;
        item->fall_speed = lara_item->fall_speed - 50;
    } else {
        item->speed = lara_item->speed + 10;
        item->fall_speed = lara_item->fall_speed + 50;
    }

    if (Flare_GenerateLight(item->pos, lara_info->flare.age)) {
        item->data = (void *)(intptr_t)(lara_info->flare.age | 0x8000);
    } else {
        item->data = (void *)(intptr_t)(lara_info->flare.age & ~0x8000);
    }

    Item_AddActive(item_num);
    item->status = IS_ACTIVE;

finish:
    M_UndrawMeshes();
    if (!thrown) {
        lara_info->flare.control = false;
    }
}
