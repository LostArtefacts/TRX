#include "game/lot.h"

#include "game/box.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <assert.h>

void __cdecl LOT_InitialiseArray(void)
{
    g_BaddieSlots =
        game_malloc(NUM_SLOTS * sizeof(CREATURE), GBUF_CREATURE_DATA);

    for (int32_t i = 0; i < NUM_SLOTS; i++) {
        CREATURE *const creature = &g_BaddieSlots[i];
        creature->item_num = NO_ITEM;
        creature->lot.node =
            game_malloc(g_BoxCount * sizeof(BOX_NODE), GBUF_CREATURE_LOT);
    }

    g_SlotsUsed = 0;
}

void __cdecl LOT_DisableBaddieAI(const int16_t item_num)
{
    CREATURE *creature;

    if (item_num == g_Lara.item_num) {
        creature = g_Lara.creature;
        g_Lara.creature = NULL;
    } else {
        ITEM *const item = &g_Items[item_num];
        creature = (CREATURE *)item->data;
        item->data = NULL;
    }

    if (creature != NULL) {
        creature->item_num = NO_ITEM;
        g_SlotsUsed--;
    }
}

bool __cdecl LOT_EnableBaddieAI(const int16_t item_num, const bool always)
{
    if (g_Lara.item_num == item_num) {
        if (g_Lara.creature != NULL) {
            return true;
        }
    } else if (g_Items[item_num].data != NULL) {
        return true;
    }

    if (g_SlotsUsed < NUM_SLOTS) {
        for (int32_t slot = 0; slot < NUM_SLOTS; slot++) {
            if (g_BaddieSlots[slot].item_num == NO_ITEM) {
                LOT_InitialiseSlot(item_num, slot);
                return true;
            }
        }
        assert(false);
    }

    int32_t worst_dist = 0;
    if (!always) {
        const ITEM *const item = &g_Items[item_num];
        const int32_t dx = (item->pos.x - g_Camera.pos.pos.x) >> 8;
        const int32_t dy = (item->pos.y - g_Camera.pos.pos.y) >> 8;
        const int32_t dz = (item->pos.z - g_Camera.pos.pos.z) >> 8;
        worst_dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
    }

    int32_t worst_slot = -1;
    for (int32_t slot = 0; slot < NUM_SLOTS; slot++) {
        const int32_t item_num = g_BaddieSlots[slot].item_num;
        const ITEM *const item = &g_Items[item_num];
        const int32_t dx = (item->pos.x - g_Camera.pos.pos.x) >> 8;
        const int32_t dy = (item->pos.y - g_Camera.pos.pos.y) >> 8;
        const int32_t dz = (item->pos.z - g_Camera.pos.pos.z) >> 8;
        const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (dist > worst_dist) {
            worst_dist = dist;
            worst_slot = slot;
        }
    }

    if (worst_slot < 0) {
        return false;
    }

    const CREATURE *const creature = &g_BaddieSlots[worst_slot];
    g_Items[creature->item_num].status = IS_INVISIBLE;
    LOT_DisableBaddieAI(creature->item_num);
    LOT_InitialiseSlot(item_num, worst_slot);
    return true;
}

void __cdecl LOT_InitialiseSlot(const int16_t item_num, const int32_t slot)
{

    CREATURE *const creature = &g_BaddieSlots[slot];
    ITEM *const item = &g_Items[item_num];

    if (item_num == g_Lara.item_num) {
        g_Lara.creature = &g_BaddieSlots[slot];
    } else {
        item->data = creature;
    }

    creature->item_num = item_num;
    creature->mood = MOOD_BORED;
    creature->neck_rotation = 0;
    creature->head_rotation = 0;
    creature->maximum_turn = PHD_DEGREE;
    creature->flags = 0;
    creature->enemy = 0;
    creature->lot.step = STEP_L;
    creature->lot.drop = -STEP_L * 2;
    creature->lot.block_mask = BOX_BLOCKED;
    creature->lot.fly = 0;

    switch (item->object_id) {
    case O_LARA:
        creature->lot.step = WALL_L * 20;
        creature->lot.drop = -WALL_L * 20;
        creature->lot.fly = STEP_L;
        break;

    case O_SHARK:
    case O_BARRACUDA:
    case O_DIVER:
    case O_JELLY:
    case O_CROW:
    case O_EAGLE:
        creature->lot.step = WALL_L * 20;
        creature->lot.drop = -WALL_L * 20;
        creature->lot.fly = STEP_L / 16;
        if (item->object_id == O_SHARK) {
            creature->lot.block_mask = BOX_BLOCKABLE;
        }
        break;

    case O_WORKER_3:
    case O_WORKER_4:
    case O_YETI:
        creature->lot.step = WALL_L;
        creature->lot.drop = -WALL_L;
        break;

    case O_SPIDER:
    case O_SKIDOO_ARMED:
        creature->lot.step = WALL_L / 2;
        creature->lot.drop = -WALL_L;
        break;

    case O_DINO:
        creature->lot.block_mask = BOX_BLOCKABLE;
        break;

    default:
        break;
    }

    LOT_ClearLOT(&creature->lot);

    if (item_num != g_Lara.item_num) {
        LOT_CreateZone(item);
    }

    g_SlotsUsed++;
}

void __cdecl LOT_CreateZone(ITEM *const item)
{
    CREATURE *const creature = item->data;

    int16_t *zone;
    int16_t *flip;
    if (creature->lot.fly) {
        zone = g_FlyZone[0];
        flip = g_FlyZone[1];
    } else {
        zone = g_GroundZone[BOX_ZONE(creature->lot.step)][0];
        flip = g_GroundZone[BOX_ZONE(creature->lot.step)][1];
    }

    const ROOM *const r = &g_Rooms[item->room_num];
    const int32_t z_sector = (item->pos.z - r->pos.z) >> WALL_SHIFT;
    const int32_t x_sector = (item->pos.x - r->pos.x) >> WALL_SHIFT;
    item->box_num = r->sectors[z_sector + x_sector * r->size.z].box;

    int16_t zone_num = zone[item->box_num];
    int16_t flip_num = flip[item->box_num];

    creature->lot.zone_count = 0;
    BOX_NODE *node = creature->lot.node;
    for (int32_t i = 0; i < g_BoxCount; i++) {
        if (zone[i] == zone_num || flip[i] == flip_num) {
            node->box_num = i;
            node++;
            creature->lot.zone_count++;
        }
    }
}

void __cdecl LOT_ClearLOT(LOT_INFO *const lot)
{
    lot->search_num = 0;
    lot->head = NO_BOX;
    lot->tail = NO_BOX;
    lot->target_box = NO_BOX;
    lot->required_box = NO_BOX;

    for (int32_t i = 0; i < g_BoxCount; i++) {
        BOX_NODE *const node = &lot->node[i];
        node->next_expansion = NO_BOX;
        node->exit_box = NO_BOX;
        node->search_num = 0;
    }
}
