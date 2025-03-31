#pragma once

#include <libtrx/game/matrix.h>
#include <libtrx/game/output/types.h>

typedef struct OUTPUT_SHADER OUTPUT_SHADER;

OUTPUT_SHADER *Output_Shader_Create(const char *path);
void Output_Shader_Free(OUTPUT_SHADER *shader);
void Output_Shader_UploadCommonUniforms(const OUTPUT_SHADER *shader);
void Output_Shader_UploadProjectionMatrix(const OUTPUT_SHADER *shader);

// TODO: these functions are poor design.
void Output_Shader_UploadMatrix(
    const OUTPUT_SHADER *shader, const MATRIX *source);
void Output_Shader_UploadWibbleEffect(
    const OUTPUT_SHADER *shader, bool is_enabled);
void Output_Shader_UploadWaterEffect(
    const OUTPUT_SHADER *shader, bool is_enabled);
void Output_Shader_UploadTint(const OUTPUT_SHADER *shader, RGB_F tint);
