#pragma once

#include <trx/core/result.h>

#include <GL/glew.h>

typedef struct OUTPUT_SHADER OUTPUT_SHADER;

RESULT Output_Shader_Create(const char *path, OUTPUT_SHADER **out_shader);

// Builds a shader from one file, with extra preprocessor text handed to every
// stage, and with a geometry stage when the file supplies one. Programs built
// from the same file with different defines are separate programs, so a caller
// that switches between them switches by binding, not by rebuilding.
RESULT Output_Shader_CreateEx(
    const char *path, const char *defines, bool has_geometry_stage,
    OUTPUT_SHADER **out_shader);
void Output_Shader_Free(OUTPUT_SHADER *shader);
void Output_Shader_Bind(const OUTPUT_SHADER *shader);

GLint Output_Shader_LookupUniform(
    const OUTPUT_SHADER *shader, const char *name);

bool Output_Shader_TryLookupUniform(
    const OUTPUT_SHADER *shader, const char *name, GLint *out_location);
