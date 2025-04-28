#pragma once

#include "game/output/shader.h"
#include "game/output/textures.h"

#define ATI_FIX 1

#if ATI_FIX
    // Evergreen‐generation GPUs (HD 5000/6000 – such as HD 5570 – have a
    // long-standing driver/firm-ware quirk: an integer vertex attribute whose
    // storage size is smaller than 32 bit is fetched as if it were 32-bit,
    // even when you declare it as 8- or 16-bit in glVertexAttribIPointer. The
    // call succeeds, no error is raised, but the hardware fetch unit still
    // steps four bytes, not one or two. If the stride you give is tighter than
    // that, the fetch unit starts reading the next vertex in the middle of the
    // current one and every attribute that follows is garbage – positions turn
    // into huge values, so the whole mesh explodes. NVIDIA, Intel and all
    // post-GCN AMD GPUs fixed the bug.
    //
    // To deal with this, we simply pack our data to increments of 4.
    #define OUTPUT_USHORT uint32_t
    #define OUTPUT_USHORT_GL GL_UNSIGNED_INT
#else
    #define OUTPUT_USHORT uint16_t
    #define OUTPUT_USHORT_GL GL_UNSIGNED_SHORT
#endif

// clang-format off
#define VERT_NO_CAUSTICS 0b0000'0001 // = 0x01
#define VERT_FLAT_SHADED 0b0000'0010 // = 0x02
#define VERT_REFLECTIVE  0b0000'0100 // = 0x04
#define VERT_NO_LIGHTING 0b0000'1000 // = 0x08
#define VERT_SPRITE      0b0001'0000 // = 0x10
// clang-format on

typedef struct {
    // attribute 2
    OUTPUT_UVW uvw;
    // attribute 3
    OUTPUT_TEXTURE_SIZE texture_size;
    // attribute 4
    float trapezoid_ratio[2];
} OUTPUT_MESH_TEXTURE;

void Output_Meshes_Init(void);
void Output_Meshes_Shutdown(void);
void Output_Meshes_UploadProjectionMatrix(void);
void Output_Meshes_ObserveLevelLoad(void);
void Output_Meshes_ObserveLevelUnload(void);
void Output_Meshes_RenderBegin(void);
OUTPUT_SHADER *Output_Meshes_GetShader(void);
