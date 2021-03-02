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
    INJECT(0x00422250, InitialiseFXArray);
}
