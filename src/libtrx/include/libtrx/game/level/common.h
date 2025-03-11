#pragma once

#include "../../virtual_file.h"
#include "./types.h"

#define ANIM_BONE_SIZE 4

void Level_ReadPalettes(VFILE *file);
void Level_ReadTexturePages(VFILE *file);
void Level_ReadRooms(VFILE *file);
void Level_ReadObjectMeshes(VFILE *file);
void Level_ReadAnims(VFILE *file);
void Level_ReadAnimChanges(VFILE *file);
void Level_ReadAnimRanges(VFILE *file);
void Level_ReadAnimCommands(VFILE *file);
void Level_ReadAnimBones(VFILE *file);
void Level_ReadAnimFrames(VFILE *file);
void Level_ReadObjects(VFILE *file);
void Level_ReadStaticObjects(VFILE *file);
void Level_ReadObjectTextures(VFILE *file);
void Level_ReadSpriteTextures(VFILE *file);
void Level_ReadSpriteSequences(VFILE *file);
void Level_ReadPathingData(VFILE *file);
void Level_ReadAnimatedTextureRanges(VFILE *file);
void Level_ReadLightMap(VFILE *file);
void Level_ReadCinematicFrames(VFILE *file);
void Level_ReadCamerasAndSinks(VFILE *file);
void Level_ReadItems(VFILE *file);
void Level_ReadDemoData(VFILE *file);
void Level_ReadSoundSources(VFILE *file);
void Level_ReadSamples(VFILE *file);

void Level_AppendObjectMeshes(
    int32_t num_indices, const int32_t *indices, VFILE *file);
void Level_AppendAnims(int32_t base_idx, int32_t num_anims, VFILE *file);
void Level_AppendAnimChanges(
    int32_t base_idx, int32_t num_changes, VFILE *file);
void Level_AppendAnimRanges(int32_t base_idx, int32_t num_ranges, VFILE *file);
void Level_AppendAnimCommands(
    int32_t base_idx, int32_t num_commands, VFILE *file);
void Level_AppendAnimBones(int32_t base_idx, int32_t num_bones, VFILE *file);
void Level_AppendAnimFrames(int32_t base_idx, int32_t num_frames, VFILE *file);
void Level_AppendObjectTextures(
    int32_t base_idx, int16_t base_page_idx, int32_t num_textures, VFILE *file);
void Level_AppendSpriteTextures(
    int32_t base_idx, int16_t base_page_idx, int32_t num_textures, VFILE *file);

void Level_LoadTexturePages(void);
void Level_LoadPalettes(void);
void Level_LoadAnimCommands(void);
void Level_LoadAnimFrames(void);
void Level_LoadObjectsAndItems(void);

LEVEL_INFO *Level_GetInfo(void);
