#include "game/items/common.h"

#include "game/carrier.h"
#include "game/game.h"
#include "game/game_buf.h"
#include "game/game_flow.h"
#include "game/lara/common.h"
#include "game/objects.h"
#include "game/output/const.h"
#include "game/rooms.h"
#include "memory.h"
#include "utils.h"

#include <string.h>

static int32_t m_LevelItemCount = 0;
static int16_t m_MaxUsedItemCount = 0;
static ITEM *m_Items = nullptr;
static int16_t m_NextItemActive = NO_ITEM;
static int16_t m_PrevItemActive = NO_ITEM;
static int16_t m_NextItemFree = NO_ITEM;

void Item_InitialiseItems(const int32_t num_items)
{
    m_Items = GameBuf_Alloc(sizeof(ITEM) * MAX_ITEMS, GBUF_ITEMS);
    m_LevelItemCount = num_items;
    m_MaxUsedItemCount = num_items;
    m_NextItemFree = num_items;
    m_NextItemActive = NO_ITEM;
    m_PrevItemActive = NO_ITEM;

    for (int32_t i = m_NextItemFree; i < MAX_ITEMS - 1; i++) {
        ITEM *const item = &m_Items[i];
        item->active = false;
        item->next_item = i + 1;
    }
    m_Items[MAX_ITEMS - 1].next_item = NO_ITEM;
}

ITEM *Item_Get(const int16_t item_num)
{
    if (item_num == NO_ITEM) {
        return nullptr;
    }
    return &m_Items[item_num];
}

int16_t Item_GetIndex(const ITEM *const item)
{
    return item - Item_Get(0);
}

ITEM *Item_Find(const OBJECT_ID obj_id)
{
    for (int32_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (item->object_id == obj_id) {
            return item;
        }
    }

    return nullptr;
}

bool Item_SetName(const int16_t item_num, const char *const name)
{
    ITEM *const item = Item_Get(item_num);
    if (item == nullptr) {
        return false;
    }
    if (name != nullptr) {
        ITEM *const existing = Item_GetByName(name);
        if (existing != nullptr && existing != item) {
            return false;
        }
    }
    if (name != nullptr) {
        item->name = GameBuf_Alloc(strlen(name) + 1, GBUF_ITEMS);
        strcpy(item->name, name);
    } else {
        item->name = nullptr;
    }
    return true;
}

ITEM *Item_GetByName(const char *const name)
{
    if (name == nullptr) {
        return nullptr;
    }
    // search through all items for matching name
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->name != nullptr && strcmp(item->name, name) == 0) {
            return item;
        }
    }
    return nullptr;
}

int32_t Item_GetLevelCount(void)
{
    return m_LevelItemCount;
}

int32_t Item_GetTotalCount(void)
{
    return m_MaxUsedItemCount;
}

int16_t Item_GetNextActive(void)
{
    return m_NextItemActive;
}

int16_t Item_GetPrevActive(void)
{
    return m_PrevItemActive;
}

void Item_SetPrevActive(const int16_t item_num)
{
    m_PrevItemActive = item_num;
}

int16_t Item_Create(void)
{
    const int16_t item_num = m_NextItemFree;
    if (item_num != NO_ITEM) {
        m_Items[item_num].flags = 0;
        m_NextItemFree = m_Items[item_num].next_item;
    }
    m_MaxUsedItemCount = MAX(m_MaxUsedItemCount, item_num + 1);
    return item_num;
}

int16_t Item_CreateLevelItem(void)
{
    const int16_t item_num = Item_Create();
    if (item_num != NO_ITEM) {
        m_LevelItemCount++;
    }
    return item_num;
}

int16_t Item_Spawn(const ITEM *const item, const OBJECT_ID obj_id)
{
    const int16_t spawn_num = Item_Create();
    if (spawn_num != NO_ITEM) {
        ITEM *const spawn = Item_Get(spawn_num);
        spawn->object_id = obj_id;
        spawn->room_num = item->room_num;
        spawn->pos = item->pos;
        spawn->rot = item->rot;
        Item_Initialise(spawn_num);
        spawn->status = IS_INACTIVE;
        spawn->shade.value_1 = SHADE_NEUTRAL;
    }
    return spawn_num;
}

void Item_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    Item_SwitchToAnim(item, 0, 0);
    item->goal_anim_state = Item_GetAnim(item)->current_anim_state;
    item->current_anim_state = item->goal_anim_state;
    item->required_anim_state = 0;
    item->rot.x = 0;
    item->rot.z = 0;
    item->speed = 0;
    item->fall_speed = 0;
    item->hit_points = obj->hit_points;
    item->max_hit_points = obj->hit_points;
    item->timer = 0;
    item->mesh_bits = 0xFFFFFFFF;
    item->touch_bits = 0;
    item->data = nullptr;
    item->priv = nullptr;
    item->carried_item = nullptr;
    item->name = nullptr;

    item->active = false;
    item->status = IS_INACTIVE;
    item->gravity = false;
    item->hit_status = false;
    item->collidable = true;
    item->looked_at = false;
    item->enable_interpolation = true;
    item->enable_shadow = true;

#if TR_VERSION >= 2
    item->killed = false;
    if ((item->flags & IF_KILLED) != 0) {
        item->killed = true;
        item->flags &= ~IF_KILLED;
    }
#endif

    if ((item->flags & IF_INVISIBLE) != 0) {
        item->status = IS_INVISIBLE;
        item->flags &= ~IF_INVISIBLE;
    } else if (TR_VERSION >= 2 && obj->intelligent) {
        item->status = IS_INVISIBLE;
    }

    if ((item->flags & IF_CODE_BITS) == IF_CODE_BITS) {
        item->flags &= ~IF_CODE_BITS;
        item->flags |= IF_REVERSE;
        Item_AddActive(item_num);
        item->status = IS_ACTIVE;
    }

    ROOM *const room = Room_Get(item->room_num);
    item->next_item = room->item_num;
    room->item_num = item_num;

    const SECTOR *const sector =
        Room_GetWorldSector(room, item->pos.x, item->pos.z);
    item->floor = sector->floor.height;

    // TODO: remove GF check once demo config reset is run before level load
    if (Game_IsBonusFlagSet(GBF_NGPLUS)
        && GF_GetCurrentLevel()->type != GFL_DEMO) {
        item->hit_points *= 2;
    }

    if (obj->initialise_func != nullptr) {
        obj->initialise_func(item_num);
    }
}

void Item_Control(void)
{
    int16_t item_num = Item_GetNextActive();
    while (item_num != NO_ITEM) {
        const ITEM *const item = Item_Get(item_num);
        const int16_t next = item->next_active;
        const OBJECT *obj = Object_Get(item->object_id);
        if ((item->flags & IF_KILLED) == 0 && obj->control_func != nullptr) {
            obj->control_func(item_num);
        }
        item_num = next;
    }

    Carrier_AnimateDrops();
}

void Item_Kill(const int16_t item_num)
{
    Item_RemoveActive(item_num);
    Item_RemoveDrawn(item_num);

    ITEM *const item = &m_Items[item_num];
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item == lara->target) {
        lara->target = nullptr;
    }

    item->flags |= IF_KILLED;

    if (item_num >= m_LevelItemCount) {
        item->next_item = m_NextItemFree;
        m_NextItemFree = item_num;
    }

    while (m_MaxUsedItemCount > 0
           && m_Items[m_MaxUsedItemCount - 1].flags & IF_KILLED) {
        m_MaxUsedItemCount--;
    }
}

void Item_RemoveActive(const int16_t item_num)
{
    ITEM *const item = &m_Items[item_num];
    if (!item->active) {
        return;
    }

    item->active = false;

    int16_t link_num = m_NextItemActive;
    if (link_num == item_num) {
        m_NextItemActive = item->next_active;
        return;
    }

    while (link_num != NO_ITEM) {
        if (m_Items[link_num].next_active == item_num) {
            m_Items[link_num].next_active = item->next_active;
            return;
        }
        link_num = m_Items[link_num].next_active;
    }
}

void Item_RemoveDrawn(const int16_t item_num)
{
    const ITEM *const item = &m_Items[item_num];
    if (item->room_num == NO_ROOM) {
        return;
    }

    ROOM *const room = Room_Get(item->room_num);
    int16_t link_num = room->item_num;
    if (link_num == item_num) {
        room->item_num = item->next_item;
        return;
    }

    while (link_num != NO_ITEM) {
        if (m_Items[link_num].next_item == item_num) {
            m_Items[link_num].next_item = item->next_item;
            return;
        }
        link_num = m_Items[link_num].next_item;
    }
}

void Item_ClearKilled(void)
{
    // Remove corpses and other killed items. Part of OG performance
    // improvements, generously used in Opera House and Barkhang Monastery
    int16_t link_num = Item_GetPrevActive();
    while (link_num != NO_ITEM) {
        ITEM *const item = Item_Get(link_num);
        Item_Kill(link_num);
        link_num = item->next_active;
        item->next_active = NO_ITEM;
    }
    Item_SetPrevActive(NO_ITEM);
}

void Item_AddActive(const int16_t item_num)
{
    ITEM *const item = &m_Items[item_num];
    if (Object_Get(item->object_id)->control_func == nullptr) {
        item->status = IS_INACTIVE;
        return;
    }

    if (item->active) {
        return;
    }

    item->active = true;
    item->next_active = m_NextItemActive;
    m_NextItemActive = item_num;
}

void Item_UpdateRoom(const int16_t item_num, const int16_t room_num)
{
    ITEM *const item = &m_Items[item_num];
    if (item->room_num == room_num) {
        return;
    }

    ROOM *room = nullptr;

    if (item->room_num != NO_ROOM) {
        room = Room_Get(item->room_num);

        int16_t link_num = room->item_num;
        if (link_num == item_num) {
            room->item_num = item->next_item;
        } else {
            while (link_num != NO_ITEM) {
                if (m_Items[link_num].next_item == item_num) {
                    m_Items[link_num].next_item = item->next_item;
                    break;
                }
                link_num = m_Items[link_num].next_item;
            }
        }
    }

    room = Room_Get(room_num);
    item->room_num = room_num;
    item->next_item = room->item_num;
    room->item_num = item_num;
}

int32_t Item_GlobalReplace(
    const OBJECT_ID src_obj_id, const OBJECT_ID dst_obj_id)
{
    int32_t changed = 0;

    for (int32_t item_num = 0; item_num < m_MaxUsedItemCount; item_num++) {
        ITEM *const item = &m_Items[item_num];
        if (item->object_id == src_obj_id) {
            item->object_id = dst_obj_id;
            changed++;
        }
    }

    return changed;
}

bool Item_IsTriggerActive(ITEM *const item)
{
    const bool ok = !(item->flags & IF_REVERSE);

    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return !ok;
    }

    if (!item->timer) {
        return ok;
    }

    if (item->timer == -1) {
        return !ok;
    }

    item->timer--;
    if (item->timer == 0) {
        item->timer = -1;
    }

    return ok;
}
