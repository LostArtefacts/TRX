#pragma once

#include <trx/game/matrix.h>
#include <trx/game/rooms/types.h>

#include <GL/glew.h>

typedef struct {
    int32_t ls_adder;
    int32_t ls_divider;
    XYZ_32 ls_vector_view;
} OUTPUT_LIGHT_INFO;

typedef struct {
    GLuint general;
    GLuint matrices;
    GLuint lights;
    GLuint ls;
    void *priv;
} OUTPUT_UNIFORMS;

OUTPUT_UNIFORMS *Output_Uniforms_Create(void);
void Output_Uniforms_Free(OUTPUT_UNIFORMS *uniforms);

void Output_Uniforms_UploadGeneral(const OUTPUT_UNIFORMS *uniforms);
void Output_Uniforms_UploadOrthoMatrix(const OUTPUT_UNIFORMS *uniforms);
void Output_Uniforms_UploadViewMatrix(
    const OUTPUT_UNIFORMS *uniforms, const MATRIX *matrix);
void Output_Uniforms_UploadRoomLights(
    const OUTPUT_UNIFORMS *uniforms, const ROOM *room);
void Output_Uniforms_UploadCPULight(
    const OUTPUT_UNIFORMS *uniforms, const OUTPUT_LIGHT_INFO *info);
