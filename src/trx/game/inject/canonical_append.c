#include <trx/core/file.h>
#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/inject/canonical.h>
#include <trx/game/level/context.h>
#include <trx/game/level/format/format.h>
#include <trx/game/level/sections/append.h>

// Glue between the pure canonical transcoders and the loaded level: pick the
// game version, run the transcode, and feed the native buffer to the level
// section readers as a memory file.

static int32_t M_GameVersion(void)
{
    return Level_Context_Get()->loader->game_version;
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

void InjectCanonical_AppendAnims(
    const INJECTION *const injection, const int32_t base_idx,
    const int32_t data_count)
{
    char *data = nullptr;
    const int32_t size = InjectCanonical_TranscodeAnims(
        injection->fp, data_count, M_GameVersion(), &data);
    TRX_FILE *const native = File_OpenBuffer(data, size);
    Level_Section_AppendAnims(base_idx, data_count, native);
    File_Close(native);
    Memory_FreePointer(&data);
}
