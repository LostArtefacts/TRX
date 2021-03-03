#include "game/lot.h"
#include "game/vars.h"
#include "specific/init.h"
#include "specific/shed.h"
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

int32_t EnableBaddieAI(int16_t item_num, int32_t always)
{
    if (Items[item_num].data) {
        return 1;
    }

    if (SlotsUsed < NUM_SLOTS) {
        for (int32_t slot = 0; slot < NUM_SLOTS; slot++) {
            CREATURE_INFO* creature = &BaddieSlots[slot];
            if (creature->item_num == NO_ITEM) {
                InitialiseSlot(item_num, slot);
                return 1;
            }
        }
        S_ExitSystem("UnpauseBaddie() grimmer!");
    }

    int32_t worst_dist = 0;
    if (!always) {
        ITEM_INFO* item = &Items[item_num];
        int32_t x = (item->pos.x - Camera.pos.x) >> 8;
        int32_t y = (item->pos.y - Camera.pos.y) >> 8;
        int32_t z = (item->pos.z - Camera.pos.z) >> 8;
        worst_dist = SQUARE(x) + SQUARE(y) + SQUARE(z);
    }

    int32_t worst_slot = -1;
    for (int32_t slot = 0; slot < NUM_SLOTS; slot++) {
        CREATURE_INFO* creature = &BaddieSlots[slot];
        ITEM_INFO* item = &Items[creature->item_num];
        int32_t x = (item->pos.x - Camera.pos.x) >> 8;
        int32_t y = (item->pos.y - Camera.pos.y) >> 8;
        int32_t z = (item->pos.z - Camera.pos.z) >> 8;
        int32_t dist = SQUARE(x) + SQUARE(y) + SQUARE(z);
        if (dist > worst_dist) {
            worst_dist = dist;
            worst_slot = slot;
        }
    }

    if (worst_slot < 0) {
        return 0;
    }

    Items[BaddieSlots[worst_slot].item_num].status = IS_INVISIBLE;
    DisableBaddieAI(BaddieSlots[worst_slot].item_num);
    InitialiseSlot(item_num, worst_slot);
    return 1;
}

void T1MInjectGameLOT()
{
    INJECT(0x0042A300, InitialiseLOTArray);
    INJECT(0x0042A360, DisableBaddieAI);
    INJECT(0x0042A3A0, EnableBaddieAI);
}
