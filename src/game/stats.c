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

typedef struct STAT_INFO {
    int32_t drops;
    int32_t kills;
    int16_t cur_idx;
    int16_t idxs[STAT_SIZE];
} STAT_INFO;

static STAT_INFO m_Pierres = { 0 };
static STAT_INFO m_Mummies = { 0 };
static STAT_INFO m_Pods = { 0 };
static STAT_INFO m_Bats = { 0 };
static STAT_INFO m_Statues = { 0 };

int16_t m_PickupObjs[] = { O_PICKUP_ITEM1,   O_PICKUP_ITEM2,  O_KEY_ITEM1,
                           O_KEY_ITEM2,      O_KEY_ITEM3,     O_KEY_ITEM4,
                           O_PUZZLE_ITEM1,   O_PUZZLE_ITEM2,  O_PUZZLE_ITEM3,
                           O_PUZZLE_ITEM4,   O_GUN_ITEM,      O_SHOTGUN_ITEM,
                           O_MAGNUM_ITEM,    O_UZI_ITEM,      O_GUN_AMMO_ITEM,
                           O_SG_AMMO_ITEM,   O_MAG_AMMO_ITEM, O_UZI_AMMO_ITEM,
                           O_EXPLOSIVE_ITEM, O_MEDI_ITEM,     O_BIGMEDI_ITEM,
                           O_SCION_ITEM,     O_SCION_ITEM2,   O_LEADBAR_ITEM,
                           NO_ITEM };

// A few enemies removed because they need trigger checks
int16_t m_KillableObjs[] = {
    O_WOLF,     O_BEAR,     O_CROCODILE,    O_ALLIGATOR, O_LION,
    O_LIONESS,  O_PUMA,     O_APE,          O_RAT,       O_VOLE,
    O_DINOSAUR, O_RAPTOR,   O_WARRIOR1,     O_WARRIOR2,  O_WARRIOR3,
    O_CENTAUR,  O_ABORTION, O_DINO_WARRIOR, O_LARSON,    O_SKATEBOARD,
    O_SKATEKID, O_COWBOY,   O_BALDY,        O_NATLA,     O_SCION_ITEM3,
    NO_ITEM
};

static bool Stats_FindElement(int16_t elem, int16_t array[], int16_t size);
static void Stats_CheckTriggers(FLOOR_INFO **floor_array);

static bool Stats_FindElement(int16_t elem, int16_t array[], int16_t size)
{
    for (int16_t i = 0; i < size; i++) {
        if (array[i] == elem) {
            return true;
        }
    }
    return false;
}

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
                                    && !Stats_FindElement(
                                        idx, m_Pierres.idxs, STAT_SIZE)) {
                                    m_LevelPickups += PIERRE_ITEMS;
                                    m_LevelKillables += 1;
                                    m_Pierres.idxs[m_Pierres.cur_idx++] = idx;
                                }

                                // Check for mummy triggers
                                if (item->object_number == O_MUMMY
                                    && !Stats_FindElement(
                                        idx, m_Mummies.idxs, STAT_SIZE)) {
                                    m_LevelKillables += 1;
                                    m_Mummies.idxs[m_Mummies.cur_idx++] = idx;
                                }

                                // Check for mutant triggers
                                if ((item->object_number == O_PODS
                                     || item->object_number == O_BIG_POD)
                                    && !Stats_FindElement(
                                        idx, m_Pods.idxs, STAT_SIZE)) {
                                    m_LevelKillables += 1;
                                    m_Pods.idxs[m_Pods.cur_idx++] = idx;
                                }

                                // Check for bat triggers
                                if ((item->object_number == O_BAT)
                                    && !Stats_FindElement(
                                        idx, m_Bats.idxs, STAT_SIZE)) {
                                    m_LevelKillables += 1;
                                    m_Bats.idxs[m_Bats.cur_idx++] = idx;
                                }

                                // Check for statue triggers
                                if ((item->object_number == O_STATUE)
                                    && !Stats_FindElement(
                                        idx, m_Statues.idxs, STAT_SIZE)) {
                                    m_LevelKillables += 1;
                                    m_Statues.idxs[m_Statues.cur_idx++] = idx;
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

void Stats_CalculateStats(int32_t uninit_item_count, FLOOR_INFO **floor_array)
{
    // Clear old values
    m_LevelPickups = 0;
    m_LevelKillables = 0;
    m_LevelSecrets = 0;
    memset(&m_Pierres, 0, sizeof(m_Pierres));
    memset(&m_Mummies, 0, sizeof(m_Mummies));
    memset(&m_Pods, 0, sizeof(m_Pods));
    memset(&m_Bats, 0, sizeof(m_Bats));
    memset(&m_Statues, 0, sizeof(m_Statues));

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
            // Spawn pickups on death
            if (item->object_number == O_SKATEKID) {
                m_LevelPickups += SKATEKID_ITEMS;
            } else if (item->object_number == O_COWBOY) {
                m_LevelPickups += COWBOY_ITEMS;
            } else if (item->object_number == O_BALDY) {
                m_LevelPickups += BALDY_ITEMS;
            }

            // Calculate number of killable objects in a level
            for (int j = 0; m_KillableObjs[j] != NO_ITEM; j++) {
                if (item->object_number == m_KillableObjs[j]) {
                    m_LevelKillables++;
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