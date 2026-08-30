#pragma once

#include <trx/core/file.h>
#include <trx/core/result.h>

#include <stdint.h>

RESULT Level_Section_AppendObjectMeshes(
    int32_t num_offsets, const int32_t *offsets, TRX_FILE *file);
void Level_Section_AppendAnims(
    int32_t base_idx, int32_t num_anims, TRX_FILE *file);
void Level_Section_AppendAnimChanges(
    int32_t base_idx, int32_t num_changes, TRX_FILE *file);
void Level_Section_AppendAnimRanges(
    int32_t base_idx, int32_t num_ranges, TRX_FILE *file);
void Level_Section_AppendAnimCommands(
    int32_t base_idx, int32_t num_commands, TRX_FILE *file);
void Level_Section_AppendAnimBones(
    int32_t base_idx, int32_t num_bones, TRX_FILE *file);
void Level_Section_AppendAnimFrames(
    int32_t base_idx, int32_t num_frames, TRX_FILE *file);
void Level_Section_AppendObjectTextures(
    int32_t base_idx, int16_t base_page_idx, int32_t num_textures,
    TRX_FILE *file);
void Level_Section_AppendSpriteTextures(
    int32_t base_idx, int16_t base_page_idx, int32_t num_textures,
    TRX_FILE *file);
void Level_Section_AppendFlybyCameras(
    int32_t base_idx, int32_t num_cameras, TRX_FILE *file);
