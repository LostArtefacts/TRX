#pragma once

#include <trx/av/image.h>

#include <GL/glew.h>
#include <stdint.h>

typedef struct {
    GLfloat u;
    GLfloat v;
} TRX_GL_2D_SURFACE_UV;

typedef struct {
    int32_t width;
    int32_t height;
    int32_t pitch;
    int32_t bit_count;
    GLenum tex_format;
    GLenum tex_type;
    TRX_GL_2D_SURFACE_UV uv[4];
} TRX_GL_2D_SURFACE_DESC;

typedef struct {
    uint8_t *buffer;
    TRX_GL_2D_SURFACE_DESC desc;
} TRX_GL_2D_SURFACE;

TRX_GL_2D_SURFACE *TRX_GL_2D_Surface_Create(const TRX_GL_2D_SURFACE_DESC *desc);
TRX_GL_2D_SURFACE *TRX_GL_2D_Surface_CreateFromImage(const IMAGE *image);
void TRX_GL_2D_Surface_Free(TRX_GL_2D_SURFACE *surface);

void TRX_GL_2D_Surface_Init(
    TRX_GL_2D_SURFACE *surface, const TRX_GL_2D_SURFACE_DESC *desc);
void TRX_GL_2D_Surface_Close(TRX_GL_2D_SURFACE *surface);

void TRX_GL_2D_Surface_Clear(TRX_GL_2D_SURFACE *surface, uint8_t value);
