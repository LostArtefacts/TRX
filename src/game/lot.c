#include "game/data.h"
#include "game/lot.h"
#include "specific/init.h"
#include "util.h"

void __cdecl InitialiseLOTArray()
{
    TRACE("");
    BaddieSlots =
        game_malloc(NUM_SLOTS * sizeof(CREATURE_INFO), GBUF_CreatureData);
    CREATURE_INFO* creature = BaddieSlots;
    for (int i = 0; i < NUM_SLOTS; i++, creature++) {
        creature->item_num = NO_ITEM;
        creature->LOT.node =
            game_malloc(sizeof(BOX_NODE) * NumberBoxes, GBUF_CreatureLot);
    }
    SlotsUsed = 0;
}

void TR1MInjectGameLOT()
{
    INJECT(0x0042A300, InitialiseLOTArray);
}
