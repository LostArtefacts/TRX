#include "game/stats.h"

#include "game/shell.h"
#include "global/vars.h"
#include "log.h"

#include <stdio.h>
#include <string.h>

#define STAT_SIZE 512
#define PIERRE_ITEMS 3
#define SKATEKID_ITEMS 1
#define COWBOY_ITEMS 1
#define BALDY_ITEMS 1

static int32_t m_LevelPickups = 0;
static int32_t m_LevelKillables = 0;
static int32_t m_LevelSecrets = 0;
static bool m_KillableItems[MAX_ITEMS] = { 0 };
static bool m_IfKillable[O_NUMBER_OF] = { 0 };

int16_t m_PickupObjs[] = { O_PICKUP_ITEM1,   O_PICKUP_ITEM2,  O_KEY_ITEM1,
                           O_KEY_ITEM2,      O_KEY_ITEM3,     O_KEY_ITEM4,
                           O_PUZZLE_ITEM1,   O_PUZZLE_ITEM2,  O_PUZZLE_ITEM3,
                           O_PUZZLE_ITEM4,   O_GUN_ITEM,      O_SHOTGUN_ITEM,
                           O_MAGNUM_ITEM,    O_UZI_ITEM,      O_GUN_AMMO_ITEM,
                           O_SG_AMMO_ITEM,   O_MAG_AMMO_ITEM, O_UZI_AMMO_ITEM,
                           O_EXPLOSIVE_ITEM, O_MEDI_ITEM,     O_BIGMEDI_ITEM,
                           O_SCION_ITEM,     O_SCION_ITEM2,   O_LEADBAR_ITEM,
                           NO_ITEM };

// Pierre has special trigger check
int16_t m_KillableObjs[] = {
    O_WOLF,     O_BEAR,        O_BAT,      O_CROCODILE, O_ALLIGATOR,
    O_LION,     O_LIONESS,     O_PUMA,     O_APE,       O_RAT,
    O_VOLE,     O_DINOSAUR,    O_RAPTOR,   O_WARRIOR1,  O_WARRIOR2,
    O_WARRIOR3, O_CENTAUR,     O_MUMMY,    O_ABORTION,  O_DINO_WARRIOR,
    O_FISH,     O_LARSON,      O_SKATEKID, O_COWBOY,    O_BALDY,
    O_NATLA,    O_SCION_ITEM3, O_STATUE,   O_PODS,      O_BIG_POD,
    NO_ITEM
};

static void Stats_CheckTriggers(FLOOR_INFO **floor_array);

static void Stats_CheckTriggers(FLOOR_INFO **floor_array)
{
    uint32_t secrets = 0;

    for (int i = 0; i < g_RoomCount; i++) {
        ROOM_INFO *r = &g_RoomInfo[i];
        for (int x_floor = 0; x_floor < r->x_size; x_floor++) {
            for (int y_floor = 0; y_floor < r->y_size; y_floor++) {

                if (x_floor == 0 || x_floor == r->x_size - 1) {
                    if (y_floor == 0 || y_floor == r->y_size - 1) {
                        continue;
                    }
                }

                FLOOR_INFO *floor =
                    &floor_array[i][x_floor + y_floor * r->x_size];

                if (!floor->index) {
                    continue;
                }

                int16_t *data = &g_FloorData[floor->index];
                int16_t type;
                int16_t trigger;
                int16_t trig_flags;
                int16_t trig_type;
                do {
                    type = *data++;

                    switch (type & DATA_TYPE) {
                    case FT_TILT:
                    case FT_ROOF:
                    case FT_DOOR:
                        data++;
                        break;

                    case FT_LAVA:
                        break;

                    case FT_TRIGGER:
                        trig_flags = *data;
                        data++;
                        trig_type = (type >> 8) & 0x3F;
                        do {
                            trigger = *data++;
                            if (TRIG_BITS(trigger) == TO_SECRET) {
                                int16_t number = trigger & VALUE_BITS;
                                if (!(secrets & (1 << number))) {
                                    secrets |= (1 << number);
                                    m_LevelSecrets++;
                                }
                            }
                            if (TRIG_BITS(trigger) != TO_OBJECT) {
                                if (TRIG_BITS(trigger) == TO_CAMERA) {
                                    trigger = *data++;
                                }
                            } else {
                                int16_t idx = trigger & VALUE_BITS;
                                ITEM_INFO *item = &g_Items[idx];

                                // Add Pierre pickup and kills if oneshot
                                if (item->object_number == O_PIERRE
                                    && trig_flags & IF_ONESHOT
                                    && !m_KillableItems[idx]) {
                                    m_KillableItems[idx] = true;
                                    m_LevelPickups += PIERRE_ITEMS;
                                    m_LevelKillables += 1;
                                }

                                // Add killable if object triggered
                                if (m_IfKillable[item->object_number]
                                    && !m_KillableItems[idx]) {
                                    m_KillableItems[idx] = true;
                                    m_LevelKillables += 1;

                                    // Add mercenary pickups
                                    if (item->object_number == O_SKATEKID) {
                                        m_LevelPickups += SKATEKID_ITEMS;
                                    }
                                    if (item->object_number == O_COWBOY) {
                                        m_LevelPickups += COWBOY_ITEMS;
                                    }
                                    if (item->object_number == O_BALDY) {
                                        m_LevelPickups += BALDY_ITEMS;
                                    }
                                }
                            }
                        } while (!(trigger & END_BIT));
                        break;
                    }
                } while (!(type & END_BIT));
            }
        }
    }
}

void Stats_Init()
{
    for (int i = 0; m_KillableObjs[i] != NO_ITEM; i++) {
        m_IfKillable[m_KillableObjs[i]] = true;
    }
}

void Stats_CalculateStats(int32_t uninit_item_count, FLOOR_INFO **floor_array)
{
    // Clear old values
    m_LevelPickups = 0;
    m_LevelKillables = 0;
    m_LevelSecrets = 0;
    memset(&m_KillableItems, 0, sizeof(m_KillableItems));

    if (uninit_item_count) {
        if (uninit_item_count > MAX_ITEMS) {
            Shell_ExitSystem(
                "Stats_GetPickupCount(): Too Many g_Items being Loaded!!");
            return;
        }

        for (int i = 0; i < uninit_item_count; i++) {
            ITEM_INFO *item = &g_Items[i];

            if (item->object_number < 0 || item->object_number >= O_NUMBER_OF) {
                Shell_ExitSystemFmt(
                    "Stats_GetPickupCount(): Bad Object number (%d) on Item %d",
                    item->object_number, i);
            }

            // Calculate number of pickups in a level
            for (int j = 0; m_PickupObjs[j] != NO_ITEM; j++) {
                if (item->object_number == m_PickupObjs[j]) {
                    m_LevelPickups++;
                }
            }
        }
    }

    // Check triggers for special pickups / killables
    Stats_CheckTriggers(floor_array);
}

int32_t Stats_GetPickups()
{
    return m_LevelPickups;
}

int32_t Stats_GetKillables()
{
    return m_LevelKillables;
}

int32_t Stats_GetSecrets()
{
    return m_LevelSecrets;
}