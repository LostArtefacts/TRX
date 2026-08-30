#pragma once

#include <trx/game/matrix.h>
#include <trx/game/output/shaders/generic.h>
#include <trx/game/output/types.h>

// clang-format off
#define VERT_NO_WIBBLE         0b0000'0000'0001 // = 0x0001
#define VERT_FLAT_SHADED       0b0000'0000'0010 // = 0x0002
#define VERT_REFLECTIVE        0b0000'0000'0100 // = 0x0004
#define VERT_NO_LIGHTING       0b0000'0000'1000 // = 0x0008
#define VERT_BILLBOARD         0b0000'0001'0000 // = 0x0010
#define VERT_ABS_SPRITE        0b0000'0010'0000 // = 0x0020
#define VERT_NO_ALPHA_DISCARD  0b0000'0100'0000 // = 0x0040
#define VERT_USE_DYNAMIC_LIGHT 0b0000'1000'0000 // = 0x0080
#define VERT_USE_OBJECT_LIGHT  0b0001'0000'0000 // = 0x0100
#define VERT_USE_OWN_LIGHT     0b0010'0000'0000 // = 0x0200
#define VERT_MOVE              0b0100'0000'0000 // = 0x0400
#define VERT_GLOW              0b1000'0000'0000 // = 0x0800
#define VERT_OVERBRIGHT      0b1'0000'0000'0000 // = 0x1000
#define VERT_TEX_WRAP       0b10'0000'0000'0000 // = 0x2000
#define VERT_ADDITIVE      0b100'0000'0000'0000 // = 0x4000
#define VERT_NO_FOG       0b1000'0000'0000'0000 // = 0x8000
// clang-format on

// GL attribute mapping in the shader
typedef enum {
    // clang-format off
    OUTPUT_MESH_ATTR_POS             = 0,
    OUTPUT_MESH_ATTR_NORMAL          = 1,
    OUTPUT_MESH_ATTR_UVW             = 2,
    OUTPUT_MESH_ATTR_TEXTURE_SIZE    = 3,
    OUTPUT_MESH_ATTR_TRAPEZOID_RATIO = 4,
    OUTPUT_MESH_ATTR_FLAGS           = 5,
    OUTPUT_MESH_ATTR_COLOR           = 6,
    OUTPUT_MESH_ATTR_SHADE           = 7,
    OUTPUT_MESH_ATTR_REFLECTIVITY    = 8,
    OUTPUT_MESH_ATTR_UV_SCROLL       = 9,
    // clang-format on
} OUTPUT_MESH_ATTRIBUTE;

typedef struct OUTPUT_MESH_SHADER OUTPUT_MESH_SHADER;

RESULT Output_MeshShader_Create(OUTPUT_MESH_SHADER **out_shader);
void Output_MeshShader_Free(OUTPUT_MESH_SHADER *shader);
void Output_MeshShader_Bind(OUTPUT_MESH_SHADER *shader);

// Binds the variant with no geometry stage, and returns to the variant the
// settings ask for once the pass is done. The geometry stage takes triangles,
// so a pass that draws lines fails on it.
void Output_MeshShader_SuspendSubdivision(
    OUTPUT_MESH_SHADER *shader, bool is_suspended);

// TODO: these could could use UBOs
void Output_MeshShader_UploadModelMatrix(
    OUTPUT_MESH_SHADER *shader, const MATRIX *source);
void Output_MeshShader_UploadWaterEffect(
    OUTPUT_MESH_SHADER *shader, int32_t water_effect);
void Output_MeshShader_UploadWibbleEffect(
    OUTPUT_MESH_SHADER *shader, bool is_enabled);
void Output_MeshShader_UploadTint(OUTPUT_MESH_SHADER *shader, RGBA_F tint);
void Output_MeshShader_UploadAlphaDiscard(
    OUTPUT_MESH_SHADER *shader, bool is_enabled);
