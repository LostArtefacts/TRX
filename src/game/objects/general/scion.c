#include "game/objects/general/scion.h"

#include "game/effects.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/objects/general/pickup.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <stdbool.h>

#define EXTRA_ANIM_PEDESTAL_SCION 0
#define EXTRA_ANIM_HOLDER_SCION 0
#define LF_PICKUPSCION 44

static XYZ_32 m_Scion_Position = { 0, 640, -310 };
static XYZ_32 m_Scion_Position4 = { 0, 280, -512 + 105 };

static const OBJECT_BOUNDS m_Scion_Bounds = {
    .shift = {
        .min = { .x = -256, .y = +640 - 100, .z = -350, },
        .max = { .x = +256, .y = +640 + 100, .z = -200, },
    },
    .rot = {
        .min = { .x = -10 * PHD_DEGREE, .y = 0, .z = 0, },
        .max = { .x = +10 * PHD_DEGREE, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_Scion_Bounds4 = {
    .shift = {
        .min = { .x = -256, .y = +256 - 50, .z = -512 - 350, },
        .max = { .x = +256, .y = +256 + 50, .z = -200, },
    },
    .rot = {
        .min = { .x = -10 * PHD_DEGREE, .y = 0, .z = 0, },
        .max = { .x = +10 * PHD_DEGREE, .y = 0, .z = 0, },
    },
};

void Scion_Setup1(OBJECT_INFO *obj)
{
    g_Objects[O_SCION_ITEM].draw_routine = Object_DrawPickupItem;
    g_Objects[O_SCION_ITEM].collision = Scion_Collision;
    g_Objects[O_SCION_ITEM].save_flags = 1;
}

void Scion_Setup2(OBJECT_INFO *obj)
{
    g_Objects[O_SCION_ITEM2].draw_routine = Object_DrawPickupItem;
    g_Objects[O_SCION_ITEM2].collision = Pickup_Collision;
    g_Objects[O_SCION_ITEM2].save_flags = 1;
}

void Scion_Setup3(OBJECT_INFO *obj)
{
    g_Objects[O_SCION_ITEM3].control = Scion_Control3;
    g_Objects[O_SCION_ITEM3].hit_points = 5;
    g_Objects[O_SCION_ITEM3].save_flags = 1;
}

void Scion_Setup4(OBJECT_INFO *obj)
{
    g_Objects[O_SCION_ITEM4].control = Scion_Control;
    g_Objects[O_SCION_ITEM4].collision = Scion_Collision4;
    g_Objects[O_SCION_ITEM4].save_flags = 1;
}

void Scion_SetupHolder(OBJECT_INFO *obj)
{
    g_Objects[O_SCION_HOLDER].control = Scion_Control;
    g_Objects[O_SCION_HOLDER].collision = Object_Collision;
    g_Objects[O_SCION_HOLDER].save_anim = 1;
    g_Objects[O_SCION_HOLDER].save_flags = 1;
}

void Scion_Control(int16_t item_num)
{
    Item_Animate(&g_Items[item_num]);
}

void Scion_Control3(int16_t item_num)
{
    static int32_t counter = 0;
    ITEM_INFO *item = &g_Items[item_num];

    if (item->hit_points > 0) {
        counter = 0;
        Item_Animate(item);
        return;
    }

    if (counter == 0) {
        item->status = IS_INVISIBLE;
        item->hit_points = DONT_TARGET;
        int16_t room_num = item->room_number;
        FLOOR_INFO *floor =
            Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        Room_TestTriggers(g_TriggerIndex, true);
        Item_RemoveDrawn(item_num);
    }

    if (counter % 10 == 0) {
        int16_t fx_num = Effect_Create(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[fx_num];
            fx->pos.x = item->pos.x + (Random_GetControl() - 0x4000) / 32;
            fx->pos.y =
                item->pos.y + (Random_GetControl() - 0x4000) / 256 - 500;
            fx->pos.z = item->pos.z + (Random_GetControl() - 0x4000) / 32;
            fx->speed = 0;
            fx->frame_number = 0;
            fx->object_number = O_EXPLOSION1;
            fx->counter = 0;
            Sound_Effect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);
            g_Camera.bounce = -200;
        }
    }

    counter++;
    if (counter >= LOGIC_FPS * 3) {
        Item_Kill(item_num);
    }
}

void Scion_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    int16_t rotx = item->rot.x;
    int16_t roty = item->rot.y;
    int16_t rotz = item->rot.z;
    item->rot.y = lara_item->rot.y;
    item->rot.x = 0;
    item->rot.z = 0;

    if (!Lara_TestPosition(item, &m_Scion_Bounds)) {
        goto cleanup;
    }

    if (lara_item->current_anim_state == LS_PICKUP) {
        if (Item_TestFrameEqual(lara_item, LF_PICKUPSCION)) {
            Overlay_AddPickup(item->object_number);
            Inv_AddItem(item->object_number);
            item->status = IS_INVISIBLE;
            Item_RemoveDrawn(item_num);
            g_GameInfo.current[g_CurrentLevel].stats.pickup_count++;
        }
    } else if (
        g_Input.action && g_Lara.gun_status == LGS_ARMLESS
        && !lara_item->gravity_status
        && lara_item->current_anim_state == LS_STOP) {
        Lara_AlignPosition(item, &m_Scion_Position);
        lara_item->current_anim_state = LS_PICKUP;
        lara_item->goal_anim_state = LS_PICKUP;
        Item_SwitchToObjAnim(
            lara_item, EXTRA_ANIM_PEDESTAL_SCION, 0, O_LARA_EXTRA);
        g_Lara.gun_status = LGS_HANDS_BUSY;
        g_Camera.type = CAM_CINEMATIC;
        g_CineFrame = 0;
        g_CinePosition.pos = lara_item->pos;
        g_CinePosition.rot.x = lara_item->rot.x;
        g_CinePosition.rot.y = lara_item->rot.y;
        g_CinePosition.rot.z = lara_item->rot.z;
    }
cleanup:
    item->rot.x = rotx;
    item->rot.y = roty;
    item->rot.z = rotz;
}

void Scion_Collision4(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    int16_t rotx = item->rot.x;
    int16_t roty = item->rot.y;
    int16_t rotz = item->rot.z;
    item->rot.y = lara_item->rot.y;
    item->rot.x = 0;
    item->rot.z = 0;

    if (!Lara_TestPosition(item, &m_Scion_Bounds4)) {
        goto cleanup;
    }

    if (g_Input.action && g_Lara.gun_status == LGS_ARMLESS
        && !lara_item->gravity_status
        && lara_item->current_anim_state == LS_STOP) {
        Lara_AlignPosition(item, &m_Scion_Position4);
        lara_item->current_anim_state = LS_PICKUP;
        lara_item->goal_anim_state = LS_PICKUP;
        Item_SwitchToObjAnim(
            lara_item, EXTRA_ANIM_HOLDER_SCION, 0, O_LARA_EXTRA);
        g_Lara.gun_status = LGS_HANDS_BUSY;
        g_Camera.type = CAM_CINEMATIC;
        g_CineFrame = 0;
        g_CinePosition.pos = lara_item->pos;
        g_CinePosition.rot.x = lara_item->rot.x;
        g_CinePosition.rot.y = lara_item->rot.y;
        g_CinePosition.rot.z = lara_item->rot.z;
        g_CinePosition.rot.y -= PHD_90;
    }
cleanup:
    item->rot.x = rotx;
    item->rot.y = roty;
    item->rot.z = rotz;
}
