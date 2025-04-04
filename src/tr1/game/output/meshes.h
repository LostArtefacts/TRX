#pragma once

#include "game/output/shader.h"

// clang-format off
#define VERT_NO_CAUSTICS 0b0000'0001 // = 0x01
#define VERT_FLAT_SHADED 0b0000'0010 // = 0x02
#define VERT_REFLECTIVE  0b0000'0100 // = 0x04
#define VERT_NO_LIGHTING 0b0000'1000 // = 0x08
// clang-format on

#pragma pack(push, 1)
typedef struct {
    // clang-format off
    XYZ_F pos;                // attribute 0
    XYZ_F normal;             // attribute 1
    int32_t uvw_idx;          // attribute 2
    float trapezoid_ratio[2]; // attribute 3
    uint16_t flags;           // attribute 4
    RGBA_8888 color;          // attribute 5
    int16_t shade;            // attribute 6
    // clang-format on
} OUTPUT_MESH_VERTEX;
#pragma pack(pop)

void Output_Meshes_Init(void);
void Output_Meshes_Shutdown(void);
void Output_Meshes_UploadProjectionMatrix(void);
void Output_Meshes_RenderBegin(void);
OUTPUT_SHADER *Output_Meshes_GetShader(void);

void Output_Meshes_AddVertices(
    int32_t count, const OUTPUT_MESH_VERTEX *vertices);
void Output_Meshes_Flush();
