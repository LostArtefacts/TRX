#pragma once

#include "../matrix.h"
#include "./types.h"

// clang-format off
#define VERT_NO_CAUSTICS 0b0000'0001 // = 0x01
#define VERT_FLAT_SHADED 0b0000'0010 // = 0x02
#define VERT_REFLECTIVE  0b0000'0100 // = 0x04
#define VERT_NO_LIGHTING 0b0000'1000 // = 0x08
#define VERT_BILLBOARD   0b0001'0000 // = 0x10
#define VERT_ABS_SPRITE  0b0010'0000 // = 0x20
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
    // clang-format on
} OUTPUT_MESH_ATTRIBUTE;

typedef struct OUTPUT_SHADER OUTPUT_SHADER;

OUTPUT_SHADER *Output_Shader_Create(const char *path);
void Output_Shader_Free(OUTPUT_SHADER *shader);
void Output_Shader_Bind(const OUTPUT_SHADER *shader);
void Output_Shader_UploadCommonUniforms(const OUTPUT_SHADER *shader);
void Output_Shader_UploadPerspProjectionMatrix(const OUTPUT_SHADER *shader);
void Output_Shader_UploadOrthoProjectionMatrix(const OUTPUT_SHADER *shader);
void Output_Shader_UploadViewMatrix(const OUTPUT_SHADER *shader);

void Output_Shader_UploadLightingMode(
    const OUTPUT_SHADER *shader, LIGHTING_MODE mode);
void Output_Shader_UploadSmoothingEnabled(
    const OUTPUT_SHADER *shader, bool is_enabled);

// TODO: these could could use UBOs
void Output_Shader_UploadViewModelMatrix(
    const OUTPUT_SHADER *shader, const MATRIX *source);
void Output_Shader_UploadWibbleEffect(OUTPUT_SHADER *shader, bool is_enabled);
void Output_Shader_UploadTint(OUTPUT_SHADER *shader, RGB_F tint);
