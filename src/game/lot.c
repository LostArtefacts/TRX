#include "game/lot.h"

#include "global/const.h"
#include "global/vars.h"
#include "specific/init.h"
#include "util.h"

#include <stddef.h>

void InitialiseLOTArray()
{
    TRACE("");
    BaddieSlots =
        game_malloc(NUM_SLOTS * sizeof(CREATURE_INFO), GBUF_CREATURE_DATA);
    for (int i = 0; i < NUM_SLOTS; i++) {
        CREATURE_INFO *creature = &BaddieSlots[i];
        creature->item_num = NO_ITEM;
        creature->LOT.node =
            game_malloc(sizeof(BOX_NODE) * NumberBoxes, GBUF_CREATURE_LOT);
    }
    SlotsUsed = 0;
}

void DisableBaddieAI(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    CREATURE_INFO *creature = item->data;
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
            CREATURE_INFO *creature = &BaddieSlots[slot];
            if (creature->item_num == NO_ITEM) {
                InitialiseSlot(item_num, slot);
                return 1;
            }
        }
        S_ExitSystem("UnpauseBaddie() grimmer!");
    }

    int32_t worst_dist = 0;
    if (!always) {
        ITEM_INFO *item = &Items[item_num];
        int32_t x = (item->pos.x - Camera.pos.x) >> 8;
        int32_t y = (item->pos.y - Camera.pos.y) >> 8;
        int32_t z = (item->pos.z - Camera.pos.z) >> 8;
        worst_dist = SQUARE(x) + SQUARE(y) + SQUARE(z);
    }

    int32_t worst_slot = -1;
    for (int32_t slot = 0; slot < NUM_SLOTS; slot++) {
        CREATURE_INFO *creature = &BaddieSlots[slot];
        ITEM_INFO *item = &Items[creature->item_num];
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

void InitialiseSlot(int16_t item_num, int32_t slot)
{
    CREATURE_INFO *creature = &BaddieSlots[slot];
    ITEM_INFO *item = &Items[item_num];
    item->data = creature;
    creature->item_num = item_num;
    creature->mood = MOOD_BORED;
    creature->head_rotation = 0;
    creature->neck_rotation = 0;
    creature->maximum_turn = PHD_DEGREE;
    creature->flags = 0;

    creature->LOT.step = STEP_L;
    creature->LOT.drop = -STEP_L;
    creature->LOT.block_mask = BLOCKED;
    creature->LOT.fly = 0;

    switch (item->object_number) {
    case O_BAT:
    case O_ALLIGATOR:
    case O_FISH:
        creature->LOT.step = WALL_L * 20;
        creature->LOT.drop = -WALL_L * 20;
        creature->LOT.fly = STEP_L / 16;
        break;

    case O_DINOSAUR:
    case O_WARRIOR1:
    case O_CENTAUR:
        creature->LOT.block_mask = BLOCKABLE;
        break;

    case O_WOLF:
    case O_LION:
    case O_LIONESS:
    case O_PUMA:
        creature->LOT.drop = -WALL_L;
        break;

    case O_APE:
        creature->LOT.step = STEP_L * 2;
        creature->LOT.drop = -WALL_L;
        break;
    }

    ClearLOT(&creature->LOT);
    CreateZone(item);

    SlotsUsed++;
}

void CreateZone(ITEM_INFO *item)
{
    CREATURE_INFO *creature = item->data;

    int16_t *zone;
    int16_t *flip;
    if (creature->LOT.fly) {
        zone = FlyZone[0];
        flip = FlyZone[1];
    } else if (creature->LOT.step == STEP_L) {
        zone = GroundZone[0];
        flip = GroundZone[1];
    } else {
        zone = GroundZone2[1];
        flip = GroundZone2[1];
    }

    ROOM_INFO *r = &RoomInfo[item->room_number];
    int32_t x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
    int32_t y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
    item->box_number = r->floor[x_floor + y_floor * r->x_size].box;

    int16_t zone_number = zone[item->box_number];
    int16_t flip_number = flip[item->box_number];

    creature->LOT.zone_count = 0;
    BOX_NODE *node = creature->LOT.node;
    for (int i = 0; i < NumberBoxes; i++) {
        if (zone[i] == zone_number || flip[i] == flip_number) {
            node->box_number = i;
            node++;
            creature->LOT.zone_count++;
        }
    }
}

int32_t InitialiseLOT(LOT_INFO *LOT)
{
    LOT->node = game_malloc(sizeof(BOX_NODE) * NumberBoxes, GBUF_CREATURE_LOT);
    ClearLOT(LOT);
    return 1;
}

void ClearLOT(LOT_INFO *LOT)
{
    LOT->search_number = 0;
    LOT->head = NO_BOX;
    LOT->tail = NO_BOX;
    LOT->target_box = NO_BOX;
    LOT->required_box = NO_BOX;

    for (int i = 0; i < NumberBoxes; i++) {
        BOX_NODE *node = &LOT->node[i];
        node->search_number = 0;
        node->exit_box = NO_BOX;
        node->next_expansion = NO_BOX;
    }
}

void T1MInjectGameLOT()
{
    INJECT(0x0042A300, InitialiseLOTArray);
    INJECT(0x0042A360, DisableBaddieAI);
    INJECT(0x0042A3A0, EnableBaddieAI);
    INJECT(0x0042A570, InitialiseSlot);
    INJECT(0x0042A6B0, CreateZone);
    INJECT(0x0042A780, InitialiseLOT);
}
