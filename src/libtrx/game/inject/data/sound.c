#include "debug.h"
#include "game/inject.h"
#include "game/sound.h"

static void M_HandleSFXData(INJECTION_CHUNK chunk);

static void M_HandleSFXData(const INJECTION_CHUNK chunk)
{
    ASSERT(chunk.num_blocks == 1);
    const INJECTION_DATA_TYPE data_type = VFile_ReadS32(chunk.injection->fp);
    ASSERT(data_type == IDT_SAMPLE_INFOS);
    const int32_t data_count = VFile_ReadS32(chunk.injection->fp);
    VFile_Skip(chunk.injection->fp, sizeof(int32_t));

    LEVEL_INFO *const level_info = Level_GetInfo();
    int16_t *const sample_lut = Sound_GetSampleLUT();
    for (int32_t i = 0; i < data_count; i++) {
        const int16_t sfx_id = VFile_ReadS16(chunk.injection->fp);
        sample_lut[sfx_id] = level_info->samples.info_count;

        SAMPLE_INFO *const sample_info = Sound_GetSampleInfo(sfx_id);
        sample_info->volume = VFile_ReadS16(chunk.injection->fp);
        sample_info->randomness = VFile_ReadS16(chunk.injection->fp);
        sample_info->flags = VFile_ReadS16(chunk.injection->fp);
        sample_info->number = level_info->samples.offset_count;

        const int16_t num_samples = (sample_info->flags >> 2) & 15;
#if TR_VERSION == 1
        for (int32_t j = 0; j < num_samples; j++) {
            const int32_t sample_length = VFile_ReadS32(chunk.injection->fp);
            VFile_Read(
                chunk.injection->fp,
                level_info->samples.data + level_info->samples.data_size,
                sizeof(char) * sample_length);

            level_info->samples.offsets[level_info->samples.offset_count] =
                level_info->samples.data_size;
            level_info->samples.data_size += sample_length;
            level_info->samples.offset_count++;
        }
#else
        const int32_t sample_id = VFile_ReadS32(chunk.injection->fp);
        for (int32_t j = 0; j < num_samples; j++) {
            level_info->samples.offsets[level_info->samples.offset_count] =
                sample_id + j;
            level_info->samples.offset_count++;
        }
#endif
        level_info->samples.info_count++;
    }
}

REGISTER_INJECTOR(ICT_SFX_DATA, M_HandleSFXData)
