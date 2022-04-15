#include "game/objects/scion.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/pickup.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

static PHD_VECTOR m_Scion_Position = { 0, 640, -310 };
static PHD_VECTOR m_Scion_Position4 = { 0, 280, -512 + 105 };

static int16_t m_Scion_Bounds[12] = {
    -256,
    +256,
    +640 - 100,
    +640 + 100,
    -350,
    -200,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
    0,
    0,
    0,
    0,
};

static int16_t m_Scion_Bounds4[12] = {
    -256,
    +256,
    +256 - 50,
    +256 + 50,
    -512 - 350,
    -200,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
    0,
    0,
    0,
    0,
};

void Scion_Setup1(OBJECT_INFO *obj)
{
    g_Objects[O_SCION_ITEM].draw_routine = DrawPickupItem;
    g_Objects[O_SCION_ITEM].collision = Scion_Collision;
}

void Scion_Setup2(OBJECT_INFO *obj)
{
    g_Objects[O_SCION_ITEM2].draw_routine = DrawPickupItem;
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
    g_Objects[O_SCION_HOLDER].collision = ObjectCollision;
    g_Objects[O_SCION_HOLDER].save_anim = 1;
    g_Objects[O_SCION_HOLDER].save_flags = 1;
}

void Scion_Control(int16_t item_num)
{
    AnimateItem(&g_Items[item_num]);
}

void Scion_Control3(int16_t item_num)
{
    static int32_t counter = 0;
    ITEM_INFO *item = &g_Items[item_num];

    if (item->hit_points > 0) {
        counter = 0;
        AnimateItem(item);
        return;
    }

    if (counter == 0) {
        item->status = IS_INVISIBLE;
        item->hit_points = DONT_TARGET;
        int16_t room_num = item->room_number;
        FLOOR_INFO *floor =
            GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        TestTriggers(g_TriggerIndex, 1);
        RemoveDrawnItem(item_num);
    }

    if (counter % 10 == 0) {
        int16_t fx_num = CreateEffect(item->room_number);
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
    if (counter >= FRAMES_PER_SECOND * 3) {
        KillItem(item_num);
    }
}

void Scion_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    item->pos.y_rot = lara_item->pos.y_rot;
    item->pos.x_rot = 0;
    item->pos.z_rot = 0;

    if (!Lara_TestPosition(item, m_Scion_Bounds)) {
        return;
    }

    if (lara_item->current_anim_state == LS_PICKUP) {
        if (lara_item->frame_number
            == g_Anims[lara_item->anim_number].frame_base + AF_PICKUPSCION) {
            Overlay_AddPickup(item->object_number);
            Inv_AddItem(item->object_number);
            item->status = IS_INVISIBLE;
            RemoveDrawnItem(item_num);
            g_GameInfo.current[g_CurrentLevel].stats.pickup_count++;
        }
    } else if (
        g_Input.action && g_Lara.gun_status == LGS_ARMLESS
        && !lara_item->gravity_status
        && lara_item->current_anim_state == LS_STOP) {
        Lara_AlignPosition(item, &m_Scion_Position);
        lara_item->current_anim_state = LS_PICKUP;
        lara_item->goal_anim_state = LS_PICKUP;
        lara_item->anim_number = g_Objects[O_LARA_EXTRA].anim_index;
        lara_item->frame_number = g_Anims[lara_item->anim_number].frame_base;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        g_Camera.type = CAM_CINEMATIC;
        g_CineFrame = 0;
        g_CinePosition = lara_item->pos;
    }
}

void Scion_Collision4(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    item->pos.y_rot = lara_item->pos.y_rot;
    item->pos.x_rot = 0;
    item->pos.z_rot = 0;

    if (!Lara_TestPosition(item, m_Scion_Bounds4)) {
        return;
    }

    if (g_Input.action && g_Lara.gun_status == LGS_ARMLESS
        && !lara_item->gravity_status
        && lara_item->current_anim_state == LS_STOP) {
        Lara_AlignPosition(item, &m_Scion_Position4);
        lara_item->current_anim_state = LS_PICKUP;
        lara_item->goal_anim_state = LS_PICKUP;
        lara_item->anim_number = g_Objects[O_LARA_EXTRA].anim_index;
        lara_item->frame_number = g_Anims[lara_item->anim_number].frame_base;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        g_Camera.type = CAM_CINEMATIC;
        g_CineFrame = 0;
        g_CinePosition = lara_item->pos;
        g_CinePosition.y_rot -= PHD_90;
    }
}
