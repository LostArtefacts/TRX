#include <trx/core/file.h>
#include <trx/debug.h>
#include <trx/game/inject.h>
#include <trx/game/pathing.h>

static void M_HandleOverlapIndices(
    const INJECTION *const injection, const int32_t data_count)
{
    const int32_t box_count = Box_GetCount();
    if (data_count != box_count) {
        LOG_WARNING(
            "Overlap indices are given for %d boxes, but the level has %d",
            data_count, box_count);
    }

    for (int32_t i = 0; i < data_count; i++) {
        const uint32_t overlap_index = File_ReadU32(injection->fp);
        BOX_INFO *const box = Box_GetBox(i);
        if (box != nullptr) {
            box->overlap_index = (box->overlap_index & ~BOX_OVERLAP_BITS)
                | (overlap_index & BOX_OVERLAP_BITS);
        }
    }
}

static void M_HandlePathingData(
    const INJECTION_CONTEXT *const ctx, const INJECTION_CHUNK chunk)
{
    for (int32_t i = 0; i < chunk.num_blocks; i++) {
        const INJECTION_DATA_TYPE data_type = File_ReadS32(chunk.injection->fp);
        const int32_t data_count = File_ReadS32(chunk.injection->fp);
        const int32_t data_size = File_ReadS32(chunk.injection->fp);

        if (ctx->mode == INJECTION_MODE_STATS) {
            File_Skip(chunk.injection->fp, data_size);
            continue;
        }

        switch (data_type) {
        case IDT_OVERLAP_INDICES:
            M_HandleOverlapIndices(chunk.injection, data_count);
            break;
        default:
            LOG_WARNING("Unknown data type: %d", data_type);
            File_Skip(chunk.injection->fp, data_size);
            break;
        }
    }
}

REGISTER_INJECTOR(ICT_PATHING_DATA, M_HandlePathingData)
