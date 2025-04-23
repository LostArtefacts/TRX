#pragma once

#include "game/output/shader.h"
#include "game/output/textures.h"

#include <libtrx/game/math/types.h>
#include <libtrx/game/output/types.h>
#include <libtrx/gfx/gl/utils.h>

#pragma pack(push, 1)
typedef struct {
    // clang-format off
    XYZ_F pos;                          // attribute 0
    XYZ_F normal;                       // attribute 1
    OUTPUT_UVW uvw;                     // attribute 2
    OUTPUT_TEXTURE_SIZE texture_size;   // attribute 3
    float trapezoid_ratio[2];           // attribute 4
    uint16_t flags;                     // attribute 5
    RGBA_8888 color;                    // attribute 6
    int16_t shade;                      // attribute 7
    // clang-format on
} OUTPUT_MESH_VERTEX;
#pragma pack(pop)

void Output_Meshes_InitDynamic(void);
void Output_Meshes_ShutdownDynamic(void);

void Output_Meshes_DrawPrimitives(
    GLuint prim_type, int32_t count, const OUTPUT_MESH_VERTEX *vertices);
void Output_Meshes_DrawTriangles(
    int32_t count, const OUTPUT_MESH_VERTEX *vertices);
