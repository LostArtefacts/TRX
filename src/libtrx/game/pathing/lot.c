#include "game/pathing/lot.h"

#include "debug.h"
#include "game/camera.h"
#include "game/game_buf.h"
#include "game/pathing.h"
#include "game/rooms.h"
#include "utils.h"

static int32_t m_SlotsUsed = 0;
static CREATURE *m_BaddieSlots = nullptr;

void LOT_InitialiseArray(void)
{
    m_BaddieSlots =
        GameBuf_Alloc(LOT_SLOT_COUNT * sizeof(CREATURE), GBUF_CREATURE_DATA);

    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        CREATURE *const creature = &m_BaddieSlots[i];
        creature->item_num = NO_ITEM;
        creature->lot.node =
            GameBuf_Alloc(Box_GetCount() * sizeof(BOX_NODE), GBUF_CREATURE_LOT);
    }

    m_SlotsUsed = 0;
}

CREATURE *LOT_GetBaddieSlot(const int32_t i)
{
    return &m_BaddieSlots[i];
}

void LOT_DisableBaddieAI(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = (CREATURE *)item->data;
    item->data = nullptr;

    if (creature != nullptr) {
        creature->item_num = NO_ITEM;
        m_SlotsUsed--;
    }
}

bool LOT_EnableBaddieAI(const int16_t item_num, const bool always)
{
    if (Item_Get(item_num)->data != nullptr) {
        return true;
    }

    if (m_SlotsUsed < LOT_SLOT_COUNT) {
        for (int32_t slot = 0; slot < LOT_SLOT_COUNT; slot++) {
            if (m_BaddieSlots[slot].item_num == NO_ITEM) {
                LOT_InitialiseSlot(item_num, slot);
                return true;
            }
        }
        ASSERT_FAIL();
    }

    int32_t worst_dist = 0;
    if (!always) {
        const ITEM *const item = Item_Get(item_num);
        const int32_t dx = (item->pos.x - g_Camera.pos.pos.x) >> 8;
        const int32_t dy = (item->pos.y - g_Camera.pos.pos.y) >> 8;
        const int32_t dz = (item->pos.z - g_Camera.pos.pos.z) >> 8;
        worst_dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
    }

    int32_t worst_slot = -1;
    for (int32_t slot = 0; slot < LOT_SLOT_COUNT; slot++) {
        const int32_t item_num = m_BaddieSlots[slot].item_num;
        const ITEM *const item = Item_Get(item_num);
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

    const CREATURE *const creature = &m_BaddieSlots[worst_slot];
    Item_Get(creature->item_num)->status = IS_INVISIBLE;
    LOT_DisableBaddieAI(creature->item_num);
    LOT_InitialiseSlot(item_num, worst_slot);
    return true;
}

void LOT_InitialiseSlot(const int16_t item_num, const int32_t slot)
{
    CREATURE *const creature = &m_BaddieSlots[slot];
    ITEM *const item = Item_Get(item_num);
    item->data = creature;

    creature->item_num = item_num;
    creature->mood = MOOD_BORED;
    creature->neck_rotation = 0;
    creature->head_rotation = 0;
    creature->maximum_turn = DEG_1;
    creature->flags = 0;
    creature->enemy = nullptr;
    creature->lot.setup.step = STEP_L;
#if TR_VERSION == 1
    creature->lot.setup.drop = -STEP_L;
#else
    creature->lot.setup.drop = -STEP_L * 2;
#endif
    creature->lot.setup.block_mask = BOX_BLOCKED;
    creature->lot.setup.fly = 0;

    switch (item->object_id) {
#if TR_VERSION == 1
    case O_BAT:
    case O_ALLIGATOR:
    case O_FISH:
        creature->lot.setup.step = WALL_L * 20;
        creature->lot.setup.drop = -WALL_L * 20;
        creature->lot.setup.fly = STEP_L / 16;
        break;

    case O_TREX:
    case O_WARRIOR_1:
    case O_CENTAUR:
        creature->lot.setup.block_mask = BOX_BLOCKABLE;
        break;

    case O_WOLF:
    case O_LION:
    case O_LIONESS:
    case O_PUMA:
        creature->lot.setup.drop = -WALL_L;
        break;

    case O_APE:
        creature->lot.setup.step = WALL_L / 2;
        creature->lot.setup.drop = -WALL_L;
        break;
#else
    case O_SHARK:
    case O_BARRACUDA:
    case O_DIVER:
    case O_JELLY:
    case O_CROW:
    case O_EAGLE:
        creature->lot.setup.step = WALL_L * 20;
        creature->lot.setup.drop = -WALL_L * 20;
        creature->lot.setup.fly = STEP_L / 16;
        if (item->object_id == O_SHARK) {
            creature->lot.setup.block_mask = BOX_BLOCKABLE;
        }
        break;

    case O_WORKER_3:
    case O_WORKER_4:
    case O_YETI:
        creature->lot.setup.step = WALL_L;
        creature->lot.setup.drop = -WALL_L;
        break;

    case O_SPIDER:
    case O_SKIDOO_ARMED:
        creature->lot.setup.step = WALL_L / 2;
        creature->lot.setup.drop = -WALL_L;
        break;

    case O_DINO:
        creature->lot.setup.block_mask = BOX_BLOCKABLE;
        break;
#endif
    default:
        break;
    }

    LOT_ClearLOT(&creature->lot);
    LOT_CreateZone(item);

    m_SlotsUsed++;
}

void LOT_CreateZone(ITEM *const item)
{
    CREATURE *const creature = item->data;

    const int16_t *zone;
    const int16_t *flip;
    if (creature->lot.setup.fly) {
        zone = Box_GetFlyZone(false);
        flip = Box_GetFlyZone(true);
    } else {
        zone = Box_GetGroundZone(false, BOX_ZONE(creature->lot.setup.step));
        flip = Box_GetGroundZone(true, BOX_ZONE(creature->lot.setup.step));
    }

    const ROOM *const room = Room_Get(item->room_num);
    item->box_num = Room_GetWorldSector(room, item->pos.x, item->pos.z)->box;

    int16_t zone_num = zone[item->box_num];
    int16_t flip_num = flip[item->box_num];

    creature->lot.zone_count = 0;
    BOX_NODE *node = creature->lot.node;
    for (int32_t i = 0; i < Box_GetCount(); i++) {
        if (zone[i] == zone_num || flip[i] == flip_num) {
            node->box_num = i;
            node++;
            creature->lot.zone_count++;
        }
    }
}

void LOT_InitialiseLOT(LOT_INFO *const lot)
{
    lot->node =
        GameBuf_Alloc(sizeof(BOX_NODE) * Box_GetCount(), GBUF_CREATURE_LOT);
    LOT_ClearLOT(lot);
}

void LOT_ClearLOT(LOT_INFO *const lot)
{
    lot->search_num = 0;
    lot->head = NO_BOX;
    lot->tail = NO_BOX;
    lot->target_box = NO_BOX;
    lot->required_box = NO_BOX;

    for (int32_t i = 0; i < Box_GetCount(); i++) {
        BOX_NODE *const node = &lot->node[i];
        node->next_expansion = NO_BOX;
        node->exit_box = NO_BOX;
        node->search_num = 0;
    }
}
