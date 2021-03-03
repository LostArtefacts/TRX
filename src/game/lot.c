#include "game/lot.h"
#include "game/vars.h"
#include "specific/init.h"
#include "util.h"

void InitialiseLOTArray()
{
    TRACE("");
    BaddieSlots =
        game_malloc(NUM_SLOTS * sizeof(CREATURE_INFO), GBUF_CREATURE_DATA);
    for (int i = 0; i < NUM_SLOTS; i++) {
        CREATURE_INFO* creature = &BaddieSlots[i];
        creature->item_num = NO_ITEM;
        creature->LOT.node =
            game_malloc(sizeof(BOX_NODE) * NumberBoxes, GBUF_CREATURE_LOT);
    }
    SlotsUsed = 0;
}

void DisableBaddieAI(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    CREATURE_INFO* creature = item->data;
    item->data = NULL;
    if (creature) {
        creature->item_num = NO_ITEM;
        SlotsUsed--;
    }
}

void T1MInjectGameLOT()
{
    INJECT(0x0042A300, InitialiseLOTArray);
    INJECT(0x0042A360, DisableBaddieAI);
}
