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
    obj->initialise_func = BaconLara_Initialise;
    obj->control_func = BaconLara_Control;
    obj->draw_func = BaconLara_Draw;
    obj->collision_func = Creature_Collision;
    obj->hit_points = LARA_MAX_HITPOINTS;
    obj->shadow_size = (UNIT_SHADOW * 10) / 16;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void BaconLara_Initialise(int16_t item_num)
{
    const OBJECT *const lara_obj = Object_Get(O_LARA);
    OBJECT *const bacon_obj = Object_Get(O_BACON_LARA);
    bacon_obj->anim_idx = lara_obj->anim_idx;
    bacon_obj->frame_base = lara_obj->frame_base;
    Item_Get(item_num)->data = nullptr;
}

bool BaconLara_InitialiseAnchor(const int32_t room_index)
{
    if (room_index >= Room_GetCount()) {
        return false;
    }

    const ROOM *const room = Room_Get(room_index);
    m_AnchorX = room->pos.x + room->size.x * (WALL_L >> 1);
    m_AnchorZ = room->pos.z + room->size.z * (WALL_L >> 1);

    return true;
}

void BaconLara_Control(int16_t item_num)
{
    if (m_AnchorX == -1) {
        return;
    }

    ITEM *const item = Item_Get(item_num);

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

        const int16_t relative_anim = Item_GetRelativeAnim(g_LaraItem);
        const int16_t relative_frame = Item_GetRelativeFrame(g_LaraItem);
        Item_SwitchToObjAnim(item, relative_anim, relative_frame, O_LARA);
        item->pos.x = x;
        item->pos.y = y;
        item->pos.z = z;
        item->rot.x = g_LaraItem->rot.x;
        item->rot.y = g_LaraItem->rot.y - DEG_180;
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

void BaconLara_Draw(const ITEM *item)
{
    if (item->current_anim_state == LS_DEATH) {
        Object_DrawAnimatingItem(item);
        return;
    }

    OBJECT_MESH *old_mesh_ptrs[LM_NUMBER_OF];

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        old_mesh_ptrs[mesh] = Lara_GetMesh(mesh);
        Lara_SwapSingleMesh(mesh, O_BACON_LARA);
    }

    Lara_Draw(item);

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        Lara_SetMesh(mesh, old_mesh_ptrs[mesh]);
    }
}
