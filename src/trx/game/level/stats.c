#include <trx/game/level/stats.h>

#include <trx/benchmark.h>
#include <trx/game/carrier.h>
#include <trx/game/creature.h>
#include <trx/game/game_buf.h>
#include <trx/game/game_flow.h>
#include <trx/game/inject.h>
#include <trx/game/level/reader.h>
#include <trx/game/lua.h>
#include <trx/game/rooms.h>
#include <trx/game/stats.h>
#include <trx/log.h>
#include <trx/virtual_file.h>

void Level_CalculateMaxStats(void)
{
    BENCHMARK benchmark = Benchmark_Start();
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        if (level->type != GFL_NORMAL && level->type != GFL_BONUS) {
            continue;
        }

        VFILE *const file = VFile_CreateFromPath(level->path);
        if (file == nullptr) {
            continue;
        }

        const LEVEL_LOADER *const loader = Level_GuessLoader(file);
        if (loader != nullptr) {
            Creature_SetAlliesHostile(false);

            Lua_ClearLevelListeners();
            Lua_SetScriptContext(LUA_CONTEXT_LEVEL);
            if (level->script_path != nullptr) {
                LUA_RESULT res = Lua_EvalFile(level->script_path);
                if (res.code != LUA_OK) {
                    LOG_ERROR("Lua level script error: %s", res.message);
                }
                Lua_FreeResult(&res);
            }
            Lua_SetScriptContext(LUA_CONTEXT_GLOBAL);
            Lua_FireEvent(LUA_EVENT_LEVEL_INIT, level->num);

            Inject_InitLevel(level, INJECTION_MODE_STATS);
            if (!loader->probe(loader, file, LEVEL_PROBE_STATS)) {
                continue;
            }
            Inject_AllInjections();
            Inject_Cleanup();

            for (int32_t item_num = 0; item_num < Item_GetTotalCount();
                 item_num++) {
                ITEM *const item = Item_Get(item_num);
                ROOM *const room = Room_Get(item->room_num);
                item->next_item = room->item_num;
                room->item_num = item_num;
            }
            Carrier_InitialiseLevel(level);

            Stats_ScanLevel(level);
        }

        GameBuf_Reset();
        VFile_Close(file);

#if 0
        const LEVEL_MAX_STATS *const max_stats = Stats_GetLevelMaxStats(level);
        LOG_INFO(
            "Level %d (%s)", GF_GetLevelOrdinalNumber(GFLT_MAIN, level),
            level->title);
        LOG_INFO("    pickups: %d", max_stats->max_pickup_count);
        LOG_INFO("    kills:   %d", max_stats->max_kill_count);
        LOG_INFO("    secrets: %d", max_stats->max_secret_count);
#endif
    }

    const FINAL_STATS final_stats = Stats_ComputeFinalStats(true);
    LOG_INFO("Max pickups: %d", final_stats.max_stats.max_pickup_count);
    LOG_INFO("Max kills:   %d", final_stats.max_stats.max_kill_count);
    LOG_INFO("Max secrets: %d", final_stats.max_stats.max_secret_count);
    Benchmark_End(&benchmark, nullptr);
}
