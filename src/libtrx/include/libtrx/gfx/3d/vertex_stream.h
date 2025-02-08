#pragma once

#include "../gl/buffer.h"
#include "../gl/vertex_array.h"

typedef enum {
    GFX_3D_PRIM_LINE = 0,
    GFX_3D_PRIM_TRI = 1,
} GFX_3D_PRIM_TYPE;

typedef struct {
    float x, y, z;
    float s, t, w;
    float r, g, b, a;
} GFX_3D_VERTEX;

typedef struct {
    GFX_3D_PRIM_TYPE prim_type;
    size_t buffer_size;
    GFX_GL_BUFFER buffer;
    GFX_GL_VERTEX_ARRAY vtc_format;
    struct {
        GFX_3D_VERTEX *data;
        size_t count;
        size_t capacity;
    } pending_vertices;
    size_t rendered_count;
    size_t transferred;
} GFX_3D_VERTEX_STREAM;

void GFX_3D_VertexStream_Init(GFX_3D_VERTEX_STREAM *vertex_stream);
void GFX_3D_VertexStream_Close(GFX_3D_VERTEX_STREAM *vertex_stream);

void GFX_3D_VertexStream_Bind(GFX_3D_VERTEX_STREAM *vertex_stream);

void GFX_3D_VertexStream_SetPrimType(
    GFX_3D_VERTEX_STREAM *vertex_stream, GFX_3D_PRIM_TYPE prim_type);

bool GFX_3D_VertexStream_PushPrimStrip(
    GFX_3D_VERTEX_STREAM *vertex_stream, const GFX_3D_VERTEX *vertices,
    int count);
bool GFX_3D_VertexStream_PushPrimFan(
    GFX_3D_VERTEX_STREAM *vertex_stream, const GFX_3D_VERTEX *vertices,
    int count);
bool GFX_3D_VertexStream_PushPrimList(
    GFX_3D_VERTEX_STREAM *vertex_stream, const GFX_3D_VERTEX *vertices,
    int count);

void GFX_3D_VertexStream_RenderPending(GFX_3D_VERTEX_STREAM *vertex_stream);
