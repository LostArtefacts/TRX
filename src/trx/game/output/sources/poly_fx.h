#pragma once

#include <trx/colors.h>
#include <trx/game/output/shaders/mesh.h>
#include <trx/game/sparks.h>
#include <trx/game/types.h>

#include <stdint.h>

void OutputSource_PolyFX_Init(void);
void OutputSource_PolyFX_Shutdown(void);

void OutputSource_PolyFX_StageSpriteQuadWorldTransparent(
    int32_t sprite_idx, const XYZ_32 world_pos[4], const RGBA_8888 color[4]);

void OutputSource_PolyFX_StageSpriteQuadWorldBlendAdd(
    int32_t sprite_idx, const XYZ_32 world_pos[4], const RGBA_8888 color[4]);

void OutputSource_PolyFX_StageQuadTransparentExt(
    int32_t sprite_idx, const XYZ_32 world_pos[4], const float disp[4][2],
    const RGBA_8888 color[4], uint16_t flags);

void OutputSource_PolyFX_StageQuadBlendAddExt(
    int32_t sprite_idx, const XYZ_32 world_pos[4], const float disp[4][2],
    const RGBA_8888 color[4], uint16_t flags);

void OutputSource_PolyFX_StageSpark(const SPARK *spark);
