#include "game/inject.h"

#include "game/room.h"
#include "global/utils.h"

#include <libtrx/benchmark.h>
#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>
#include <libtrx/virtual_file.h>

#define INJECTION_MAGIC MKTAG('T', '2', 'X', 'J')
#define INJECTION_CURRENT_VERSION 1
#define NULL_FD_INDEX ((uint16_t)(-1))

typedef struct {
    VFILE *fp;
    INJECTION_VERSION version;
    INJECTION_FILE_TYPE type;
    bool relevant;
} INJECTION;

static int32_t m_NumInjections = 0;
static INJECTION *m_Injections = nullptr;
static int32_t m_DataCounts[IDT_NUMBER_OF] = {};

static void M_LoadFromFile(INJECTION *injection, const char *filename);

static void M_FloorDataEdits(const INJECTION *injection, int32_t data_count);
static void M_TriggerTypeChange(
    const INJECTION *injection, const SECTOR *sector);
static void M_TriggerParameterChange(
    const INJECTION *injection, const SECTOR *sector);
static void M_SetMusicOneShot(const SECTOR *sector);
static void M_InsertFloorData(const INJECTION *injection, SECTOR *sector);
static void M_RoomShift(const INJECTION *injection, int16_t room_num);
static void M_RoomProperties(const INJECTION *injection, int16_t room_num);

static void M_ItemEdits(const INJECTION *injection, int32_t data_count);

static void M_LoadFromFile(INJECTION *const injection, const char *filename)
{
    injection->relevant = false;

    VFILE *const fp = VFile_CreateFromPath(filename);
    injection->fp = fp;
    if (fp == nullptr) {
        LOG_WARNING("Could not open %s", filename);
        return;
    }

    const uint32_t magic = VFile_ReadU32(fp);
    if (magic != INJECTION_MAGIC) {
        LOG_WARNING("Invalid injection magic in %s", filename);
        return;
    }

    injection->version = VFile_ReadS32(fp);
    if (injection->version < INJ_VERSION_1
        || injection->version > INJECTION_CURRENT_VERSION) {
        LOG_WARNING(
            "%s uses unsupported version %d", filename, injection->version);
        return;
    }

    injection->type = VFile_ReadS32(fp);

    switch (injection->type) {
    case IFT_GENERAL:
        injection->relevant = true;
        break;
    case IFT_FLOOR_DATA:
        injection->relevant = g_Config.gameplay.fix_floor_data_issues;
        break;
    case IFT_ITEM_POSITION:
        injection->relevant = g_Config.visuals.fix_item_rots;
        break;
    default:
        LOG_WARNING("%s is of unknown type %d", filename, injection->type);
        break;
    }

    if (!injection->relevant) {
        return;
    }

    const size_t start_pos = VFile_GetPos(fp);
    const int32_t block_count = VFile_ReadS32(fp);

    for (int32_t i = 0; i < block_count; i++) {
        const INJECTION_DATA_TYPE type = (INJECTION_DATA_TYPE)VFile_ReadS32(fp);
        const int32_t data_count = VFile_ReadS32(fp);
        m_DataCounts[type] += data_count;

        const int32_t data_size = VFile_ReadS32(fp);
        VFile_Skip(fp, data_size);
    }

    VFile_SetPos(fp, start_pos);

    LOG_INFO("%s queued for injection", filename);
}

static void M_FloorDataEdits(
    const INJECTION *const injection, const int32_t data_count)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < data_count; i++) {
        const int16_t room_num = VFile_ReadS16(fp);
        const uint16_t x = VFile_ReadU16(fp);
        const uint16_t z = VFile_ReadU16(fp);
        const int32_t fd_edit_count = VFile_ReadS32(fp);

        // Verify that the given room and coordinates are accurate.
        // Individual FD functions must check that sector is actually set.
        const ROOM *room = nullptr;
        SECTOR *sector = nullptr;
        if (room_num < 0 || room_num >= Room_GetCount()) {
            LOG_WARNING("Room index %d is invalid", room_num);
        } else {
            room = Room_Get(room_num);
            if (x >= room->size.x || z >= room->size.z) {
                LOG_WARNING(
                    "Sector [%d,%d] is invalid for room %d", x, z, room_num);
            } else {
                sector = Room_GetUnitSector(room, x, z);
            }
        }

        for (int32_t j = 0; j < fd_edit_count; j++) {
            const FLOOR_EDIT_TYPE edit_type = VFile_ReadS32(fp);
            switch (edit_type) {
            case FET_TRIGGER_TYPE:
                M_TriggerTypeChange(injection, sector);
                break;
            case FET_TRIGGER_PARAM:
                M_TriggerParameterChange(injection, sector);
                break;
            case FET_MUSIC_ONESHOT:
                M_SetMusicOneShot(sector);
                break;
            case FET_FD_INSERT:
                M_InsertFloorData(injection, sector);
                break;
            case FET_ROOM_SHIFT:
                M_RoomShift(injection, room_num);
                break;
            case FET_TRIGGER_ITEM:
                LOG_WARNING("Item injection is not currently supported");
                break;
            case FET_ROOM_PROPERTIES:
                M_RoomProperties(injection, room_num);
                break;
            default:
                LOG_WARNING("Unknown floor data edit type: %d", edit_type);
                break;
            }
        }
    }

    Benchmark_End(benchmark, nullptr);
}

static void M_TriggerTypeChange(
    const INJECTION *const injection, const SECTOR *const sector)
{
    const uint8_t new_type = VFile_ReadU8(injection->fp);

    if (sector == nullptr || sector->trigger == nullptr) {
        return;
    }

    sector->trigger->type = new_type;
}

static void M_TriggerParameterChange(
    const INJECTION *const injection, const SECTOR *const sector)
{
    VFILE *const fp = injection->fp;

    const uint8_t cmd_type = VFile_ReadU8(fp);
    const int16_t old_param = VFile_ReadS16(fp);
    const int16_t new_param = VFile_ReadS16(fp);

    if (sector == nullptr || sector->trigger == nullptr) {
        return;
    }

    // If we can find an action item for the given sector that matches
    // the command type and old (current) parameter, change it to the
    // new parameter.
    TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type != cmd_type) {
            continue;
        }

        if (cmd->type == TO_CAMERA) {
            TRIGGER_CAMERA_DATA *const cam_data =
                (TRIGGER_CAMERA_DATA *)cmd->parameter;
            if (cam_data->camera_num == old_param) {
                cam_data->camera_num = new_param;
                break;
            }
        } else {
            if ((int16_t)(intptr_t)cmd->parameter == old_param) {
                cmd->parameter = (void *)(intptr_t)new_param;
                break;
            }
        }
    }
}

static void M_SetMusicOneShot(const SECTOR *const sector)
{
    if (sector == nullptr || sector->trigger == nullptr) {
        return;
    }

    const TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type == TO_CD) {
            sector->trigger->one_shot = true;
            break;
        }
    }
}

static void M_InsertFloorData(
    const INJECTION *const injection, SECTOR *const sector)
{
    VFILE *const fp = injection->fp;

    const int32_t data_length = VFile_ReadS32(fp);

    int16_t data[data_length];
    VFile_Read(fp, data, sizeof(int16_t) * data_length);

    if (sector == nullptr) {
        return;
    }

    // This will reset all FD properties in the sector based on the raw data
    // imported. We pass a dummy null index to allow it to read from the
    // beginning of the array.
    Room_PopulateSectorData(sector, data, 0, NULL_FD_INDEX);
}

static void M_RoomShift(
    const INJECTION *const injection, const int16_t room_num)
{
    VFILE *const fp = injection->fp;

    const uint32_t x_shift = ROUND_TO_SECTOR(VFile_ReadU32(fp));
    const uint32_t z_shift = ROUND_TO_SECTOR(VFile_ReadU32(fp));
    const int32_t y_shift = ROUND_TO_CLICK(VFile_ReadS32(fp));

    ROOM *const room = Room_Get(room_num);
    room->pos.x += x_shift;
    room->pos.z += z_shift;
    room->min_floor += y_shift;
    room->max_ceiling += y_shift;

    // Move any items in the room to match.
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->room_num != room_num) {
            continue;
        }

        item->pos.x += x_shift;
        item->pos.y += y_shift;
        item->pos.z += z_shift;
    }

    if (y_shift == 0) {
        return;
    }

    // Update the sector floor and ceiling heights to match.
    for (int32_t i = 0; i < room->size.z * room->size.x; i++) {
        SECTOR *const sector = &room->sectors[i];
        if (sector->floor.height == NO_HEIGHT
            || sector->ceiling.height == NO_HEIGHT) {
            continue;
        }

        sector->floor.height += y_shift;
        sector->ceiling.height += y_shift;
    }

    // Update vertex Y values to match; x and z are room-relative.
    for (int32_t i = 0; i < room->mesh.num_vertices; i++) {
        room->mesh.vertices[i].pos.y += y_shift;
    }
}

static void M_RoomProperties(
    const INJECTION *const injection, const int16_t room_num)
{
    const uint16_t flags = VFile_ReadU16(injection->fp);

    ROOM *const room = Room_Get(room_num);
    room->flags = flags;
}

static void M_ItemEdits(
    const INJECTION *const injection, const int32_t data_count)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < data_count; i++) {
        const int16_t item_num = VFile_ReadS16(fp);
        const int16_t y_rot = VFile_ReadS16(fp);
        const XYZ_32 pos = {
            .x = VFile_ReadS32(fp),
            .y = VFile_ReadS32(fp),
            .z = VFile_ReadS32(fp),
        };
        const int16_t room_num = VFile_ReadS16(fp);

        if (item_num < 0 || item_num >= Item_GetTotalCount()) {
            LOG_WARNING("Item number %d is out of level item range", item_num);
            continue;
        }

        ITEM *const item = Item_Get(item_num);
        item->rot.y = y_rot;
        item->pos = pos;
        item->room_num = room_num;
    }

    Benchmark_End(benchmark, nullptr);
}

int32_t Inject_GetDataCount(const INJECTION_DATA_TYPE type)
{
    return m_DataCounts[type];
}

void Inject_Init(const int32_t injection_count, char *filenames[])
{
    m_NumInjections = injection_count;
    if (m_NumInjections == 0) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    m_Injections = Memory_Alloc(sizeof(INJECTION) * m_NumInjections);
    for (int32_t i = 0; i < m_NumInjections; i++) {
        M_LoadFromFile(&m_Injections[i], filenames[i]);
    }

    Benchmark_End(benchmark, nullptr);
}

void Inject_AllInjections(void)
{
    if (m_NumInjections == 0) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    for (int32_t i = 0; i < m_NumInjections; i++) {
        const INJECTION *const injection = &m_Injections[i];
        if (!injection->relevant) {
            continue;
        }

        VFILE *const fp = injection->fp;
        const int32_t block_count = VFile_ReadS32(fp);

        for (int32_t j = 0; j < block_count; j++) {
            const INJECTION_DATA_TYPE type =
                (INJECTION_DATA_TYPE)VFile_ReadS32(fp);
            const int32_t data_count = VFile_ReadS32(fp);
            const int32_t data_size = VFile_ReadS32(fp);

            switch (type) {
            case IDT_FLOOR_EDITS:
                M_FloorDataEdits(injection, data_count);
                break;

            case IDT_ITEM_EDITS:
                M_ItemEdits(injection, data_count);
                break;

            default:
                VFile_Skip(fp, data_size);
                LOG_WARNING("Unknown injection data type %d", type);
                break;
            }
        }
    }

    Benchmark_End(benchmark, nullptr);
}

void Inject_Cleanup(void)
{
    if (m_NumInjections == 0) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();
    ASSERT(m_Injections != nullptr);

    for (int32_t i = 0; i < m_NumInjections; i++) {
        INJECTION *const injection = &m_Injections[i];
        if (injection->fp != nullptr) {
            VFile_Close(injection->fp);
            injection->fp = nullptr;
        }
    }

    for (int32_t i = 0; i < IDT_NUMBER_OF; i++) {
        m_DataCounts[i] = 0;
    }

    Memory_FreePointer(&m_Injections);
    Benchmark_End(benchmark, nullptr);
    m_NumInjections = 0;
}

INJECTION_MESH_META Inject_GetRoomMeshMeta(const int32_t room_index)
{
    // TODO: implement during injection refactor
    INJECTION_MESH_META summed_meta = {};
    return summed_meta;
}
