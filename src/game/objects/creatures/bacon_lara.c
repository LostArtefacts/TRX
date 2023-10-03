#include "game/objects/creatures/bacon_lara.h"

#include "game/creature.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lara/lara_draw.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stddef.h>

#define BLF_FASTFALL 1

static int32_t m_AnchorX = -1;
static int32_t m_AnchorZ = -1;

void BaconLara_Setup(OBJECT_INFO *obj)
{
    obj->initialise = BaconLara_Initialise;
    obj->control = BaconLara_Control;
    obj->draw_routine = BaconLara_Draw;
    obj->collision = Creature_Collision;
    obj->hit_points = LARA_HITPOINTS;
    obj->shadow_size = (UNIT_SHADOW * 10) / 16;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
}

void BaconLara_Initialise(int16_t item_num)
{
    g_Objects[O_BACON_LARA].anim_index = g_Objects[O_LARA].anim_index;
    g_Objects[O_BACON_LARA].frame_base = g_Objects[O_LARA].frame_base;
    g_Items[item_num].data = NULL;
}

bool BaconLara_InitialiseAnchor(int32_t room_index)
{
    if (room_index >= g_RoomCount) {
        return false;
    }

    ROOM_INFO *r = &g_RoomInfo[room_index];
    m_AnchorX = r->x + r->y_size * (WALL_L >> 1);
    m_AnchorZ = r->z + r->x_size * (WALL_L >> 1);

    return true;
}

void BaconLara_Control(int16_t item_num)
{
    if (m_AnchorX == -1) {
        return;
    }

    ITEM_INFO *item = &g_Items[item_num];

    if (item->hit_points < LARA_HITPOINTS) {
        Lara_TakeDamage((LARA_HITPOINTS - item->hit_points) * 10, false);
        item->hit_points = LARA_HITPOINTS;
    }

    if (!item->data) {
        int32_t x = 2 * m_AnchorX - g_LaraItem->pos.x;
        int32_t y = g_LaraItem->pos.y;
        int32_t z = 2 * m_AnchorZ - g_LaraItem->pos.z;

        int16_t room_num = item->room_number;
        FLOOR_INFO *floor = Room_GetFloor(x, y, z, &room_num);
        int32_t h = Room_GetHeight(floor, x, y, z);
        item->floor = h;

        room_num = g_LaraItem->room_number;
        floor = Room_GetFloor(
            g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z, &room_num);
        int32_t lh = Room_GetHeight(
            floor, g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z);

        int16_t relative_anim = g_LaraItem->anim_number
            - g_Objects[g_LaraItem->object_number].anim_index;
        int16_t relative_frame = g_LaraItem->frame_number
            - g_Anims[g_LaraItem->anim_number].frame_base;
        Item_SwitchToObjAnim(item, relative_anim, relative_frame, O_LARA);
        item->pos.x = x;
        item->pos.y = y;
        item->pos.z = z;
        item->pos.x_rot = g_LaraItem->pos.x_rot;
        item->pos.y_rot = g_LaraItem->pos.y_rot - PHD_180;
        item->pos.z_rot = g_LaraItem->pos.z_rot;
        Item_NewRoom(item_num, g_LaraItem->room_number);

        if (h >= lh + WALL_L && !g_LaraItem->gravity_status) {
            item->current_anim_state = LS_FAST_FALL;
            item->goal_anim_state = LS_FAST_FALL;
            Item_SwitchToAnim(item, LA_FAST_FALL, BLF_FASTFALL);
            item->speed = 0;
            item->fall_speed = 0;
            item->gravity_status = 1;
            item->data = (void *)-1;
            item->pos.y += 50;
        }
    }

    if (item->data) {
        Item_Animate(item);

        int32_t x = item->pos.x;
        int32_t y = item->pos.y;
        int32_t z = item->pos.z;

        int16_t room_num = item->room_number;
        FLOOR_INFO *floor = Room_GetFloor(x, y, z, &room_num);
        int32_t h = Room_GetHeight(floor, x, y, z);
        item->floor = h;

        Room_TestTriggers(g_TriggerIndex, true);
        if (item->pos.y >= h) {
            item->floor = h;
            item->pos.y = h;
            floor = Room_GetFloor(x, h, z, &room_num);
            Room_GetHeight(floor, x, h, z);
            Room_TestTriggers(g_TriggerIndex, true);
            item->gravity_status = 0;
            item->fall_speed = 0;
            item->goal_anim_state = LS_DEATH;
            item->required_anim_state = LS_DEATH;
        }
    }
}

void BaconLara_Draw(ITEM_INFO *item)
{
    if (item->current_anim_state == LS_DEATH) {
        Object_DrawAnimatingItem(item);
        return;
    }

    int16_t *old_mesh_ptrs[LM_NUMBER_OF];

    for (int i = 0; i < LM_NUMBER_OF; i++) {
        old_mesh_ptrs[i] = g_Lara.mesh_ptrs[i];
        g_Lara.mesh_ptrs[i] = g_Meshes[g_Objects[O_BACON_LARA].mesh_index + i];
    }

    Lara_Draw(item);

    for (int i = 0; i < LM_NUMBER_OF; i++) {
        g_Lara.mesh_ptrs[i] = old_mesh_ptrs[i];
    }
}
