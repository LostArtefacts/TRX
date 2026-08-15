#pragma once

#include <trx/core/file.h>
#include <trx/core/result.h>
#include <trx/game/level/context.h>

#define ANIM_BONE_SIZE 4

RESULT Level_Section_ReadPalettes(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadTexturePages(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadRooms(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadObjectMeshes(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadAnims(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadAnimChanges(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadAnimRanges(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadAnimCommands(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadAnimBones(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadAnimFrames(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadObjects(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadStaticObjects(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadObjectTextures(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadSpriteTextures(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadSpriteSequences(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadPathingData(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadAnimatedTextureRanges(
    LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadLightMap(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadCinematicFrames(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadCamerasAndSinks(LEVEL_CONTEXT *ctx, TRX_FILE *file);
void Level_Section_PrepareTR123FlybyCameras(void);
RESULT Level_Section_ReadFlybyCameras(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadItems(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadDemoData(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadSoundSources(LEVEL_CONTEXT *ctx, TRX_FILE *file);
RESULT Level_Section_ReadSamples(LEVEL_CONTEXT *ctx, TRX_FILE *file);
