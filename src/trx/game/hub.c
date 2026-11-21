#include <trx/game/hub.h>

#include <trx/core/json.h>
#include <trx/core/log.h>
#include <trx/core/strings.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/game/game_flow.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/level.h>
#include <trx/game/rooms.h>
#include <trx/game/savegame.h>
#include <trx/game/savegame/file.h>
#include <trx/version.h>

typedef struct {
    int32_t level_num;
    JSON_VALUE *state;
} M_LEVEL_STATE;

static int32_t m_NextLevel = 0;
static int32_t m_LaraStartPos = 0;
static const HUB_START_POINT *m_StartPoints = nullptr;
static int32_t m_StartPointCount = 0;
static int32_t m_EntryLevelNum = -1;
static int32_t m_EntryStartPos = 0;
static OBJECT_ID m_Vehicle = NO_OBJECT;
static VECTOR *m_Levels = nullptr;

static void M_Shutdown(void)
{
    Hub_Reset();
    Vector_Free(m_Levels);
    m_Levels = nullptr;
    m_NextLevel = 0;
    m_LaraStartPos = 0;
    m_StartPoints = nullptr;
    m_StartPointCount = 0;
    m_EntryLevelNum = -1;
    m_EntryStartPos = 0;
    m_Vehicle = NO_OBJECT;
}

static const GF_LEVEL *M_GetNextLevel(const int32_t level_idx)
{
    return GF_GetLevel(GFLT_MAIN, level_idx - 1);
}

static M_LEVEL_STATE *M_FindLevel(const int32_t level_num)
{
    if (m_Levels == nullptr) {
        return nullptr;
    }
    for (int32_t i = 0; i < m_Levels->count; i++) {
        M_LEVEL_STATE *const entry = Vector_Get(m_Levels, i);
        if (entry->level_num == level_num) {
            return entry;
        }
    }
    return nullptr;
}

static void M_SetLevel(const int32_t level_num, JSON_VALUE *const state)
{
    M_LEVEL_STATE *const entry = M_FindLevel(level_num);
    if (entry != nullptr) {
        JSON_ValueFree(entry->state);
        entry->state = state;
        return;
    }
    if (m_Levels == nullptr) {
        m_Levels = Vector_Create(sizeof(M_LEVEL_STATE));
    }
    Vector_Add(
        m_Levels,
        &(M_LEVEL_STATE) {
            .level_num = level_num,
            .state = state,
        });
}

// Reports whether the game flow resets a hub anywhere. Only such a game has
// levels that Lara comes back to, so only such a game remembers them.
static bool M_IsEnabled(void)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_SEQUENCE *const sequence = &level_table->levels[i].sequence;
        for (int32_t j = 0; j < sequence->length; j++) {
            if (sequence->events[j].type == GFS_RESET_HUB) {
                return true;
            }
        }
    }
    return false;
}

static void M_BringVehicle(const ITEM *const lara_item)
{
    if (m_Vehicle == NO_OBJECT) {
        return;
    }

    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->object_id != m_Vehicle || item->room_num == NO_ROOM) {
            continue;
        }
        item->pos = lara_item->pos;
        item->rot.y = lara_item->rot.y;
        if (item->room_num != lara_item->room_num) {
            Item_UpdateRoom(i, lara_item->room_num);
        }
        int16_t room_num = item->room_num;
        const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
        item->floor = Room_GetHeight(sector, item->pos);
        break;
    }
}

void Hub_SetNextLevelIndex(const int32_t level_idx)
{
    if (level_idx <= 0) {
        return;
    }

    if (M_GetNextLevel(level_idx) != nullptr) {
        m_NextLevel = level_idx;
    }
}

const GF_LEVEL *Hub_GetNextLevel(void)
{
    if (m_NextLevel <= 0) {
        m_NextLevel = 0;
        return nullptr;
    }

    const GF_LEVEL *const level = M_GetNextLevel(m_NextLevel);
    m_NextLevel = 0;
    return level;
}

int32_t Hub_GetNextLevelIndex(void)
{
    return m_NextLevel;
}

void Hub_SetLaraStartIndex(const int32_t start_idx)
{
    m_LaraStartPos = start_idx;
}

int32_t Hub_GetLaraStartIndex(void)
{
    return m_LaraStartPos;
}

void Hub_SetStartPoints(
    const HUB_START_POINT *const points, const int32_t count)
{
    m_StartPoints = points;
    m_StartPointCount = count;
}

OBJECT_ID Hub_GetVehicle(void)
{
    return m_Vehicle;
}

void Hub_SetVehicle(const OBJECT_ID object_id)
{
    m_Vehicle = object_id;
}

int32_t Hub_GetEntryStartIndex(const GF_LEVEL *const level)
{
    return level != nullptr && level->num == m_EntryLevelNum ? m_EntryStartPos
                                                             : 0;
}

void Hub_SetEntryStartIndex(
    const GF_LEVEL *const level, const int32_t start_idx)
{
    m_EntryLevelNum = level != nullptr ? level->num : -1;
    m_EntryStartPos = start_idx;
}

void Hub_RepeatEntry(const GF_LEVEL *const level)
{
    m_LaraStartPos = Hub_GetEntryStartIndex(level);
}

void Hub_InitialiseLaraStart(const GF_LEVEL *const level)
{
    Hub_SetEntryStartIndex(level, m_LaraStartPos);
    ITEM *const lara_item = Lara_GetItem();
    if (m_LaraStartPos > 0 && m_LaraStartPos <= m_StartPointCount) {
        const HUB_START_POINT *const point = &m_StartPoints[m_LaraStartPos - 1];
        if (point->room_num != NO_ROOM) {
            lara_item->pos = point->pos;
            lara_item->rot.y = point->rot_y;
            Item_UpdateRoom(Item_GetIndex(lara_item), point->room_num);
        }
    }
    M_BringVehicle(lara_item);

    m_LaraStartPos = 0;
    m_Vehicle = NO_OBJECT;
}

void Hub_StoreLevel(const GF_LEVEL *const level)
{
    m_Vehicle = NO_OBJECT;
    const int16_t vehicle_num = Lara_Vehicle_GetIndex();
    if (g_TRVersion >= 4 && vehicle_num != NO_ITEM) {
        m_Vehicle = Item_Get(vehicle_num)->object_id;
    }

    if (!M_IsEnabled() || !Level_IsWorldLoaded() || level == nullptr
        || (level->type != GFL_NORMAL && level->type != GFL_BONUS)) {
        return;
    }

    JSON_WRITE_IO *const io = JSON_WriteIO_Create();
    JSONW_WRITE(io, "version", (int32_t)SG_CURRENT_VERSION);
    SG_File_DumpLevelState(io, level);
    M_SetLevel(level->num, JSON_ValueCopy(JSON_WriteIO_GetRoot(io)));
    JSON_WriteIO_Destroy(io);
}

bool Hub_HasLevel(const GF_LEVEL *const level)
{
    return level != nullptr && M_FindLevel(level->num) != nullptr;
}

RESULT Hub_RestoreLevel(const GF_LEVEL *const level)
{
    const M_LEVEL_STATE *const entry = M_FindLevel(level->num);
    if (entry == nullptr) {
        return OK;
    }

    const JSON_OBJECT *const obj = JSON_ValueAsObject(entry->state);
    const int32_t version = JSON_ObjectGetInt(obj, "version", -1);
    FAIL_IF(version < 0, "the state of level %d names no version", level->num);

    JSON_READ_IO *const io = JSON_ReadIO_Create(
        entry->state, version, String_FormatStatic("hub/%d", level->num));
    const RESULT result = SG_File_LoadLevelState(io, level);
    JSON_ReadIO_Destroy(io);
    return result;
}

void Hub_Reset(void)
{
    if (m_Levels == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_Levels->count; i++) {
        M_LEVEL_STATE *const entry = Vector_Get(m_Levels, i);
        JSON_ValueFree(entry->state);
    }
    Vector_Clear(m_Levels);
}

void Hub_Dump(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_OBJECT(io);
    JSON_OBJECT *const obj = JSON_WriteIO_GetCurrentObject(io);
    for (int32_t i = 0; m_Levels != nullptr && i < m_Levels->count; i++) {
        const M_LEVEL_STATE *const entry = Vector_Get(m_Levels, i);
        JSON_ObjectAppend(
            obj, String_FormatStatic("%d", entry->level_num),
            JSON_ValueCopy(entry->state));
    }
    JSONW_POP_AND_SET_NZ(io, "hub");
}

RESULT Hub_Load(JSON_READ_IO *const io)
{
    Hub_Reset();
    if (!JSON_ReadIO_HasKey(io, "hub")) {
        return OK;
    }

    MUST(JSON_PUSH(io, "hub"));
    const int32_t count = JSON_ReadIO_GetKeyCount(io);
    for (int32_t i = 0; i < count; i++) {
        const char *const key = JSON_ReadIO_GetKeyAt(io, i);
        int32_t level_num = -1;
        if (!String_ParseInteger(key, &level_num)
            || GF_GetLevel(GFLT_MAIN, level_num) == nullptr) {
            LOG_WARNING("Not restoring the state of level '%s'", key);
            continue;
        }
        MUST(JSON_PUSH(io, key));
        M_SetLevel(level_num, JSON_ValueCopy(JSON_ReadIO_GetCurrentValue(io)));
        MUST(JSON_POP(io));
    }
    MUST(JSON_POP(io));
    return OK;
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
