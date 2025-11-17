#pragma once

#include <trx/game/matrix.h>

#include <GL/glew.h>

typedef struct {
    GLuint general;
    GLuint matrices;
} OUTPUT_UNIFORMS;

OUTPUT_UNIFORMS *Output_Uniforms_Create(void);
void Output_Uniforms_Free(OUTPUT_UNIFORMS *uniforms);

void Output_Uniforms_UploadGeneral(const OUTPUT_UNIFORMS *uniforms);
void Output_Uniforms_UploadOrthoMatrix(const OUTPUT_UNIFORMS *uniforms);
void Output_Uniforms_UploadViewMatrix(
    const OUTPUT_UNIFORMS *uniforms, const MATRIX *matrix);
