#include "game/ai/evil_lara.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "global/vars.h"

void SetupEvilLara(OBJECT_INFO *obj)
{
    obj->initialise = InitialiseEvilLara;
    obj->control = ControlEvilLara;
    obj->draw_routine = DrawEvilLara;
    obj->collision = CreatureCollision;
    obj->hit_points = LARA_HITPOINTS;
    obj->shadow_size = (UNIT_SHADOW * 10) / 16;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
}

void InitialiseEvilLara(int16_t item_num)
{
    g_Objects[O_EVIL_LARA].frame_base = g_Objects[O_LARA].frame_base;
    g_Items[item_num].data = NULL;
}

void ControlEvilLara(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->hit_points < LARA_HITPOINTS) {
        g_LaraItem->hit_points -= (LARA_HITPOINTS - item->hit_points) * 10;
        item->hit_points = LARA_HITPOINTS;
    }

    if (!item->data) {
        int32_t x = 2 * 36 * WALL_L - g_LaraItem->pos.x;
        int32_t y = g_LaraItem->pos.y;
        int32_t z = 2 * 60 * WALL_L - g_LaraItem->pos.z;

        int16_t room_num = item->room_number;
        FLOOR_INFO *floor = GetFloor(x, y, z, &room_num);
        int32_t h = GetHeight(floor, x, y, z);
        item->floor = h;

        room_num = g_LaraItem->room_number;
        floor = GetFloor(
            g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z, &room_num);
        int32_t lh = GetHeight(
            floor, g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z);

        item->anim_number = g_LaraItem->anim_number;
        item->frame_number = g_LaraItem->frame_number;
        item->pos.x = x;
        item->pos.y = y;
        item->pos.z = z;
        item->pos.x_rot = g_LaraItem->pos.x_rot;
        item->pos.y_rot = g_LaraItem->pos.y_rot - PHD_180;
        item->pos.z_rot = g_LaraItem->pos.z_rot;
        ItemNewRoom(item_num, g_LaraItem->room_number);

        if (h >= lh + WALL_L && !g_LaraItem->gravity_status) {
            item->current_anim_state = AS_FASTFALL;
            item->goal_anim_state = AS_FASTFALL;
            item->anim_number = AA_FASTFALL;
            item->frame_number = AF_FASTFALL;
            item->speed = 0;
            item->fall_speed = 0;
            item->gravity_status = 1;
            item->data = (void *)-1;
            item->pos.y += 50;
        }
    }

    if (item->data) {
        AnimateItem(item);

        int32_t x = item->pos.x;
        int32_t y = item->pos.y;
        int32_t z = item->pos.z;

        int16_t room_num = item->room_number;
        FLOOR_INFO *floor = GetFloor(x, y, z, &room_num);
        int32_t h = GetHeight(floor, x, y, z);
        item->floor = h;

        TestTriggers(g_TriggerIndex, 1);
        if (item->pos.y >= h) {
            item->floor = h;
            item->pos.y = h;
            floor = GetFloor(x, h, z, &room_num);
            GetHeight(floor, x, h, z);
            TestTriggers(g_TriggerIndex, 1);
            item->gravity_status = 0;
            item->fall_speed = 0;
            item->goal_anim_state = AS_DEATH;
            item->required_anim_state = AS_DEATH;
        }
    }
}

void DrawEvilLara(ITEM_INFO *item)
{
    int16_t *old_mesh_ptrs[LM_NUMBER_OF];

    for (int i = 0; i < LM_NUMBER_OF; i++) {
        old_mesh_ptrs[i] = g_Lara.mesh_ptrs[i];
        g_Lara.mesh_ptrs[i] = g_Meshes[g_Objects[O_EVIL_LARA].mesh_index + i];
    }

    DrawLara(item);

    for (int i = 0; i < LM_NUMBER_OF; i++) {
        g_Lara.mesh_ptrs[i] = old_mesh_ptrs[i];
    }
}
