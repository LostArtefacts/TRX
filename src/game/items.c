#include "game/const.h"
#include "game/items.h"
#include "game/vars.h"
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
    INJECT(0x00422250, InitialiseFXArray);
}
