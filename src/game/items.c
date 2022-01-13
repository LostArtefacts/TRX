#include "game/items.h"

#include "game/shell.h"
#include "global/const.h"
#include "global/vars.h"

#include <stdio.h>

void InitialiseItemArray(int32_t num_items)
{
    g_NextItemActive = NO_ITEM;
    g_NextItemFree = g_LevelItemCount;
    for (int i = g_LevelItemCount; i < num_items - 1; i++) {
        g_Items[i].next_item = i + 1;
    }
    g_Items[num_items - 1].next_item = NO_ITEM;
}

void KillItem(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    ROOM_INFO *r = &g_RoomInfo[item->room_number];

    int16_t linknum = g_NextItemActive;
    if (linknum == item_num) {
        g_NextItemActive = item->next_active;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Items[linknum].next_active) {
            if (g_Items[linknum].next_active == item_num) {
                g_Items[linknum].next_active = item->next_active;
                break;
            }
        }
    }

    linknum = r->item_number;
    if (linknum == item_num) {
        r->item_number = item->next_item;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Items[linknum].next_item) {
            if (g_Items[linknum].next_item == item_num) {
                g_Items[linknum].next_item = item->next_item;
                break;
            }
        }
    }

    if (item == g_Lara.target) {
        g_Lara.target = NULL;
    }

    if (item_num < g_LevelItemCount) {
        item->flags |= IF_KILLED_ITEM;
    } else {
        item->next_item = g_NextItemFree;
        g_NextItemFree = item_num;
    }
}

int16_t CreateItem()
{
    int16_t item_num = g_NextItemFree;
    if (item_num != NO_ITEM) {
        g_Items[item_num].flags = 0;
        g_NextItemFree = g_Items[item_num].next_item;
    }
    return item_num;
}

void InitialiseItem(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    OBJECT_INFO *object = &g_Objects[item->object_number];

    item->anim_number = object->anim_index;
    item->frame_number = g_Anims[item->anim_number].frame_base;
    item->current_anim_state = g_Anims[item->anim_number].current_anim_state;
    item->goal_anim_state = item->current_anim_state;
    item->required_anim_state = 0;
    item->pos.x_rot = 0;
    item->pos.z_rot = 0;
    item->speed = 0;
    item->fall_speed = 0;
    item->status = IS_NOT_ACTIVE;
    item->active = 0;
    item->gravity_status = 0;
    item->hit_status = 0;
    item->looked_at = 0;
    item->collidable = 1;
    item->hit_points = object->hit_points;
    item->timer = 0;
    item->mesh_bits = -1;
    item->touch_bits = 0;
    item->data = NULL;

    if (item->flags & IF_NOT_VISIBLE) {
        item->status = IS_INVISIBLE;
        item->flags -= IF_NOT_VISIBLE;
    }

    if ((item->flags & IF_CODE_BITS) == IF_CODE_BITS) {
        item->flags -= IF_CODE_BITS;
        item->flags |= IF_REVERSE;
        AddActiveItem(item_num);
        item->status = IS_ACTIVE;
    }

    ROOM_INFO *r = &g_RoomInfo[item->room_number];
    item->next_item = r->item_number;
    r->item_number = item_num;
    int32_t x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
    int32_t y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
    FLOOR_INFO *floor = &r->floor[x_floor + y_floor * r->x_size];
    item->floor = floor->floor << 8;

    if (g_GameInfo.bonus_flag & GBF_NGPLUS) {
        item->hit_points *= 2;
    }
    if (object->initialise) {
        object->initialise(item_num);
    }
}

void RemoveActiveItem(int16_t item_num)
{
    if (!g_Items[item_num].active) {
        Shell_ExitSystem("Item already deactive");
    }

    g_Items[item_num].active = 0;

    int16_t linknum = g_NextItemActive;
    if (linknum == item_num) {
        g_NextItemActive = g_Items[item_num].next_active;
        return;
    }

    for (; linknum != NO_ITEM; linknum = g_Items[linknum].next_active) {
        if (g_Items[linknum].next_active == item_num) {
            g_Items[linknum].next_active = g_Items[item_num].next_active;
            break;
        }
    }
}

void RemoveDrawnItem(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    ROOM_INFO *r = &g_RoomInfo[item->room_number];

    int16_t linknum = r->item_number;
    if (linknum == item_num) {
        r->item_number = item->next_item;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Items[linknum].next_item) {
            if (g_Items[linknum].next_item == item_num) {
                g_Items[linknum].next_item = item->next_item;
                break;
            }
        }
    }
}

void AddActiveItem(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!g_Objects[item->object_number].control) {
        item->status = IS_NOT_ACTIVE;
        return;
    }

    if (item->active) {
        Shell_ExitSystemFmt(
            "Item(%d)(Obj%d) already Active\n", item_num, item->object_number);
    }

    item->active = 1;
    item->next_active = g_NextItemActive;
    g_NextItemActive = item_num;
}

void ItemNewRoom(int16_t item_num, int16_t room_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    ROOM_INFO *r = &g_RoomInfo[item->room_number];

    int16_t linknum = r->item_number;
    if (linknum == item_num) {
        r->item_number = item->next_item;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Items[linknum].next_item) {
            if (g_Items[linknum].next_item == item_num) {
                g_Items[linknum].next_item = item->next_item;
                break;
            }
        }
    }

    r = &g_RoomInfo[room_num];
    item->room_number = room_num;
    item->next_item = r->item_number;
    r->item_number = item_num;
}

int16_t SpawnItem(ITEM_INFO *item, int16_t object_num)
{
    int16_t spawn_num = CreateItem();
    if (spawn_num != NO_ITEM) {
        ITEM_INFO *spawn = &g_Items[spawn_num];
        spawn->object_number = object_num;
        spawn->room_number = item->room_number;
        spawn->pos = item->pos;
        InitialiseItem(spawn_num);
        spawn->status = IS_NOT_ACTIVE;
        spawn->shade = 4096;
    }
    return spawn_num;
}

int32_t GlobalItemReplace(int32_t src_object_num, int32_t dst_object_num)
{
    int32_t changed = 0;
    for (int i = 0; i < g_RoomCount; i++) {
        ROOM_INFO *r = &g_RoomInfo[i];
        for (int16_t item_num = r->item_number; item_num != NO_ITEM;
             item_num = g_Items[item_num].next_item) {
            if (g_Items[item_num].object_number == src_object_num) {
                g_Items[item_num].object_number = dst_object_num;
                changed++;
            }
        }
    }
    return changed;
}

void InitialiseFXArray()
{
    g_NextFxActive = NO_ITEM;
    g_NextFxFree = 0;
    for (int i = 0; i < NUM_EFFECTS - 1; i++) {
        g_Effects[i].next_fx = i + 1;
    }
    g_Effects[NUM_EFFECTS - 1].next_fx = NO_ITEM;
}

int16_t CreateEffect(int16_t room_num)
{
    int16_t fx_num = g_NextFxFree;
    if (fx_num == NO_ITEM) {
        return fx_num;
    }

    FX_INFO *fx = &g_Effects[fx_num];
    g_NextFxFree = fx->next_fx;

    ROOM_INFO *r = &g_RoomInfo[room_num];
    fx->room_number = room_num;
    fx->next_fx = r->fx_number;
    r->fx_number = fx_num;

    fx->next_active = g_NextFxActive;
    g_NextFxActive = fx_num;

    return fx_num;
}

void KillEffect(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];

    int16_t linknum = g_NextFxActive;
    if (linknum == fx_num) {
        g_NextFxActive = fx->next_active;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Effects[linknum].next_active) {
            if (g_Effects[linknum].next_active == fx_num) {
                g_Effects[linknum].next_active = fx->next_active;
                break;
            }
        }
    }

    ROOM_INFO *r = &g_RoomInfo[fx->room_number];
    linknum = r->fx_number;
    if (linknum == fx_num) {
        r->fx_number = fx->next_fx;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Effects[linknum].next_fx) {
            if (g_Effects[linknum].next_fx == fx_num) {
                g_Effects[linknum].next_fx = fx->next_fx;
                break;
            }
        }
    }

    fx->next_fx = g_NextFxFree;
    g_NextFxFree = fx_num;
}

void EffectNewRoom(int16_t fx_num, int16_t room_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    ROOM_INFO *r = &g_RoomInfo[fx->room_number];

    int16_t linknum = r->fx_number;
    if (linknum == fx_num) {
        r->fx_number = fx->next_fx;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Effects[linknum].next_fx) {
            if (g_Effects[linknum].next_fx == fx_num) {
                g_Effects[linknum].next_fx = fx->next_fx;
                break;
            }
        }
    }

    r = &g_RoomInfo[room_num];
    fx->room_number = room_num;
    fx->next_fx = r->fx_number;
    r->fx_number = fx_num;
}
