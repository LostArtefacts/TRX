#include <trx/game/game_flow/common.h>

#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/game_flow/vars.h>
#include <trx/game/level/cache.h>

static const GF_LEVEL *m_CurrentLevel = nullptr;
static GF_COMMAND m_OverrideCommand = { .action = GF_NOOP };

static bool M_SkipLevel(const GF_LEVEL *const level)
{
    return level->type == GFL_DUMMY || level->type == GFL_CURRENT;
}

static void M_FreeSequence(GF_SEQUENCE *const sequence)
{
    Memory_Free(sequence->events);
}

static void M_FreeInjections(INJECTION_DATA *const injections)
{
    if (injections->data_paths != nullptr) {
        for (int32_t i = 0; i < injections->count; i++) {
            Memory_FreePointer(&injections->data_paths[i]);
        }
    }
    Memory_FreePointer(&injections->data_paths);
    injections->count = 0;
}

static void M_FreeLevel(GF_LEVEL *const level)
{
    Memory_FreePointer(&level->path);
    Memory_FreePointer(&level->key);
    Memory_FreePointer(&level->title);
    Memory_FreePointer(&level->script_path);
    Memory_FreePointer(&level->lara_outfit);
    M_FreeInjections(&level->injections);
    M_FreeSequence(&level->sequence);

    if (level->item_drops.count > 0) {
        for (int32_t i = 0; i < level->item_drops.count; i++) {
            Memory_FreePointer(&level->item_drops.data[i].object_ids);
        }
        Memory_FreePointer(&level->item_drops.data);
    }
    Memory_FreePointer(&level->settings.sfx_path);
}

static void M_FreeLevelTable(GF_LEVEL_TABLE *const level_table)
{
    if (level_table != nullptr) {
        for (int32_t i = 0; i < level_table->count; i++) {
            M_FreeLevel(&level_table->levels[i]);
        }
        Memory_FreePointer(&level_table->levels);
        level_table->count = 0;
    }
}

static void M_FreeFMVs(GAME_FLOW *const gf)
{
    for (int32_t i = 0; i < gf->fmv_count; i++) {
        Memory_FreePointer(&gf->fmvs[i].path);
    }
    Memory_FreePointer(&gf->fmvs);
    gf->fmv_count = 0;
}

void GF_Init(void)
{
}

void GF_Shutdown(void)
{
    m_CurrentLevel = nullptr;
    m_OverrideCommand = (GF_COMMAND) { .action = GF_NOOP };

    LevelCache_Reset();

    GAME_FLOW *const gf = &g_GameFlow;
    M_FreeInjections(&gf->injections);

    for (int32_t i = 0; i < GFLT_NUMBER_OF; i++) {
        M_FreeLevelTable(&gf->level_tables[i]);
    }
    M_FreeFMVs(gf);

    Memory_FreePointer(&gf->globe.entries);
    gf->globe.count = 0;

    if (gf->title_level != nullptr) {
        M_FreeLevel(gf->title_level);
        Memory_FreePointer(&gf->title_level);
    }

    Memory_FreePointer(&gf->main_menu_background_path);
    Memory_FreePointer(&gf->savegame_file_fmt);
    Memory_FreePointer(&gf->ambient_tracks.ids);
    gf->ambient_tracks.count = 0;
    Memory_FreePointer(&gf->settings.sfx_path);
    Memory_FreePointer(&gf->meta.name);
    Memory_FreePointer(&gf->meta.extends);
    Memory_FreePointer(&gf->path);

    // A mod switch loads the next gameflow into this same struct, and the
    // reader only writes the fields its gameflow names. Anything left here
    // would be read as the new mod's setting.
    *gf = (GAME_FLOW) {};
}

void GF_OverrideCommand(const GF_COMMAND command)
{
    m_OverrideCommand = command;
}

GF_COMMAND GF_GetOverrideCommand(void)
{
    return m_OverrideCommand;
}

GF_LEVEL_TABLE_TYPE GF_GetLevelTableType(const GF_LEVEL_TYPE level_type)
{
    switch (level_type) {
    case GFL_GYM:
    case GFL_NORMAL:
    case GFL_BONUS:
    case GFL_DUMMY:
    case GFL_CURRENT:
        return GFLT_MAIN;

    case GFL_CUTSCENE:
        return GFLT_CUTSCENES;

    case GFL_DEMO:
        return GFLT_DEMOS;

    case GFL_TITLE:
        return GFLT_TITLE;
    }

    ASSERT_FAIL();
}

const GF_LEVEL_TABLE *GF_GetLevelTable(
    const GF_LEVEL_TABLE_TYPE level_table_type)
{
    return &g_GameFlow.level_tables[level_table_type];
}

int32_t GF_GetLevelCount(const GF_LEVEL_TABLE_TYPE level_table_type)
{
    int32_t count = 0;
    const GF_LEVEL_TABLE *const tbl = GF_GetLevelTable(level_table_type);
    for (int32_t i = 0; i < tbl->count; i++) {
        const GF_LEVEL *const level = &tbl->levels[i];
        if (level->type == GFL_GYM || M_SkipLevel(level)) {
            continue;
        }
        count++;
    }
    return count;
}

int32_t GF_GetLevelOrdinalNumber(
    GF_LEVEL_TABLE_TYPE level_table_type, const GF_LEVEL *const ref_level)
{
    int32_t ordinal = 1;
    const GF_LEVEL_TABLE *const tbl = GF_GetLevelTable(level_table_type);
    for (int32_t i = 0; i < tbl->count; i++) {
        const GF_LEVEL *level = &tbl->levels[i];
        if (M_SkipLevel(level)) {
            continue;
        }
        if (level == ref_level) {
            // Special case: gym levels have no ordinal
            return (level->type == GFL_GYM) ? 0 : ordinal;
        }
        if (level->type != GFL_GYM) {
            ordinal++;
        }
    }
    return -1;
}

GF_LEVEL *GF_GetLevelByOrdinalNumber(
    GF_LEVEL_TABLE_TYPE level_table_type, const int32_t level_num)
{
    const GF_LEVEL_TABLE *const tbl = GF_GetLevelTable(level_table_type);
    for (int32_t i = 0; i < tbl->count; i++) {
        GF_LEVEL *const level = &tbl->levels[i];
        if (GF_GetLevelOrdinalNumber(level_table_type, level) == level_num) {
            return level;
        }
    }
    return nullptr;
}

const GF_LEVEL *GF_FindPlayableLevelByQuery(const char *const query)
{
    VECTOR *source = Vector_Create(sizeof(STRING_FUZZY_SOURCE));
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (M_SkipLevel(level) || level->type == GFL_GYM) {
            continue;
        }

        const STRING_FUZZY_SOURCE source_item = {
            .key = level->title,
            .value = (void *)level,
            .weight = 1,
        };
        if (source_item.key != nullptr) {
            Vector_Add(source, &source_item);
        }
    }

    const GF_LEVEL *const gym_level = GF_GetGymLevel();
    if (gym_level != nullptr) {
        const STRING_FUZZY_SOURCE source_item = {
            .key = "gym",
            .value = (void *)gym_level,
            .weight = 1,
        };
        Vector_Add(source, &source_item);
    }

    const GF_LEVEL *result = nullptr;
    VECTOR *matches = String_FuzzyMatch(query, source);
    if (matches->count >= 1) {
        const STRING_FUZZY_MATCH *const match = Vector_Get(matches, 0);
        result = (const GF_LEVEL *)match->value;
    }

    Vector_Free(matches);
    Vector_Free(source);
    return result;
}

const GF_LEVEL *GF_GetCurrentLevel(void)
{
    return m_CurrentLevel;
}

const GF_LEVEL *GF_GetTitleLevel(void)
{
    return g_GameFlow.title_level;
}

const GF_LEVEL *GF_GetGymLevel(void)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type == GFL_GYM && !M_SkipLevel(level)) {
            return level;
        }
    }
    return nullptr;
}

const GF_LEVEL *GF_GetFirstLevel(void)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type == GFL_GYM || M_SkipLevel(level)) {
            continue;
        }
        return level;
    }
    return nullptr;
}

const GF_LEVEL *GF_GetLastLevel(void)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    const GF_LEVEL *result = nullptr;
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type == GFL_GYM || M_SkipLevel(level)) {
            continue;
        }
        result = level;
    }
    return result;
}

const GF_LEVEL *GF_GetLevel(
    const GF_LEVEL_TABLE_TYPE level_table_type, const int32_t num)
{
    if (level_table_type == GFLT_TITLE) {
        return GF_GetTitleLevel();
    }
    const GF_LEVEL_TABLE *const level_table =
        GF_GetLevelTable(level_table_type);
    ASSERT(level_table != nullptr);
    if (num < 0 || num >= level_table->count) {
        LOG_ERROR("Invalid cutscene number: %d", num);
        return nullptr;
    }
    return &level_table->levels[num];
}

const GF_LEVEL *GF_GetLevelAfter(const GF_LEVEL *const level)
{
    if (level == nullptr) {
        return nullptr;
    }
    const GF_LEVEL_TABLE_TYPE level_table_type =
        GF_GetLevelTableType(level->type);
    const GF_LEVEL_TABLE *const level_table =
        GF_GetLevelTable(level_table_type);
    for (int32_t i = level->num + 1; i < level_table->count; i++) {
        const GF_LEVEL *const next_level = &level_table->levels[i];
        if (!M_SkipLevel(next_level)) {
            return next_level;
        }
    }
    return nullptr;
}

const GF_LEVEL *GF_GetLevelBefore(const GF_LEVEL *const level)
{
    if (level == nullptr) {
        return nullptr;
    }
    const GF_LEVEL_TABLE_TYPE level_table_type =
        GF_GetLevelTableType(level->type);
    const GF_LEVEL_TABLE *const level_table =
        GF_GetLevelTable(level_table_type);
    for (int32_t i = level->num - 1; i >= 0; i--) {
        const GF_LEVEL *const prev_level = &level_table->levels[i];
        if (!M_SkipLevel(prev_level)) {
            return prev_level;
        }
    }
    return nullptr;
}

void GF_SetCurrentLevel(const GF_LEVEL *const level)
{
    m_CurrentLevel = level;
}

void GF_SetLevelTitle(GF_LEVEL *const level, const char *const title)
{
    Memory_FreePointer(&level->title);
    level->title = title != nullptr ? Memory_DupStr(title) : nullptr;
}

REGISTER_SUBSYSTEM(.shutdown = GF_Shutdown)
