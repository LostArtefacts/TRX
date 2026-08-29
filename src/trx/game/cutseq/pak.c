#include <trx/game/cutseq/pak.h>

#include <trx/core/filesystem.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/debug.h>
#include <trx/game/objects.h>
#include <trx/game/paths.h>

#include <string.h>
#include <zlib.h>

#define M_PAK_FILE_NAME "cutseq.pak"
#define M_HEADER_ENTRY_SIZE 8 // int32 offset + int32 length
#define M_DESCRIPTOR_SIZE 104 // NEW_CUTSCENE with 10 packed actor entries
// Retail cutseq.pak inflates to a few megabytes. The ceiling is here so a
// corrupt size header fails the load rather than the allocator.
#define M_MAX_INFLATED_SIZE (64 * 1024 * 1024)

static uint8_t *m_Data = nullptr;
static uint32_t m_DataSize = 0;
static int32_t m_CutsceneCount = 0;
// The file the pak in memory came from, or the one a failed read named. Every
// level load asks for the pak: one naming the file already read is answered
// from memory, and a game without one says so once. A level naming another
// file - a different game, or a mod carrying its own - reads again.
static char *m_Path = nullptr;
static bool m_Attempted = false;

static int32_t M_ReadS32(const uint8_t *const data)
{
    return (int32_t)(data[0] | (data[1] << 8) | (data[2] << 16)
                     | ((uint32_t)data[3] << 24));
}

static int16_t M_ReadS16(const uint8_t *const data)
{
    return (int16_t)(data[0] | (data[1] << 8));
}

static bool M_Inflate(const char *const path)
{
    char *file_data = nullptr;
    size_t file_size = 0;
    if (!SHOULD(FS_Load(path, &file_data, &file_size))) {
        return false;
    }
    if (file_size < sizeof(uint32_t)) {
        Memory_FreePointer(&file_data);
        return false;
    }

    const uint32_t uncompressed_size = M_ReadS32((const uint8_t *)file_data);
    if (uncompressed_size < M_DESCRIPTOR_SIZE
        || uncompressed_size > M_MAX_INFLATED_SIZE) {
        LOG_ERROR(
            "%s declares an implausible size of %u bytes", path,
            uncompressed_size);
        Memory_FreePointer(&file_data);
        return false;
    }

    uint8_t *const payload = Memory_Alloc(uncompressed_size);
    uLongf out_size = uncompressed_size;
    const int32_t error = uncompress(
        (Bytef *)payload, &out_size, (const Bytef *)file_data + 4,
        file_size - 4);
    Memory_FreePointer(&file_data);
    if (error != Z_OK || out_size != uncompressed_size) {
        LOG_ERROR("Failed to inflate %s", path);
        Memory_Free(payload);
        return false;
    }

    m_Data = payload;
    m_DataSize = uncompressed_size;
    return true;
}

static void M_DetectCutsceneCount(void)
{
    // The file starts with an {offset, length} pair per cutscene; the first
    // referenced payload marks where the header table ends.
    uint32_t header_end = m_DataSize;
    int32_t count = 0;
    // Counting whole entries only keeps every later read of the table, here
    // and in CutSeq_Pak_GetCutscene, inside the buffer.
    for (int32_t i = 0; (uint32_t)(i + 1) * M_HEADER_ENTRY_SIZE <= header_end;
         i++) {
        const uint32_t offset = M_ReadS32(&m_Data[i * M_HEADER_ENTRY_SIZE]);
        if (offset != 0 && offset < header_end) {
            header_end = offset;
        }
        count++;
    }
    m_CutsceneCount = count;
}

static bool M_IsPathAlreadyRead(const char *const path)
{
    if (path == nullptr || m_Path == nullptr) {
        return path == m_Path;
    }
    return strcmp(path, m_Path) == 0;
}

bool CutSeq_Pak_Load(void)
{
    const char *const path =
        GamePath_TryResolve(GAME_DYNAMIC_PATH_LEVEL_FILE, M_PAK_FILE_NAME);
    if (m_Attempted && M_IsPathAlreadyRead(path)) {
        return m_Data != nullptr;
    }

    // Before the dup below, which is what the next call compares against.
    CutSeq_Pak_Unload();
    m_Attempted = true;
    m_Path = Memory_DupStr(path);

    if (path == nullptr) {
        LOG_WARNING("Missing " M_PAK_FILE_NAME ", cutscenes are disabled");
        return false;
    }
    if (!M_Inflate(path)) {
        return false;
    }

    M_DetectCutsceneCount();
    LOG_INFO(
        "Loaded %s: %u bytes, %d cutscene slots", path, m_DataSize,
        m_CutsceneCount);
    return true;
}

void CutSeq_Pak_Unload(void)
{
    Memory_FreePointer(&m_Data);
    Memory_FreePointer(&m_Path);
    m_DataSize = 0;
    m_CutsceneCount = 0;
    m_Attempted = false;
}

bool CutSeq_Pak_IsLoaded(void)
{
    return m_Data != nullptr;
}

int32_t CutSeq_Pak_GetCutsceneCount(void)
{
    return m_CutsceneCount;
}

bool CutSeq_Pak_GetCutscene(const int32_t num, CUTSEQ_INFO *const info)
{
    if (m_Data == nullptr || num < 0 || num >= m_CutsceneCount) {
        return false;
    }

    const uint32_t offset = M_ReadS32(&m_Data[num * M_HEADER_ENTRY_SIZE]);
    const uint32_t length = M_ReadS32(&m_Data[num * M_HEADER_ENTRY_SIZE + 4]);
    if (offset == 0 || length < M_DESCRIPTOR_SIZE || offset > m_DataSize
        || length > m_DataSize - offset) {
        return false;
    }

    const uint8_t *const desc = &m_Data[offset];
    *info = (CUTSEQ_INFO) {
        .num_actors = M_ReadS16(desc),
        .num_frames = M_ReadS16(desc + 2),
        .origin = {
            .x = M_ReadS32(desc + 4),
            .y = M_ReadS32(desc + 8),
            .z = M_ReadS32(desc + 12),
        },
        .audio_track = M_ReadS32(desc + 16),
        .camera_offset = M_ReadS32(desc + 20),
        .data = desc,
        .data_size = length,
    };
    if (info->num_actors < 1 || info->num_actors > CUTSEQ_MAX_ACTORS
        || info->num_frames <= 0 || info->camera_offset >= length) {
        LOG_ERROR("Malformed cutscene %d descriptor", num);
        return false;
    }

    for (int32_t i = 0; i < info->num_actors; i++) {
        const uint8_t *const actor_desc = desc + 24 + i * 8;
        CUTSEQ_ACTOR_INFO *const actor = &info->actors[i];
        actor->data_offset = M_ReadS32(actor_desc);
        actor->game_obj_slot = M_ReadS16(actor_desc + 4);
        actor->node_count = M_ReadS16(actor_desc + 6);
        actor->obj_id = Object_SlotToID(actor->game_obj_slot);
        if (actor->data_offset >= length || actor->node_count < 1) {
            LOG_ERROR("Malformed cutscene %d actor %d", num, i);
            return false;
        }
    }

    return true;
}

REGISTER_SUBSYSTEM(.shutdown = CutSeq_Pak_Unload)
