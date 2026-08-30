#include <trx/core/benchmark.h>
#include <trx/core/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/game/anims.h>
#include <trx/game/inject.h>
#include <trx/game/level/format/format.h>
#include <trx/game/level/sections/append.h>
#include <trx/game/level/sections/read.h>

#define M_LAYOUT_TR4 LEVEL_FORMAT_LAYOUT_TR4

static void M_ReadPosition(XYZ_32 *const pos, TRX_FILE *const file)
{
    pos->x = File_ReadS32(file);
    pos->y = File_ReadS32(file);
    pos->z = File_ReadS32(file);
}

// The anims are read before the frames they point into, so the offsets they
// hold are checked once the frame data is in.
static RESULT M_CheckAnimFrameOffsets(const int32_t raw_data_count)
{
    const uint32_t data_bytes = (uint32_t)raw_data_count * sizeof(int16_t);
    const int32_t anim_count = Anim_GetTotalCount();
    for (int32_t i = 0; i < anim_count; i++) {
        const ANIM *const anim = Anim_GetAnim(i);
        FAIL_IF(
            anim->frame_ofs % sizeof(int16_t) != 0,
            "anim %d starts at byte %u, which is no frame boundary", i,
            anim->frame_ofs);
        FAIL_IF(
            anim->frame_ofs > data_bytes,
            "anim %d starts at byte %u of the %u the frames hold", i,
            anim->frame_ofs, data_bytes);
    }
    return OK;
}

RESULT Level_Section_ReadAnims(LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_anims = File_ReadCountS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->anims.anim_count = num_anims;
    LOG_INFO("anims: %d", num_anims);
    Anim_InitialiseAnims(num_anims + Inject_GetDataCount(IDT_ANIMS));
    Level_Section_AppendAnims(0, num_anims, file);
    Benchmark_End(&benchmark, nullptr);
    return OK;
}

void Level_Section_AppendAnims(
    const int32_t base_idx, const int32_t num_anims, TRX_FILE *const file)
{
    const LEVEL_FORMAT_LOADER *const loader = Level_Context_Get()->loader;
    for (int32_t i = 0; i < num_anims; i++) {
        ANIM *const anim = Anim_GetAnim(base_idx + i);
        anim->frame_ofs = File_ReadU32(file);
        anim->frame_ptr = nullptr; // filled later by the animation frame loader
        anim->interpolation = File_ReadU8(file);
        anim->frame_size = File_ReadU8(file);
        anim->current_anim_state = File_ReadS16(file);
        anim->velocity = File_ReadS32(file);
        anim->acceleration = File_ReadS32(file);
        if (loader->layout == M_LAYOUT_TR4) {
            anim->lateral_velocity = File_ReadS32(file);
            anim->lateral_acceleration = File_ReadS32(file);
        } else {
            anim->lateral_velocity = 0;
            anim->lateral_acceleration = 0;
        }
        anim->frame_base = File_ReadS16(file);
        anim->frame_end = File_ReadS16(file);
        anim->jump_anim_num = File_ReadS16(file);
        anim->jump_frame_num = File_ReadS16(file);
        anim->num_changes = File_ReadS16(file);
        anim->change_idx = File_ReadS16(file);
        anim->num_commands = File_ReadS16(file);
        anim->command_idx = File_ReadS16(file);
    }
}

RESULT Level_Section_ReadAnimChanges(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_anim_changes = File_ReadCountS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->anims.change_count = num_anim_changes;
    LOG_INFO("anim changes: %d", num_anim_changes);
    Anim_InitialiseChanges(
        num_anim_changes + Inject_GetDataCount(IDT_ANIM_CHANGES));
    Level_Section_AppendAnimChanges(0, num_anim_changes, file);
    Benchmark_End(&benchmark, nullptr);
    return OK;
}

void Level_Section_AppendAnimChanges(
    const int32_t base_idx, const int32_t num_changes, TRX_FILE *const file)
{
    for (int32_t i = 0; i < num_changes; i++) {
        ANIM_CHANGE *const anim_change = Anim_GetChange(base_idx + i);
        anim_change->goal_anim_state = File_ReadS16(file);
        anim_change->num_ranges = File_ReadS16(file);
        anim_change->range_idx = File_ReadS16(file);
    }
}

RESULT Level_Section_ReadAnimRanges(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_anim_ranges = File_ReadCountS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->anims.range_count = num_anim_ranges;
    LOG_INFO("anim ranges: %d", num_anim_ranges);
    Anim_InitialiseRanges(
        num_anim_ranges + Inject_GetDataCount(IDT_ANIM_RANGES));
    Level_Section_AppendAnimRanges(0, num_anim_ranges, file);
    Benchmark_End(&benchmark, nullptr);
    return OK;
}

void Level_Section_AppendAnimRanges(
    const int32_t base_idx, const int32_t num_ranges, TRX_FILE *const file)
{
    for (int32_t i = 0; i < num_ranges; i++) {
        ANIM_RANGE *const anim_range = Anim_GetRange(base_idx + i);
        anim_range->start_frame = File_ReadS16(file);
        anim_range->end_frame = File_ReadS16(file);
        anim_range->link_anim_num = File_ReadS16(file);
        anim_range->link_frame_num = File_ReadS16(file);
    }
}

RESULT Level_Section_ReadAnimCommands(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_commands = File_ReadCountS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->anims.command_count = num_commands;
    LOG_INFO("anim commands: %d", num_commands);
    info->anims.commands = Memory_Alloc(
        sizeof(int16_t)
        * (num_commands + Inject_GetDataCount(IDT_ANIM_COMMANDS)));
    Level_Section_AppendAnimCommands(0, num_commands, file);
    Benchmark_End(&benchmark, nullptr);
    return OK;
}

void Level_Section_AppendAnimCommands(
    const int32_t base_idx, const int32_t num_commands, TRX_FILE *const file)
{
    LEVEL_CONTEXT_INFO *const info = Level_Context_GetInfo();
    File_ReadData(
        file, &info->anims.commands[base_idx], sizeof(int16_t) * num_commands);
}

RESULT Level_Section_ReadAnimBones(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_anim_bones = File_ReadCountS32(file) / ANIM_BONE_SIZE;
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->anims.bone_count = num_anim_bones;
    LOG_INFO("anim bones: %d", num_anim_bones);
    Anim_InitialiseBones(num_anim_bones + Inject_GetDataCount(IDT_ANIM_BONES));
    Level_Section_AppendAnimBones(0, num_anim_bones, file);
    Benchmark_End(&benchmark, nullptr);
    return OK;
}

void Level_Section_AppendAnimBones(
    const int32_t base_idx, const int32_t num_bones, TRX_FILE *const file)
{
    for (int32_t i = 0; i < num_bones; i++) {
        ANIM_BONE *const bone = Anim_GetBone(base_idx + i);
        const int32_t flags = File_ReadS32(file);
        bone->matrix_pop = (flags & 1) != 0;
        bone->matrix_push = (flags & 2) != 0;
        bone->rot.x = false;
        bone->rot.y = false;
        bone->rot.z = false;
        M_ReadPosition(&bone->pos, file);
    }
}

RESULT Level_Section_ReadAnimFrames(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t raw_data_count = File_ReadCountS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->anims.frame_count = raw_data_count;
    LOG_INFO("raw anim frames: %d", raw_data_count);
    info->anims.frames = Memory_Alloc(
        sizeof(int16_t)
        * (raw_data_count + Inject_GetDataCount(IDT_ANIM_FRAMES)));
    Level_Section_AppendAnimFrames(0, raw_data_count, file);
    MUST(M_CheckAnimFrameOffsets(raw_data_count));
    Benchmark_End(&benchmark, nullptr);
    return OK;
}

void Level_Section_AppendAnimFrames(
    const int32_t base_idx, const int32_t num_frames, TRX_FILE *const file)
{
    LEVEL_CONTEXT_INFO *const info = Level_Context_GetInfo();
    File_ReadData(
        file, &info->anims.frames[base_idx], sizeof(int16_t) * num_frames);
}
