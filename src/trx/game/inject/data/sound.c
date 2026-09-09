#include <trx/core/file.h>
#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/const.h>
#include <trx/game/inject.h>
#include <trx/game/sound.h>
#include <trx/version.h>

// Return the slot for a named sample, binding a free slot when needed.
static SAMPLE_SLOT M_SlotForSymbol(
    const INJECTION *const injection, const int32_t symbol_idx)
{
    if (symbol_idx < 0 || symbol_idx >= injection->num_symbols) {
        LOG_WARNING("Symbol %d is out of table range", symbol_idx);
        return -1;
    }
    const INJECTION_SYMBOL symbol = injection->symbols[symbol_idx];
    if (symbol.context != CATALOG_SAMPLES || symbol.id == NO_CATALOG_ID) {
        LOG_WARNING("Symbol %d names no sound", symbol_idx);
        return -1;
    }

    int32_t slot = Catalog_IDToSlot(CATALOG_SAMPLES, symbol.id, -1);
    if (slot < 0
        && !SHOULD(
            Catalog_BindFreeSlot(CATALOG_SAMPLES, symbol.id, &slot),
            "the sound is left out")) {
        return -1;
    }
    return (SAMPLE_SLOT)slot;
}

static void M_ReadSampleInfos(
    const INJECTION_CHUNK chunk, const int32_t data_count, const bool named)
{
    for (int32_t i = 0; i < data_count; i++) {
        const int16_t stated = File_ReadS16(chunk.injection->fp);
        const SAMPLE_SLOT sfx_id = named
            ? M_SlotForSymbol(chunk.injection, stated)
            : (SAMPLE_SLOT)stated;
        // Read rows with invalid sounds so the rest of the block stays in
        // sync.
        SAMPLE_INFO discard = {};
        SAMPLE_INFO *const sample_info =
            sfx_id < 0 ? &discard : Sound_GetOrCreateSample(sfx_id);
        sample_info->volume = File_ReadS16(chunk.injection->fp);
        sample_info->randomness = File_ReadS16(chunk.injection->fp);
        sample_info->flags.all = File_ReadU16(chunk.injection->fp);
        if (chunk.injection->version >= INJ_VERSION_6) {
            sample_info->range = File_ReadS32(chunk.injection->fp);
            sample_info->pitch = File_ReadS8(chunk.injection->fp);
        } else {
            sample_info->range = 10 * WALL_L;
            sample_info->pitch = 0;
        }

        if (g_TRVersion == 1) {
            switch (sample_info->flags.mode_bits) {
            case 0:
                sample_info->mode = SAMPLE_MODE_WAIT;
                break;
            case 1:
                sample_info->mode = SAMPLE_MODE_RESTART;
                break;
            case 2:
                sample_info->mode = SAMPLE_MODE_LOOPED;
                break;
            case 3:
                sample_info->mode = SAMPLE_MODE_NORMAL;
                break;
            }
        } else {
            switch (sample_info->flags.mode_bits) {
            case 0:
                sample_info->mode = SAMPLE_MODE_NORMAL;
                break;
            case 1:
                sample_info->mode = SAMPLE_MODE_WAIT;
                break;
            case 2:
                sample_info->mode = SAMPLE_MODE_RESTART;
                break;
            case 3:
                sample_info->mode = SAMPLE_MODE_LOOPED;
                break;
            }
        }

        const int16_t num_samples = sample_info->flags.num_samples;
        if (g_TRVersion == 1 || chunk.injection->version >= INJ_VERSION_4) {
            sample_info->number = Sound_ReserveSampleData(-1, num_samples);
            for (int32_t j = 0; j < num_samples; j++) {
                const int32_t sample_length = File_ReadS32(chunk.injection->fp);
                char *const data = Memory_Alloc(sample_length);
                File_ReadData(chunk.injection->fp, data, sample_length);
                SHOULD(Sound_LoadSampleData(
                    sample_info->number + j, data, sample_length));
                Memory_Free(data);
            }
        } else if (g_TRVersion >= 2) {
            File_Skip(chunk.injection->fp, sizeof(int32_t));
        }
    }
}

static void M_HandleSFXData(
    const INJECTION_CONTEXT *const ctx, const INJECTION_CHUNK chunk)
{
    for (int32_t i = 0; i < chunk.num_blocks; i++) {
        const INJECTION_DATA_TYPE data_type = File_ReadS32(chunk.injection->fp);
        const int32_t data_count = File_ReadS32(chunk.injection->fp);
        const int32_t data_size = File_ReadS32(chunk.injection->fp);
        switch (data_type) {
        case IDT_SAMPLE_INFOS:
            M_ReadSampleInfos(chunk, data_count, false);
            break;
        case IDT_NAMED_SAMPLE_INFOS:
            M_ReadSampleInfos(chunk, data_count, true);
            break;
        default:
            LOG_WARNING("Unrecognised sound data type %d", data_type);
            File_Skip(chunk.injection->fp, data_size);
            break;
        }
    }
}

REGISTER_INJECTOR(ICT_SFX_DATA, M_HandleSFXData)
