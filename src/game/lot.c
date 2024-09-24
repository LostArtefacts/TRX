#include "game/lot.h"

#include "game/gamebuf.h"
#include "game/items.h"
#include "game/shell.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <stddef.h>

static int32_t m_SlotsUsed = 0;
static CREATURE *m_BaddieSlots = NULL;

void LOT_InitialiseArray(void)
{
    m_BaddieSlots =
        GameBuf_Alloc(NUM_SLOTS * sizeof(CREATURE), GBUF_CREATURE_INFO);
    for (int i = 0; i < NUM_SLOTS; i++) {
        CREATURE *creature = &m_BaddieSlots[i];
        creature->item_num = NO_ITEM;
        creature->lot.node =
            GameBuf_Alloc(sizeof(BOX_NODE) * g_NumberBoxes, GBUF_CREATURE_LOT);
    }
    m_SlotsUsed = 0;
}

void LOT_DisableBaddieAI(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];
    CREATURE *creature = item->data;
    item->data = NULL;
    if (creature) {
        creature->item_num = NO_ITEM;
        m_SlotsUsed--;
    }
}

bool LOT_EnableBaddieAI(int16_t item_num, int32_t always)
{
    if (g_Items[item_num].data) {
        return true;
    }

    if (m_SlotsUsed < NUM_SLOTS) {
        for (int32_t slot = 0; slot < NUM_SLOTS; slot++) {
            CREATURE *creature = &m_BaddieSlots[slot];
            if (creature->item_num == NO_ITEM) {
                LOT_InitialiseSlot(item_num, slot);
                return true;
            }
        }
        Shell_ExitSystem("UnpauseBaddie() grimmer!");
    }

    int32_t worst_dist = 0;
    if (!always) {
        ITEM *item = &g_Items[item_num];
        int32_t x = (item->pos.x - g_Camera.pos.x) >> 8;
        int32_t y = (item->pos.y - g_Camera.pos.y) >> 8;
        int32_t z = (item->pos.z - g_Camera.pos.z) >> 8;
        worst_dist = SQUARE(x) + SQUARE(y) + SQUARE(z);
    }

    int32_t worst_slot = -1;
    for (int32_t slot = 0; slot < NUM_SLOTS; slot++) {
        CREATURE *creature = &m_BaddieSlots[slot];
        ITEM *item = &g_Items[creature->item_num];
        int32_t x = (item->pos.x - g_Camera.pos.x) >> 8;
        int32_t y = (item->pos.y - g_Camera.pos.y) >> 8;
        int32_t z = (item->pos.z - g_Camera.pos.z) >> 8;
        int32_t dist = SQUARE(x) + SQUARE(y) + SQUARE(z);
        if (dist > worst_dist) {
            worst_dist = dist;
            worst_slot = slot;
        }
    }

    if (worst_slot < 0) {
        return false;
    }

    g_Items[m_BaddieSlots[worst_slot].item_num].status = IS_INVISIBLE;
    LOT_DisableBaddieAI(m_BaddieSlots[worst_slot].item_num);
    LOT_InitialiseSlot(item_num, worst_slot);
    return true;
}

void LOT_InitialiseSlot(int16_t item_num, int32_t slot)
{
    CREATURE *creature = &m_BaddieSlots[slot];
    ITEM *item = &g_Items[item_num];
    item->data = creature;
    creature->item_num = item_num;
    creature->mood = MOOD_BORED;
    creature->head_rotation = 0;
    creature->neck_rotation = 0;
    creature->maximum_turn = PHD_DEGREE;
    creature->flags = 0;

    creature->lot.step = STEP_L;
    creature->lot.drop = -STEP_L;
    creature->lot.block_mask = BLOCKED;
    creature->lot.fly = 0;

    switch (item->object_id) {
    case O_BAT:
    case O_ALLIGATOR:
    case O_FISH:
        creature->lot.step = WALL_L * 20;
        creature->lot.drop = -WALL_L * 20;
        creature->lot.fly = STEP_L / 16;
        break;

    case O_TREX:
    case O_WARRIOR_1:
    case O_CENTAUR:
        creature->lot.block_mask = BLOCKABLE;
        break;

    case O_WOLF:
    case O_LION:
    case O_LIONESS:
    case O_PUMA:
        creature->lot.drop = -WALL_L;
        break;

    case O_APE:
        creature->lot.step = STEP_L * 2;
        creature->lot.drop = -WALL_L;
        break;

    default:
        break;
    }

    LOT_ClearLOT(&creature->lot);
    LOT_CreateZone(item);

    m_SlotsUsed++;
}

void LOT_CreateZone(ITEM *item)
{
    CREATURE *creature = item->data;

    int16_t *zone;
    int16_t *flip;
    if (creature->lot.fly) {
        zone = g_FlyZone[0];
        flip = g_FlyZone[1];
    } else if (creature->lot.step == STEP_L) {
        zone = g_GroundZone[0];
        flip = g_GroundZone[1];
    } else {
        zone = g_GroundZone2[1];
        flip = g_GroundZone2[1];
    }

    const ROOM *const r = &g_RoomInfo[item->room_num];
    const int32_t z_sector = (item->pos.z - r->pos.z) >> WALL_SHIFT;
    const int32_t x_sector = (item->pos.x - r->pos.x) >> WALL_SHIFT;
    item->box_num = r->sectors[z_sector + x_sector * r->size.z].box;

    int16_t zone_num = zone[item->box_num];
    int16_t flip_num = flip[item->box_num];

    creature->lot.zone_count = 0;
    BOX_NODE *node = creature->lot.node;
    for (int i = 0; i < g_NumberBoxes; i++) {
        if (zone[i] == zone_num || flip[i] == flip_num) {
            node->box_num = i;
            node++;
            creature->lot.zone_count++;
        }
    }
}

void LOT_InitialiseLOT(LOT_INFO *LOT)
{
    LOT->node =
        GameBuf_Alloc(sizeof(BOX_NODE) * g_NumberBoxes, GBUF_CREATURE_LOT);
    LOT_ClearLOT(LOT);
}

void LOT_ClearLOT(LOT_INFO *LOT)
{
    LOT->search_num = 0;
    LOT->head = NO_BOX;
    LOT->tail = NO_BOX;
    LOT->target_box = NO_BOX;
    LOT->required_box = NO_BOX;

    for (int i = 0; i < g_NumberBoxes; i++) {
        BOX_NODE *node = &LOT->node[i];
        node->search_num = 0;
        node->exit_box = NO_BOX;
        node->next_expansion = NO_BOX;
    }
}
