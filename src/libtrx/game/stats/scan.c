#include "benchmark.h"
#include "debug.h"
#include "game/carrier.h"
#include "game/game_buf.h"
#include "game/inject.h"
#include "game/items.h"
#include "game/level.h"
#include "game/objects.h"
#include "game/rooms.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/stats.h"
#include "log.h"
#include "memory.h"
#include "version.h"

static bool m_KillableItems[MAX_ITEMS] = {};

static void M_IncludeKillableItem(
    LEVEL_MAX_STATS *const stats, int16_t item_num)
{
    m_KillableItems[item_num] = true;
    stats->max_kill_count++;
    const ITEM *const item = Item_Get(item_num);
    LOG_TRACE(
        "Killable item %d: object = %s", item_num,
        Object_GetName(item->object_id));
    if (Carrier_GetItemCount(item_num) > 0) {
        LOG_TRACE(
            "+%d pickups from carrier %d", Carrier_GetItemCount(item_num),
            item_num);
    }
    stats->max_pickup_count += Carrier_GetItemCount(item_num);
}

static bool M_HasObjectSecrets(void)
{
    for (int32_t i = 0; g_SecretObjects[i] != NO_OBJECT; i++) {
        if (Object_Get(g_SecretObjects[i])->loaded) {
            return true;
        }
    }
    return false;
}

static uint32_t M_ReserveSecretBit(
    LEVEL_MAX_STATS *const stats, const OBJECT_ID object_id)
{
    uint32_t n = stats->all_secrets_mask;
    // Find unused bit
    int32_t position = 0;
    while ((n & 1) == 1) {
        n >>= 1;
        position++;
    }
    LOG_TRACE("Reserving bit %d for secret %d", position, object_id);
    stats->all_secrets_mask |= 1 << position;
    stats->max_secret_count++;
    stats->secret_objects[position].assigned_object_id = object_id;
    stats->secret_objects[position].item_num = NO_ITEM;
    return 1 << position;
}

static void M_CheckTriggers(
    LEVEL_MAX_STATS *const stats, const ROOM *const room,
    const int32_t room_num, const int32_t z_sector, const int32_t x_sector)
{
    if (z_sector == 0 || z_sector == room->size.z - 1) {
        if (x_sector == 0 || x_sector == room->size.x - 1) {
            return;
        }
    }
    const SECTOR *const sector = Room_GetUnitSector(room, x_sector, z_sector);

    if (sector->trigger == nullptr) {
        return;
    }

    const TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type == TO_SECRET) {
            if (M_HasObjectSecrets()) {
                // At the moment, we can't mix tile-based and pickup-based
                // secrets – OG TR2 has both in Bartoli Hideout and a couple of
                // other levels.
                continue;
            }
            const int16_t secret_num = 1 << (int16_t)(intptr_t)cmd->parameter;
            if (!(stats->all_secrets_mask & secret_num)) {
                stats->all_secrets_mask |= secret_num;
                stats->max_secret_count++;
            }
        } else if (cmd->type == TO_OBJECT) {
            const int16_t item_num = (int16_t)(intptr_t)cmd->parameter;
            if (m_KillableItems[item_num]) {
                continue;
            }

            const ITEM *const item = Item_Get(item_num);
            switch (item->object_id) {
            case O_PIERRE:
                // Add Pierre pickup and kills if oneshot
                if (sector->trigger->one_shot) {
                    M_IncludeKillableItem(stats, item_num);
                }
                break;

            case O_PODS:
            case O_BIG_POD:
                // Check for only valid pods
                if (item->data != nullptr) {
                    const int16_t bug_item_num = (intptr_t)item->data;
                    const ITEM *const bug_item = Item_Get(bug_item_num);
                    if (Object_Get(bug_item->object_id)->loaded) {
                        M_IncludeKillableItem(stats, item_num);
                    }
                }
                break;

            case O_BARTOLI:
            case O_DRAGON_BACK:
            case O_DRAGON_FRONT:
                if (Object_Get(O_DRAGON_BACK)->loaded
                    && Object_Get(O_DRAGON_FRONT)->loaded) {
                    M_IncludeKillableItem(stats, item_num);
                    if (Object_Get(O_PUZZLE_ITEM_2)->loaded) {
                        LOG_TRACE("+1 pickup from dragon");
                        stats->max_pickup_count++;
                    }
                }
                break;

            case O_EEL:
            case O_BIG_EEL:
                break;

            case O_SCION_ITEM_3:
                M_IncludeKillableItem(stats, item_num);
                break;

            default:
                // Add killable if object triggered
                if (Object_IsType(item->object_id, g_EnemyObjects)) {
                    M_IncludeKillableItem(stats, item_num);
                }
                break;
            }
        }
    }
}

static void M_TraverseFloor(LEVEL_MAX_STATS *const stats)
{
    uint32_t secrets = 0;
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        for (int32_t z_sector = 0; z_sector < room->size.z; z_sector++) {
            for (int32_t x_sector = 0; x_sector < room->size.x; x_sector++) {
                M_CheckTriggers(stats, room, i, z_sector, x_sector);
            }
        }
    }
}

static void M_CalculateStats(LEVEL_MAX_STATS *const stats)
{
    memset(stats, 0, sizeof(*stats));
    for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
        stats->secret_objects[i].assigned_object_id = NO_OBJECT;
        stats->secret_objects[i].item_num = NO_ITEM;
    }

    memset(&m_KillableItems, 0, sizeof(m_KillableItems));

    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        const ITEM *const item = Item_Get(i);
        if (Object_IsType(item->object_id, g_PickupObjects)
            && !Carrier_IsItemCarried(i)) {
            LOG_TRACE(
                "+1 pickup from pickup item %d in room %d", i, item->room_num);
            stats->max_pickup_count++;
        }
    }

    // Check triggers for special pickups / killables
    M_TraverseFloor(stats);

    int32_t secret_count = 0;
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->object_id < O_FIRST || item->object_id >= O_NUMBER_OF) {
            LOG_ERROR("Bad Object number (%d) on Item %d", item->object_id, i);
            continue;
        }

        if (item->object_id == O_COMBAT_END) {
            M_IncludeKillableItem(stats, i);
        }

        if (Object_IsType(item->object_id, g_SecretObjects)) {
            if (secret_count >= STATS_MAX_SECRETS) {
                LOG_ERROR("Too many secrets, max %d", STATS_MAX_SECRETS);
                break;
            }
            stats->secret_objects[secret_count].assigned_object_id =
                item->object_id;
            stats->secret_objects[secret_count].item_num = i;
            secret_count++;
        }
    }

    for (int32_t i = 0; i < secret_count; i++) {
        for (int32_t j = i + 1; j < secret_count; j++) {
            if (stats->secret_objects[i].assigned_object_id
                > stats->secret_objects[j].assigned_object_id) {
                SWAP(stats->secret_objects[i], stats->secret_objects[j]);
            }
        }
    }

    for (int32_t k = 0; k < secret_count; k++) {
        ITEM *const item = Item_Get(stats->secret_objects[k].item_num);
        item->data = (void *)(intptr_t)M_ReserveSecretBit(
            stats, stats->secret_objects[k].assigned_object_id);
    }
}

void Stats_ScanLevel(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    ASSERT(level != nullptr);
    BENCHMARK benchmark = Benchmark_Start();
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    if (resume == nullptr) {
        return;
    }
    M_CalculateStats(&resume->max_stats);
    resume->max_stats.max_pickup_count += GF_GetSecretRewardCount(level);
    resume->max_stats.max_pickup_count -= level->unobtainable.pickups;
    resume->max_stats.max_kill_count -= level->unobtainable.kills;
    resume->max_stats.max_secret_count -= level->unobtainable.secrets;
    LOG_INFO("Scanned level %s:", level->title);
    LOG_INFO("  Max secrets = %d", resume->max_stats.max_secret_count);
    LOG_INFO("  Max pickups = %d", resume->max_stats.max_pickup_count);
    LOG_INFO("  Max kills = %d", resume->max_stats.max_kill_count);
    Benchmark_End(&benchmark, nullptr);
}

const LEVEL_MAX_STATS *Stats_GetLevelMaxStats(const GF_LEVEL *const level)
{
    // fetch precomputed max stats from savegame resume info
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    if (resume == nullptr) {
        return nullptr;
    }
    return &resume->max_stats;
}
