#include "game/const.h"
#include "game/items.h"
#include "game/vars.h"
#include "specific/shed.h"
#include "util.h"

void InitialiseItemArray(int32_t num_items)
{
    NextItemActive = NO_ITEM;
    NextItemFree = LevelItemCount;
    for (int i = LevelItemCount; i < num_items - 1; i++) {
        Items[i].next_item = i + 1;
    }
    Items[num_items - 1].next_item = NO_ITEM;
}

void KillItem(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    int16_t linknum = NextItemActive;
    if (linknum == item_num) {
        NextItemActive = item->next_active;
    } else {
        for (; linknum != NO_ITEM; linknum = Items[linknum].next_active) {
            if (Items[linknum].next_active == item_num) {
                Items[linknum].next_active = item->next_active;
                break;
            }
        }
    }

    linknum = RoomInfo[item->room_number].item_number;
    if (linknum == item_num) {
        RoomInfo[item->room_number].item_number = item->next_item;
    } else {
        for (; linknum != NO_ITEM; linknum = Items[linknum].next_item) {
            if (Items[linknum].next_item == item_num) {
                Items[linknum].next_item = item->next_item;
                break;
            }
        }
    }

    if (item == Lara.target) {
        Lara.target = NULL;
    }

    if (item_num < LevelItemCount) {
        item->flags |= IF_KILLED_ITEM;
    } else {
        item->next_item = NextItemFree;
        NextItemFree = item_num;
    }
}

int16_t CreateItem()
{
    int16_t item_num = NextItemFree;
    if (item_num != NO_ITEM) {
        Items[item_num].flags = 0;
        NextItemFree = Items[item_num].next_item;
    }
    return item_num;
}

void InitialiseItem(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    OBJECT_INFO* object = &Objects[item->object_number];

    item->anim_number = object->anim_index;
    item->frame_number = Anims[item->anim_number].frame_base;
    item->current_anim_state = Anims[item->anim_number].current_anim_state;
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

    ROOM_INFO* r = &RoomInfo[item->room_number];
    item->next_item = r->item_number;
    r->item_number = item_num;
    int32_t x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
    int32_t y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
    FLOOR_INFO* floor = &r->floor[x_floor + y_floor * r->x_size];
    item->floor = floor->floor << 8;

    if (SaveGame[0].bonus_flag) {
        item->hit_points *= 2;
    }
    if (object->initialise) {
        object->initialise(item_num);
    }
}

void RemoveActiveItem(int16_t item_num)
{
    if (!Items[item_num].active) {
        S_ExitSystem("Item already deactive");
    }

    Items[item_num].active = 0;

    int16_t linknum = NextItemActive;
    if (linknum == item_num) {
        NextItemActive = Items[item_num].next_active;
        return;
    }

    for (; linknum != NO_ITEM; linknum = Items[linknum].next_active) {
        if (Items[linknum].next_active == item_num) {
            Items[linknum].next_active = Items[item_num].next_active;
            break;
        }
    }
}

void InitialiseFXArray()
{
    NextFxActive = NO_ITEM;
    NextFxFree = 0;
    for (int i = 0; i < NUM_EFFECTS - 1; i++) {
        Effects[i].next_fx = i + 1;
    }
    Effects[NUM_EFFECTS - 1].next_fx = NO_ITEM;
}

void T1MInjectGameItems()
{
    INJECT(0x00421B10, InitialiseItemArray);
    INJECT(0x00421B50, KillItem);
    INJECT(0x00421C80, CreateItem);
    INJECT(0x00421CC0, InitialiseItem);
    INJECT(0x00421EB0, RemoveActiveItem);
    INJECT(0x00422250, InitialiseFXArray);
}
