#include "game/stats.h"

#include "game/carrier.h"
#include "game/clock.h"
#include "game/gamebuf.h"
#include "game/gameflow.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/log.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_TEXTSTRINGS 10

static int32_t m_CachedItemCount = 0;
static SECTOR **m_CachedSectorArray = NULL;
static int32_t m_LevelPickups = 0;
static int32_t m_LevelKillables = 0;
static int32_t m_LevelSecrets = 0;
static uint32_t m_SecretRoom = 0;
static bool m_KillableItems[MAX_ITEMS] = { 0 };
static bool m_IfKillable[O_NUMBER_OF] = { 0 };

static struct {
    double start_counter;
    int32_t start_timer;
} m_StatsTimer = { 0 };

static void M_TraverseFloor(void);
static void M_CheckTriggers(ROOM *r, int room_num, int z_sector, int x_sector);
static void M_IncludeKillableItem(int16_t item_num);

static void M_TraverseFloor(void)
{
    uint32_t secrets = 0;

    for (int i = 0; i < g_RoomCount; i++) {
        ROOM *r = &g_RoomInfo[i];
        for (int z_sector = 0; z_sector < r->size.z; z_sector++) {
            for (int x_sector = 0; x_sector < r->size.x; x_sector++) {
                M_CheckTriggers(r, i, z_sector, x_sector);
            }
        }
    }
}

static void M_CheckTriggers(ROOM *r, int room_num, int z_sector, int x_sector)
{
    if (z_sector == 0 || z_sector == r->size.z - 1) {
        if (x_sector == 0 || x_sector == r->size.x - 1) {
            return;
        }
    }

    const SECTOR *const sector =
        &m_CachedSectorArray[room_num][z_sector + x_sector * r->size.z];

    if (sector->trigger == NULL) {
        return;
    }

    for (int32_t i = 0; i < sector->trigger->command_count; i++) {
        const TRIGGER_CMD *const cmd = &sector->trigger->commands[i];

        if (cmd->type == TO_SECRET) {
            const int16_t secret_num = 1 << (int16_t)(intptr_t)cmd->parameter;
            if (!(m_SecretRoom & secret_num)) {
                m_SecretRoom |= secret_num;
                m_LevelSecrets++;
            }
        } else if (cmd->type == TO_OBJECT) {
            const int16_t item_num = (int16_t)(intptr_t)cmd->parameter;
            if (m_KillableItems[item_num]) {
                continue;
            }

            const ITEM *const item = &g_Items[item_num];

            if (item->object_id == O_PIERRE) {
                // Add Pierre pickup and kills if oneshot
                if (sector->trigger->one_shot) {
                    M_IncludeKillableItem(item_num);
                }
                continue;
            }

            if (item->object_id == O_PODS || item->object_id == O_BIG_POD) {
                // Check for only valid pods
                if (item->data != NULL) {
                    const int16_t bug_item_num = *(int16_t *)item->data;
                    const ITEM *const bug_item = &g_Items[bug_item_num];
                    if (g_Objects[bug_item->object_id].loaded) {
                        M_IncludeKillableItem(item_num);
                    }
                }
                continue;
            }

            // Add killable if object triggered
            if (Object_IsObjectType(item->object_id, g_EnemyObjects)) {
                M_IncludeKillableItem(item_num);
            }
        }
    }
}

static void M_IncludeKillableItem(int16_t item_num)
{
    m_KillableItems[item_num] = true;
    m_LevelKillables += 1;
    m_LevelPickups += Carrier_GetItemCount(item_num);
}

void Stats_ComputeTotal(
    GAMEFLOW_LEVEL_TYPE level_type, TOTAL_STATS *total_stats)
{
    memset(total_stats, 0, sizeof(TOTAL_STATS));

    int16_t secret_flags = 0;

    for (int i = 0; i < g_GameFlow.level_count; i++) {
        if (g_GameFlow.levels[i].level_type != level_type) {
            continue;
        }
        const GAME_STATS *level_stats = &g_GameInfo.current[i].stats;

        total_stats->player_kill_count += level_stats->kill_count;
        total_stats->player_pickup_count += level_stats->pickup_count;
        secret_flags = level_stats->secret_flags;
        for (int j = 0; j < MAX_SECRETS; j++) {
            if (secret_flags & 1) {
                total_stats->player_secret_count++;
            }
            secret_flags >>= 1;
        }

        total_stats->timer += level_stats->timer;
        total_stats->death_count += level_stats->death_count;
        total_stats->total_kill_count += level_stats->max_kill_count;
        total_stats->total_secret_count += level_stats->max_secret_count;
        total_stats->total_pickup_count += level_stats->max_pickup_count;
    }
}

void Stats_ObserveRoomsLoad(void)
{
    m_CachedSectorArray =
        GameBuf_Alloc(g_RoomCount * sizeof(SECTOR *), GBUF_ROOM_SECTOR);
    for (int i = 0; i < g_RoomCount; i++) {
        const ROOM *current_room_info = &g_RoomInfo[i];
        const int32_t count =
            current_room_info->size.x * current_room_info->size.z;
        m_CachedSectorArray[i] =
            GameBuf_Alloc(count * sizeof(SECTOR), GBUF_ROOM_SECTOR);
        memcpy(
            m_CachedSectorArray[i], current_room_info->sectors,
            count * sizeof(SECTOR));
    }
}

void Stats_ObserveItemsLoad(void)
{
    m_CachedItemCount = g_LevelItemCount;
}

void Stats_CalculateStats(void)
{
    m_LevelPickups = 0;
    m_LevelKillables = 0;
    m_LevelSecrets = 0;
    m_SecretRoom = 0;
    memset(&m_KillableItems, 0, sizeof(m_KillableItems));

    if (m_CachedItemCount) {
        if (m_CachedItemCount > MAX_ITEMS) {
            LOG_ERROR("Too Many g_Items being Loaded!!");
            return;
        }

        for (int i = 0; i < m_CachedItemCount; i++) {
            ITEM *item = &g_Items[i];

            if (item->object_id < 0 || item->object_id >= O_NUMBER_OF) {
                LOG_ERROR(
                    "Bad Object number (%d) on Item %d", item->object_id, i);
                continue;
            }

            if (Object_IsObjectType(item->object_id, g_PickupObjects)) {
                m_LevelPickups++;
            }
        }
    }

    // Check triggers for special pickups / killables
    M_TraverseFloor();

    m_LevelPickups -= g_GameFlow.levels[g_CurrentLevel].unobtainable.pickups;
    m_LevelKillables -= g_GameFlow.levels[g_CurrentLevel].unobtainable.kills;
    m_LevelSecrets -= g_GameFlow.levels[g_CurrentLevel].unobtainable.secrets;
}

int32_t Stats_GetPickups(void)
{
    return m_LevelPickups;
}

int32_t Stats_GetKillables(void)
{
    return m_LevelKillables;
}

int32_t Stats_GetSecrets(void)
{
    return m_LevelSecrets;
}

bool Stats_CheckAllSecretsCollected(GAMEFLOW_LEVEL_TYPE level_type)
{
    TOTAL_STATS total_stats = { 0 };
    Stats_ComputeTotal(level_type, &total_stats);
    return total_stats.player_secret_count >= total_stats.total_secret_count;
}

void Stats_StartTimer(void)
{
    m_StatsTimer.start_counter = Clock_GetHighPrecisionCounter();
    m_StatsTimer.start_timer = g_GameInfo.current[g_CurrentLevel].stats.timer;
}

void Stats_UpdateTimer(void)
{
    const double elapsed =
        (Clock_GetHighPrecisionCounter() - m_StatsTimer.start_counter)
        * LOGIC_FPS / 1000.0;
    g_GameInfo.current[g_CurrentLevel].stats.timer =
        m_StatsTimer.start_timer + elapsed;
}
