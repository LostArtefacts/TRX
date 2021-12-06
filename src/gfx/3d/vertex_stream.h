#pragma once

#include "ati3dcif/ATI3DCIF.h"
#include "gfx/gl/buffer.h"
#include "gfx/gl/vertex_array.h"

#ifdef __cplusplus
    #include <cstdbool>
extern "C" {
#else
    #include <stdbool.h>
#endif

typedef struct GFX_3D_VertexStream {
    C3D_EPRIM prim_type;
    size_t buffer_size;
    GFX_GL_Buffer buffer;
    GFX_GL_VertexArray vtc_format;
    struct {
        C3D_VTCF *data;
        size_t count;
        size_t capacity;
    } pending_vertices;
} GFX_3D_VertexStream;

void GFX_3D_VertexStream_Init(GFX_3D_VertexStream *vertex_stream);
void GFX_3D_VertexStream_Close(GFX_3D_VertexStream *vertex_stream);

void GFX_3D_VertexStream_Bind(GFX_3D_VertexStream *vertex_stream);

void GFX_3D_VertexStream_SetPrimType(
    GFX_3D_VertexStream *vertex_stream, C3D_EPRIM prim_type);

bool GFX_3D_VertexStream_PushPrimStrip(
    GFX_3D_VertexStream *vertex_stream, C3D_VTCF *vertices, int count);
bool GFX_3D_VertexStream_PushPrimList(
    GFX_3D_VertexStream *vertex_stream, C3D_VTCF *vertices, int count);

void GFX_3D_VertexStream_RenderPending(GFX_3D_VertexStream *vertex_stream);

#ifdef __cplusplus
}
#endif
