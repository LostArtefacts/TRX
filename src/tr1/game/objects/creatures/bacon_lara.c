#include "game/objects/creatures/bacon_lara.h"

#include "game/creature.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lara/draw.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#define BLF_FASTFALL 1

static int32_t m_AnchorX = -1;
static int32_t m_AnchorZ = -1;

void BaconLara_Setup(OBJECT *obj)
{
    obj->initialise = BaconLara_Initialise;
    obj->control = BaconLara_Control;
    obj->draw_routine = BaconLara_Draw;
    obj->collision = Creature_Collision;
    obj->hit_points = LARA_MAX_HITPOINTS;
    obj->shadow_size = (UNIT_SHADOW * 10) / 16;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void BaconLara_Initialise(int16_t item_num)
{
    g_Objects[O_BACON_LARA].anim_idx = g_Objects[O_LARA].anim_idx;
    g_Objects[O_BACON_LARA].frame_base = g_Objects[O_LARA].frame_base;
    g_Items[item_num].data = NULL;
}

bool BaconLara_InitialiseAnchor(int32_t room_index)
{
    if (room_index >= g_RoomCount) {
        return false;
    }

    ROOM *r = &g_RoomInfo[room_index];
    m_AnchorX = r->pos.x + r->size.x * (WALL_L >> 1);
    m_AnchorZ = r->pos.z + r->size.z * (WALL_L >> 1);

    return true;
}

void BaconLara_Control(int16_t item_num)
{
    if (m_AnchorX == -1) {
        return;
    }

    ITEM *item = &g_Items[item_num];

    if (item->hit_points < LARA_MAX_HITPOINTS) {
        Lara_TakeDamage((LARA_MAX_HITPOINTS - item->hit_points) * 10, false);
        item->hit_points = LARA_MAX_HITPOINTS;
    }

    if (!item->data) {
        int32_t x = 2 * m_AnchorX - g_LaraItem->pos.x;
        int32_t y = g_LaraItem->pos.y;
        int32_t z = 2 * m_AnchorZ - g_LaraItem->pos.z;

        int16_t room_num = item->room_num;
        const SECTOR *sector = Room_GetSector(x, y, z, &room_num);
        const int32_t h = Room_GetHeight(sector, x, y, z);
        item->floor = h;

        room_num = g_LaraItem->room_num;
        sector = Room_GetSector(
            g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z, &room_num);
        int32_t lh = Room_GetHeight(
            sector, g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z);

        int16_t relative_anim =
            g_LaraItem->anim_num - g_Objects[g_LaraItem->object_id].anim_idx;
        int16_t relative_frame =
            g_LaraItem->frame_num - g_Anims[g_LaraItem->anim_num].frame_base;
        Item_SwitchToObjAnim(item, relative_anim, relative_frame, O_LARA);
        item->pos.x = x;
        item->pos.y = y;
        item->pos.z = z;
        item->rot.x = g_LaraItem->rot.x;
        item->rot.y = g_LaraItem->rot.y - PHD_180;
        item->rot.z = g_LaraItem->rot.z;
        Item_NewRoom(item_num, g_LaraItem->room_num);

        if (h >= lh + WALL_L && !g_LaraItem->gravity) {
            item->current_anim_state = LS_FAST_FALL;
            item->goal_anim_state = LS_FAST_FALL;
            Item_SwitchToAnim(item, LA_FAST_FALL, BLF_FASTFALL);
            item->speed = 0;
            item->fall_speed = 0;
            item->gravity = 1;
            item->data = (void *)-1;
            item->pos.y += 50;
        }
    }

    if (item->data) {
        Item_Animate(item);

        int32_t x = item->pos.x;
        int32_t y = item->pos.y;
        int32_t z = item->pos.z;

        int16_t room_num = item->room_num;
        const SECTOR *sector = Room_GetSector(x, y, z, &room_num);
        const int32_t h = Room_GetHeight(sector, x, y, z);
        item->floor = h;

        Room_TestTriggers(item);
        if (item->pos.y >= h) {
            item->floor = h;
            item->pos.y = h;
            Room_TestTriggers(item);
            item->gravity = 0;
            item->fall_speed = 0;
            item->goal_anim_state = LS_DEATH;
            item->required_anim_state = LS_DEATH;
        }
    }
}

void BaconLara_Draw(ITEM *item)
{
    if (item->current_anim_state == LS_DEATH) {
        Object_DrawAnimatingItem(item);
        return;
    }

    int16_t *old_mesh_ptrs[LM_NUMBER_OF];

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        old_mesh_ptrs[mesh] = g_Lara.mesh_ptrs[mesh];
        Lara_SwapSingleMesh(mesh, O_BACON_LARA);
    }

    Lara_Draw(item);

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        g_Lara.mesh_ptrs[mesh] = old_mesh_ptrs[mesh];
    }
}
