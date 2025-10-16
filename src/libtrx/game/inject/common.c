#include "game/inject/common.h"

#include "benchmark.h"
#include "config.h"
#include "debug.h"
#include "game/items.h"
#include "game/level.h"
#include "game/rooms.h"
#include "memory.h"
#include "vector.h"
#include "version.h"

#include <string.h>
#include <zlib.h>

#define M_INJECTION_CURRENT_VERSION 5
#define M_VIRTUAL_NAME "virtual_injection"

static bool (*m_Testers[ITT_NUMBER_OF])(const INJECTION *injection) = {};
static void (*m_Handlers[ICT_NUMBER_OF])(INJECTION_CHUNK chunk) = {};

static int32_t m_NumInjections = 0;
static INJECTION *m_Injections = nullptr;

static int32_t m_DataCounts[IDT_NUMBER_OF] = {};
static VECTOR *m_RoomMeta = nullptr;
static LEVEL_INFO m_CachedInfo = {};
static uint16_t *m_PaletteMap = nullptr;

static bool M_IsRelevant(const INJECTION_FILE_TYPE type)
{
    switch (type) {
    case IFT_GENERAL:
    case IFT_LARA_ANIMS:
        return true;
    case IFT_FLOOR_DATA:
        return g_Config.gameplay.fix_floor_data_issues;
    case IFT_ITEM_POSITION:
        return g_Config.visuals.fix_item_rots;
    case IFT_TEXTURE_FIX:
        return g_Config.visuals.fix_texture_issues;
    case IFT_ALTER_ANIM_SPRITE:
        return g_Config.visuals.fix_animated_sprites == (g_TRVersion == 2);
#if TR_VERSION == 1
    case IFT_SKYBOX:
        return true;
    case IFT_BRAID:
        return g_Config.visuals.enable_braid;
    case IFT_UZI_SFX:
        return g_Config.audio.enable_ps_uzi_sfx;
    case IFT_PS1_ENEMY:
        return g_Config.gameplay.restore_ps1_enemies;
    case IFT_PS1_CRYSTAL:
        return g_Config.gameplay.enable_save_crystals
            && g_Config.visuals.enable_ps1_crystals;
#elif TR_VERSION == 2
    case IFT_BAREFOOT_SFX:
        return g_Config.audio.enable_barefoot_sfx;
#endif
    default:
        return false;
    }
}

static INJECTION_CHUNK M_ReadChunk(const INJECTION *const injection)
{
    return (INJECTION_CHUNK) {
        .injection = injection,
        .type = VFile_ReadS32(injection->fp),
        .num_blocks = VFile_ReadS32(injection->fp),
        .total_size = VFile_ReadS32(injection->fp),
    };
}

static void M_InitialiseBlock(
    VFILE *const file, const INJECTION_VERSION version)
{
    const INJECTION_DATA_TYPE data_type = VFile_ReadS32(file);
    const int32_t data_count = VFile_ReadS32(file);
    const int32_t data_size = VFile_ReadS32(file);
    if (data_type >= 0 && data_type < IDT_NUMBER_OF) {
        m_DataCounts[data_type] += data_count;
    }

    switch (data_type) {
    case IDT_ROOM_EDIT_META: {
        if (m_RoomMeta == nullptr) {
            m_RoomMeta = Vector_Create(sizeof(INJECTION_MESH_META));
        }
        for (int32_t i = 0; i < data_count; i++) {
            INJECTION_MESH_META meta = {
                .room_index = VFile_ReadS16(file),
                .num_vertices = VFile_ReadS16(file),
                .num_quads = VFile_ReadS16(file),
                .num_triangles = VFile_ReadS16(file),
                .num_static_2ds = VFile_ReadS16(file),
            };
            if (version >= INJ_VERSION_3) {
                meta.num_static_3ds = VFile_ReadS16(file);
            }
            Vector_Add(m_RoomMeta, &meta);
        }

        return;
    }

    case IDT_SAMPLE_INFOS: {
        for (int32_t i = 0; i < data_count; i++) {
            VFile_Skip(file, 3 * sizeof(int16_t));
            const int16_t flags = VFile_ReadS16(file);
            const int16_t num_samples = (flags >> 2) & 0xF;
            m_DataCounts[IDT_SAMPLE_INDICES] += num_samples;
            if (g_TRVersion == 1 || version >= INJ_VERSION_4) {
                for (int32_t j = 0; j < num_samples; j++) {
                    const int32_t sample_length = VFile_ReadS32(file);
                    m_DataCounts[IDT_SAMPLE_DATA] += sample_length;
                    VFile_Skip(file, sizeof(char) * sample_length);
                }
            } else if (g_TRVersion == 2) {
                VFile_Skip(file, sizeof(uint32_t));
            }
        }

        return;
    }

    default:
        break;
    }

    VFile_Skip(file, data_size);
}

static void M_ReadVFile(
    INJECTION *const injection, VFILE *const file, const char *const file_name)
{
    const char *const inj_name =
        file_name == nullptr ? M_VIRTUAL_NAME : file_name;
    char *payload = nullptr;

    const uint32_t magic = VFile_ReadU32(file);
    if (magic != INJECTION_MAGIC) {
        LOG_WARNING("Invalid injection magic in %s", inj_name);
        goto cleanup;
    }

    injection->version = VFile_ReadS32(file);
    if (injection->version < INJ_VERSION_2
        || injection->version > M_INJECTION_CURRENT_VERSION) {
        LOG_WARNING(
            "%s uses unsupported version %d", inj_name, injection->version);
        goto cleanup;
    }

    injection->type = VFile_ReadS32(file);
    if (injection->type < 0 || injection->type >= IFT_NUMBER_OF) {
        LOG_WARNING("%s is of unknown type %d", inj_name, injection->type);
        goto cleanup;
    }

    injection->relevant = M_IsRelevant(injection->type);
    if (!injection->relevant) {
        goto cleanup;
    }

    const int32_t uncompressed_size = VFile_ReadS32(file);
    const int32_t compressed_size = VFile_ReadS32(file);

    const char *compressed = file->cur_ptr;
    payload = Memory_Alloc(uncompressed_size);

    uLongf uncompressed_sizef = uncompressed_size;
    const int32_t error_code = uncompress(
        (Bytef *)payload, &uncompressed_sizef, (const Bytef *)compressed,
        (uLongf)compressed_size);
    if (error_code != Z_OK) {
        LOG_WARNING("Failed to decompress injection payload (%d)", error_code);
        injection->relevant = false;
        goto cleanup;
    }

    injection->fp = VFile_CreateFromBuffer(payload, uncompressed_size);

    {
        // Tests are executed after the main level data is loaded.
        VFile_Skip(injection->fp, sizeof(int32_t));
        const int32_t test_size = VFile_ReadS32(injection->fp);
        VFile_Skip(injection->fp, test_size);
    }

    const int32_t num_chunks = VFile_ReadS32(injection->fp);
    for (int32_t i = 0; i < num_chunks; i++) {
        const INJECTION_CHUNK chunk = M_ReadChunk(injection);
        for (int32_t j = 0; j < chunk.num_blocks; j++) {
            M_InitialiseBlock(injection->fp, injection->version);
        }
    }

    VFile_SetPos(injection->fp, 0);
    LOG_INFO("%s queued for injection", inj_name);

cleanup:
    Memory_FreePointer(&payload);
    VFile_Close(file);
}

static void M_LoadFromFile(
    INJECTION *const injection, const char *const file_name)
{
    VFILE *const file = VFile_CreateFromPath(file_name);
    if (file == nullptr) {
        LOG_WARNING("Could not open %s", file_name);
        return;
    }

    M_ReadVFile(injection, file, file_name);
}

static bool M_IsApplicable(const INJECTION *const injection)
{
    const int32_t test_count = VFile_ReadS32(injection->fp);
    VFile_Skip(injection->fp, sizeof(int32_t));

    bool applicable = true;
    for (int32_t i = 0; i < test_count; i++) {
        const INJECTION_TEST_TYPE type = VFile_ReadS32(injection->fp);
        if (m_Testers[type] == nullptr) {
            LOG_WARNING("Unknown injection test type %d", type);
            applicable = false;
            break;
        } else {
            applicable &= m_Testers[type](injection);
        }
    }

    return applicable;
}

void Inject_RegisterTester(
    const INJECTION_TEST_TYPE type,
    bool (*test_func)(const INJECTION *injection))
{
    m_Testers[type] = test_func;
}

void Inject_RegisterHandler(
    const INJECTION_CHUNK_TYPE type, void (*handle_func)(INJECTION_CHUNK chunk))
{
    m_Handlers[type] = handle_func;
}

void Inject_RegisterPaletteMap(const uint16_t *palette_map, const int32_t size)
{
    Memory_FreePointer(&m_PaletteMap);
    m_PaletteMap = Memory_Alloc(size * sizeof(int16_t));
    memcpy(m_PaletteMap, palette_map, size * sizeof(int16_t));
}

uint16_t Inject_GetPaletteIndex(const uint16_t index)
{
    return m_PaletteMap == nullptr ? 0 : m_PaletteMap[index];
}

void Inject_InitLevel(const GF_LEVEL *const level)
{
    m_NumInjections = level->injections.count;
    if (m_NumInjections == 0) {
        return;
    }

    BENCHMARK benchmark = Benchmark_Start();

    m_Injections = Memory_Alloc(sizeof(INJECTION) * m_NumInjections);
    for (int32_t i = 0; i < m_NumInjections; i++) {
        M_LoadFromFile(&m_Injections[i], level->injections.data_paths[i]);
    }

    Benchmark_End(&benchmark, nullptr);
}

void Inject_AppendInjection(VFILE *const file)
{
    m_Injections =
        Memory_Realloc(m_Injections, sizeof(INJECTION) * (m_NumInjections + 1));
    INJECTION *const injection = &m_Injections[m_NumInjections++];
    M_ReadVFile(injection, file, nullptr);
}

void Inject_AllInjections(void)
{
    if (m_Injections == nullptr) {
        return;
    }

    BENCHMARK benchmark = Benchmark_Start();

    for (int32_t i = 0; i < m_NumInjections; i++) {
        INJECTION *const injection = &m_Injections[i];
        if (!injection->relevant) {
            continue;
        }

        // Allow checks to be done on an injection's applicability after the
        // main level data has loaded.
        if (!M_IsApplicable(injection)) {
            LOG_WARNING(
                "Injection type %d is not applicable to the current level",
                injection->type);
            continue;
        }

        // Cache the current status to allow individual handlers to increment
        // counts but still have access to current indices as required.
        m_CachedInfo = *Level_GetInfo();

        const int32_t num_chunks = VFile_ReadS32(injection->fp);
        for (int32_t j = 0; j < num_chunks; j++) {
            const INJECTION_CHUNK chunk = M_ReadChunk(injection);
            if (chunk.type < 0 || chunk.type >= ICT_NUMBER_OF
                || m_Handlers[chunk.type] == nullptr) {
                LOG_WARNING("Unrecognised chunk type %d", chunk.type);
                VFile_Skip(injection->fp, chunk.total_size);
                continue;
            }

            m_Handlers[chunk.type](chunk);
        }

        ASSERT(VFile_GetPos(injection->fp) == injection->fp->size);
    }

    Benchmark_End(&benchmark, nullptr);
}

void Inject_Cleanup(void)
{
    if (m_Injections == nullptr) {
        return;
    }

    BENCHMARK benchmark = Benchmark_Start();

    for (int32_t i = 0; i < m_NumInjections; i++) {
        const INJECTION *const injection = &m_Injections[i];
        if (injection->fp != nullptr) {
            VFile_Close(injection->fp);
        }
    }

    for (int32_t i = 0; i < IDT_NUMBER_OF; i++) {
        m_DataCounts[i] = 0;
    }

    Memory_FreePointer(&m_Injections);
    Memory_FreePointer(&m_PaletteMap);
    m_NumInjections = 0;
    m_CachedInfo = (LEVEL_INFO) {};

    if (m_RoomMeta != nullptr) {
        Vector_Free(m_RoomMeta);
        m_RoomMeta = nullptr;
    }

    Benchmark_End(&benchmark, nullptr);
}

INJECTION_MESH_META Inject_GetRoomMeshMeta(const int32_t room_index)
{
    INJECTION_MESH_META summed_meta = {};
    if (m_RoomMeta == nullptr) {
        return summed_meta;
    }

    for (int32_t i = 0; i < m_RoomMeta->count; i++) {
        const INJECTION_MESH_META *const meta = Vector_Get(m_RoomMeta, i);
        if (meta->room_index != room_index) {
            continue;
        }

        summed_meta.num_vertices += meta->num_vertices;
        summed_meta.num_quads += meta->num_quads;
        summed_meta.num_triangles += meta->num_triangles;
        summed_meta.num_static_2ds += meta->num_static_2ds;
        summed_meta.num_static_3ds += meta->num_static_3ds;
    }

    return summed_meta;
}

int32_t Inject_GetDataCount(const INJECTION_DATA_TYPE type)
{
    return m_DataCounts[type];
}

LEVEL_INFO Inject_GetCachedInfo(void)
{
    return m_CachedInfo;
}
