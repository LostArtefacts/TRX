#include "debug.h"
#include "game/anims.h"
#include "game/inject.h"
#include "memory.h"

static void M_HandleAnimData(INJECTION_CHUNK chunk);

static void M_HandleAnimData(const INJECTION_CHUNK chunk)
{
    LEVEL_INFO *const level_info = Level_GetInfo();
    const LEVEL_INFO cached_info = Inject_GetCachedInfo();

    for (int32_t i = 0; i < chunk.num_blocks; i++) {
        const INJECTION_DATA_TYPE data_type =
            VFile_ReadS32(chunk.injection->fp);
        const int32_t data_count = VFile_ReadS32(chunk.injection->fp);
        const int32_t data_size = VFile_ReadS32(chunk.injection->fp);

        switch (data_type) {
        case IDT_ANIMS: {
            Level_ReadAnims(
                level_info->anims.anim_count, data_count, chunk.injection->fp);
            level_info->anims.anim_count += data_count;

            for (int32_t i = 0; i < data_count; i++) {
                ANIM *const anim =
                    Anim_GetAnim(cached_info.anims.anim_count + i);
                anim->jump_anim_num += cached_info.anims.anim_count;
                anim->frame_ofs += cached_info.anims.frame_count * 2;
                anim->change_idx += cached_info.anims.change_count;
                anim->command_idx += cached_info.anims.command_count;
            }
            break;
        }

        case IDT_ANIM_BONES: {
            Level_ReadAnimBones(
                level_info->anims.bone_count, data_count, chunk.injection->fp);
            level_info->anims.bone_count += data_count;
            break;
        }

        case IDT_ANIM_CHANGES: {
            Level_ReadAnimChanges(
                level_info->anims.change_count, data_count,
                chunk.injection->fp);
            level_info->anims.change_count += data_count;

            for (int32_t i = 0; i < data_count; i++) {
                ANIM_CHANGE *const change =
                    Anim_GetChange(cached_info.anims.change_count + i);
                change->range_idx += cached_info.anims.range_count;
            }
            break;
        }

        case IDT_ANIM_COMMANDS: {
            VFile_Read(
                chunk.injection->fp,
                level_info->anims.commands + level_info->anims.command_count,
                data_count * sizeof(int16_t));
            level_info->anims.command_count += data_count;
            break;
        }

        case IDT_ANIM_FRAMES: {
            VFile_Read(
                chunk.injection->fp,
                level_info->anims.frames + level_info->anims.frame_count,
                data_count * sizeof(int16_t));
            level_info->anims.frame_count += data_count;
            break;
        }

        case IDT_ANIM_RANGES: {
            Level_ReadAnimRanges(
                level_info->anims.range_count, data_count, chunk.injection->fp);
            level_info->anims.range_count += data_count;

            for (int32_t i = 0; i < data_count; i++) {
                ANIM_RANGE *const range =
                    Anim_GetRange(cached_info.anims.range_count + i);
                range->link_anim_num += cached_info.anims.anim_count;
            }
            break;
        }

        default:
            LOG_WARNING("Unknown data type: %d", data_type);
            VFile_Skip(chunk.injection->fp, data_size);
            break;
        }
    }
}

REGISTER_INJECTOR(ICT_ANIMATION_DATA, M_HandleAnimData)
