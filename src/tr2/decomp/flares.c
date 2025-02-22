#include "decomp/flares.h"

#include "game/gun/gun.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara/misc.h"
#include "game/output.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/lara/common.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

#define FLARE_INTENSITY 12
#define FLARE_FALL_OFF 11
#define MAX_FLARE_AGE (60 * FRAMES_PER_SECOND) // = 1800
#define FLARE_OLD_AGE (MAX_FLARE_AGE - 2 * FRAMES_PER_SECOND) // = 1740
#define FLARE_YOUNG_AGE (FRAMES_PER_SECOND) // = 30

typedef enum {
    // clang-format off
    LA_FLARES_HOLD   = 0,
    LA_FLARES_THROW  = 1,
    LA_FLARES_DRAW   = 2,
    LA_FLARES_IGNITE = 3,
    LA_FLARES_IDLE   = 4,
    // clang-format on
} LARA_FLARES_ANIMATION;

static bool M_CanThrowFlare(void);
static void M_DoIgniteEffects(XYZ_32 flare_pos, int16_t room_num);
static void M_DoBurnEffects(
    XYZ_32 sound_pos, XYZ_32 flare_pos, int16_t room_num);

static bool M_CanThrowFlare(void)
{
    if (g_Lara.gun_status != LGS_ARMLESS) {
        return false;
    }

    return !g_Config.gameplay.fix_flare_throw_priority
        || (!g_LaraItem->gravity && !g_Input.jump)
        || g_LaraItem->current_anim_state == LS_FAST_FALL
        || g_LaraItem->current_anim_state == LS_SWAN_DIVE
        || g_LaraItem->current_anim_state == LS_FAST_DIVE;
}

static void M_DoIgniteEffects(const XYZ_32 flare_pos, int16_t room_num)
{
    Room_GetSector(flare_pos.x, flare_pos.y, flare_pos.z, &room_num);
    const ROOM *const room = Room_Get(room_num);
    const SOUND_PLAY_MODE mode =
        (room->flags & RF_UNDERWATER) != 0 ? SPM_UNDERWATER : SPM_NORMAL;
    Sound_Effect(SFX_LARA_FLARE_IGNITE, &flare_pos, mode);
}

static void M_DoBurnEffects(
    const XYZ_32 sound_pos, const XYZ_32 flare_pos, int16_t room_num)
{
    Room_GetSector(flare_pos.x, flare_pos.y, flare_pos.z, &room_num);
    if ((Room_Get(room_num)->flags & RF_UNDERWATER) != 0) {
        Sound_Effect(SFX_LARA_FLARE_BURN, &sound_pos, SPM_UNDERWATER);
        if (Random_GetDraw() < 0x4000) {
            Spawn_Bubble(&flare_pos, room_num);
        }
    } else {
        Sound_Effect(SFX_LARA_FLARE_BURN, &sound_pos, SPM_NORMAL);
    }
}

int32_t Flare_DoLight(const XYZ_32 *const pos, const int32_t flare_age)
{
    if (flare_age >= MAX_FLARE_AGE) {
        return false;
    }

    const int32_t random = Random_GetDraw();
    const XYZ_32 light_pos = {
        .x = pos->x + (random & 0xF),
        .y = pos->y,
        .z = pos->z,
    };

    if (flare_age < FLARE_YOUNG_AGE) {
        const int32_t intensity = FLARE_INTENSITY
                * (flare_age - FLARE_YOUNG_AGE) / (2 * FLARE_YOUNG_AGE)
            + FLARE_INTENSITY;
        Output_AddDynamicLight(light_pos, intensity, FLARE_FALL_OFF);
        return true;
    }

    if (flare_age < FLARE_OLD_AGE) {
        Output_AddDynamicLight(light_pos, FLARE_INTENSITY, FLARE_FALL_OFF);
        return true;
    }

    if (random > 0x2000) {
        Output_AddDynamicLight(
            light_pos, FLARE_INTENSITY - (random & 3), FLARE_FALL_OFF);
        return true;
    }

    Output_AddDynamicLight(light_pos, FLARE_INTENSITY, FLARE_FALL_OFF / 2);
    return false;
}

void Flare_DoInHand(const int32_t flare_age)
{
    XYZ_32 vec = {
        .x = 11,
        .y = 32,
        .z = 41,
    };
    Lara_GetJointAbsPosition(&vec, LM_HAND_L);

    if (flare_age == 0) {
        M_DoIgniteEffects(vec, g_LaraItem->room_num);
    }

    g_Lara.left_arm.flash_gun = Flare_DoLight(&vec, flare_age);

    if (g_Lara.flare_age < MAX_FLARE_AGE) {
        g_Lara.flare_age++;
        M_DoBurnEffects(g_LaraItem->pos, vec, g_LaraItem->room_num);
    } else if (M_CanThrowFlare()) {
        g_Lara.gun_status = LGS_UNDRAW;
    }
}

void Flare_DrawInAir(const ITEM *const item)
{
    int32_t rate;
    ANIM_FRAME *frames[2];
    Item_GetFrames(item, frames, &rate);
    Matrix_Push();
    Matrix_TranslateAbs32(item->pos);
    Matrix_Rot16(item->rot);
    const int32_t clip = Output_GetObjectBounds(&frames[0]->bounds);
    if (clip != 0) {
        Output_CalculateObjectLighting(item, &frames[0]->bounds);
        Object_DrawMesh(Object_Get(O_FLARE_ITEM)->mesh_idx, clip, false);
        if (((int32_t)(intptr_t)item->data) & 0x8000) {
            Matrix_TranslateRel(-6, 6, 80);
            Matrix_RotX(-90 * DEG_1);
            Matrix_RotY((int16_t)(2 * Random_GetDraw()));
            Output_CalculateStaticLight(8 * 256);
            Object_DrawMesh(Object_Get(O_FLARE_FIRE)->mesh_idx, clip, false);
        }
    }
    Matrix_Pop();
}

void Flare_Create(const bool thrown)
{
    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    item->object_id = O_FLARE_ITEM;
    item->room_num = g_LaraItem->room_num;

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
        item->pos.x = g_LaraItem->pos.x;
        item->pos.y = vec.y;
        item->pos.z = g_LaraItem->pos.z;
        item->rot.y = -g_LaraItem->rot.y;
        item->room_num = g_LaraItem->room_num;
    } else {
        item->pos.x = vec.x;
        item->pos.y = vec.y;
        item->pos.z = vec.z;
        if (thrown) {
            item->rot.y = g_LaraItem->rot.y;
        } else {
            item->rot.y = g_LaraItem->rot.y - DEG_45;
        }
    }

    Item_Initialise(item_num);

    item->rot.z = 0;
    item->rot.x = 0;
    item->shade.value_1 = -1;

    if (thrown) {
        item->speed = g_LaraItem->speed + 50;
        item->fall_speed = g_LaraItem->fall_speed - 50;
    } else {
        item->speed = g_LaraItem->speed + 10;
        item->fall_speed = g_LaraItem->fall_speed + 50;
    }

    if (Flare_DoLight(&item->pos, g_Lara.flare_age)) {
        item->data = (void *)(intptr_t)(g_Lara.flare_age | 0x8000);
    } else {
        item->data = (void *)(intptr_t)(g_Lara.flare_age & ~0x8000);
    }

    Item_AddActive(item_num);
    item->status = IS_ACTIVE;
}

void Flare_SetArm(const int32_t frame)
{
    int16_t anim_idx;
    if (frame < LF_FL_THROW) {
        anim_idx = LA_FLARES_HOLD;
    } else if (frame < LF_FL_DRAW) {
        anim_idx = LA_FLARES_THROW;
    } else if (frame < LF_FL_IGNITE) {
        anim_idx = LA_FLARES_DRAW;
    } else if (frame < LF_FL_2_HOLD) {
        anim_idx = LA_FLARES_IGNITE;
    } else {
        anim_idx = LA_FLARES_IDLE;
    }

    const OBJECT *const obj = Object_Get(O_LARA_FLARE);
    const ANIM *const anim = Object_GetAnim(obj, anim_idx);
    g_Lara.left_arm.anim_num = obj->anim_idx + anim_idx;
    g_Lara.left_arm.frame_base = anim->frame_ptr;
}

void Flare_Draw(void)
{
    if (g_LaraItem->current_anim_state == LS_FLARE_PICKUP
        || g_LaraItem->current_anim_state == LS_PICKUP) {
        Flare_DoInHand(g_Lara.flare_age);
        g_Lara.flare_control_left = 0;
        g_Lara.left_arm.frame_num = LF_FL_2_HOLD - 2;
        Flare_SetArm(g_Lara.left_arm.frame_num);
        return;
    }

    int32_t frame_num = g_Lara.left_arm.frame_num + 1;

    g_Lara.flare_control_left = 1;

    if (frame_num < LF_FL_DRAW || frame_num > LF_FL_2_HOLD - 1) {
        frame_num = LF_FL_DRAW;
    } else if (frame_num == LF_FL_DRAW_GOT_IT) {
        Flare_DrawMeshes();
        if (!g_SaveGame.bonus_flag) {
            Inv_RemoveItem(O_FLARES_ITEM);
        }
    } else if (frame_num >= LF_FL_IGNITE && frame_num <= LF_FL_2_HOLD - 2) {
        if (frame_num == LF_FL_IGNITE) {
            g_Lara.flare_age = 0;
        }
        Flare_DoInHand(g_Lara.flare_age);
    } else if (frame_num == LF_FL_2_HOLD - 1) {
        Flare_Ready();
        Flare_DoInHand(g_Lara.flare_age);
        frame_num = LF_FL_HOLD;
    }

    g_Lara.left_arm.frame_num = frame_num;
    Flare_SetArm(frame_num);
}

void Flare_Undraw(void)
{
    int16_t frame_num_1 = g_Lara.left_arm.frame_num;
    int16_t frame_num_2 = g_Lara.flare_frame;

    g_Lara.flare_control_left = 1;

    if (g_LaraItem->goal_anim_state == LS_STOP && g_Lara.skidoo == NO_ITEM) {
        if (Item_TestAnimEqual(g_LaraItem, LA_STAND_IDLE)) {
            Item_SwitchToAnim(g_LaraItem, LA_FLARE_THROW, frame_num_1);
            g_Lara.flare_frame = g_LaraItem->frame_num;
            frame_num_2 = g_LaraItem->frame_num;
        }

        if (Item_TestAnimEqual(g_LaraItem, LA_FLARE_THROW)) {
            g_Lara.flare_control_left = 0;

            if (frame_num_2 >= Anim_GetAnim(LA_FLARE_THROW)->frame_base
                    + LF_FL_THROW_FT - 1) {
                g_Lara.gun_type = g_Lara.last_gun_type;
                g_Lara.request_gun_type = g_Lara.last_gun_type;
                g_Lara.gun_status = LGS_ARMLESS;
                Gun_InitialiseNewWeapon();
                g_Lara.target = nullptr;
                g_Lara.right_arm.lock = 0;
                g_Lara.left_arm.lock = 0;
                Item_SwitchToAnim(g_LaraItem, LA_STAND_STILL, 0);
                g_Lara.flare_frame = g_LaraItem->frame_num;
                g_LaraItem->current_anim_state = LS_STOP;
                g_LaraItem->goal_anim_state = LS_STOP;
                return;
            }
            g_Lara.flare_frame = frame_num_2 + 1;
        }
    } else if (
        g_LaraItem->current_anim_state == LS_STOP && g_Lara.skidoo == -1) {
        Item_SwitchToAnim(g_LaraItem, LA_STAND_STILL, 0);
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
            Flare_UndrawMeshes();
        } else if (frame_num_1 == LF_FL_DRAW) {
            frame_num_1 = 0;
            g_Lara.gun_type = g_Lara.last_gun_type;
            g_Lara.request_gun_type = g_Lara.last_gun_type;
            g_Lara.gun_status = LGS_ARMLESS;
            Gun_InitialiseNewWeapon();
            g_Lara.target = nullptr;
            g_Lara.flare_control_left = 0;
            g_Lara.right_arm.lock = 0;
            g_Lara.left_arm.lock = 0;
            g_Lara.flare_frame = 0;
        }
    } else if (frame_num_1 >= LF_FL_2_HOLD && frame_num_1 < LF_FL_END) {
        frame_num_1++;
        if (frame_num_1 == LF_FL_END) {
            frame_num_1 = LF_FL_THROW;
        }
    }

    if (frame_num_1 >= LF_FL_THROW && frame_num_1 < LF_FL_THROW_RELEASE) {
        Flare_DoInHand(g_Lara.flare_age);
    }

    g_Lara.left_arm.frame_num = frame_num_1;
    Flare_SetArm(frame_num_1);
}

void Flare_DrawMeshes(void)
{
    Lara_SwapSingleMesh(LM_HAND_L, O_LARA_FLARE);
}

void Flare_UndrawMeshes(void)
{
    Lara_SwapSingleMesh(LM_HAND_L, O_LARA);
}

void Flare_Ready(void)
{
    g_Lara.gun_status = LGS_ARMLESS;
    g_Lara.left_arm.rot.x = 0;
    g_Lara.left_arm.rot.y = 0;
    g_Lara.left_arm.rot.z = 0;
    g_Lara.right_arm.rot.x = 0;
    g_Lara.right_arm.rot.y = 0;
    g_Lara.right_arm.rot.z = 0;
    g_Lara.left_arm.lock = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.target = nullptr;
}

void Flare_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->fall_speed) {
        item->rot.x += DEG_1 * 3;
        item->rot.z += DEG_1 * 5;
    } else {
        item->rot.x = 0;
        item->rot.z = 0;
    }

    const int32_t x = item->pos.x;
    const int32_t y = item->pos.y;
    const int32_t z = item->pos.z;
    item->pos.z += (item->speed * Math_Cos(item->rot.y)) >> W2V_SHIFT;
    item->pos.x += (item->speed * Math_Sin(item->rot.y)) >> W2V_SHIFT;

    if (Room_Get(item->room_num)->flags & RF_UNDERWATER) {
        item->fall_speed += (5 - item->fall_speed) / 2;
        item->speed = item->speed + (5 - item->speed) / 2;
    } else {
        item->fall_speed += GRAVITY;
    }
    item->pos.y += item->fall_speed;

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);

    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    if (item->pos.y < height) {
        const int32_t ceiling =
            Room_GetCeiling(sector, item->pos.x, item->pos.y, item->pos.z);
        if (item->pos.y < ceiling) {
            item->fall_speed = -item->fall_speed;
            item->pos.y = ceiling;
        }
    } else {
        if (y > height) {
            item->pos.x = x;
            item->pos.y = y;
            item->pos.z = z;
            item->rot.y += DEG_180;
            item->speed /= 2;
            room_num = item->room_num;
        } else {
            if (item->fall_speed > 40) {
                item->fall_speed = 40 - item->fall_speed;
                CLAMPL(item->fall_speed, -100);
            } else {
                item->fall_speed = 0;
                item->speed -= 3;
                CLAMPL(item->speed, 0);
            }
            item->pos.y = height;
        }
    }

    if (room_num != item->room_num) {
        Item_NewRoom(item_num, room_num);
    }

    int32_t flare_age = ((int32_t)(intptr_t)item->data) & 0x7FFF;
    if (flare_age < MAX_FLARE_AGE) {
        flare_age++;
    } else if (item->fall_speed == 0 && item->speed == 0) {
        Item_Kill(item_num);
        return;
    }

    if (Flare_DoLight(&item->pos, flare_age)) {
        flare_age |= 0x8000u;
        M_DoBurnEffects(item->pos, item->pos, item->room_num);
    }

    item->data = (void *)(intptr_t)flare_age;
}
