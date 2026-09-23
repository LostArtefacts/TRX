#include <trx/core/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/debug.h>
#include <trx/game/inject/canonical.h>
#include <trx/game/level/context.h>
#include <trx/game/level/format/format.h>
#include <trx/game/level/sections/append.h>

// Glue between the pure canonical transcoders and the loaded level: pick the
// game version, run the transcode, and feed the native buffer to the level
// section readers as a memory file.

// The frame maps of the most recently transcoded frames block; anims and
// object records in the same file resolve their ordinals through them.
static VECTOR *m_FrameOffsets = nullptr;
static VECTOR *m_FrameRotCounts = nullptr;

static int32_t M_GameVersion(void)
{
    return Level_Context_Get()->loader->game_version;
}

static void M_Shutdown(void)
{
    if (m_FrameOffsets != nullptr) {
        Vector_Free(m_FrameOffsets);
        Vector_Free(m_FrameRotCounts);
        m_FrameOffsets = nullptr;
        m_FrameRotCounts = nullptr;
    }
}

void InjectCanonical_AppendObjectTextures(
    const INJECTION *const injection, const int32_t base_idx,
    const int16_t base_page_idx, const int32_t data_count)
{
    char *data = nullptr;
    const int32_t size = InjectCanonical_TranscodeObjectTextures(
        injection->fp, data_count, M_GameVersion(), &data);
    TRX_FILE *const native = File_OpenBuffer(data, size);
    Level_Section_AppendObjectTextures(
        base_idx, base_page_idx, data_count, native);
    File_Close(native);
    Memory_FreePointer(&data);
}

void InjectCanonical_AppendObjectMeshes(
    const INJECTION *const injection, const int32_t num_offsets,
    int32_t *const offsets, const int32_t data_size)
{
    char *data = nullptr;
    const int32_t size = InjectCanonical_TranscodeObjectMeshes(
        injection->fp, data_size, M_GameVersion(), offsets, num_offsets, &data);
    TRX_FILE *const native = File_OpenBuffer(data, size);
    SHOULD(Level_Section_AppendObjectMeshes(num_offsets, offsets, native));
    File_Close(native);
    Memory_FreePointer(&data);
}

int32_t InjectCanonical_FrameOffset(const int32_t ordinal)
{
    if (m_FrameOffsets == nullptr || ordinal < 0
        || ordinal >= m_FrameOffsets->count) {
        // Ordinal zero in a file with no frames of its own denotes the join
        // of the level's frame arena, which the caller's rebase supplies.
        if (ordinal != 0) {
            LOG_WARNING("no transcoded frame %d", ordinal);
        }
        return 0;
    }
    return *(const int32_t *)Vector_Get(m_FrameOffsets, ordinal);
}

int32_t InjectCanonical_AppendAnimFrames(
    const INJECTION *const injection, const int32_t base_idx,
    const int32_t data_count, const int32_t data_size)
{
    if (m_FrameOffsets != nullptr) {
        Vector_Free(m_FrameOffsets);
        Vector_Free(m_FrameRotCounts);
    }
    m_FrameOffsets = Vector_Create(sizeof(int32_t));
    m_FrameRotCounts = Vector_Create(sizeof(int32_t));

    char *data = nullptr;
    const int32_t size = InjectCanonical_TranscodeAnimFrames(
        injection->fp, data_count, M_GameVersion(), &data, m_FrameOffsets,
        m_FrameRotCounts);
    TRX_FILE *const native = File_OpenBuffer(data, size);
    const int32_t word_count = size / (int32_t)sizeof(int16_t);
    Level_Section_AppendAnimFrames(base_idx, word_count, native);
    File_Close(native);
    Memory_FreePointer(&data);
    return word_count;
}

void InjectCanonical_AppendAnims(
    const INJECTION *const injection, const int32_t base_idx,
    const int32_t data_count)
{
    VECTOR *const offsets = m_FrameOffsets != nullptr
        ? m_FrameOffsets
        : Vector_Create(sizeof(int32_t));
    VECTOR *const rots = m_FrameRotCounts != nullptr
        ? m_FrameRotCounts
        : Vector_Create(sizeof(int32_t));

    char *data = nullptr;
    const int32_t size = InjectCanonical_TranscodeAnims(
        injection->fp, data_count, M_GameVersion(), offsets, rots, &data);
    TRX_FILE *const native = File_OpenBuffer(data, size);
    Level_Section_AppendAnims(base_idx, data_count, native);
    File_Close(native);
    Memory_FreePointer(&data);

    if (offsets != m_FrameOffsets) {
        Vector_Free(offsets);
        Vector_Free(rots);
    }
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
